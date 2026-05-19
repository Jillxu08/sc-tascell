// ============================================================
// filling.c - Multi-process + Multi-GPU per process with MPI version
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <mpi.h>
#include <openacc.h>
#include <cblas.h>
#include <limits.h>
#include <time.h>
#include "filling3.h"

// ============================================================
// 全局变量 - 动态GPU管理
// ============================================================

// MPI信息
static int my_rank = -1;
static int num_processes = -1;

// GPU资源管理（每个进程独立）
static int num_gpus = 0;
static pthread_mutex_t *gpu_mutexes = NULL;
static int *gpu_data_ready = NULL;

// GPU统计信息（每个进程独立）
typedef struct {
    pthread_mutex_t mutex;
    int task_count;
    int ds_count;
    int lr_count;
    uint64_t total_size;
    uint64_t ds_size;
    uint64_t lr_size;
    double total_time;
} gpu_stats_t;

static gpu_stats_t *gpu_stats = NULL;

// 详细记录（每个进程独立）
#define INITIAL_RECORD_CAPACITY 10000

typedef struct {
    int worker_id;
    int gpu_id;
    uint64_t task_size;
    double duration;
    char type[8];
} gpu_usage_record_t;

typedef struct {
    gpu_usage_record_t *records;
    int capacity;
    int count;
    pthread_mutex_t mutex;
} gpu_record_vector_t;

static gpu_record_vector_t gpu_record_vec = {NULL, 0, 0};

// 初始化控制
static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
static int multi_gpu_initialized = 0;

struct cluster* resultCTlist;  //store nodes of cluster tree
#pragma acc declare create(resultCTlist)

int countCT=0;
int nofc, nNode;
int kparam = 50; 
double (*zgmid)[3];
double (*bgmid)[3];
int (*f2n)[3];
// int denseB;
int th_ds, th_lr; // threshold for dense and low-rank matrix filling

// ============================================================
// 初始化多GPU环境（每个进程独立调用）
// ============================================================
void initialize_multi_gpu() {
    pthread_mutex_lock(&init_mutex);
    
    if (!multi_gpu_initialized) {
        // 获取MPI信息
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
        
        // 获取本进程可见的GPU数量
        num_gpus = acc_get_num_devices(acc_device_nvidia);
        
        if (num_gpus == 0) {
            fprintf(stderr, "ERROR: Process %d detected no GPUs\n", my_rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        // ========== 动态分配GPU资源 ==========
        gpu_mutexes = (pthread_mutex_t *)malloc(num_gpus * sizeof(pthread_mutex_t));
        gpu_data_ready = (int *)calloc(num_gpus, sizeof(int));
        gpu_stats = (gpu_stats_t *)calloc(num_gpus, sizeof(gpu_stats_t));
        
        if (!gpu_mutexes || !gpu_data_ready || !gpu_stats) {
            fprintf(stderr, "ERROR: Process %d failed to allocate GPU resources\n", my_rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        for (int i = 0; i < num_gpus; i++) {
            pthread_mutex_init(&gpu_mutexes[i], NULL);
            pthread_mutex_init(&gpu_stats[i].mutex, NULL);
        }

        // ========== 初始化记录vector ==========
        pthread_mutex_init(&gpu_record_vec.mutex, NULL);
        gpu_record_vec.capacity = INITIAL_RECORD_CAPACITY;
        gpu_record_vec.count = 0;
        gpu_record_vec.records = (gpu_usage_record_t *)malloc(
            gpu_record_vec.capacity * sizeof(gpu_usage_record_t));
        
        if (!gpu_record_vec.records) {
            fprintf(stderr, "WARNING: Process %d failed to allocate record buffer\n", my_rank);
        }
        
        multi_gpu_initialized = 1;
        
        // 计算内存占用
        size_t gpu_mem = num_gpus * (sizeof(pthread_mutex_t) + sizeof(int) + sizeof(gpu_stats_t));
        size_t record_mem = gpu_record_vec.capacity * sizeof(gpu_usage_record_t);
        
        // fprintf(stderr, "\n");
        // fprintf(stderr, "╔════════════════════════════════════════════════════╗\n");
        // fprintf(stderr, "║   Multi-GPU Init (Process %d/%d)                   ║\n", 
        //         my_rank, num_processes);
        // fprintf(stderr, "╠════════════════════════════════════════════════════╣\n");
        // fprintf(stderr, "║  GPUs on this process: %d                           ║\n", num_gpus);
        // fprintf(stderr, "║  GPU Memory: %.2f KB                              ║\n", gpu_mem / 1024.0);
        // fprintf(stderr, "║  Record Capacity: %d (%.2f MB)                    ║\n", 
        //         gpu_record_vec.capacity, record_mem / (1024.0 * 1024.0));
        // fprintf(stderr, "║  Strategy: Dynamic Round-Robin                    ║\n");
        // fprintf(stderr, "╚════════════════════════════════════════════════════╝\n");
        // fprintf(stderr, "\n");
    }
    
    pthread_mutex_unlock(&init_mutex);
}


// ============================================================
// 动态获取任何可用的GPU（带worker ID偏移的轮询策略）
// ============================================================
int try_acquire_any_gpu(int worker_id) {
    if (!multi_gpu_initialized) {
        initialize_multi_gpu();
    }
    
    // 从worker_id % num_gpus开始尝试，避免所有worker都从GPU 0开始竞争
    int start_offset = worker_id % num_gpus;
    
    for (int i = 0; i < num_gpus; i++) {
        int gpu_id = (start_offset + i) % num_gpus;
        
        if (pthread_mutex_trylock(&gpu_mutexes[gpu_id]) == 0) {
            // 成功获取GPU
            return gpu_id;
        }
    }
    
    // 所有GPU都被占用
    return -1;
}

// ============================================================
// 释放GPU
// ============================================================
void release_gpu(int gpu_id) {
    if (gpu_id >= 0 && gpu_id < num_gpus) {
        pthread_mutex_unlock(&gpu_mutexes[gpu_id]);
    }
}

// ============================================================
// 将几何数据传输到所有GPU
// ============================================================
void transfer_data_to_all_gpus() {
    if (!multi_gpu_initialized) {
        initialize_multi_gpu();
    }
    
    // fprintf(stderr, "\n");
    // fprintf(stderr, "╔════════════════════════════════════════════════════╗\n");
    // fprintf(stderr, "║   Data Transfer (Process %d)                       ║\n", my_rank);
    // fprintf(stderr, "╚════════════════════════════════════════════════════╝\n");
    
    for (int gpu_id = 0; gpu_id < num_gpus; gpu_id++) {
        if (gpu_data_ready[gpu_id]) {
            // fprintf(stderr, "Process %d: GPU %d already has data (skipped)\n", 
            //         my_rank, gpu_id);
            continue;
        }
        
        // fprintf(stderr, "Process %d: Transferring to GPU %d/%d... ", 
        //         my_rank, gpu_id, num_gpus);
        // fflush(stderr);
        
        acc_set_device_num(gpu_id, acc_device_nvidia);
        
        #pragma acc enter data copyin(nofc, nNode, \
                                      zgmid[0:nofc][0:3], \
                                      f2n[0:nofc][0:3], \
                                      bgmid[0:nNode][0:3])
        
        gpu_data_ready[gpu_id] = 1;
        
        // size_t data_size = (nofc * 3 * 2 + nNode * 3) * sizeof(double);
        // double data_mb = data_size / (1024.0 * 1024.0);
        
        // fprintf(stderr, "✅ Done (%.2f MB)\n", data_mb);
    }
    fprintf(stderr, "Geometry data transferred to GPU\n");
    
    // fprintf(stderr, "\n");
    // fprintf(stderr, "Process %d: All %d GPUs ready\n", my_rank, num_gpus);
    // fprintf(stderr, "\n");
}

// ============================================================
// 替换原有的data_transfer函数
// ============================================================
void data_transfer() {
    transfer_data_to_all_gpus();
}

// ============================================================
// 记录GPU使用（自动扩容）
// ============================================================
void record_gpu_usage(int worker_id, int gpu_id, uint64_t size, 
                      double duration, const char* type) {
    if (gpu_id < 0 || gpu_id >= num_gpus) return;
    
    // ========== 更新统计（每个GPU独立锁，减少竞争）==========
    pthread_mutex_lock(&gpu_stats[gpu_id].mutex);
    
    gpu_stats[gpu_id].task_count++;
    gpu_stats[gpu_id].total_size += size;
    gpu_stats[gpu_id].total_time += duration;
    
    if (strcmp(type, "DS") == 0) {
        gpu_stats[gpu_id].ds_count++;
        gpu_stats[gpu_id].ds_size += size;
    } else if (strcmp(type, "LR") == 0) {
        gpu_stats[gpu_id].lr_count++;
        gpu_stats[gpu_id].lr_size += size;
    }
    
    pthread_mutex_unlock(&gpu_stats[gpu_id].mutex);

    // ========== 记录详细信息（可选，自动扩容）==========
    if (gpu_record_vec.records != NULL) {
        pthread_mutex_lock(&gpu_record_vec.mutex);
        
        // 检查容量，需要时扩容
        if (gpu_record_vec.count >= gpu_record_vec.capacity) {
            int new_capacity = gpu_record_vec.capacity * 2;
            gpu_usage_record_t *new_records = (gpu_usage_record_t *)realloc(
                gpu_record_vec.records,
                new_capacity * sizeof(gpu_usage_record_t));
            
            if (new_records) {
                gpu_record_vec.records = new_records;
                gpu_record_vec.capacity = new_capacity;
                
                static int expand_count = 0;
                if (expand_count < 3) {
                    // fprintf(stderr, "[Process %d Record] Expanded to %d (%.2f MB)\n",
                    //         my_rank, new_capacity,
                    //         new_capacity * sizeof(gpu_usage_record_t) / (1024.0 * 1024.0));
                    expand_count++;
                }
            }
        }
        
        if (gpu_record_vec.count < gpu_record_vec.capacity) {
            int idx = gpu_record_vec.count;
            gpu_record_vec.records[idx].worker_id = worker_id;
            gpu_record_vec.records[idx].gpu_id = gpu_id;
            gpu_record_vec.records[idx].task_size = size;
            gpu_record_vec.records[idx].duration = duration;
            strncpy(gpu_record_vec.records[idx].type, type, 7);
            gpu_record_vec.records[idx].type[7] = '\0';
            gpu_record_vec.count++;
        }
        
        pthread_mutex_unlock(&gpu_record_vec.mutex);
    }
}

void print_local_gpu_usage() {
    fprintf(stderr, "[DEBUG] Process %d: Entering print_local_gpu_usage\n", my_rank);
    fflush(stderr);
    
    // 所有进程先准备好数据
    int total_tasks = 0;
    uint64_t total_size = 0;
    double total_time = 0.0;
    
    fprintf(stderr, "[DEBUG] Process %d: Calculating totals...\n", my_rank);
    fflush(stderr);
    
    for (int i = 0; i < num_gpus; i++) {
        pthread_mutex_lock(&gpu_stats[i].mutex);
        total_tasks += gpu_stats[i].task_count;
        total_size += gpu_stats[i].total_size;
        total_time += gpu_stats[i].total_time;
        pthread_mutex_unlock(&gpu_stats[i].mutex);
    }
    
    fprintf(stderr, "[DEBUG] Process %d: Total tasks=%d, starting print...\n", my_rank, total_tasks);
    fflush(stderr);
    
    // ========== 简化：直接打印，不管顺序 ==========
    fprintf(stderr, "\n");
    fprintf(stderr, "╔════════════════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║         Local GPU Statistics (Process %d/%d)                           ║\n", 
            my_rank, num_processes);
    fprintf(stderr, "╠════════════════════════════════════════════════════════════════════════╣\n");
    fprintf(stderr, "║ GPU │ Total │  Dense  │   LR    │   Total Size   │ Total Time │  Avg  ║\n");
    fprintf(stderr, "╟─────┼───────┼─────────┼─────────┼────────────────┼────────────┼───────╢\n");
    fflush(stderr);  // ← 立即flush
    
    for (int i = 0; i < num_gpus; i++) {
        pthread_mutex_lock(&gpu_stats[i].mutex);
        
        double avg = gpu_stats[i].task_count > 0 ?
            gpu_stats[i].total_time / gpu_stats[i].task_count : 0.0;
        
        double percent = total_tasks > 0 ?
            100.0 * gpu_stats[i].task_count / total_tasks : 0.0;
        
        fprintf(stderr, "║  %d  │ %5d │  %5d  │  %5d  │ %10lu (%4.1f%%) │  %7.2fs │ %.4fs ║\n",
                i,
                gpu_stats[i].task_count,
                gpu_stats[i].ds_count,
                gpu_stats[i].lr_count,
                gpu_stats[i].total_size,
                percent,
                gpu_stats[i].total_time,
                avg);
        fflush(stderr);  // ← 每行都flush
        
        pthread_mutex_unlock(&gpu_stats[i].mutex);
    }
    
    fprintf(stderr, "╟─────┼───────┼─────────┼─────────┼────────────────┼────────────┼───────╢\n");
    
    double overall_avg = total_tasks > 0 ? total_time / total_tasks : 0.0;
    fprintf(stderr, "║ ALL │ %5d │    -    │    -    │ %14lu │  %7.2fs │ %.4fs ║\n",
            total_tasks, total_size, total_time, overall_avg);
    fprintf(stderr, "╚════════════════════════════════════════════════════════════════════════╝\n");
    fflush(stderr);  // ← flush
    
    // 负载均衡分析
    if (num_gpus > 1) {
        int max_tasks = 0, min_tasks = INT_MAX;
        for (int i = 0; i < num_gpus; i++) {
            if (gpu_stats[i].task_count > max_tasks) max_tasks = gpu_stats[i].task_count;
            if (gpu_stats[i].task_count < min_tasks) min_tasks = gpu_stats[i].task_count;
        }
        
        double balance = min_tasks > 0 ? (double)max_tasks / min_tasks : 0.0;
        fprintf(stderr, "\nProcess %d Local Load Balance: %.2f ", my_rank, balance);
        
        if (balance < 1.2) {
            fprintf(stderr, "✅ Excellent\n");
        } else if (balance < 1.5) {
            fprintf(stderr, "✓ Good\n");
        } else if (balance < 2.0) {
            fprintf(stderr, "⚠ Fair\n");
        } else {
            fprintf(stderr, "❌ Poor\n");
        }
    }
    
    fprintf(stderr, "\n");
    fflush(stderr);  // ← flush
    
    fprintf(stderr, "[DEBUG] Process %d: Print complete, entering barrier...\n", my_rank);
    fflush(stderr);
    
    // ========== 只在最后一个barrier ==========
    MPI_Barrier(MPI_COMM_WORLD);
    
    fprintf(stderr, "[DEBUG] Process %d: Barrier complete\n", my_rank);
    fflush(stderr);
}

void print_gpu_usage_report() {
    fprintf(stderr, "[DEBUG] Process %d: Entering print_gpu_usage_report\n", my_rank);
    fflush(stderr);
    
    // ========== 步骤1：打印本地统计（简化版）==========
    print_local_gpu_usage();
    
    fprintf(stderr, "[DEBUG] Process %d: Local stats printed, preparing data...\n", my_rank);
    fflush(stderr);
    
    // ========== 步骤2：准备数据 ==========
    int *local_task_counts = (int *)malloc(num_gpus * sizeof(int));
    int *local_ds_counts = (int *)malloc(num_gpus * sizeof(int));
    int *local_lr_counts = (int *)malloc(num_gpus * sizeof(int));
    uint64_t *local_total_sizes = (uint64_t *)malloc(num_gpus * sizeof(uint64_t));
    double *local_total_times = (double *)malloc(num_gpus * sizeof(double));
    
    if (!local_task_counts || !local_ds_counts || !local_lr_counts || 
        !local_total_sizes || !local_total_times) {
        fprintf(stderr, "ERROR: Process %d failed to allocate memory for MPI_Gather\n", my_rank);
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    fprintf(stderr, "[DEBUG] Process %d: Memory allocated, collecting data...\n", my_rank);
    fflush(stderr);
    
    for (int i = 0; i < num_gpus; i++) {
        pthread_mutex_lock(&gpu_stats[i].mutex);
        local_task_counts[i] = gpu_stats[i].task_count;
        local_ds_counts[i] = gpu_stats[i].ds_count;
        local_lr_counts[i] = gpu_stats[i].lr_count;
        local_total_sizes[i] = gpu_stats[i].total_size;
        local_total_times[i] = gpu_stats[i].total_time;
        pthread_mutex_unlock(&gpu_stats[i].mutex);
    }
    
    fprintf(stderr, "[DEBUG] Process %d: Data collected, entering MPI_Gather...\n", my_rank);
    fflush(stderr);
    
    // ========== 步骤3：Rank 0准备接收缓冲区 ==========
    int *all_task_counts = NULL;
    int *all_ds_counts = NULL;
    int *all_lr_counts = NULL;
    uint64_t *all_total_sizes = NULL;
    double *all_total_times = NULL;
    
    if (my_rank == 0) {
        all_task_counts = (int *)malloc(num_processes * num_gpus * sizeof(int));
        all_ds_counts = (int *)malloc(num_processes * num_gpus * sizeof(int));
        all_lr_counts = (int *)malloc(num_processes * num_gpus * sizeof(int));
        all_total_sizes = (uint64_t *)malloc(num_processes * num_gpus * sizeof(uint64_t));
        all_total_times = (double *)malloc(num_processes * num_gpus * sizeof(double));
        
        if (!all_task_counts || !all_ds_counts || !all_lr_counts || 
            !all_total_sizes || !all_total_times) {
            fprintf(stderr, "ERROR: Rank 0 failed to allocate memory for MPI_Gather\n");
            fflush(stderr);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        fprintf(stderr, "[DEBUG] Rank 0: Receive buffers allocated\n");
        fflush(stderr);
    }
    
    // ========== 步骤4：MPI_Gather（所有进程都参与）==========
    fprintf(stderr, "[DEBUG] Process %d: Calling MPI_Gather...\n", my_rank);
    fflush(stderr);
    
    MPI_Gather(local_task_counts, num_gpus, MPI_INT,
               all_task_counts, num_gpus, MPI_INT, 0, MPI_COMM_WORLD);
    
    fprintf(stderr, "[DEBUG] Process %d: MPI_Gather 1/5 complete\n", my_rank);
    fflush(stderr);
    
    MPI_Gather(local_ds_counts, num_gpus, MPI_INT,
               all_ds_counts, num_gpus, MPI_INT, 0, MPI_COMM_WORLD);
    
    fprintf(stderr, "[DEBUG] Process %d: MPI_Gather 2/5 complete\n", my_rank);
    fflush(stderr);
    
    MPI_Gather(local_lr_counts, num_gpus, MPI_INT,
               all_lr_counts, num_gpus, MPI_INT, 0, MPI_COMM_WORLD);
    
    fprintf(stderr, "[DEBUG] Process %d: MPI_Gather 3/5 complete\n", my_rank);
    fflush(stderr);
    
    MPI_Gather(local_total_sizes, num_gpus, MPI_UNSIGNED_LONG_LONG,
               all_total_sizes, num_gpus, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
    
    fprintf(stderr, "[DEBUG] Process %d: MPI_Gather 4/5 complete\n", my_rank);
    fflush(stderr);
    
    MPI_Gather(local_total_times, num_gpus, MPI_DOUBLE,
               all_total_times, num_gpus, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    fprintf(stderr, "[DEBUG] Process %d: All MPI_Gather complete\n", my_rank);
    fflush(stderr);
    
    // ========== 步骤5：Rank 0打印全局统计 ==========
    if (my_rank == 0) {
        fprintf(stderr, "[DEBUG] Rank 0: Printing global stats...\n");
        fflush(stderr);
        
        fprintf(stderr, "\n");
        fprintf(stderr, "╔════════════════════════════════════════════════════════════════════════════════╗\n");
        fprintf(stderr, "║              Global GPU Statistics (All Processes)                            ║\n");
        fprintf(stderr, "╠════════════════════════════════════════════════════════════════════════════════╣\n");
        fprintf(stderr, "║ Proc │ GPU │  Total │  Dense │   LR   │   Size (MB)  │  Time (s) │  Avg (ms) ║\n");
        fprintf(stderr, "╟──────┼─────┼────────┼────────┼────────┼──────────────┼───────────┼───────────╢\n");
        fflush(stderr);
        
        int global_total_tasks = 0;
        int global_ds = 0;
        int global_lr = 0;
        uint64_t global_size = 0;
        double global_time = 0.0;
        
        for (int p = 0; p < num_processes; p++) {
            for (int g = 0; g < num_gpus; g++) {
                int idx = p * num_gpus + g;
                
                double size_mb = all_total_sizes[idx] / (1024.0 * 1024.0);
                double avg_ms = all_task_counts[idx] > 0 ? 
                    (all_total_times[idx] / all_task_counts[idx]) * 1000.0 : 0.0;
                
                fprintf(stderr, "║  %d   │  %d  │  %5d │  %5d │  %5d │   %8.2f   │   %7.2f  │   %7.2f  ║\n",
                        p, g,
                        all_task_counts[idx],
                        all_ds_counts[idx],
                        all_lr_counts[idx],
                        size_mb,
                        all_total_times[idx],
                        avg_ms);
                fflush(stderr);  // ← 每行flush
                
                global_total_tasks += all_task_counts[idx];
                global_ds += all_ds_counts[idx];
                global_lr += all_lr_counts[idx];
                global_size += all_total_sizes[idx];
                global_time += all_total_times[idx];
            }
        }
        
        fprintf(stderr, "╟──────┼─────┼────────┼────────┼────────┼──────────────┼───────────┼───────────╢\n");
        
        double global_size_mb = global_size / (1024.0 * 1024.0);
        double global_avg_ms = global_total_tasks > 0 ? 
            (global_time / global_total_tasks) * 1000.0 : 0.0;
        
        fprintf(stderr, "║ GLOBAL    │  %5d │  %5d │  %5d │   %8.2f   │   %7.2f  │   %7.2f  ║\n",
                global_total_tasks, global_ds, global_lr,
                global_size_mb, global_time, global_avg_ms);
        fprintf(stderr, "╚════════════════════════════════════════════════════════════════════════════════╝\n");
        fflush(stderr);
        
        fprintf(stderr, "\n");
        fprintf(stderr, "Summary:\n");
        fprintf(stderr, "  Total Processes: %d\n", num_processes);
        fprintf(stderr, "  GPUs per Process: %d\n", num_gpus);
        fprintf(stderr, "  Total GPUs: %d\n", num_processes * num_gpus);
        fprintf(stderr, "  Total GPU Tasks: %d\n", global_total_tasks);
        fprintf(stderr, "  Average Tasks per GPU: %.1f\n", 
                (double)global_total_tasks / (num_processes * num_gpus));
        fflush(stderr);
        
        // 负载均衡分析
        int max_tasks = 0, min_tasks = INT_MAX;
        for (int i = 0; i < num_processes * num_gpus; i++) {
            if (all_task_counts[i] > max_tasks) max_tasks = all_task_counts[i];
            if (all_task_counts[i] < min_tasks) min_tasks = all_task_counts[i];
        }
        
        double global_balance = min_tasks > 0 ? (double)max_tasks / min_tasks : 0.0;
        fprintf(stderr, "  Global Load Balance Ratio: %.2f ", global_balance);
        
        if (global_balance < 1.2) {
            fprintf(stderr, "✅ Excellent\n");
        } else if (global_balance < 1.5) {
            fprintf(stderr, "✓ Good\n");
        } else if (global_balance < 2.0) {
            fprintf(stderr, "⚠ Fair\n");
        } else {
            fprintf(stderr, "❌ Poor\n");
        }
        
        fprintf(stderr, "\n");
        fflush(stderr);
        
        fprintf(stderr, "[DEBUG] Rank 0: Freeing receive buffers...\n");
        fflush(stderr);
        
        // 释放接收缓冲区
        free(all_task_counts);
        free(all_ds_counts);
        free(all_lr_counts);
        free(all_total_sizes);
        free(all_total_times);
    }
    
    fprintf(stderr, "[DEBUG] Process %d: Freeing local buffers...\n", my_rank);
    fflush(stderr);
    
    // ========== 步骤6：所有进程释放本地缓冲区 ==========
    free(local_task_counts);
    free(local_ds_counts);
    free(local_lr_counts);
    free(local_total_sizes);
    free(local_total_times);
    
    fprintf(stderr, "[DEBUG] Process %d: Entering final barrier...\n", my_rank);
    fflush(stderr);
    
    // ========== 步骤7：最后同步 ==========
    MPI_Barrier(MPI_COMM_WORLD);
    
    fprintf(stderr, "[DEBUG] Process %d: print_gpu_usage_report complete\n", my_rank);
    fflush(stderr);
}

void print_gpu_distribution() {
    print_gpu_usage_report();
}

// ============================================================
// 清理资源
// ============================================================
void cleanup_multi_gpu() {
    if (gpu_mutexes) {
        for (int i = 0; i < num_gpus; i++) {
            pthread_mutex_destroy(&gpu_mutexes[i]);
        }
        free(gpu_mutexes);
    }
    
    if (gpu_stats) {
        for (int i = 0; i < num_gpus; i++) {
            pthread_mutex_destroy(&gpu_stats[i].mutex);
        }
        free(gpu_stats);
    }
    
    if (gpu_data_ready) free(gpu_data_ready);
    
    if (gpu_record_vec.records) {
        pthread_mutex_destroy(&gpu_record_vec.mutex);
        free(gpu_record_vec.records);
    }
    
    multi_gpu_initialized = 0;
}

// ============================================================
// acaplus - 低秩矩阵ACA算法（动态GPU版本）
// ============================================================
int acaplus(double* zaa, double* zab, int ndl, int ndt, int nstrtl, int nstrtt, 
            int kmax, double eps, double znrmmat, double pACA_EPS, 
            double* pa_ref, double* pb_ref, int* lrow_done, int* lcol_done, 
            int nofc, double zgmid[][3], int f2n[][3], double bgmid[][3], 
            int id, int use_gpu, int acquired_gpu_id,  // ← 修改：acquired_gpu_id
            double *lr_cpu_time, double *lr_gpu_time)
{
  
  int nmax = (ndl > ndt) ? ndl : ndt;
  double* zau = (double*)calloc(nmax, sizeof(double));

  // printf("zab present: %d\n", acc_is_present(zab, ndt * kmax * sizeof(double)));
  // printf("zaa present: %d\n", acc_is_present(zaa, ndl * kmax * sizeof(double)));
  // printf("lcol_done present: %d\n", acc_is_present(lcol_done, ndt * sizeof(int)));// 1 exist 
  // printf("lrow_done present: %d\n", acc_is_present(lrow_done, ndl * sizeof(int)));
  // printf("pa_ref present: %d\n", acc_is_present(pa_ref, ndl * sizeof(double)));
  // printf("pb_ref present: %d\n", acc_is_present(pb_ref, ndt * sizeof(double)));
  // printf("zau present: %d\n", acc_is_present(zau, nmax * sizeof(double)));
  // printf("ndl present: %d\n", acc_is_present(&ndl, sizeof(int)));
  
  int INCY = 1;
  double t0=0.0, t1=0.0;
  double lr_gpu_start = 0.0;
  
  // ========== GPU数据传输 ==========
  if (use_gpu) {
      t0 = get_time();
      #pragma acc enter data copyin(zab[0:ndt*kmax], zaa[0:ndl*kmax],\
                                    pa_ref[0:ndl], pb_ref[0:ndt],\
                                    lrow_done[0:ndl], lcol_done[0:ndt],\
                                    zau[0:nmax], INCY)
      t1 = get_time();
      *lr_gpu_time += (t1 - t0);
      lr_gpu_start = t0;
  }

  double *prow, *pcol;
  int il,it,ib;
  // int INCY = 1;
  double za_ACA_EPS = 1.0e-10;

  double znrm = znrmmat * sqrt((double)ndl * (double)ndt);
  double ACA_EPS = pACA_EPS;

  int ntries = max(ndl, ndt) + 1;
  int ntries_row = 6;
  int ntries_col = 6;

  int k = 0;
  int j_ref = 0;
  // pa_ref = (double *)malloc(ndl * sizeof(double));                            
  
  double (*zaa2)[ndl] = (double(*)[ndl])zaa;
  double (*zab2)[ndt] = (double(*)[ndt])zab;
  
  comp_col(zaa, zab, ndl, ndt, k, j_ref, pa_ref, nstrtl, nstrtt, lrow_done, zau, nofc, zgmid, f2n, bgmid, use_gpu, id, lr_cpu_time, lr_gpu_time);

  double colnorm = cblas_dnrm2(ndl, pa_ref, INCY); // blas used
  int i_ref = minabsvalloc_d(pa_ref, ndl);
  double rownorm = fabs(pa_ref[i_ref]);
  // pb_ref = (double *)malloc(ndt * sizeof(double));
  comp_row(zaa, zab, ndl, ndt, k, i_ref, pb_ref, nstrtl, nstrtt, lcol_done, zau, nofc, zgmid, f2n, bgmid, use_gpu, id, lr_cpu_time, lr_gpu_time);

  rownorm = cblas_dnrm2(ndt, pb_ref, INCY); // blas used

  double apxnorm = 0.0;
  int lstop_aca = 0;
  
  double col_maxval, row_maxval;
  // Main ACA loop with OpenACC parallelization
  while(k < kmax && (ntries_row > 0 || ntries_col > 0) && ntries > 0){
    ntries--;
    pcol = &zaa2[k][0];
    prow = &zab2[k][0];
    col_maxval = 0.0;
    int i = maxabsvalloc_d(pa_ref, ndl); // find the index of the maximum absolute value in pa_ref
    col_maxval = fabs(pa_ref[i]); // get the maximum absolute value in pa_ref
    row_maxval = 0.0;
    int j = maxabsvalloc_d(pb_ref, ndt); // find the index of the maximum absolute value in pb_ref
    row_maxval = fabs(pb_ref[j]); // get the maximum absolute value in pb_ref
    double zinvmax;
    if(row_maxval > col_maxval){
      if(j != j_ref){
        comp_col(zaa, zab, ndl, ndt, k, j, pcol, nstrtl, nstrtt, lrow_done, zau, nofc, zgmid, f2n, bgmid, use_gpu, id, lr_cpu_time, lr_gpu_time);
      }else{
        memcpy(pcol, pa_ref, sizeof(double) * ndl); 
      }
      i = maxabsvalloc_d(pcol, ndl); // find the index of the maximum absolute value in pcol
      col_maxval = fabs(pcol[i]); // get the maximum absolute value in pcol
      // printf("i=%d, col_maxval=%f\n", i, col_maxval);

      if(col_maxval < ACA_EPS && k >= 1){
        lstop_aca = 1;
      }else{
        comp_row(zaa, zab, ndl, ndt, k, i, prow, nstrtl, nstrtt, lcol_done, zau, nofc, zgmid, f2n, bgmid, use_gpu, id, lr_cpu_time, lr_gpu_time);
        if(fabs(pcol[i]) > 1.0e-20){
          zinvmax = 1.0 / pcol[i];
        }else{
          k = max(k-1, 0);
          // fprintf(stderr, "break\n");
          break;
        }
        for(il=0;il<ndl;il++){
          pcol[il] *= zinvmax;
        } 
      }
    }else{
      if(i != i_ref){
        comp_row(zaa, zab, ndl, ndt, k, i, prow, nstrtl, nstrtt, lcol_done, zau, nofc, zgmid, f2n, bgmid, use_gpu, id, lr_cpu_time, lr_gpu_time);
      }else{
        memcpy(prow, pb_ref, sizeof(double) * ndt);
      }
      j = maxabsvalloc_d(prow, ndt);
      row_maxval = fabs(prow[j]);

      if(row_maxval < ACA_EPS && k >= 1){
        lstop_aca = 1;
      }else{
        comp_col(zaa, zab, ndl, ndt, k, j, pcol, nstrtl, nstrtt, lrow_done, zau, nofc, zgmid, f2n, bgmid, use_gpu, id, lr_cpu_time, lr_gpu_time);
        if(fabs(prow[j]) > 1.0e-20){
          zinvmax = 1.0 / prow[j];
        }else{
          k = max(k-1, 0);
          break;
        }
        for(il=0;il<ndt;il++){
          prow[il] *= zinvmax;
        } 
      }
    }
    lrow_done[i] = 1;
    lcol_done[j] = 1;

    if(i != i_ref){
      zinvmax = -pcol[i_ref];
      for(il=0;il<ndt;il++){
        pb_ref[il] += prow[il] * zinvmax;
      }
      rownorm = cblas_dnrm2(ndt, pb_ref, INCY); // blas used
    }
    if(i == i_ref || rownorm < ACA_EPS){
      if(i == i_ref){
        ntries_row++;
      }
      if(ntries_row > 0){
        rownorm = 0.0;
        i = i_ref;
        while(i != (i_ref + ndl - 1) % ndl && rownorm < za_ACA_EPS && ntries_row > 0){
          if(lrow_done[i] == 0){
            comp_row(zaa, zab, ndl, ndt, k+1, i, pb_ref, nstrtl, nstrtt, lcol_done, zau, nofc, zgmid, f2n, bgmid, use_gpu, id, lr_cpu_time, lr_gpu_time);
            rownorm = cblas_dnrm2(ndt, pb_ref, INCY);
            if(rownorm < ACA_EPS){
              lrow_done[i] = 1;
            }
            ntries_row--;
          }else{
            rownorm = 0.0;
          }
          i = (i+1) % ndl;
        }
        i_ref = (i + ndl - 1) % ndl;
      }
    }

    if(j != j_ref){
      zinvmax = -prow[j_ref];
      for(il=0;il<ndl;il++){
        pa_ref[il] += pcol[il] * zinvmax;
      }
      colnorm = cblas_dnrm2(ndl, pa_ref, INCY); // blas used
    }
    if(j == j_ref || colnorm < ACA_EPS){
      if(j == j_ref){
        ntries_col++;
      }
      if(ntries_col > 0){
        colnorm = 0.0;
        j = j_ref;
        while(j != (j_ref + ndt - 1) % ndt && colnorm < za_ACA_EPS && ntries_col > 0){
          if(lcol_done[j] == 0){
            comp_col(zaa, zab, ndl, ndt, k+1, j, pa_ref, nstrtl, nstrtt, lrow_done, zau, nofc, zgmid, f2n, bgmid, use_gpu, id, lr_cpu_time, lr_gpu_time);
            colnorm = cblas_dnrm2(ndl, pa_ref, INCY); // blas used
            if(colnorm < ACA_EPS){
              lcol_done[j] = 1;
            }
            ntries_col--;
          }else{
            colnorm = 0.0;
          }
          j = (j+1) % ndt;
        } 
        j_ref = (j + ndt - 1) % ndt;
      }
    }

    if(colnorm < ACA_EPS && rownorm < ACA_EPS && k >= 1){
      lstop_aca = 1;
      k = k + 1;
    }

    if(lstop_aca == 0){
      double blknorm = cblas_dnrm2(ndl, pcol, INCY) * cblas_dnrm2(ndt, prow, INCY);  // blas used
      if(k == 0){
        apxnorm = blknorm;
      }else{
        double compared = apxnorm * eps;
        if(blknorm < compared && rownorm < compared && colnorm < compared && k >= 1){
          lstop_aca = 1;
        }
      }
    }
    if(lstop_aca == 1 && k >= 1){
      break;
    }

    k++;
    // printf("k=%d, ntries_row=%d, ntries_col=%d, ntries=%d\n", k, ntries_row, ntries_col, ntries);
  }

  // ========== GPU数据清理 & 释放 ==========
  if (use_gpu) {
      t0 = get_time();
      #pragma acc exit data delete(zaa[0:ndl*kmax], zab[0:ndt*kmax], \
                                  pa_ref[0:ndl], pb_ref[0:ndt], \
                                  lrow_done[0:ndl], lcol_done[0:ndt], zau[0:nmax])
      t1 = get_time();
      *lr_gpu_time += (t1 - t0);
      
      double total_duration = t1 - lr_gpu_start;
      uint64_t workload = k * (ndl + ndt);
      
      // 记录GPU使用
      record_gpu_usage(id, acquired_gpu_id, workload, total_duration, "LR");
      
      // ========== 释放GPU ==========
      release_gpu(acquired_gpu_id);
  }
  
  free(zau);

  if(k < 1){
    printf("alert!\n");
    printf("colnorm=%f rownorm=%f ACA_EPS=%f\n", colnorm, rownorm, ACA_EPS);
    printf("col_maxval=%f row_maxval=%f\n", col_maxval, row_maxval);
    printf("ntries_row=%d ntries_col=%d ntries=%d\n", ntries_row, ntries_col, ntries);
    printf("k=%d\n", k);
  }
  return k;
}

// ============================================================
// fill_sub_leafmtx - 使用动态GPU分配
// ============================================================
void fill_sub_leafmtx(struct leafmtx *st_lf, double znrmmat, int id, 
                      int *gpu_lr_cnt, int *cpu_lr_cnt, 
                      int *gpu_ds_cnt, int *cpu_ds_cnt, 
                      uint64_t *gpu_lr_elem, uint64_t *cpu_lr_elem, 
                      uint64_t *gpu_ds_elem, uint64_t *cpu_ds_elem, 
                      uint64_t *pen_ds_gpu_subm, uint64_t *pen_ds_gpu_elem, 
                      uint64_t *pen_lr_gpu_subm, uint64_t *pen_lr_gpu_elem, 
                      double *ds_cpu_time, double *lr_cpu_time, 
                      double *ds_gpu_time, double *lr_gpu_time) {
    
  // ========== Worker首次打印信息（仅打印前20个）==========
  static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
  static int printed_count = 0;
  
  pthread_mutex_lock(&print_mutex);
  if (printed_count < 20) {
      int my_rank;
      MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
      
      // fprintf(stderr, "[Worker Init] Process=%d Worker=%2d: Ready (Dynamic GPU)\n",
      //         my_rank, id);
      
      printed_count++;
  }
  pthread_mutex_unlock(&print_mutex);   

  double eps = 1.0e-8;    
  double ACA_EPS = 0.9 * eps;   
  int kparam = 50; //max rank of the partition
  int ip,il,it; 

  int ndl = st_lf->ndl; // the length of the partition
  int ndt = st_lf->ndt; // the width of the partition
  int ns = ndl * ndt; // the size of the partition
  int nstrtl = st_lf->nstrtl; // the coordination of the first element of partition
  int nstrtt = st_lf->nstrtt; // the coordination of the first element of partition
  int ltmtx = st_lf->ltmtx;    // the kind of the matrix; 1:rk 2:full
  int kt;
  int use_gpu = 0;

  // Use low-rank approximate storage
  if(ltmtx == 1){
    // printf("workload: LR=%d, ndl=%d, ndt=%d\n", kparam * (ndl + ndt), ndl, ndt);
    st_lf->a1 = (double*)malloc(sizeof(double) * ndt * kparam); 
    st_lf->a2 = (double*)malloc(sizeof(double) * ndl * kparam);
    if(!st_lf->a1 || !st_lf->a2){
      printf("allocate a1 or a2 failed!\n");
      exit(99);
    }

    double *pa_ref = (double *)malloc(ndl * sizeof(double));
    double *pb_ref = (double *)malloc(ndt * sizeof(double));
    int *lrow_done = (int *)calloc(ndl, sizeof(int));
    int *lcol_done = (int *)calloc(ndt, sizeof(int));

    // ========== 动态获取GPU ==========
    int workload = kparam * (ndl + ndt);
    int acquired_gpu = -1;

    if(workload >= th_lr){
      acquired_gpu = try_acquire_any_gpu(id);  // ← 动态获取
    }
  
    if (acquired_gpu >= 0) {
      use_gpu = 1;
      (*gpu_lr_cnt)++;
      
      // 设置当前GPU
      acc_set_device_num(acquired_gpu, acc_device_nvidia);
      
      // 验证（前10次）
      static pthread_mutex_t verify_mutex = PTHREAD_MUTEX_INITIALIZER;
      static int verify_count = 0;
      
      pthread_mutex_lock(&verify_mutex);
      if (verify_count < 10) {
          int my_rank;
          MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
          int current_gpu = acc_get_device_num(acc_device_nvidia);
          
          // fprintf(stderr, "[LR Dynamic] Process=%d Worker=%2d: Acquired GPU %d ✅\n",
          //         my_rank, id, acquired_gpu);
          
          verify_count++;
      }
      pthread_mutex_unlock(&verify_mutex);
    } else {
      (*cpu_lr_cnt)++;
    }

    // double t_start = get_time();
    kt = acaplus(st_lf->a2, st_lf->a1, ndl, ndt, nstrtl, nstrtt,
                     kparam, eps, znrmmat, ACA_EPS,
                     pa_ref, pb_ref, lrow_done, lcol_done, 
                     nofc, zgmid, f2n, bgmid, 
                     id, use_gpu, acquired_gpu,  // ← 传递acquired_gpu
                     lr_cpu_time, lr_gpu_time);
    // double t_end = get_time();
    // fprintf(stderr, "LR: use_gpu=%d, Worker %d: acaplus time: %.6f seconds, nstrtl=%d, nstrtt=%d, ndl=%d, ndt=%d\n", use_gpu, id, t_end - t_start, nstrtl, nstrtt, ndl, ndt);

    st_lf->kt = kt; // store the actual rank of the partition

    if(kt > kparam){ 
      printf("WARNING: Insufficient k: kt=%d, kparam=%d, nstrtl=%d, nstrtt=%d, ndl=%d, ndt=%d\n", kt, kparam, nstrtl, nstrtt, ndl, ndt);
    }

    st_lf->a1 = (double *)realloc(st_lf->a1, kt * ndt * sizeof(double));
    st_lf->a2 = (double *)realloc(st_lf->a2, kt * ndl * sizeof(double)); //realloc memory so that it matches the rank corresponding to kt.

    if(use_gpu){
      (*gpu_lr_elem) += kt * (ndl + ndt);
    }else{
      (*cpu_lr_elem) += kt * (ndl + ndt);
    } 

    if(workload >= th_lr){
      (*pen_lr_gpu_subm)++;
      (*pen_lr_gpu_elem) += kt * (ndl + ndt);
    }

    free(pa_ref);
    free(pb_ref);
    free(lrow_done);
    free(lcol_done);
  }else if(ltmtx == 2){
    // printf("workload: DS=%d, ndl=%d, ndt=%d\n", ns, ndl, ndt);
    st_lf->a1 = (double *)malloc(sizeof(double) * ns);
    if(!st_lf->a1){
            printf("ERROR: allocate a1 failed!\n");
            exit(99);}
    
    // ========== 动态获取GPU ==========
    int can_use_gpu = (ns >= th_ds);
    int acquired_gpu = -1;

    if (can_use_gpu) {
      (*pen_ds_gpu_subm)++;
      (*pen_ds_gpu_elem) += ns;
      acquired_gpu = try_acquire_any_gpu(id);  // ← 动态获取
    }

    if (acquired_gpu >= 0) {
      use_gpu = 1;
      (*gpu_ds_cnt)++;
      (*gpu_ds_elem) += ns;
    
      // 设置当前GPU
      acc_set_device_num(acquired_gpu, acc_device_nvidia);
      
      // ========== 验证（前10次）==========
      static pthread_mutex_t verify_mutex = PTHREAD_MUTEX_INITIALIZER;
      static int verify_count = 0;
      
      pthread_mutex_lock(&verify_mutex);
      if (verify_count < 10) {
        int my_rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
        int current_gpu = acc_get_device_num(acc_device_nvidia);
        
        // fprintf(stderr, "[DS Dynamic] Process=%d Worker=%2d: Acquired GPU %d %s\n",
        //         my_rank, id, acquired_gpu,
        //         (current_gpu == acquired_gpu) ? "✅" : "❌ ERROR!");
        
        verify_count++;
      }
      pthread_mutex_unlock(&verify_mutex);
    } else {
      use_gpu = 0;
      (*cpu_ds_cnt)++;
      (*cpu_ds_elem) += ns;
    }

    // ========== 计时 ==========
    double ds_cpu_start1 = 0.0, ds_cpu_end1 = 0.0;
    double de_gpu_start1 = 0.0, de_gpu_end1 = 0.0;
    
    if (!use_gpu) {
        ds_cpu_start1 = get_time();
    } else {
        de_gpu_start1 = get_time();
    }

    #pragma acc data if(use_gpu) copy(st_lf->a1[0:ns])
    {
      double (*tempa1)[ndt] = (double(*)[ndt])st_lf->a1;
      double xf[3], yf[3], zf[3];
      double xp, yp, zp; 

      #pragma acc parallel loop collapse(2) if(use_gpu) \
                 present(zgmid[0:nofc][0:3], f2n[0:nofc][0:3], bgmid[0:nNode][0:3], st_lf->a1[0:ns])  \
                 private(xf, yf, zf, xp, yp, zp)
      for(int il = 0; il < ndl; il++) {
          for(int it = 0; it < ndt; it++) {
            int ill = il + nstrtl; 
            int itt = it + nstrtt;

            xp = zgmid[ill][0];
            yp = zgmid[ill][1];
            zp = zgmid[ill][2];

            int ni0 = f2n[itt][0], ni1 = f2n[itt][1], ni2 = f2n[itt][2];
            xf[0] = bgmid[ni0][0]; xf[1] = bgmid[ni1][0]; xf[2] = bgmid[ni2][0];
            yf[0] = bgmid[ni0][1]; yf[1] = bgmid[ni1][1]; yf[2] = bgmid[ni2][1];
            zf[0] = bgmid[ni0][2]; zf[1] = bgmid[ni1][2]; zf[2] = bgmid[ni2][2];

            tempa1[il][it] = face_integral2(xf, yf, zf, xp, yp, zp);
          }
        }         
    }

    // ========== 计时结束 & 释放GPU ==========
    if (!use_gpu) {
        ds_cpu_end1 = get_time();
        *ds_cpu_time += (ds_cpu_end1 - ds_cpu_start1);
    } else {
        de_gpu_end1 = get_time();
        double duration = de_gpu_end1 - de_gpu_start1;
        
        // 记录GPU使用
        record_gpu_usage(id, acquired_gpu, ns, duration, "DS");
        
        // 释放GPU
        release_gpu(acquired_gpu);  // ← 立即释放
        
        *ds_gpu_time += duration;
    }
  }
}   

void comp_row(double* zaa, double* zab,
              int ndl, int ndt, int k, int il,
              double* row,
              int nstrtl, int nstrtt,
              int* lrow_done,
              double* zau,
              int nofc,
              double zgmid[][3],
              int f2n[][3],
              double bgmid[][3],
              int use_gpu,
              int id,
              double *lr_cpu_time,
              double *lr_gpu_time
            )
{
    int it;
    double xf[3], yf[3], zf[3];
    double xp, yp, zp;

    double t_start, t_end;

    /*========================
      主计算部分计时
     ========================*/
    t_start = get_time();

    if (use_gpu) {
        #pragma acc update device(zab[0:ndt*kparam], \
                                  zaa[0:ndl*kparam], \
                                  row[0:ndt], \
                                  lrow_done[0:ndt])
    }

    #pragma acc parallel if(use_gpu) \
        present(zab[0:ndt*kparam], zaa[0:ndl*kparam], \
                zgmid[0:nofc][0:3], f2n[0:nofc][0:3], \
                bgmid[0:nNode][0:3], \
                row[0:ndt], lrow_done[0:ndt])
    {
        #pragma acc loop private(xf, yf, zf) \
                         firstprivate(ndt, nstrtl, nstrtt)
        for (it = 0; it < ndt; it++) {
            if (lrow_done[it] == 0) {
                int ill = il + nstrtl;
                int itt = it + nstrtt;

                int n[3];

                xp = zgmid[ill][0];
                yp = zgmid[ill][1];
                zp = zgmid[ill][2];

                for (int i = 0; i < 3; i++) {
                    n[i] = f2n[itt][i];
                }

                #pragma acc loop seq
                for (int i = 0; i < 3; i++) {
                    xf[i] = bgmid[n[i]][0];
                    yf[i] = bgmid[n[i]][1];
                    zf[i] = bgmid[n[i]][2];
                }

                row[it] = face_integral2(xf, yf, zf, xp, yp, zp);
            }
        }
    }

    if (k == 0) {
        #pragma acc update host(row[0:ndt]) if(use_gpu)
        t_end = get_time();

        if (use_gpu)
            *lr_gpu_time += (t_end - t_start);
        else
            *lr_cpu_time += (t_end - t_start);

        return;
    }

    adotsub_dsm(row, zab, zaa, il, ndt, k, ndt, ndl, zau);

    #pragma acc parallel loop if(use_gpu) \
        present(lrow_done[0:ndt], row[0:ndt]) \
        firstprivate(ndt)
    for (it = 0; it < ndt; it++) {
        if (lrow_done[it] != 0) {
            row[it] = 0.0;
        }
    }

    if (use_gpu) {
        #pragma acc update host(row[0:ndt])
    }

    t_end = get_time();

    /*========================
      时间累加
     ========================*/
    if (use_gpu)
        *lr_gpu_time += (t_end - t_start);
    else
        *lr_cpu_time += (t_end - t_start);
}

void comp_col(double* zaa, double* zab,
                int ndl, int ndt, int k, int it,
                double* col,
                int nstrtl, int nstrtt,
                int* lrow_done,
                double* zau,
                int nofc,
                double zgmid[][3],
                int f2n[][3],
                double bgmid[][3],
                int use_gpu,
                int id,
                double *lr_cpu_time,
                double *lr_gpu_time)
{
    double xf[3], yf[3], zf[3];
    double t_start, t_end;

    /*========================
      计时开始
     ========================*/
    t_start = get_time();

    if (use_gpu) {
        #pragma acc update device(zab[0:ndt*kparam], \
                                  zaa[0:ndl*kparam], \
                                  col[0:ndl], \
                                  lrow_done[0:ndl])
    }

    #pragma acc parallel if(use_gpu) \
        present(zab[0:ndt*kparam], zaa[0:ndl*kparam], \
                zgmid[0:nofc][0:3], f2n[0:nofc][0:3], \
                bgmid[0:nNode][0:3], \
                col[0:ndl], lrow_done[0:ndl])
    {
        #pragma acc loop private(xf, yf, zf) \
                         firstprivate(ndl, it, nstrtl, nstrtt)
        for (int il = 0; il < ndl; il++) {
            if (lrow_done[il] == 0) {
                int ill = il + nstrtl;
                int itt = it + nstrtt;

                double xp = zgmid[ill][0];
                double yp = zgmid[ill][1];
                double zp = zgmid[ill][2];

                #pragma acc loop seq
                for (int i = 0; i < 3; i++) {
                    int ni = f2n[itt][i];
                    xf[i] = bgmid[ni][0];
                    yf[i] = bgmid[ni][1];
                    zf[i] = bgmid[ni][2];
                }

                col[il] = face_integral2(xf, yf, zf, xp, yp, zp);
            }
        }
    }

    if (k == 0) {
        #pragma acc update host(col[0:ndl]) if(use_gpu)

        t_end = get_time();
        if (use_gpu)
            *lr_gpu_time += (t_end - t_start);
        else
            *lr_cpu_time += (t_end - t_start);

        return;
    }

    adotsub_dsm(col, zaa, zab, it, ndl, k, ndl, ndt, zau);

    #pragma acc parallel loop if(use_gpu) \
        present(lrow_done[0:ndl], col[0:ndl]) \
        firstprivate(ndl)
    for (int il = 0; il < ndl; il++) {
        if (lrow_done[il] != 0) {
            col[il] = 0.0;
        }
    }

    if (use_gpu) {
        #pragma acc update host(col[0:ndl])
    }

    /*========================
      计时结束 & 累加
     ========================*/
    t_end = get_time();

    if (use_gpu)
        *lr_gpu_time += (t_end - t_start);
    else
        *lr_cpu_time += (t_end - t_start);
}

#pragma acc routine seq 
int minabsvalloc_d(double* za, int nd){
  int il = 0;
  double zz = fabs(za[0]);
  int it;
  for(it=1;it<nd;it++){
    if(fabs(za[it]) < zz){
      il = it;
      zz = fabs(za[it]);
    }
  }
  return il;
}

#pragma acc routine seq
int maxabsvalloc_d(double* za, int nd){
  int il = 0;
  double zz = 0.0;
  int it;
  for(it=0;it<nd;it++){
    if(fabs(za[it]) > zz){
      il = it;
      zz = fabs(za[it]);
    }
  }
  return il;
}

#pragma acc routine seq
double entry_ij(int i, int j){
  int il;
  int n[3];
  double xf[3], yf[3], zf[3];
  double xp, yp, zp;

  xp = zgmid[i][0];
  yp = zgmid[i][1];
  zp = zgmid[i][2];
  
  for(il=0;il<3;il++){
    n[il] = f2n[j][il];
  }
  for(il=0;il<3;il++){
    xf[il] = bgmid[n[il]][0];
    yf[il] = bgmid[n[il]][1];
    zf[il] = bgmid[n[il]][2];
  }

  double result = face_integral2(xf, yf, zf, xp, yp, zp);

  return result;
}

#pragma acc routine seq
double face_integral2(double xs[], double ys[], double zs[], double x, double y, double z){
  int il;
  double PI = 3.1415927;
  double EPSILON_0 = 8.854188 * 1e-12;

  double r[3];
  double xi, xj, yi, dx, dy, t, l, m, d, ti, tj;
  double theta, omega, q, g, zp, zpabs;

  int i, j;
  // double *u, *v, *w;
  double u[3],v[3],w[3];
  double ox, oy, oz;

  for(il=0;il<3;il++){
    r[il] = sqrt( pow((xs[il] - x),2.0) + pow((ys[il] - y),2.0) + pow((zs[il] - z),2.0) );
  }

  u[0] = xs[1] - xs[0];
  u[1] = ys[1] - ys[0];
  u[2] = zs[1] - zs[0];

  v[0] = xs[2] - xs[1];
  v[1] = ys[2] - ys[1];
  v[2] = zs[2] - zs[1];
  
  cross_product(u, v, w);
  
  double dw = sqrt( dot_product(w,w,3));
  for(il=0;il<3;il++){
    w[il] = w[il] / dw;
  }
  u[0] = x - xs[0];
  u[1] = y - ys[0];
  u[2] = z - zs[0];
  zp = dot_product(u,w,3);
  
  ox = x - zp * w[0];
  oy = y - zp * w[1];
  oz = z - zp * w[2];
  zpabs = fabs(zp);

  double face_integral = 0.0;
  for(i=0;i<3;i++){
    j = (i + 1) % 3;
    u[0] = xs[j] - ox;
    u[1] = ys[j] - oy;
    u[2] = zs[j] - oz;
    xj = sqrt( dot_product(u,u,3) );

    for(il=0;il<3;il++){
      u[il] = u[il] / xj;
    }
    cross_product(w, u, v);
    xi = (xs[i] - ox) * u[0] + (ys[i] - oy) * u[1] + (zs[i] - oz) * u[2];
    yi = (xs[i] - ox) * v[0] + (ys[i] - oy) * v[1] + (zs[i] - oz) * v[2];

    dx = xj - xi;
    dy = - yi;
    t = sqrt ((dx*dx) + (dy*dy));
    l = dx / t;
    m = dy / t;
    d = (l * yi) - (m * xi);
    ti = (l * xi) + (m * yi);
    tj = l * xj;

    theta = atan2(yi, xi);
    omega = theta - atan2( r[i] * d, zpabs * ti ) + atan2( r[j] * d, zpabs * tj );
    q = log( (r[j] + tj) / (r[i] + ti) );
    g = d * q - zpabs * omega;
    face_integral = face_integral + g;
  }
  
  return fabs(face_integral) / (4.0 * PI * EPSILON_0);
  // return 0.0;

}

#pragma acc routine seq
void cross_product(double* u, double* v, double* w){
  w[0] = u[1] * v[2] - u[2] * v[1];
  w[1] = u[2] * v[0] - u[0] * v[2];
  w[2] = u[0] * v[1] - u[1] * v[0];
}

// 用来逐步消去矩阵中的低秩近似部分，
// 对每个 il ∈ [0, ndl-1]，进行矩阵 zaa 的第 it 行和 zab 的第 it 行第 im 列相乘求和，累加到 zau[il] 中。
#pragma acc routine seq
void adotsub_dsm(double* zr, double* zaa, double* zab, int it, int ndl, int ndt, int mdl, int mdt, double* zau){
  int il;
  // double* zau = (double*)calloc(ndl,sizeof(double));
  for(il=0;il<ndl;il++){
    zau[il] = 0.0;
  }

  adot_dsm(zau,zaa,zab,it,ndl,ndt,mdl,mdt);
  for(il=0;il<ndl;il++){
    zr[il] = zr[il] - zau[il];
  }
  // free(zau);
}

// 构建一个近似基的向量
#pragma acc routine seq
void adot_dsm(double* zau, double* zaa, double* zab, int im, int ndl, int ndt, int mdl, int mdt){
  int it,il;
  double (*zaa2)[mdl];
  double (*zab2)[mdt];
  zaa2 = (double(*)[mdl])zaa;
  zab2 = (double(*)[mdt])zab;
  for(it=0;it<ndt;it++){
    for(il=0;il<ndl;il++){
      zau[il] = zau[il] + zaa2[it][il] * zab2[it][im];
    }
  }
}

#pragma acc routine seq
double dot_product(double* v, double* u, int n){
  double result1 = 0.0;
  int i;
  for (i = 0; i < n; i++){
    result1 += v[i] * u[i];
  }
  return result1;
}

#pragma acc routine seq
int max(int a, int b){
  if(a >= b){
    return a;
  }
  return b;
}

double dist_2cluster(int st_cltl,int st_cltt){
  double zs = 0.0;
  int id;
  for(id=0;id<resultCTlist[st_cltl].ndim;id++){
    if(resultCTlist[st_cltl].bmax[id] < resultCTlist[st_cltt].bmin[id]){
      zs = zs + (resultCTlist[st_cltt].bmin[id] - resultCTlist[st_cltl].bmax[id]) * (resultCTlist[st_cltt].bmin[id] - resultCTlist[st_cltl].bmax[id]);
    }else if(resultCTlist[st_cltt].bmax[id]< resultCTlist[st_cltl].bmin[id]){
      zs = zs+ (resultCTlist[st_cltl].bmin[id] - resultCTlist[st_cltt].bmax[id]) * (resultCTlist[st_cltl].bmin[id] - resultCTlist[st_cltt].bmax[id]);
    }
  }
  return sqrt(zs);
}

int create_cluster(int ndpth,int nstrt,int nsize,int ndim,int nson){
  int st_clt;
  st_clt = countCT;
  countCT++;
  resultCTlist[st_clt].nstrt = nstrt;
  resultCTlist[st_clt].nsize = nsize;
  resultCTlist[st_clt].ndim = ndim;
  resultCTlist[st_clt].nnson = nson;
  resultCTlist[st_clt].ndpth = ndpth;

  return st_clt;
}

int create_ctree_ssgeom(int st_clt,   //the current node
			      double (*zgmid)[3],     //coordination of objects
            int (*face2node)[3],
			      int ndpth,         //depth of the tree
			      int ndscd,
			      int nsrt,          //the start index of list
			      int nd,            //the length of list
			      int md,            //number of data
			      int ndim){
  int id,il,nson;
  // double minsz = 50.0;
  double minsz = 1000.0;
  double zcoef = 1.1;
  double zlmin[ndim],zlmax[ndim];
  ndpth = ndpth + 1;
  
  if(nd <= minsz){
    nson = 0;
    st_clt = create_cluster(ndpth,nsrt,nd,ndim,nson);
  }else{
    for(id=0;id<ndim;id++){
      zlmin[id] = zgmid[0][id];
      zlmax[id] = zlmin[id];
      for(il=1;il<nd;il++){
	      double zg = zgmid[il][id];
        if(zg < zlmin[id]){
          zlmin[id] = zg;
        }else if(zlmax[id] < zg){
          zlmax[id] = zg;
        }
      }
    }
    double zdiff = zlmax[0] - zlmin[0];
    int ncut = 0;
    for(id=0;id<ndim;id++){
      double zidiff = zlmax[id]-zlmin[id];
      if(zidiff > zcoef * zdiff){
        zdiff = zidiff;
        ncut = id;
      }
    }
    double zlmid = 0.5 * (zlmax[ncut] + zlmin[ncut]);
    int nl = 0;
    int nr = nd-1;
    while(nl < nr){
      while(nl < nd && zgmid[nl][ncut] <= zlmid){
        nl = nl + 1;
      }
      while(nr >= 0 && zgmid[nr][ncut] > zlmid){
        nr = nr - 1;
      }
      if(nl < nr){
        for(id=0;id<ndim;id++){
          double nh = zgmid[nl][id];
          zgmid[nl][id] = zgmid[nr][id];
          zgmid[nr][id] = nh;
        }
        for(id=0;id<ndim;id++){
          int mh = face2node[nl][id];
          face2node[nl][id] = face2node[nr][id];
          face2node[nr][id] = mh;
        }
      }
    }
    
    if(nl == nd || nl == 0){
      // #ifndef _OPENACC
      // fprintf (stdout, "nl = %ld, nr = %ld\n", nl, nr);
      // #endif
      // printf (stdout, "nl = %ld, nr = %ld\n", nl, nr);
      nson = 0;
      st_clt = create_cluster(ndpth,nsrt,nd,ndim,nson);
    }else{
      nson = 2;
      st_clt = create_cluster(ndpth,nsrt,nd,ndim,nson);
      int nsrt1 = nsrt;
      int nd1 = nl;
      resultCTlist[st_clt].offsets[0] = create_ctree_ssgeom(resultCTlist[st_clt].offsets[0],zgmid,face2node,
					       ndpth,ndscd,nsrt1,nd1,md,ndim);

      nsrt1 = nsrt + nl;
      nd1 = nd - nl;
      resultCTlist[st_clt].offsets[1] = create_ctree_ssgeom(resultCTlist[st_clt].offsets[1],&zgmid[nl],&face2node[nl],
					       ndpth,ndscd,nsrt1,nd1,md,ndim);
    }
  }
  resultCTlist[st_clt].ndscd = nd;
  //bounding box
  double zeps = 1.0e-5;
  if(resultCTlist[st_clt].nnson > 0){
    for(id=0;id<ndim;id++){
      resultCTlist[st_clt].bmin[id] = resultCTlist[resultCTlist[st_clt].offsets[0]].bmin[id];
      resultCTlist[st_clt].bmax[id] = resultCTlist[resultCTlist[st_clt].offsets[0]].bmax[id];
    }
    for(il=1;il<resultCTlist[st_clt].nnson;il++){
      for(id=0;id<ndim;id++){
	      if(resultCTlist[resultCTlist[st_clt].offsets[il]].bmin[id] < resultCTlist[st_clt].bmin[id]){
	        resultCTlist[st_clt].bmin[id] = resultCTlist[resultCTlist[st_clt].offsets[il]].bmin[id];
	      }
	      if(resultCTlist[st_clt].bmax[id] < resultCTlist[resultCTlist[st_clt].offsets[il]].bmax[id]){
	        resultCTlist[st_clt].bmax[id] = resultCTlist[resultCTlist[st_clt].offsets[il]].bmax[id];
	      }
      }
    }
  }else{
    for(id=0;id<ndim;id++){
      resultCTlist[st_clt].bmin[id] = zgmid[0][id];
      resultCTlist[st_clt].bmax[id] = zgmid[0][id];
    }
    for(id=0;id<ndim;id++){
      for(il=1;il<resultCTlist[st_clt].nsize;il++){
	      if(zgmid[il][id] < resultCTlist[st_clt].bmin[id]){
	        resultCTlist[st_clt].bmin[id] = zgmid[il][id];
	      }
	      if(resultCTlist[st_clt].bmax[id] < zgmid[il][id]){
	        resultCTlist[st_clt].bmax[id] = zgmid[il][id];
	      }
      }
    }
  }
  double zwdth = (resultCTlist[st_clt].bmax[0] - resultCTlist[st_clt].bmin[0]) * (resultCTlist[st_clt].bmax[0] - resultCTlist[st_clt].bmin[0]);
  for(id=1;id<ndim;id++){
    zwdth = zwdth + (resultCTlist[st_clt].bmax[id] - resultCTlist[st_clt].bmin[id]) * (resultCTlist[st_clt].bmax[id] - resultCTlist[st_clt].bmin[id]);
  }
  zwdth = sqrt(zwdth);
  for(id=0;id<ndim;id++){
    double bdiff = resultCTlist[st_clt].bmax[id] - resultCTlist[st_clt].bmin[id];
    if(bdiff < zeps * zwdth){
      resultCTlist[st_clt].bmax[id] = resultCTlist[st_clt].bmax[id] + 0.5 * (zeps * zwdth - bdiff);
      resultCTlist[st_clt].bmin[id] = resultCTlist[st_clt].bmin[id] - 0.5 * (zeps * zwdth - bdiff);
    }
  }
  zwdth = (resultCTlist[st_clt].bmax[0] - resultCTlist[st_clt].bmin[0]) * (resultCTlist[st_clt].bmax[0] - resultCTlist[st_clt].bmin[0]);
  for(id=1;id<ndim;id++){
    zwdth += (resultCTlist[st_clt].bmax[id] - resultCTlist[st_clt].bmin[id]) * (resultCTlist[st_clt].bmax[id] - resultCTlist[st_clt].bmin[id]);
  }
  resultCTlist[st_clt].zwdth = sqrt(zwdth);
  //end of bounding box
  return st_clt;
}

double get_time(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    // printf("get_time: sec=%ld, nsec=%ld, time=%f\n", ts.tv_sec, ts.tv_nsec, (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

