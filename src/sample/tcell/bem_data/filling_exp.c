// ============================================================
// filling.c - Enhanced Version with Detailed Statistics
// Multi-process + Multi-GPU per process 
// ============================================================
// 新增功能:
// 1. Direct CPU vs Eligible 任务分析
// 2. GPU 利用率统计
// 3. 时间分解 (H2D/Compute/D2H) - 修正版
// ============================================================
// Profiling 模式控制 (与 Version 2 对齐):
//   编译期: -DGPU_PROFILING 启用精确计时（插入 acc wait）
//   运行时: 设置环境变量 GPU_PROFILING=1 启用（需编译期 -DGPU_PROFILING_RUNTIME）
//   两者均未定义时，不插入额外 acc wait，时间分解字段全部报告为 0
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <mpi.h>
#include <openacc.h>
#include <cblas.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include "filling_h.h"

// ============================================================
// Profiling 模式宏定义 (修正点4: 与 Version 2 对齐)
// ============================================================

#ifdef GPU_PROFILING
  #define PROF_ENABLED 1
  static int profiling_enabled = 1;
  #define PROF_ACC_WAIT() acc_wait_all()
#elif defined(GPU_PROFILING_RUNTIME)
  #define PROF_ENABLED profiling_enabled
  static int profiling_enabled = 0;
  #define PROF_ACC_WAIT() do { if (profiling_enabled) { acc_wait_all(); } } while(0)
#else
  #define PROF_ENABLED 0
  static int profiling_enabled = 0;
  #define PROF_ACC_WAIT() ((void)0)
#endif

// 用法: PROF_TIME_START(var) ... PROF_ACC_WAIT(); PROF_TIME_END(var, accumulator)
// 非 profiling 模式下完全零开销
#define PROF_TIME_START(var)       double var = (PROF_ENABLED) ? get_time() : 0.0
#define PROF_TIME_END(var, accum)  do { if (PROF_ENABLED) { accum += get_time() - var; } } while(0)

// 初始化 profiling（运行时模式下读取环境变量）
static void init_profiling() {
#ifdef GPU_PROFILING_RUNTIME
    const char *env = getenv("GPU_PROFILING");
    if (env && (strcmp(env, "1") == 0 || strcmp(env, "true") == 0 ||
                strcmp(env, "yes") == 0 || strcmp(env, "on") == 0)) {
        profiling_enabled = 1;
    }
#endif
}

// ============================================================
// Global Variables
// ============================================================
static int my_rank = -1;
static int num_processes = -1;

int num_gpus = 0;
static int total_hw_gpus = 0;  // 节点上的物理 GPU 总数
static pthread_mutex_t *gpu_mutexes = NULL;
static int *gpu_data_ready = NULL;

// 节点和物理GPU信息
static char my_hostname[256] = "";
static int *physical_gpu_ids = NULL;

// ============================================================
// Dense 块 Device Buffer 复用池（每 GPU 一个预分配缓冲区）
// ============================================================
static double **ds_buf = NULL;
static int *ds_buf_cap = NULL;
static int ds_buf_init_cap = 0;

// Dense buffer 统计
typedef struct {
    uint64_t reuse_count;
    uint64_t realloc_count;
    uint64_t max_ns;
} ds_buf_stats_t;
static ds_buf_stats_t *ds_buf_stat = NULL;

// ============================================================
// GPU 统计结构（修正点1: 分离 lr_free_time，与 Version 2 对齐）
// ============================================================
typedef struct {
    // 基础统计
    int task_count;
    int ds_count;
    int lr_count;
    uint64_t ds_elements;
    uint64_t lr_elements;
    double total_time;
    double ds_time;
    double lr_time;
    
    // 初始几何数据传输（data_transfer中的copyin）
    double init_h2d_time;
    
    // Dense 时间分解
    double ds_h2d_time;
    double ds_compute_time;
    double ds_d2h_time;
    
    // LowRank 时间分解（修正点1: 分离 free_time）
    double lr_h2d_time;
    double lr_compute_time;
    double lr_d2h_time;
    double lr_free_time;        // 新增: GPU 内存释放时间（exit data delete），不再合并到 d2h

    // Dense buffer 复用统计
    uint64_t ds_buf_reuse;
    uint64_t ds_buf_realloc;
    uint64_t ds_buf_max_ns;
    int ds_buf_final_cap;
} gpu_stats_t;

// ============================================================
// 进程级统计结构
// ============================================================
typedef struct {
    // GPU获取统计
    int gpu_acquire_attempts;
    int gpu_acquire_failures;
    
    // 任务过滤统计
    int direct_cpu_ds_tasks;
    int direct_cpu_lr_tasks;
    int eligible_ds_tasks;
    int eligible_lr_tasks;
    
    // Wall clock time
    double wall_start_time;
    double wall_end_time;
} process_stats_t;

static process_stats_t proc_stats = {0};

// ============================================================
// 进程报告结构（修正点1: 新增 total_lr_free_time）
// ============================================================
typedef struct {
    int rank;
    int num_gpus;
    int most_used_gpu;
    int gpu_acquire_failures;
    int gpu_acquire_attempts;
    
    // 任务过滤统计
    int direct_cpu_ds_tasks;
    int direct_cpu_lr_tasks;
    int eligible_ds_tasks;
    int eligible_lr_tasks;
    
    // Wall clock time
    double wall_clock_time;
    
    uint64_t total_gpu_tasks;
    uint64_t total_ds_tasks;
    uint64_t total_lr_tasks;
    double total_gpu_time;
    
    // 初始几何数据传输时间
    double total_init_h2d_time;
    
    // Dense 时间分解总计
    double total_ds_h2d_time;
    double total_ds_compute_time;
    double total_ds_d2h_time;
    
    // LowRank 时间分解总计（修正点1: 分离 free）
    double total_lr_h2d_time;
    double total_lr_compute_time;
    double total_lr_d2h_time;
    double total_lr_free_time;      // 新增
    
    char hostname[256];
    int physical_gpu_ids[8];
    gpu_stats_t gpu[8];
    cpu_task_stats_t cpu_stats; 
} process_report_t;

static gpu_stats_t *gpu_stats = NULL;
static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
static int multi_gpu_initialized = 0;

// 报告文件名
static char gpu_report_filename[256] = "gpu_usage_report.txt";

struct cluster* resultCTlist;
#pragma acc declare create(resultCTlist)

int countCT = 0;
int nofc, nNode;
int kparam = 50;
double (*zgmid)[3];
double (*bgmid)[3];
int (*f2n)[3];
int th_ds, th_lr, th_minsz;

// ============================================================
// Utility: imax
// ============================================================
#pragma acc routine seq
static inline int imax(int a, int b) {
    return (a >= b) ? a : b;
}

// ============================================================
// 设置报告文件名
// ============================================================
void set_gpu_report_filename(const char *filename) {
    if (filename && strlen(filename) < sizeof(gpu_report_filename)) {
        strncpy(gpu_report_filename, filename, sizeof(gpu_report_filename) - 1);
        gpu_report_filename[sizeof(gpu_report_filename) - 1] = '\0';
    }
}

// ============================================================
// Wall Clock Time 管理
// ============================================================
void start_wall_clock() {
    proc_stats.wall_start_time = get_time();
}

void stop_wall_clock() {
    proc_stats.wall_end_time = get_time();
}

// 简化版：直接设置 wall clock time
static double wall_clock_time_value = 0.0;

void set_wall_clock_time(double t) {
    wall_clock_time_value = t;
}

double get_wall_clock_time() {
    return wall_clock_time_value;
}

// ============================================================
// 获取物理GPU ID映射
// ============================================================
static void get_physical_gpu_mapping() {
    total_hw_gpus = num_gpus;

    int local_rank = 0;
    int local_size = 1;

    char (*all_hostnames)[256] = (char (*)[256])malloc(num_processes * 256);
    if (!all_hostnames) {
        fprintf(stderr, "ERROR: Process %d failed to allocate hostname buffer\n", my_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    MPI_Allgather(my_hostname, 256, MPI_CHAR, all_hostnames, 256, MPI_CHAR, MPI_COMM_WORLD);

    local_size = 0;
    local_rank = 0;
    for (int i = 0; i < num_processes; i++) {
        if (strcmp(my_hostname, all_hostnames[i]) == 0) {
            if (i < my_rank) {
                local_rank++;
            }
            local_size++;
        }
    }
    free(all_hostnames);

    int gpus_per_proc = total_hw_gpus / local_size;
    if (gpus_per_proc < 1) gpus_per_proc = 1;

    num_gpus = gpus_per_proc;

    physical_gpu_ids = (int *)malloc(num_gpus * sizeof(int));
    if (!physical_gpu_ids) {
        fprintf(stderr, "ERROR: Process %d failed to allocate physical_gpu_ids\n", my_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int gpu_start = local_rank * gpus_per_proc;
    for (int i = 0; i < num_gpus; i++) {
        physical_gpu_ids[i] = gpu_start + i;
    }

    // fprintf(stderr, "Process %d on %s: local_rank=%d/%d, hw_gpus=%d, gpus_per_proc=%d, GPUs [%d->%d",
    //         my_rank, my_hostname, local_rank, local_size, total_hw_gpus, gpus_per_proc,
    //         0, physical_gpu_ids[0]);
    // for (int i = 1; i < num_gpus; i++) {
    //     fprintf(stderr, ", %d->%d", i, physical_gpu_ids[i]);
    // }
    // fprintf(stderr, "]\n");
}

// ============================================================
// Initialize multi-GPU environment
// ============================================================
void initialize_multi_gpu() {
    pthread_mutex_lock(&init_mutex);

    if (!multi_gpu_initialized) {
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

        // 修正点4: 使用与 Version 2 对齐的 profiling 初始化
        init_profiling();

        if (gethostname(my_hostname, sizeof(my_hostname)) != 0) {
            snprintf(my_hostname, sizeof(my_hostname), "unknown");
        }
        my_hostname[sizeof(my_hostname) - 1] = '\0';

        num_gpus = acc_get_num_devices(acc_device_nvidia);
        
        fprintf(stderr, "Process %d of %d on node [%s]: auto-detected %d GPU(s), profiling=%s\n", 
                my_rank, num_processes, my_hostname, num_gpus,
                PROF_ENABLED ? "ON" : "OFF");

        if (num_gpus == 0) {
            fprintf(stderr, "ERROR: Process %d - no GPUs available!\n", my_rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        const char *env_filename = getenv("GPU_REPORT_FILE");
        if (env_filename) {
            set_gpu_report_filename(env_filename);
        }

        // 先计算物理GPU映射（会更新 num_gpus 为每进程实际使用的数量）
        get_physical_gpu_mapping();

        // 用更新后的 num_gpus 分配资源
        gpu_mutexes = (pthread_mutex_t *)malloc(num_gpus * sizeof(pthread_mutex_t));
        gpu_data_ready = (int *)calloc(num_gpus, sizeof(int));
        gpu_stats = (gpu_stats_t *)calloc(num_gpus, sizeof(gpu_stats_t));

        if (!gpu_mutexes || !gpu_data_ready || !gpu_stats) {
            fprintf(stderr, "ERROR: Process %d failed to allocate GPU resources\n", my_rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        for (int i = 0; i < num_gpus; i++) {
            pthread_mutex_init(&gpu_mutexes[i], NULL);
        }

        // 初始化进程统计
        memset(&proc_stats, 0, sizeof(process_stats_t));
        
        multi_gpu_initialized = 1;
    }

    pthread_mutex_unlock(&init_mutex);
}

// ============================================================
// GPU acquisition
// ============================================================
int try_acquire_any_gpu(int worker_id) {
    if (!multi_gpu_initialized) {
        initialize_multi_gpu();
    }

    __sync_fetch_and_add(&proc_stats.gpu_acquire_attempts, 1);

    int start_offset = worker_id % num_gpus;
    for (int i = 0; i < num_gpus; i++) {
        int gpu_id = (start_offset + i) % num_gpus;
        if (pthread_mutex_trylock(&gpu_mutexes[gpu_id]) == 0) {
            return gpu_id;
        }
    }

    __sync_fetch_and_add(&proc_stats.gpu_acquire_failures, 1);
    return -1;
}

// ============================================================
// 获取统计数据（供外部调用）
// ============================================================
int get_gpu_acquire_attempts() {
    return proc_stats.gpu_acquire_attempts;
}

int get_gpu_acquire_failures() {
    return proc_stats.gpu_acquire_failures;
}

int get_direct_cpu_ds_tasks() {
    return proc_stats.direct_cpu_ds_tasks;
}

int get_direct_cpu_lr_tasks() {
    return proc_stats.direct_cpu_lr_tasks;
}

int get_eligible_ds_tasks() {
    return proc_stats.eligible_ds_tasks;
}

int get_eligible_lr_tasks() {
    return proc_stats.eligible_lr_tasks;
}

void release_gpu(int gpu_id) {
    if (gpu_id >= 0 && gpu_id < num_gpus) {
        pthread_mutex_unlock(&gpu_mutexes[gpu_id]);
    }
}

// ============================================================
// 获取物理GPU ID
// ============================================================
int get_physical_gpu_id(int logical_gpu_id) {
    if (!multi_gpu_initialized) {
        initialize_multi_gpu();
    }
    if (logical_gpu_id >= 0 && logical_gpu_id < num_gpus && physical_gpu_ids) {
        return physical_gpu_ids[logical_gpu_id];
    }
    return logical_gpu_id;
}

// ============================================================
// Transfer geometry data to all GPUs
// ============================================================
void data_transfer() {
    if (!multi_gpu_initialized) {
        initialize_multi_gpu();
    }

    // 初始化 Dense buffer 池（首次调用时）
    if (!ds_buf) {
        ds_buf = (double **)calloc(num_gpus, sizeof(double *));
        ds_buf_cap = (int *)calloc(num_gpus, sizeof(int));
        ds_buf_stat = (ds_buf_stats_t *)calloc(num_gpus, sizeof(ds_buf_stats_t));
        if (!ds_buf || !ds_buf_cap || !ds_buf_stat) {
            fprintf(stderr, "ERROR: Process %d failed to allocate Dense buffer pool\n", my_rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        const char *env_cap = getenv("DS_BUF_INIT_CAP");
        ds_buf_init_cap = env_cap ? atoi(env_cap) : (512 * 512);
        if (ds_buf_init_cap < 1024) ds_buf_init_cap = 1024;
    }

    for (int gpu_id = 0; gpu_id < num_gpus; gpu_id++) {
        if (gpu_data_ready[gpu_id]) continue;

        acc_set_device_num(physical_gpu_ids[gpu_id], acc_device_nvidia);

        // 修正点4: 使用 PROF_TIME_START/END 宏，非 profiling 零开销
        PROF_TIME_START(_dt_start);
        #pragma acc enter data copyin(nofc, nNode, \
                                      zgmid[0:nofc][0:3], \
                                      f2n[0:nofc][0:3], \
                                      bgmid[0:nNode][0:3])
        PROF_ACC_WAIT();
        PROF_TIME_END(_dt_start, gpu_stats[gpu_id].init_h2d_time);

        // 为该 GPU 预分配 Dense 复用缓冲区
        PROF_TIME_START(_buf_start);
        ds_buf_cap[gpu_id] = ds_buf_init_cap;
        ds_buf[gpu_id] = (double *)malloc(sizeof(double) * ds_buf_cap[gpu_id]);
        if (!ds_buf[gpu_id]) {
            fprintf(stderr, "ERROR: Process %d failed to allocate Dense buffer for GPU %d\n", my_rank, gpu_id);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        #pragma acc enter data create(ds_buf[gpu_id:1][0:ds_buf_cap[gpu_id]])
        PROF_ACC_WAIT();
        PROF_TIME_END(_buf_start, gpu_stats[gpu_id].init_h2d_time);

        gpu_data_ready[gpu_id] = 1;

        if (PROF_ENABLED) {
            fprintf(stderr, "Rank %d: data_transfer GPU L%d->P%d: init_h2d %.3f s, Dense buf %d doubles (%.1f MB)\n",
                    my_rank, gpu_id, physical_gpu_ids[gpu_id], gpu_stats[gpu_id].init_h2d_time,
                    ds_buf_cap[gpu_id], ds_buf_cap[gpu_id] * 8.0 / 1048576.0);
        }
    }
}

// ============================================================
// Dense buffer 按需扩容
// ============================================================
static void ds_buf_ensure(int gpu_id, int required_cap) {
    if (required_cap <= ds_buf_cap[gpu_id]) {
        ds_buf_stat[gpu_id].reuse_count++;
        return;
    }

    ds_buf_stat[gpu_id].realloc_count++;

    #pragma acc exit data delete(ds_buf[gpu_id:1][0:ds_buf_cap[gpu_id]])
    free(ds_buf[gpu_id]);

    int new_cap = required_cap + (required_cap >> 1);
    ds_buf[gpu_id] = (double *)malloc(sizeof(double) * new_cap);
    if (!ds_buf[gpu_id]) {
        fprintf(stderr, "ERROR: Dense buffer realloc failed: gpu_id=%d, new_cap=%d (%.1f MB)\n",
                gpu_id, new_cap, new_cap * 8.0 / 1048576.0);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    #pragma acc enter data create(ds_buf[gpu_id:1][0:new_cap])

    int old_cap = ds_buf_cap[gpu_id];
    ds_buf_cap[gpu_id] = new_cap;

    fprintf(stderr, "Rank %d: Dense buf GPU L%d grow %d -> %d doubles (%.1f -> %.1f MB)\n",
            my_rank, gpu_id, old_cap, new_cap,
            old_cap * 8.0 / 1048576.0, new_cap * 8.0 / 1048576.0);
}

// ============================================================
// Record GPU usage（修正点1: Dense 和 LowRank 分别记录，LR 分离 free_time）
// ============================================================
static void record_gpu_usage_dense(int gpu_id, uint64_t size,
                                   double h2d_time, double compute_time, double d2h_time) {
    if (gpu_id < 0 || gpu_id >= num_gpus) return;

    double total_duration = h2d_time + compute_time + d2h_time;
    
    gpu_stats[gpu_id].task_count++;
    gpu_stats[gpu_id].total_time += total_duration;
    gpu_stats[gpu_id].ds_count++;
    gpu_stats[gpu_id].ds_time += total_duration;
    gpu_stats[gpu_id].ds_elements += size;
    gpu_stats[gpu_id].ds_h2d_time += h2d_time;
    gpu_stats[gpu_id].ds_compute_time += compute_time;
    gpu_stats[gpu_id].ds_d2h_time += d2h_time;
}

static void record_gpu_usage_lr(int gpu_id, uint64_t size,
                                double h2d_time, double compute_time,
                                double d2h_time, double free_time) {
    if (gpu_id < 0 || gpu_id >= num_gpus) return;

    double total_duration = h2d_time + compute_time + d2h_time + free_time;
    
    gpu_stats[gpu_id].task_count++;
    gpu_stats[gpu_id].total_time += total_duration;
    gpu_stats[gpu_id].lr_count++;
    gpu_stats[gpu_id].lr_time += total_duration;
    gpu_stats[gpu_id].lr_elements += size;
    gpu_stats[gpu_id].lr_h2d_time += h2d_time;
    gpu_stats[gpu_id].lr_compute_time += compute_time;
    gpu_stats[gpu_id].lr_d2h_time += d2h_time;
    gpu_stats[gpu_id].lr_free_time += free_time;
}

// ============================================================
// CPU 任务时间记录（运行时 PROF_ENABLED 控制）
// ============================================================
static pthread_mutex_t cpu_stats_mutex = PTHREAD_MUTEX_INITIALIZER;

// cpu_task_stats_t 已在 filling.h 中定义
static cpu_task_stats_t cpu_stats = {0};

static void record_cpu_task_dense(double duration, uint64_t elements, int is_fallback) {
    if (!PROF_ENABLED) return;
    pthread_mutex_lock(&cpu_stats_mutex);
    cpu_stats.cpu_ds_tasks++;
    cpu_stats.cpu_ds_total_time += duration;
    cpu_stats.cpu_ds_total_elements += elements;
    if (is_fallback) cpu_stats.cpu_ds_fallback_tasks++;
    else             cpu_stats.cpu_ds_direct_tasks++;
    if (cpu_stats.cpu_ds_tasks == 1) {
        cpu_stats.cpu_ds_min_time = duration;
        cpu_stats.cpu_ds_max_time = duration;
    } else {
        if (duration < cpu_stats.cpu_ds_min_time) cpu_stats.cpu_ds_min_time = duration;
        if (duration > cpu_stats.cpu_ds_max_time) cpu_stats.cpu_ds_max_time = duration;
    }
    pthread_mutex_unlock(&cpu_stats_mutex);
}

static void record_cpu_task_lr(double duration, uint64_t elements, int is_fallback) {
    if (!PROF_ENABLED) return;
    pthread_mutex_lock(&cpu_stats_mutex);
    cpu_stats.cpu_lr_tasks++;
    cpu_stats.cpu_lr_total_time += duration;
    cpu_stats.cpu_lr_total_elements += elements;
    if (is_fallback) cpu_stats.cpu_lr_fallback_tasks++;
    else             cpu_stats.cpu_lr_direct_tasks++;
    if (cpu_stats.cpu_lr_tasks == 1) {
        cpu_stats.cpu_lr_min_time = duration;
        cpu_stats.cpu_lr_max_time = duration;
    } else {
        if (duration < cpu_stats.cpu_lr_min_time) cpu_stats.cpu_lr_min_time = duration;
        if (duration > cpu_stats.cpu_lr_max_time) cpu_stats.cpu_lr_max_time = duration;
    }
    pthread_mutex_unlock(&cpu_stats_mutex);
}

// ============================================================
// GPU usage report - 输出到文件（增强版，修正点1: LR 分离 free）
// ============================================================
void print_gpu_usage_report() {
    MPI_Barrier(MPI_COMM_WORLD);
    
    if (proc_stats.wall_end_time == 0) {
        stop_wall_clock();
    }
    
    // 准备本进程的报告数据
    process_report_t my_report;
    memset(&my_report, 0, sizeof(process_report_t));
    
    my_report.rank = my_rank;
    my_report.num_gpus = num_gpus;
    my_report.gpu_acquire_failures = proc_stats.gpu_acquire_failures;
    my_report.gpu_acquire_attempts = proc_stats.gpu_acquire_attempts;
    
    my_report.direct_cpu_ds_tasks = proc_stats.direct_cpu_ds_tasks;
    my_report.direct_cpu_lr_tasks = proc_stats.direct_cpu_lr_tasks;
    my_report.eligible_ds_tasks = proc_stats.eligible_ds_tasks;
    my_report.eligible_lr_tasks = proc_stats.eligible_lr_tasks;
    my_report.wall_clock_time = get_wall_clock_time();
    
    strncpy(my_report.hostname, my_hostname, sizeof(my_report.hostname) - 1);
    my_report.hostname[sizeof(my_report.hostname) - 1] = '\0';
    
    for (int i = 0; i < num_gpus && i < 8; i++) {
        my_report.physical_gpu_ids[i] = physical_gpu_ids ? physical_gpu_ids[i] : i;
    }
    
    int most_used_gpu = 0;
    int max_tasks = 0;
    
    for (int i = 0; i < num_gpus && i < 8; i++) {
        // 将 Dense buffer 统计写入 gpu_stats
        if (ds_buf_stat) {
            gpu_stats[i].ds_buf_reuse = ds_buf_stat[i].reuse_count;
            gpu_stats[i].ds_buf_realloc = ds_buf_stat[i].realloc_count;
            gpu_stats[i].ds_buf_max_ns = ds_buf_stat[i].max_ns;
        }
        if (ds_buf_cap) {
            gpu_stats[i].ds_buf_final_cap = ds_buf_cap[i];
        }

        my_report.gpu[i] = gpu_stats[i];
        my_report.total_gpu_tasks += gpu_stats[i].task_count;
        my_report.total_ds_tasks += gpu_stats[i].ds_count;
        my_report.total_lr_tasks += gpu_stats[i].lr_count;
        my_report.total_gpu_time += gpu_stats[i].total_time;
        
        my_report.total_init_h2d_time += gpu_stats[i].init_h2d_time;
        my_report.total_ds_h2d_time += gpu_stats[i].ds_h2d_time;
        my_report.total_ds_compute_time += gpu_stats[i].ds_compute_time;
        my_report.total_ds_d2h_time += gpu_stats[i].ds_d2h_time;
        my_report.total_lr_h2d_time += gpu_stats[i].lr_h2d_time;
        my_report.total_lr_compute_time += gpu_stats[i].lr_compute_time;
        my_report.total_lr_d2h_time += gpu_stats[i].lr_d2h_time;
        my_report.total_lr_free_time += gpu_stats[i].lr_free_time;
        
        if (gpu_stats[i].task_count > max_tasks) {
            max_tasks = gpu_stats[i].task_count;
            most_used_gpu = i;
        }
    }
    my_report.most_used_gpu = most_used_gpu;
    
    // 收集所有进程的报告到 rank 0
    process_report_t *all_reports = NULL;
    if (my_rank == 0) {
        all_reports = (process_report_t *)malloc(num_processes * sizeof(process_report_t));
    }
    
    MPI_Gather(&my_report, sizeof(process_report_t), MPI_BYTE,
               all_reports, sizeof(process_report_t), MPI_BYTE,
               0, MPI_COMM_WORLD);
    
    // ================================================================
    // ★★★ 修正: MPI_Reduce 在 if(my_rank==0) 之外，所有进程都参与 ★★★
    // ================================================================
    uint64_t total_cpu_ds_tasks = 0;
    uint64_t total_cpu_ds_fallback = 0;
    uint64_t total_cpu_ds_direct = 0;
    uint64_t total_cpu_ds_elements = 0;
    double total_cpu_ds_time = 0.0;
    double min_cpu_ds_time = 1e9;
    double max_cpu_ds_time_val = 0.0;

    uint64_t total_cpu_lr_tasks = 0;
    uint64_t total_cpu_lr_fallback = 0;
    uint64_t total_cpu_lr_direct = 0;
    uint64_t total_cpu_lr_elements = 0;
    double total_cpu_lr_time = 0.0;
    double min_cpu_lr_time = 1e9;
    double max_cpu_lr_time_val = 0.0;

    double total_cpu_ds_entry_time = 0.0;
    double total_cpu_lr_entry_time = 0.0;
    double total_cpu_lr_blas_time = 0.0;

    if (PROF_ENABLED) {
        MPI_Reduce(&cpu_stats.cpu_ds_tasks, &total_cpu_ds_tasks, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&cpu_stats.cpu_ds_fallback_tasks, &total_cpu_ds_fallback, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&cpu_stats.cpu_ds_direct_tasks, &total_cpu_ds_direct, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&cpu_stats.cpu_ds_total_elements, &total_cpu_ds_elements, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&cpu_stats.cpu_ds_total_time, &total_cpu_ds_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&cpu_stats.cpu_ds_min_time, &min_cpu_ds_time, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
        MPI_Reduce(&cpu_stats.cpu_ds_max_time, &max_cpu_ds_time_val, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

        MPI_Reduce(&cpu_stats.cpu_lr_tasks, &total_cpu_lr_tasks, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&cpu_stats.cpu_lr_fallback_tasks, &total_cpu_lr_fallback, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&cpu_stats.cpu_lr_direct_tasks, &total_cpu_lr_direct, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&cpu_stats.cpu_lr_total_elements, &total_cpu_lr_elements, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&cpu_stats.cpu_lr_total_time, &total_cpu_lr_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&cpu_stats.cpu_lr_min_time, &min_cpu_lr_time, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
        MPI_Reduce(&cpu_stats.cpu_lr_max_time, &max_cpu_lr_time_val, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

        MPI_Reduce(&cpu_stats.cpu_ds_entry_compute_time, &total_cpu_ds_entry_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&cpu_stats.cpu_lr_entry_compute_time, &total_cpu_lr_entry_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(&cpu_stats.cpu_lr_blas_time, &total_cpu_lr_blas_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    }
    
    // Rank 0 写入文件
    if (my_rank == 0) {
        FILE *fp = fopen(gpu_report_filename, "w");
        if (!fp) {
            fprintf(stderr, "WARNING: Cannot open %s for writing, using stdout\n", gpu_report_filename);
            fp = stdout;
        }
        
        time_t now = time(NULL);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
        
        fprintf(fp, "================================================================================\n");
        fprintf(fp, "                    GPU USAGE REPORT (Enhanced Version)                        \n");
        fprintf(fp, "================================================================================\n");
        fprintf(fp, "Generated: %s\n", time_str);
        fprintf(fp, "Configuration: %d MPI processes x %d GPUs/process = %d total GPU contexts\n",
               num_processes, num_gpus, num_processes * num_gpus);
        fprintf(fp, "Thresholds: th_ds=%d, th_lr=%d\n", th_ds, th_lr);
        fprintf(fp, "Profiling Mode: %s\n\n",
               PROF_ENABLED ? "ON (precise H2D/Compute/D2H/Free)"
                            : "OFF (time breakdown unavailable)");
        
        // ================================================================
        // 节点分布
        // ================================================================
        fprintf(fp, "┌─────────────────────────────────────────────────────────────────────────────┐\n");
        fprintf(fp, "│                           NODE DISTRIBUTION                                 │\n");
        fprintf(fp, "├─────────────────────────────────────────────────────────────────────────────┤\n");
        
        typedef struct {
            char hostname[256];
            int process_count;
            int ranks[64];
        } node_info_t;
        
        node_info_t *nodes = (node_info_t *)calloc(num_processes, sizeof(node_info_t));
        int num_nodes = 0;
        
        for (int r = 0; r < num_processes; r++) {
            int found = 0;
            for (int n = 0; n < num_nodes; n++) {
                if (strcmp(nodes[n].hostname, all_reports[r].hostname) == 0) {
                    nodes[n].ranks[nodes[n].process_count] = r;
                    nodes[n].process_count++;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                strncpy(nodes[num_nodes].hostname, all_reports[r].hostname, 255);
                nodes[num_nodes].ranks[0] = r;
                nodes[num_nodes].process_count = 1;
                num_nodes++;
            }
        }
        
        fprintf(fp, "│  Total Nodes: %-3d                                                           │\n", num_nodes);
        fprintf(fp, "├─────────────────────────────────────────────────────────────────────────────┤\n");
        
        for (int n = 0; n < num_nodes; n++) {
            fprintf(fp, "│  Node: %-20s  Processes: %-3d                              │\n",
                   nodes[n].hostname, nodes[n].process_count);
            fprintf(fp, "│    Ranks: ");
            int chars_printed = 11;
            for (int i = 0; i < nodes[n].process_count && i < 64; i++) {
                int needed = snprintf(NULL, 0, "%d%s", nodes[n].ranks[i], 
                                     (i < nodes[n].process_count - 1) ? "," : "");
                if (chars_printed + needed > 75) {
                    fprintf(fp, "\n│           ");
                    chars_printed = 11;
                }
                chars_printed += fprintf(fp, "%d%s", nodes[n].ranks[i], 
                                        (i < nodes[n].process_count - 1) ? "," : "");
            }
            for (; chars_printed < 76; chars_printed++) fprintf(fp, " ");
            fprintf(fp, "│\n");
        }
        fprintf(fp, "└─────────────────────────────────────────────────────────────────────────────┘\n\n");
        
        free(nodes);
        
        // ================================================================
        // 全局统计汇总
        // ================================================================
        uint64_t global_gpu_tasks = 0;
        uint64_t global_ds_tasks = 0;
        uint64_t global_lr_tasks = 0;
        uint64_t global_ds_elements = 0;
        uint64_t global_lr_elements = 0;
        double global_gpu_time = 0.0;
        double global_init_h2d_time = 0.0;
        double global_ds_h2d_time = 0.0;
        double global_ds_compute_time = 0.0;
        double global_ds_d2h_time = 0.0;
        double global_lr_h2d_time = 0.0;
        double global_lr_compute_time = 0.0;
        double global_lr_d2h_time = 0.0;
        double global_lr_free_time = 0.0;
        int global_failures = 0;
        int global_attempts = 0;
        int global_direct_cpu_ds = 0;
        int global_direct_cpu_lr = 0;
        int global_eligible_ds = 0;
        int global_eligible_lr = 0;
        double max_wall_time = 0.0;
        
        for (int r = 0; r < num_processes; r++) {
            global_gpu_tasks += all_reports[r].total_gpu_tasks;
            global_ds_tasks += all_reports[r].total_ds_tasks;
            global_lr_tasks += all_reports[r].total_lr_tasks;
            global_gpu_time += all_reports[r].total_gpu_time;
            global_init_h2d_time += all_reports[r].total_init_h2d_time;
            global_ds_h2d_time += all_reports[r].total_ds_h2d_time;
            global_ds_compute_time += all_reports[r].total_ds_compute_time;
            global_ds_d2h_time += all_reports[r].total_ds_d2h_time;
            global_lr_h2d_time += all_reports[r].total_lr_h2d_time;
            global_lr_compute_time += all_reports[r].total_lr_compute_time;
            global_lr_d2h_time += all_reports[r].total_lr_d2h_time;
            global_lr_free_time += all_reports[r].total_lr_free_time;
            global_failures += all_reports[r].gpu_acquire_failures;
            global_attempts += all_reports[r].gpu_acquire_attempts;
            global_direct_cpu_ds += all_reports[r].direct_cpu_ds_tasks;
            global_direct_cpu_lr += all_reports[r].direct_cpu_lr_tasks;
            global_eligible_ds += all_reports[r].eligible_ds_tasks;
            global_eligible_lr += all_reports[r].eligible_lr_tasks;
            
            if (all_reports[r].wall_clock_time > max_wall_time) {
                max_wall_time = all_reports[r].wall_clock_time;
            }
            
            for (int g = 0; g < num_gpus && g < 8; g++) {
                global_ds_elements += all_reports[r].gpu[g].ds_elements;
                global_lr_elements += all_reports[r].gpu[g].lr_elements;
            }
        }
        
        double success_rate = (global_attempts > 0) ? 
            100.0 * (global_attempts - global_failures) / global_attempts : 0.0;
        
        // ================================================================
        // 任务过滤分析
        // ================================================================
        fprintf(fp, "┌─────────────────────────────────────────────────────────────────────────────┐\n");
        fprintf(fp, "│                    ★ TASK FILTERING ANALYSIS ★                             │\n");
        fprintf(fp, "├─────────────────────────────────────────────────────────────────────────────┤\n");
        
        int total_direct_cpu = global_direct_cpu_ds + global_direct_cpu_lr;
        int total_eligible = global_eligible_ds + global_eligible_lr;
        int total_all_tasks = total_direct_cpu + total_eligible;
        int total_gpu_executed = (int)global_gpu_tasks;
        int total_fallback = total_eligible - total_gpu_executed;
        
        fprintf(fp, "│                                                                             │\n");
        fprintf(fp, "│  ALL TASKS: %-10d                                                      │\n", total_all_tasks);
        fprintf(fp, "│  ┌─────────────────────────────────────────────────────────────────────┐   │\n");
        fprintf(fp, "│  │                                                                     │   │\n");
        
        if (total_all_tasks > 0) {
            fprintf(fp, "│  │  ┌─ Direct CPU (size < threshold): %-8d (%5.1f%%)              │   │\n",
                    total_direct_cpu, 100.0 * total_direct_cpu / total_all_tasks);
            fprintf(fp, "│  │  │    ├─ Dense:   %-8d                                        │   │\n",
                    global_direct_cpu_ds);
            fprintf(fp, "│  │  │    └─ LowRank: %-8d                                        │   │\n",
                    global_direct_cpu_lr);
            fprintf(fp, "│  │  │                                                               │   │\n");
            fprintf(fp, "│  │  └─ GPU Eligible (size >= threshold): %-8d (%5.1f%%)          │   │\n",
                    total_eligible, 100.0 * total_eligible / total_all_tasks);
            fprintf(fp, "│  │       ├─ Dense:   %-8d                                        │   │\n",
                    global_eligible_ds);
            fprintf(fp, "│  │       └─ LowRank: %-8d                                        │   │\n",
                    global_eligible_lr);
        }
        
        fprintf(fp, "│  │                                                                     │   │\n");
        fprintf(fp, "│  └─────────────────────────────────────────────────────────────────────┘   │\n");
        fprintf(fp, "│                                                                             │\n");
        
        if (total_eligible > 0) {
            fprintf(fp, "│  GPU ELIGIBLE BREAKDOWN:                                                    │\n");
            fprintf(fp, "│  ┌─────────────────────────────────────────────────────────────────────┐   │\n");
            fprintf(fp, "│  │  ├─ GPU Executed:  %-8d (%5.1f%% of eligible)                   │   │\n",
                    total_gpu_executed, 100.0 * total_gpu_executed / total_eligible);
            fprintf(fp, "│  │  └─ Fallback CPU:  %-8d (%5.1f%% of eligible)                   │   │\n",
                    total_fallback, 100.0 * total_fallback / total_eligible);
            fprintf(fp, "│  └─────────────────────────────────────────────────────────────────────┘   │\n");
        }
        
        fprintf(fp, "│                                                                             │\n");
        fprintf(fp, "│  SUMMARY:                                                                   │\n");
        fprintf(fp, "│    Total CPU Executed: %-8d (Direct: %-6d + Fallback: %-6d)         │\n",
                total_direct_cpu + total_fallback, total_direct_cpu, total_fallback);
        fprintf(fp, "│    Total GPU Executed: %-8d                                             │\n",
                total_gpu_executed);
        fprintf(fp, "│                                                                             │\n");
        fprintf(fp, "└─────────────────────────────────────────────────────────────────────────────┘\n\n");
        
        // ================================================================
        // 全局汇总
        // ================================================================
        fprintf(fp, "┌─────────────────────────────────────────────────────────────────────────────┐\n");
        fprintf(fp, "│                           GLOBAL SUMMARY                                    │\n");
        fprintf(fp, "├─────────────────────────────────────────────────────────────────────────────┤\n");
        fprintf(fp, "│  Total GPU Tasks:      %-12lu                                        │\n", (unsigned long)global_gpu_tasks);
        fprintf(fp, "│    - Dense Tasks:      %-12lu  (elements: %-16lu)        │\n", (unsigned long)global_ds_tasks, (unsigned long)global_ds_elements);
        fprintf(fp, "│    - LowRank Tasks:    %-12lu  (elements: %-16lu)        │\n", (unsigned long)global_lr_tasks, (unsigned long)global_lr_elements);
        fprintf(fp, "│  Total GPU Time:       %-12.3f s                                       │\n", global_gpu_time);
        fprintf(fp, "├─────────────────────────────────────────────────────────────────────────────┤\n");
        fprintf(fp, "│  GPU Acquire Statistics:                                                    │\n");
        fprintf(fp, "│    - Attempts:         %-12d                                        │\n", global_attempts);
        fprintf(fp, "│    - Failures:         %-12d (fell back to CPU)                      │\n", global_failures);
        fprintf(fp, "│    - Success Rate:     %-12.2f %%                                       │\n", success_rate);
        fprintf(fp, "├─────────────────────────────────────────────────────────────────────────────┤\n");
        fprintf(fp, "│  Wall Clock Time:      %-12.3f s                                       │\n", max_wall_time);
        fprintf(fp, "│  Avg Tasks/Process:    %-12.1f                                        │\n", (double)global_gpu_tasks / num_processes);
        fprintf(fp, "│  Avg Time/GPU:         %-12.3f s                                       │\n", global_gpu_time / (num_processes * num_gpus));
        fprintf(fp, "└─────────────────────────────────────────────────────────────────────────────┘\n\n");
        
        // ================================================================
        // GPU时间分解
        // ================================================================
        fprintf(fp, "┌─────────────────────────────────────────────────────────────────────────────┐\n");
        fprintf(fp, "│                    ★ GPU TIME BREAKDOWN ★                                  │\n");
        fprintf(fp, "├─────────────────────────────────────────────────────────────────────────────┤\n");
        
        int has_profiling_data = 0;
        for (int r = 0; r < num_processes && !has_profiling_data; r++) {
            if (all_reports[r].total_init_h2d_time > 0 ||
                all_reports[r].total_ds_h2d_time > 0 ||
                all_reports[r].total_lr_h2d_time > 0) {
                has_profiling_data = 1;
            }
        }
        
        if (!has_profiling_data && global_gpu_time > 0) {
            fprintf(fp, "│  ⚠ Profiling mode was OFF. Time breakdown is unavailable.                  │\n");
            fprintf(fp, "│  To enable: compile with -DGPU_PROFILING or -DGPU_PROFILING_RUNTIME        │\n");
            fprintf(fp, "│  (runtime mode: set GPU_PROFILING=1 environment variable)                   │\n");
            fprintf(fp, "│                                                                             │\n");
            fprintf(fp, "│  Total GPU Time: %.3f s (Dense + Low-Rank, no per-phase breakdown)        │\n",
                    global_gpu_time);
        } else if (global_gpu_time > 0 || global_init_h2d_time > 0) {
            double ds_total = 0.0, lr_total = 0.0;
            for (int r = 0; r < num_processes; r++) {
                for (int g = 0; g < num_gpus && g < 8; g++) {
                    ds_total += all_reports[r].gpu[g].ds_time;
                    lr_total += all_reports[r].gpu[g].lr_time;
                }
            }
            
            fprintf(fp, "│                                                                             │\n");
            fprintf(fp, "│  INITIAL DATA TRANSFER (zgmid/f2n/bgmid copyin + Dense buf, once per GPU): │\n");
            fprintf(fp, "│    Total:                       %10.3f s                               │\n",
                    global_init_h2d_time);
            fprintf(fp, "│    (Not included in Dense/LR breakdown below)                               │\n");
            
            fprintf(fp, "│                                                                             │\n");
            fprintf(fp, "│  DENSE BLOCKS:                                                              │\n");
            if (ds_total > 0) {
                double ds_h2d_pct = 100.0 * global_ds_h2d_time / ds_total;
                double ds_comp_pct = 100.0 * global_ds_compute_time / ds_total;
                double ds_d2h_pct = 100.0 * global_ds_d2h_time / ds_total;
                
                fprintf(fp, "│    Buf Ensure (reuse/grow):      %10.3f s  (%5.1f%%)                   │\n",
                        global_ds_h2d_time, ds_h2d_pct);
                fprintf(fp, "│    GPU Compute:                 %10.3f s  (%5.1f%%)                   │\n",
                        global_ds_compute_time, ds_comp_pct);
                fprintf(fp, "│    D2H (update host):           %10.3f s  (%5.1f%%)                   │\n",
                        global_ds_d2h_time, ds_d2h_pct);
                fprintf(fp, "│    ─────────────────────────────────────────                             │\n");
                fprintf(fp, "│    Dense Total:                 %10.3f s  (100.0%%)                   │\n", ds_total);
                
                fprintf(fp, "│    Visual: [");
                int h_bars = (int)(ds_h2d_pct / 2.5); if (h_bars > 40) h_bars = 40;
                int c_bars = (int)(ds_comp_pct / 2.5); if (c_bars > 40) c_bars = 40;
                int d_bars = (int)(ds_d2h_pct / 2.5); if (d_bars > 40) d_bars = 40;
                for (int b = 0; b < h_bars; b++) fprintf(fp, ">");
                for (int b = 0; b < c_bars; b++) fprintf(fp, "#");
                for (int b = 0; b < d_bars; b++) fprintf(fp, "<");
                int tb = h_bars + c_bars + d_bars;
                for (int b = tb; b < 40; b++) fprintf(fp, " ");
                fprintf(fp, "]     │\n");
                fprintf(fp, "│            Legend: > H2D, # Compute, < D2H                                │\n");
            } else {
                fprintf(fp, "│    No Dense GPU tasks recorded.                                           │\n");
            }
            
            fprintf(fp, "│                                                                             │\n");
            fprintf(fp, "│  LOW-RANK BLOCKS:                                                           │\n");
            if (lr_total > 0) {
                double lr_h2d_pct = 100.0 * global_lr_h2d_time / lr_total;
                double lr_comp_pct = 100.0 * global_lr_compute_time / lr_total;
                double lr_d2h_pct = 100.0 * global_lr_d2h_time / lr_total;
                double lr_free_pct = 100.0 * global_lr_free_time / lr_total;
                
                fprintf(fp, "│    H2D (copyin + update device):   %10.3f s  (%5.1f%%)                 │\n",
                        global_lr_h2d_time, lr_h2d_pct);
                fprintf(fp, "│    GPU Compute (acc parallel):     %10.3f s  (%5.1f%%)                 │\n",
                        global_lr_compute_time, lr_comp_pct);
                fprintf(fp, "│    D2H (update host):              %10.3f s  (%5.1f%%)                 │\n",
                        global_lr_d2h_time, lr_d2h_pct);
                fprintf(fp, "│    GPU Mem Free (exit data delete):%10.3f s  (%5.1f%%)                 │\n",
                        global_lr_free_time, lr_free_pct);
                fprintf(fp, "│    ─────────────────────────────────────────                             │\n");
                fprintf(fp, "│    Low-Rank Total:                 %10.3f s  (100.0%%)                 │\n",
                        lr_total);
                
                fprintf(fp, "│    Visual: [");
                int h_bars = (int)(lr_h2d_pct / 2.5); if (h_bars > 40) h_bars = 40;
                int c_bars = (int)(lr_comp_pct / 2.5); if (c_bars > 40) c_bars = 40;
                int d_bars = (int)(lr_d2h_pct / 2.5); if (d_bars > 40) d_bars = 40;
                int f_bars = (int)(lr_free_pct / 2.5); if (f_bars > 40) f_bars = 40;
                for (int b = 0; b < h_bars; b++) fprintf(fp, ">");
                for (int b = 0; b < c_bars; b++) fprintf(fp, "#");
                for (int b = 0; b < d_bars; b++) fprintf(fp, "<");
                for (int b = 0; b < f_bars; b++) fprintf(fp, "F");
                int tb = h_bars + c_bars + d_bars + f_bars;
                for (int b = tb; b < 40; b++) fprintf(fp, " ");
                fprintf(fp, "]     │\n");
                fprintf(fp, "│            Legend: > H2D, # Compute, < D2H, F Free                        │\n");
            } else {
                fprintf(fp, "│    No Low-Rank GPU tasks recorded.                                        │\n");
            }
            
            fprintf(fp, "│                                                                             │\n");
            fprintf(fp, "│  OVERALL:                                                                   │\n");
            double total_all_gpu = global_init_h2d_time + global_gpu_time;
            fprintf(fp, "│    Init H2D (geom):    %10.3f s  (%5.1f%%)                               │\n",
                    global_init_h2d_time, 
                    total_all_gpu > 0 ? 100.0 * global_init_h2d_time / total_all_gpu : 0.0);
            fprintf(fp, "│    Dense GPU Time:     %10.3f s  (%5.1f%%)                               │\n",
                    ds_total, total_all_gpu > 0 ? 100.0 * ds_total / total_all_gpu : 0.0);
            fprintf(fp, "│    Low-Rank GPU Time:  %10.3f s  (%5.1f%%)                               │\n",
                    lr_total, total_all_gpu > 0 ? 100.0 * lr_total / total_all_gpu : 0.0);
            fprintf(fp, "│    ─────────────────────────────────────────                                 │\n");
            fprintf(fp, "│    Total GPU Time:     %10.3f s  (100.0%%)                               │\n",
                    total_all_gpu);
        } else {
            fprintf(fp, "│  No GPU time recorded.                                                      │\n");
        }
        fprintf(fp, "│                                                                             │\n");
        fprintf(fp, "└─────────────────────────────────────────────────────────────────────────────┘\n\n");
        
        // ================================================================
        // Dense Buffer Pool 统计
        // ================================================================
        fprintf(fp, "┌─────────────────────────────────────────────────────────────────────────────┐\n");
        fprintf(fp, "│                    ★ DENSE BUFFER POOL ★                                   │\n");
        fprintf(fp, "├─────────────────────────────────────────────────────────────────────────────┤\n");
        
        uint64_t total_reuse = 0, total_realloc = 0;
        for (int r = 0; r < num_processes; r++) {
            for (int g = 0; g < all_reports[r].num_gpus && g < 8; g++) {
                total_reuse += all_reports[r].gpu[g].ds_buf_reuse;
                total_realloc += all_reports[r].gpu[g].ds_buf_realloc;
            }
        }
        uint64_t total_ds_gpu_uses = total_reuse + total_realloc;
        double hit_rate = (total_ds_gpu_uses > 0) ? 100.0 * total_reuse / total_ds_gpu_uses : 0.0;
        
        fprintf(fp, "│  Global: reuse %-8lu  realloc %-8lu  hit rate %6.2f%%                │\n",
                (unsigned long)total_reuse, (unsigned long)total_realloc, hit_rate);
        fprintf(fp, "│                                                                             │\n");
        
        for (int r = 0; r < num_processes; r++) {
            process_report_t *rpt = &all_reports[r];
            fprintf(fp, "│  Rank %d (%s):                                                  │\n",
                    rpt->rank, rpt->hostname);
            for (int g = 0; g < rpt->num_gpus && g < 8; g++) {
                int phys_id = rpt->physical_gpu_ids[g];
                fprintf(fp, "│    L%d->P%d: reuse %-6lu realloc %-4lu max_ns %-10lu cap %-10d│\n",
                        g, phys_id,
                        (unsigned long)rpt->gpu[g].ds_buf_reuse,
                        (unsigned long)rpt->gpu[g].ds_buf_realloc,
                        (unsigned long)rpt->gpu[g].ds_buf_max_ns,
                        rpt->gpu[g].ds_buf_final_cap);
            }
        }
        fprintf(fp, "│                                                                             │\n");
        fprintf(fp, "└─────────────────────────────────────────────────────────────────────────────┘\n\n");

        // ================================================================
        // GPU利用率
        // ================================================================
        fprintf(fp, "┌─────────────────────────────────────────────────────────────────────────────┐\n");
        fprintf(fp, "│                    ★ GPU UTILIZATION ★                                     │\n");
        fprintf(fp, "├─────────────────────────────────────────────────────────────────────────────┤\n");
        fprintf(fp, "│  Wall Clock Time: %.3f s                                                   │\n", max_wall_time);
        fprintf(fp, "│                                                                             │\n");
        
        for (int r = 0; r < num_processes; r++) {
            process_report_t *rpt = &all_reports[r];
            fprintf(fp, "│  Rank %d (%s):                                                  │\n",
                    rpt->rank, rpt->hostname);
            for (int g = 0; g < rpt->num_gpus && g < 8; g++) {
                double gpu_time = rpt->gpu[g].total_time;
                double util = (max_wall_time > 0) ? 100.0 * gpu_time / max_wall_time : 0.0;
                int phys_id = rpt->physical_gpu_ids[g];
                
                fprintf(fp, "│    L%d->P%d: %6.2f%% [", g, phys_id, util);
                int bars = (int)(util / 5);
                for (int b = 0; b < 20; b++) {
                    if (b < bars) fprintf(fp, "█");
                    else fprintf(fp, "░");
                }
                fprintf(fp, "] %7.3fs  DS:%-5d LR:%-5d │\n",
                        gpu_time, rpt->gpu[g].ds_count, rpt->gpu[g].lr_count);
            }
        }
        
        double total_possible_time = max_wall_time * num_processes * num_gpus;
        double overall_util = (total_possible_time > 0) ?
            100.0 * global_gpu_time / total_possible_time : 0.0;
        fprintf(fp, "│                                                                             │\n");
        fprintf(fp, "│  Overall GPU Utilization: %6.2f%%                                          │\n", overall_util);
        fprintf(fp, "│  (Total GPU Time / (Wall Time × Processes × GPUs))                         │\n");
        fprintf(fp, "│                                                                             │\n");
        fprintf(fp, "└─────────────────────────────────────────────────────────────────────────────┘\n\n");
        
        // ================================================================
        // 每进程摘要
        // ================================================================
        fprintf(fp, "┌──────────────────────────────────────────────────────────────────────────────────────────────────────────────┐\n");
        fprintf(fp, "│                                         PER-PROCESS SUMMARY                                                │\n");
        fprintf(fp, "├──────┬──────────────────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬────────────┤\n");
        fprintf(fp, "│ Rank │        Node          │   Tasks  │   Dense  │    LR    │ Time (s) │ Attempts │ Failures │  GPUs(P)   │\n");
        fprintf(fp, "├──────┼──────────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┼──────────┼────────────┤\n");
        
        for (int r = 0; r < num_processes; r++) {
            process_report_t *rpt = &all_reports[r];
            char gpu_str[64] = "";
            int pos = 0;
            for (int g = 0; g < rpt->num_gpus && g < 8; g++) {
                if (g > 0) pos += snprintf(gpu_str + pos, sizeof(gpu_str) - pos, ",");
                pos += snprintf(gpu_str + pos, sizeof(gpu_str) - pos, "%d", rpt->physical_gpu_ids[g]);
            }
            fprintf(fp, "│ %4d │ %-20s │ %8lu │ %8lu │ %8lu │ %8.3f │ %8d │ %8d │ %-10s │\n",
                   rpt->rank,
                   rpt->hostname,
                   (unsigned long)rpt->total_gpu_tasks,
                   (unsigned long)rpt->total_ds_tasks,
                   (unsigned long)rpt->total_lr_tasks,
                   rpt->total_gpu_time,
                   rpt->gpu_acquire_attempts,
                   rpt->gpu_acquire_failures,
                   gpu_str);
        }
        fprintf(fp, "└──────┴──────────────────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴────────────┘\n\n");
        
        // ================================================================
        // 节点GPU使用情况
        // ================================================================
        fprintf(fp, "┌─────────────────────────────────────────────────────────────────────────────┐\n");
        fprintf(fp, "│                      PER-NODE GPU USAGE SUMMARY                             │\n");
        fprintf(fp, "└─────────────────────────────────────────────────────────────────────────────┘\n");
        
        typedef struct {
            char hostname[256];
            int gpu_tasks[16];
            double gpu_time[16];
            int max_physical_gpu;
        } node_gpu_stats_t;
        
        node_gpu_stats_t *node_stats = (node_gpu_stats_t *)calloc(num_processes, sizeof(node_gpu_stats_t));
        int num_unique_nodes = 0;
        
        for (int r = 0; r < num_processes; r++) {
            int node_idx = -1;
            for (int n = 0; n < num_unique_nodes; n++) {
                if (strcmp(node_stats[n].hostname, all_reports[r].hostname) == 0) {
                    node_idx = n;
                    break;
                }
            }
            if (node_idx < 0) {
                node_idx = num_unique_nodes++;
                strncpy(node_stats[node_idx].hostname, all_reports[r].hostname, 255);
                node_stats[node_idx].max_physical_gpu = -1;
            }
            
            for (int g = 0; g < all_reports[r].num_gpus && g < 8; g++) {
                int phys_id = all_reports[r].physical_gpu_ids[g];
                if (phys_id >= 0 && phys_id < 16) {
                    node_stats[node_idx].gpu_tasks[phys_id] += all_reports[r].gpu[g].task_count;
                    node_stats[node_idx].gpu_time[phys_id] += all_reports[r].gpu[g].total_time;
                    if (phys_id > node_stats[node_idx].max_physical_gpu) {
                        node_stats[node_idx].max_physical_gpu = phys_id;
                    }
                }
            }
        }
        
        for (int n = 0; n < num_unique_nodes; n++) {
            fprintf(fp, "\n  Node: %s\n", node_stats[n].hostname);
            fprintf(fp, "  ┌──────────────┬──────────────┬──────────────┬──────────────┐\n");
            fprintf(fp, "  │ Physical GPU │    Tasks     │   Time (s)   │ Utilization  │\n");
            fprintf(fp, "  ├──────────────┼──────────────┼──────────────┼──────────────┤\n");
            
            for (int g = 0; g <= node_stats[n].max_physical_gpu; g++) {
                if (node_stats[n].gpu_tasks[g] > 0 || node_stats[n].gpu_time[g] > 0) {
                    double node_util = (max_wall_time > 0) ? 
                        100.0 * node_stats[n].gpu_time[g] / max_wall_time : 0.0;
                    fprintf(fp, "  │     GPU %-3d  │ %12d │ %12.3f │ %10.2f%% │\n",
                           g, node_stats[n].gpu_tasks[g], node_stats[n].gpu_time[g], node_util);
                }
            }
            fprintf(fp, "  └──────────────┴──────────────┴──────────────┴──────────────┘\n");
        }
        
        free(node_stats);
        fprintf(fp, "\n");
        
        // ================================================================
        // GPU 负载均衡分析
        // ================================================================
        fprintf(fp, "┌─────────────────────────────────────────────────────────────────────────────┐\n");
        fprintf(fp, "│                      GPU LOAD BALANCE ANALYSIS                              │\n");
        fprintf(fp, "├─────────────────────────────────────────────────────────────────────────────┤\n");
        
        double gpu_times[8] = {0};
        int gpu_tasks_arr[8] = {0};
        for (int g = 0; g < num_gpus && g < 8; g++) {
            for (int r = 0; r < num_processes; r++) {
                gpu_times[g] += all_reports[r].gpu[g].total_time;
                gpu_tasks_arr[g] += all_reports[r].gpu[g].task_count;
            }
        }
        
        double mean_time = 0.0, mean_tasks = 0.0;
        for (int g = 0; g < num_gpus; g++) {
            mean_time += gpu_times[g];
            mean_tasks += gpu_tasks_arr[g];
        }
        mean_time /= num_gpus;
        mean_tasks /= num_gpus;
        
        double std_time = 0.0, std_tasks = 0.0;
        for (int g = 0; g < num_gpus; g++) {
            std_time += (gpu_times[g] - mean_time) * (gpu_times[g] - mean_time);
            std_tasks += (gpu_tasks_arr[g] - mean_tasks) * (gpu_tasks_arr[g] - mean_tasks);
        }
        std_time = sqrt(std_time / num_gpus);
        std_tasks = sqrt(std_tasks / num_gpus);
        
        double cv_time = (mean_time > 0) ? 100.0 * std_time / mean_time : 0.0;
        double cv_tasks = (mean_tasks > 0) ? 100.0 * std_tasks / mean_tasks : 0.0;
        
        fprintf(fp, "│  Per-GPU Statistics (across all processes):                                 │\n");
        fprintf(fp, "│    Time  - Mean: %8.3f s, Std: %8.3f s, CV: %6.2f %%                   │\n", 
                mean_time, std_time, cv_time);
        fprintf(fp, "│    Tasks - Mean: %8.1f,   Std: %8.1f,   CV: %6.2f %%                   │\n", 
                mean_tasks, std_tasks, cv_tasks);
        fprintf(fp, "├─────────────────────────────────────────────────────────────────────────────┤\n");
        
        int busiest_gpu = 0, idlest_gpu = 0;
        for (int g = 1; g < num_gpus; g++) {
            if (gpu_times[g] > gpu_times[busiest_gpu]) busiest_gpu = g;
            if (gpu_times[g] < gpu_times[idlest_gpu]) idlest_gpu = g;
        }
        
        double imbalance = (gpu_times[idlest_gpu] > 0) ? 
            gpu_times[busiest_gpu] / gpu_times[idlest_gpu] : 0.0;
        
        fprintf(fp, "│  Busiest GPU: %d (%.3f s, %d tasks)                                        │\n",
                busiest_gpu, gpu_times[busiest_gpu], gpu_tasks_arr[busiest_gpu]);
        fprintf(fp, "│  Idlest  GPU: %d (%.3f s, %d tasks)                                        │\n",
                idlest_gpu, gpu_times[idlest_gpu], gpu_tasks_arr[idlest_gpu]);
        fprintf(fp, "│  Imbalance Ratio: %.2f                                                      │\n", imbalance);
        fprintf(fp, "└─────────────────────────────────────────────────────────────────────────────┘\n\n");

        // ================================================================
        // CPU 任务执行统计（★ 使用前面已归约好的数据，不再调用 MPI_Reduce）
        // ================================================================
        if (PROF_ENABLED) {
            fprintf(fp, "┌─────────────────────────────────────────────────────────────────────────────┐\n");
            fprintf(fp, "│                          CPU TASK STATISTICS                                │\n");
            fprintf(fp, "├─────────────────────────────────────────────────────────────────────────────┤\n");
            fprintf(fp, "│                                                                             │\n");
            
            // Dense CPU统计
            fprintf(fp, "│  ★ Dense CPU Tasks:                                                         │\n");
            fprintf(fp, "│    ┌─────────────────────────────────────────────────────────────────┐     │\n");
            fprintf(fp, "│    │  Total Tasks:        %10llu                                  │     │\n", 
                    (unsigned long long)total_cpu_ds_tasks);
            fprintf(fp, "│    │  Direct (size<th):   %10llu (%5.1f%%)                       │     │\n",
                    (unsigned long long)total_cpu_ds_direct,
                    total_cpu_ds_tasks > 0 ? 100.0 * total_cpu_ds_direct / total_cpu_ds_tasks : 0.0);
            fprintf(fp, "│    │  Fallback (GPU→CPU): %10llu (%5.1f%%)                       │     │\n",
                    (unsigned long long)total_cpu_ds_fallback,
                    total_cpu_ds_tasks > 0 ? 100.0 * total_cpu_ds_fallback / total_cpu_ds_tasks : 0.0);
            fprintf(fp, "│    ├─────────────────────────────────────────────────────────────────┤     │\n");
            fprintf(fp, "│    │  Total Time:         %10.3f s                               │     │\n",
                    total_cpu_ds_time);
            fprintf(fp, "│    │  Avg Time/Task:      %10.6f s                               │     │\n",
                    total_cpu_ds_tasks > 0 ? total_cpu_ds_time / total_cpu_ds_tasks : 0.0);
            fprintf(fp, "│    │  Min Time:           %10.6f s                               │     │\n",
                    min_cpu_ds_time < 1e9 ? min_cpu_ds_time : 0.0);
            fprintf(fp, "│    │  Max Time:           %10.6f s                               │     │\n",
                    max_cpu_ds_time_val);
            fprintf(fp, "│    ├─────────────────────────────────────────────────────────────────┤     │\n");
            fprintf(fp, "│    │  Total Elements:     %10llu                                  │     │\n",
                    (unsigned long long)total_cpu_ds_elements);
            fprintf(fp, "│    │  Entry Compute:      %10.3f s (%5.1f%%)                      │     │\n",
                    total_cpu_ds_entry_time,
                    total_cpu_ds_time > 0 ? 100.0 * total_cpu_ds_entry_time / total_cpu_ds_time : 0.0);
            fprintf(fp, "│    └─────────────────────────────────────────────────────────────────┘     │\n");
            fprintf(fp, "│                                                                             │\n");
            
            // LowRank CPU统计
            fprintf(fp, "│  ★ LowRank CPU Tasks:                                                       │\n");
            fprintf(fp, "│    ┌─────────────────────────────────────────────────────────────────┐     │\n");
            fprintf(fp, "│    │  Total Tasks:        %10llu                                  │     │\n",
                    (unsigned long long)total_cpu_lr_tasks);
            fprintf(fp, "│    │  Direct (size<th):   %10llu (%5.1f%%)                       │     │\n",
                    (unsigned long long)total_cpu_lr_direct,
                    total_cpu_lr_tasks > 0 ? 100.0 * total_cpu_lr_direct / total_cpu_lr_tasks : 0.0);
            fprintf(fp, "│    │  Fallback (GPU→CPU): %10llu (%5.1f%%)                       │     │\n",
                    (unsigned long long)total_cpu_lr_fallback,
                    total_cpu_lr_tasks > 0 ? 100.0 * total_cpu_lr_fallback / total_cpu_lr_tasks : 0.0);
            fprintf(fp, "│    ├─────────────────────────────────────────────────────────────────┤     │\n");
            fprintf(fp, "│    │  Total Time:         %10.3f s                               │     │\n",
                    total_cpu_lr_time);
            fprintf(fp, "│    │  Avg Time/Task:      %10.6f s                               │     │\n",
                    total_cpu_lr_tasks > 0 ? total_cpu_lr_time / total_cpu_lr_tasks : 0.0);
            fprintf(fp, "│    │  Min Time:           %10.6f s                               │     │\n",
                    min_cpu_lr_time < 1e9 ? min_cpu_lr_time : 0.0);
            fprintf(fp, "│    │  Max Time:           %10.6f s                               │     │\n",
                    max_cpu_lr_time_val);
            fprintf(fp, "│    ├─────────────────────────────────────────────────────────────────┤     │\n");
            fprintf(fp, "│    │  Total Elements:     %10llu                                  │     │\n",
                    (unsigned long long)total_cpu_lr_elements);
            fprintf(fp, "│    │  Entry Compute:      %10.3f s (%5.1f%%)                      │     │\n",
                    total_cpu_lr_entry_time,
                    total_cpu_lr_time > 0 ? 100.0 * total_cpu_lr_entry_time / total_cpu_lr_time : 0.0);
            fprintf(fp, "│    │  BLAS Operations:    %10.3f s (%5.1f%%)                      │     │\n",
                    total_cpu_lr_blas_time,
                    total_cpu_lr_time > 0 ? 100.0 * total_cpu_lr_blas_time / total_cpu_lr_time : 0.0);
            fprintf(fp, "│    └─────────────────────────────────────────────────────────────────┘     │\n");
            fprintf(fp, "│                                                                             │\n");
            
            // CPU vs GPU性能对比
            double total_cpu_time = total_cpu_ds_time + total_cpu_lr_time;
            uint64_t total_cpu_tasks_all = total_cpu_ds_tasks + total_cpu_lr_tasks;
            
            fprintf(fp, "│  ★ CPU Performance Summary:                                                 │\n");
            fprintf(fp, "│    ┌─────────────────────────────────────────────────────────────────┐     │\n");
            fprintf(fp, "│    │  Total CPU Time:     %10.3f s                               │     │\n",
                    total_cpu_time);
            fprintf(fp, "│    │  Total CPU Tasks:    %10llu                                  │     │\n",
                    (unsigned long long)total_cpu_tasks_all);
            fprintf(fp, "│    │  CPU vs Wall Time:   %10.1f %%                              │     │\n",
                    max_wall_time > 0 ? 100.0 * total_cpu_time / max_wall_time : 0.0);
            
            if (global_gpu_time > 0) {
                fprintf(fp, "│    │  CPU vs GPU Time:    %10.1f %%                              │     │\n",
                        100.0 * total_cpu_time / global_gpu_time);
            }
            
            uint64_t total_fb = total_cpu_ds_fallback + total_cpu_lr_fallback;
            if (total_fb > 0) {
                fprintf(fp, "│    │  Total Fallbacks:    %10llu  (GPU contention)              │     │\n",
                        (unsigned long long)total_fb);
                fprintf(fp, "│    │  Fallback Rate:      %10.1f %%                              │     │\n",
                        total_cpu_tasks_all > 0 ? 100.0 * total_fb / total_cpu_tasks_all : 0.0);
            }
            
            fprintf(fp, "│    └─────────────────────────────────────────────────────────────────┘     │\n");
            fprintf(fp, "└─────────────────────────────────────────────────────────────────────────────┘\n\n");
        } else {
            // Profiling未启用时的提示
            fprintf(fp, "┌─────────────────────────────────────────────────────────────────────────────┐\n");
            fprintf(fp, "│                          CPU TASK STATISTICS                                │\n");
            fprintf(fp, "├─────────────────────────────────────────────────────────────────────────────┤\n");
            fprintf(fp, "│                                                                             │\n");
            fprintf(fp, "│  ⚠ Profiling mode is OFF - CPU statistics unavailable                       │\n");
            fprintf(fp, "│                                                                             │\n");
            fprintf(fp, "│  To enable CPU task tracking:                                               │\n");
            fprintf(fp, "│    1. Compile with: -DGPU_PROFILING_RUNTIME                                 │\n");
            fprintf(fp, "│    2. Run with:     export GPU_PROFILING=1                                  │\n");
            fprintf(fp, "│                                                                             │\n");
            fprintf(fp, "└─────────────────────────────────────────────────────────────────────────────┘\n\n");
        }
                
        // ================================================================
        // GPU 任务分布矩阵
        // ================================================================
        fprintf(fp, "┌─────────────────────────────────────────────────────────────────────────────┐\n");
        fprintf(fp, "│                GPU TASK DISTRIBUTION MATRIX (Logical GPU IDs)              │\n");
        fprintf(fp, "├──────");
        for (int g = 0; g < num_gpus; g++) fprintf(fp, "┬──────────");
        fprintf(fp, "┤\n");
        
        fprintf(fp, "│ Rank ");
        for (int g = 0; g < num_gpus; g++) fprintf(fp, "│  GPU %-3d ", g);
        fprintf(fp, "│\n");
        
        fprintf(fp, "├──────");
        for (int g = 0; g < num_gpus; g++) fprintf(fp, "┼──────────");
        fprintf(fp, "┤\n");
        
        for (int r = 0; r < num_processes; r++) {
            fprintf(fp, "│ %4d ", r);
            for (int g = 0; g < num_gpus && g < 8; g++) {
                fprintf(fp, "│ %8d ", all_reports[r].gpu[g].task_count);
            }
            fprintf(fp, "│\n");
        }
        
        fprintf(fp, "├──────");
        for (int g = 0; g < num_gpus; g++) fprintf(fp, "┼──────────");
        fprintf(fp, "┤\n");
        
        fprintf(fp, "│ SUM  ");
        for (int g = 0; g < num_gpus; g++) {
            int col_sum = 0;
            for (int r = 0; r < num_processes; r++) {
                col_sum += all_reports[r].gpu[g].task_count;
            }
            fprintf(fp, "│ %8d ", col_sum);
        }
        fprintf(fp, "│\n");
        
        fprintf(fp, "└──────");
        for (int g = 0; g < num_gpus; g++) fprintf(fp, "┴──────────");
        fprintf(fp, "┘\n\n");
        
        fprintf(fp, "================================================================================\n");
        fprintf(fp, "                              END OF REPORT                                    \n");
        fprintf(fp, "================================================================================\n");
        
        if (fp != stdout) {
            fclose(fp);
            printf("\n[GPU Report] Written to: %s\n", gpu_report_filename);
        }
        
        free(all_reports);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
}

// ============================================================
// 简化版摘要输出
// ============================================================
void print_gpu_usage_summary() {
    MPI_Barrier(MPI_COMM_WORLD);
    
    if (proc_stats.wall_end_time == 0) {
        stop_wall_clock();
    }
    
    uint64_t local_tasks = 0;
    double local_time = 0.0;
    for (int i = 0; i < num_gpus; i++) {
        local_tasks += gpu_stats[i].task_count;
        local_time += gpu_stats[i].total_time;
    }
    
    uint64_t global_tasks = 0;
    double global_time = 0.0;
    int global_failures = 0;
    int global_attempts = 0;
    
    MPI_Reduce(&local_tasks, &global_tasks, 1, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_time, &global_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&proc_stats.gpu_acquire_failures, &global_failures, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&proc_stats.gpu_acquire_attempts, &global_attempts, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    
    if (my_rank == 0) {
        double success_rate = (global_attempts > 0) ? 
            100.0 * (global_attempts - global_failures) / global_attempts : 0.0;
        printf("\n[GPU Summary] Tasks: %lu | Time: %.3fs | Attempts: %d | Failures: %d | Success: %.2f%% | Report: %s\n",
               (unsigned long)global_tasks, global_time, global_attempts, global_failures, success_rate, gpu_report_filename);
        fflush(stdout);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
}

// ============================================================
// Cleanup resources
// ============================================================
void cleanup_multi_gpu() {
    // 释放 Dense buffer 池
    if (ds_buf) {
        for (int i = 0; i < num_gpus; i++) {
            if (ds_buf[i]) {
                acc_set_device_num(physical_gpu_ids[i], acc_device_nvidia);
                #pragma acc exit data delete(ds_buf[i:1][0:ds_buf_cap[i]])
                free(ds_buf[i]);
            }
        }
        free(ds_buf);
        ds_buf = NULL;
    }
    if (ds_buf_cap) {
        free(ds_buf_cap);
        ds_buf_cap = NULL;
    }
    if (ds_buf_stat) {
        free(ds_buf_stat);
        ds_buf_stat = NULL;
    }

    if (gpu_mutexes) {
        for (int i = 0; i < num_gpus; i++)
            pthread_mutex_destroy(&gpu_mutexes[i]);
        free(gpu_mutexes);
        gpu_mutexes = NULL;
    }
    if (gpu_stats) {
        free(gpu_stats);
        gpu_stats = NULL;
    }
    if (gpu_data_ready) {
        free(gpu_data_ready);
        gpu_data_ready = NULL;
    }
    if (physical_gpu_ids) {
        free(physical_gpu_ids);
        physical_gpu_ids = NULL;
    }
    multi_gpu_initialized = 0;
}

// ============================================================
// acaplus - Low-rank ACA algorithm
// 修正点1: lr_free_acc 独立于 lr_d2h_acc
// 修正点4: 使用 PROF_TIME_START/END 宏
// ============================================================
int acaplus(double* zaa, double* zab, int ndl, int ndt, int nstrtl, int nstrtt,
            int kmax, double eps, double znrmmat, double pACA_EPS,
            double* pa_ref, double* pb_ref, int* lrow_done, int* lcol_done,
            int nofc, double zgmid[][3], int f2n[][3], double bgmid[][3],
            int id, int use_gpu, int acquired_gpu_id,
            double *lr_cpu_time, double *lr_gpu_time)
{
    int INCY = 1;

    // LR profiling accumulators (修正点1: 分离 free)
    double lr_h2d_acc = 0.0;
    double lr_compute_acc = 0.0;
    double lr_d2h_acc = 0.0;
    double lr_free_acc = 0.0;       // 新增: 独立于 d2h

    if (use_gpu) {
        // 修正点4: 使用 PROF_TIME_START/END
        PROF_TIME_START(_lr_entry_h2d);
        #pragma acc enter data copyin(zab[0:ndt*kmax], zaa[0:ndl*kmax], \
                                      lrow_done[0:ndl], lcol_done[0:ndt]) \
                               create(pa_ref[0:ndl], pb_ref[0:ndt])
        PROF_ACC_WAIT();
        PROF_TIME_END(_lr_entry_h2d, lr_h2d_acc);
    }

    double *prow, *pcol;
    int il;
    double za_ACA_EPS = 1.0e-10;
    double ACA_EPS = pACA_EPS;

    int ntries = imax(ndl, ndt) + 1;
    int ntries_row = 6;
    int ntries_col = 6;

    int k = 0;
    int j_ref = 0;

    double (*zaa2)[ndl] = (double(*)[ndl])zaa;
    double (*zab2)[ndt] = (double(*)[ndt])zab;

    comp_col(zaa, zab, ndl, ndt, k, j_ref, pa_ref, nstrtl, nstrtt,
             lrow_done, nofc, zgmid, f2n, bgmid, use_gpu, id,
             lr_cpu_time, &lr_h2d_acc, &lr_compute_acc, &lr_d2h_acc);

    double colnorm = cblas_dnrm2(ndl, pa_ref, INCY);
    int i_ref = minabsvalloc_d(pa_ref, ndl);
    double rownorm = fabs(pa_ref[i_ref]);

    comp_row(zaa, zab, ndl, ndt, k, i_ref, pb_ref, nstrtl, nstrtt,
             lcol_done, nofc, zgmid, f2n, bgmid, use_gpu, id,
             lr_cpu_time, &lr_h2d_acc, &lr_compute_acc, &lr_d2h_acc);

    rownorm = cblas_dnrm2(ndt, pb_ref, INCY);

    double apxnorm = 0.0;
    int lstop_aca = 0;
    double col_maxval, row_maxval;
    double zinvmax;

    while (k < kmax && (ntries_row > 0 || ntries_col > 0) && ntries > 0) {
        ntries--;
        pcol = &zaa2[k][0];
        prow = &zab2[k][0];

        int i = maxabsvalloc_d(pa_ref, ndl);
        col_maxval = fabs(pa_ref[i]);

        int j = maxabsvalloc_d(pb_ref, ndt);
        row_maxval = fabs(pb_ref[j]);

        if (row_maxval > col_maxval) {
            if (j != j_ref) {
                comp_col(zaa, zab, ndl, ndt, k, j, pcol, nstrtl, nstrtt,
                         lrow_done, nofc, zgmid, f2n, bgmid, use_gpu, id,
                         lr_cpu_time, &lr_h2d_acc, &lr_compute_acc, &lr_d2h_acc);
            } else {
                memcpy(pcol, pa_ref, sizeof(double) * ndl);
            }
            i = maxabsvalloc_d(pcol, ndl);
            col_maxval = fabs(pcol[i]);

            if (col_maxval < ACA_EPS && k >= 1) {
                lstop_aca = 1;
            } else {
                comp_row(zaa, zab, ndl, ndt, k, i, prow, nstrtl, nstrtt,
                         lcol_done, nofc, zgmid, f2n, bgmid, use_gpu, id,
                         lr_cpu_time, &lr_h2d_acc, &lr_compute_acc, &lr_d2h_acc);
                if (fabs(pcol[i]) > 1.0e-20) {
                    zinvmax = 1.0 / pcol[i];
                } else {
                    k = imax(k - 1, 0);
                    break;
                }
                for (il = 0; il < ndl; il++)
                    pcol[il] *= zinvmax;
            }
        } else {
            if (i != i_ref) {
                comp_row(zaa, zab, ndl, ndt, k, i, prow, nstrtl, nstrtt,
                         lcol_done, nofc, zgmid, f2n, bgmid, use_gpu, id,
                         lr_cpu_time, &lr_h2d_acc, &lr_compute_acc, &lr_d2h_acc);
            } else {
                memcpy(prow, pb_ref, sizeof(double) * ndt);
            }
            j = maxabsvalloc_d(prow, ndt);
            row_maxval = fabs(prow[j]);

            if (row_maxval < ACA_EPS && k >= 1) {
                lstop_aca = 1;
            } else {
                comp_col(zaa, zab, ndl, ndt, k, j, pcol, nstrtl, nstrtt,
                         lrow_done, nofc, zgmid, f2n, bgmid, use_gpu, id,
                         lr_cpu_time, &lr_h2d_acc, &lr_compute_acc, &lr_d2h_acc);
                if (fabs(prow[j]) > 1.0e-20) {
                    zinvmax = 1.0 / prow[j];
                } else {
                    k = imax(k - 1, 0);
                    break;
                }
                for (il = 0; il < ndt; il++)
                    prow[il] *= zinvmax;
            }
        }

        lrow_done[i] = 1;
        lcol_done[j] = 1;
        if (use_gpu) {
            PROF_TIME_START(_lr_upd1);
            #pragma acc update device(lrow_done[i:1], lcol_done[j:1])
            PROF_ACC_WAIT();
            PROF_TIME_END(_lr_upd1, lr_h2d_acc);
        }

        if (i != i_ref) {
            zinvmax = -pcol[i_ref];
            for (il = 0; il < ndt; il++)
                pb_ref[il] += prow[il] * zinvmax;
            rownorm = cblas_dnrm2(ndt, pb_ref, INCY);
        }
        if (i == i_ref || rownorm < ACA_EPS) {
            if (i == i_ref) ntries_row++;
            if (ntries_row > 0) {
                rownorm = 0.0;
                i = i_ref;
                while (i != (i_ref + ndl - 1) % ndl && rownorm < za_ACA_EPS && ntries_row > 0) {
                    if (lrow_done[i] == 0) {
                        comp_row(zaa, zab, ndl, ndt, k + 1, i, pb_ref, nstrtl, nstrtt,
                                 lcol_done, nofc, zgmid, f2n, bgmid, use_gpu, id,
                                 lr_cpu_time, &lr_h2d_acc, &lr_compute_acc, &lr_d2h_acc);
                        rownorm = cblas_dnrm2(ndt, pb_ref, INCY);
                        if (rownorm < ACA_EPS) {
                            lrow_done[i] = 1;
                            if (use_gpu) {
                                PROF_TIME_START(_lr_upd2);
                                #pragma acc update device(lrow_done[i:1])
                                PROF_ACC_WAIT();
                                PROF_TIME_END(_lr_upd2, lr_h2d_acc);
                            }
                        }
                        ntries_row--;
                    } else {
                        rownorm = 0.0;
                    }
                    i = (i + 1) % ndl;
                }
                i_ref = (i + ndl - 1) % ndl;
            }
        }

        if (j != j_ref) {
            zinvmax = -prow[j_ref];
            for (il = 0; il < ndl; il++)
                pa_ref[il] += pcol[il] * zinvmax;
            colnorm = cblas_dnrm2(ndl, pa_ref, INCY);
        }
        if (j == j_ref || colnorm < ACA_EPS) {
            if (j == j_ref) ntries_col++;
            if (ntries_col > 0) {
                colnorm = 0.0;
                j = j_ref;
                while (j != (j_ref + ndt - 1) % ndt && colnorm < za_ACA_EPS && ntries_col > 0) {
                    if (lcol_done[j] == 0) {
                        comp_col(zaa, zab, ndl, ndt, k + 1, j, pa_ref, nstrtl, nstrtt,
                                 lrow_done, nofc, zgmid, f2n, bgmid, use_gpu, id,
                                 lr_cpu_time, &lr_h2d_acc, &lr_compute_acc, &lr_d2h_acc);
                        colnorm = cblas_dnrm2(ndl, pa_ref, INCY);
                        if (colnorm < ACA_EPS) {
                            lcol_done[j] = 1;
                            if (use_gpu) {
                                PROF_TIME_START(_lr_upd3);
                                #pragma acc update device(lcol_done[j:1])
                                PROF_ACC_WAIT();
                                PROF_TIME_END(_lr_upd3, lr_h2d_acc);
                            }
                        }
                        ntries_col--;
                    } else {
                        colnorm = 0.0;
                    }
                    j = (j + 1) % ndt;
                }
                j_ref = (j + ndt - 1) % ndt;
            }
        }

        if (colnorm < ACA_EPS && rownorm < ACA_EPS && k >= 1) {
            lstop_aca = 1;
            k = k + 1;
        }

        if (lstop_aca == 0) {
            double blknorm = cblas_dnrm2(ndl, pcol, INCY) * cblas_dnrm2(ndt, prow, INCY);
            if (k == 0) {
                apxnorm = blknorm;
            } else {
                double compared = apxnorm * eps;
                if (blknorm < compared && rownorm < compared && colnorm < compared && k >= 1)
                    lstop_aca = 1;
            }
        }
        if (lstop_aca == 1 && k >= 1)
            break;

        k++;
    }

    if (use_gpu) {
        // 修正点1: free 独立计时，不合并到 d2h
        PROF_TIME_START(_lr_free);
        #pragma acc exit data delete(zaa[0:ndl*kmax], zab[0:ndt*kmax], \
                                     pa_ref[0:ndl], pb_ref[0:ndt], \
                                     lrow_done[0:ndl], lcol_done[0:ndt])
        PROF_ACC_WAIT();
        PROF_TIME_END(_lr_free, lr_free_acc);

        uint64_t workload = k * (ndl + ndt);
        
        // 修正点1: 4 个阶段分别传入
        record_gpu_usage_lr(acquired_gpu_id, workload,
                            lr_h2d_acc, lr_compute_acc, lr_d2h_acc, lr_free_acc);
        
        // 累加到外层 lr_gpu_time（总GPU时间）
        *lr_gpu_time += (lr_h2d_acc + lr_compute_acc + lr_d2h_acc + lr_free_acc);
        
        release_gpu(acquired_gpu_id);
    }

    if (k < 1) {
        fprintf(stderr, "acaplus alert: colnorm=%f rownorm=%f ACA_EPS=%f "
                "col_maxval=%f row_maxval=%f ntries_row=%d ntries_col=%d ntries=%d k=%d\n",
                colnorm, rownorm, ACA_EPS, col_maxval, row_maxval,
                ntries_row, ntries_col, ntries, k);
    }

    // ★ ACA kt 诊断（运行时 PROF_ENABLED 控制，无需 -DGPU_PROFILING）
    // if (PROF_ENABLED) {
    //     fprintf(stderr, "ACA[%d] %s ndl=%d ndt=%d kt=%d\n",
    //             id, use_gpu ? "gpu" : "cpu", ndl, ndt, k);
    // }

    return k;
}

// ============================================================
// fill_sub_leafmtx（增强版：包含任务过滤统计和时间分解）
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

    double eps = 1.0e-5;
    double ACA_EPS = 0.9 * eps;

    int ndl = st_lf->ndl;
    int ndt = st_lf->ndt;
    int ns = ndl * ndt;
    int nstrtl = st_lf->nstrtl;
    int nstrtt = st_lf->nstrtt;
    int ltmtx = st_lf->ltmtx;

    if (ltmtx == 1) {
        // === Low-rank block ===
        st_lf->a1 = (double*)malloc(sizeof(double) * ndt * kparam);
        st_lf->a2 = (double*)malloc(sizeof(double) * ndl * kparam);
        if (!st_lf->a1 || !st_lf->a2) {
            fprintf(stderr, "ERROR: allocate a1 or a2 failed!\n");
            exit(99);
        }

        double *pa_ref = (double *)malloc(ndl * sizeof(double));
        double *pb_ref = (double *)malloc(ndt * sizeof(double));
        int *lrow_done = (int *)calloc(ndl, sizeof(int));
        int *lcol_done = (int *)calloc(ndt, sizeof(int));

        int workload = kparam * (ndl + ndt);
        int eligible_gpu = (workload >= th_lr);
        int acquired_gpu = -1;

        if (eligible_gpu) {
            __sync_fetch_and_add(&proc_stats.eligible_lr_tasks, 1);
            (*pen_lr_gpu_subm)++;
            acquired_gpu = try_acquire_any_gpu(id);
        } else {
            __sync_fetch_and_add(&proc_stats.direct_cpu_lr_tasks, 1);
        }

        int use_gpu = (acquired_gpu >= 0);

        if (use_gpu) {
            (*gpu_lr_cnt)++;
            acc_set_device_num(physical_gpu_ids[acquired_gpu], acc_device_nvidia);
        } else {
            (*cpu_lr_cnt)++;
        }

        // ★ LR 任务计时（用于 CPU task 统计，运行时 PROF_ENABLED 控制）
        double lr_task_start = (PROF_ENABLED && !use_gpu) ? get_time() : 0.0;

        int kt = acaplus(st_lf->a2, st_lf->a1, ndl, ndt, nstrtl, nstrtt,
                         kparam, eps, znrmmat, ACA_EPS,
                         pa_ref, pb_ref, lrow_done, lcol_done,
                         nofc, zgmid, f2n, bgmid,
                         id, use_gpu, acquired_gpu,
                         lr_cpu_time, lr_gpu_time);

        // ★ CPU LR 任务统计（运行时 PROF_ENABLED 控制）
        if (PROF_ENABLED && !use_gpu) {
            double lr_duration = get_time() - lr_task_start;
            uint64_t lr_elements = (uint64_t)kt * (ndl + ndt);
            record_cpu_task_lr(lr_duration, lr_elements, eligible_gpu);
        }

        st_lf->kt = kt;
        if (kt > kparam) {
            fprintf(stderr, "WARNING: Insufficient k: kt=%d, kparam=%d, nstrtl=%d, nstrtt=%d, ndl=%d, ndt=%d\n",
                    kt, kparam, nstrtl, nstrtt, ndl, ndt);
        }

        if (kt > 0) {
            double *tmp1 = (double *)realloc(st_lf->a1, kt * ndt * sizeof(double));
            double *tmp2 = (double *)realloc(st_lf->a2, kt * ndl * sizeof(double));
            if (!tmp1 || !tmp2) {
                fprintf(stderr, "ERROR: realloc a1/a2 failed (kt=%d, ndl=%d, ndt=%d)\n",
                        kt, ndl, ndt);
                exit(99);
            }
            st_lf->a1 = tmp1;
            st_lf->a2 = tmp2;
        } else {
            free(st_lf->a1);
            free(st_lf->a2);
            st_lf->a1 = NULL;
            st_lf->a2 = NULL;
        }

        if (use_gpu) (*gpu_lr_elem) += kt * (ndl + ndt);
        else         (*cpu_lr_elem) += kt * (ndl + ndt);

        if (eligible_gpu) {
            (*pen_lr_gpu_elem) += kt * (ndl + ndt);
        }

        free(pa_ref);
        free(pb_ref);
        free(lrow_done);
        free(lcol_done);

    } else if (ltmtx == 2) {
        // === Dense block ===
        st_lf->a1 = (double *)malloc(sizeof(double) * ns);
        if (!st_lf->a1) {
            fprintf(stderr, "ERROR: allocate a1 failed!\n");
            exit(99);
        }

        int eligible_gpu = (ns >= th_ds);
        int acquired_gpu = -1;

        if (eligible_gpu) {
            __sync_fetch_and_add(&proc_stats.eligible_ds_tasks, 1);
            (*pen_ds_gpu_subm)++;
            (*pen_ds_gpu_elem) += ns;
            acquired_gpu = try_acquire_any_gpu(id);
        } else {
            __sync_fetch_and_add(&proc_stats.direct_cpu_ds_tasks, 1);
        }

        int use_gpu = (acquired_gpu >= 0);

        if (use_gpu) {
            (*gpu_ds_cnt)++;
            (*gpu_ds_elem) += ns;
            acc_set_device_num(physical_gpu_ids[acquired_gpu], acc_device_nvidia);
        } else {
            (*cpu_ds_cnt)++;
            (*cpu_ds_elem) += ns;
        }

        double h2d_time = 0.0, compute_time = 0.0, d2h_time = 0.0;
        double t_start = get_time();

        // ── Unified compute target: GPU buffer or host a1 ──
        double *compute_buf;
        if (use_gpu) {
            PROF_TIME_START(_ds_h2d);
            ds_buf_ensure(acquired_gpu, ns);
            PROF_ACC_WAIT();
            PROF_TIME_END(_ds_h2d, h2d_time);

            if ((uint64_t)ns > ds_buf_stat[acquired_gpu].max_ns) {
                ds_buf_stat[acquired_gpu].max_ns = (uint64_t)ns;
            }
            compute_buf = ds_buf[acquired_gpu];
        } else {
            compute_buf = st_lf->a1;
        }

        // ── Unified kernel: single code path for CPU and GPU ──
        PROF_TIME_START(_ds_compute);
        {
            double (*tempa1)[ndt] = (double(*)[ndt])compute_buf;
            double xf[3], yf[3], zf[3];

            #pragma acc parallel loop gang \
                       if(use_gpu) \
                       present(zgmid[0:nofc][0:3], f2n[0:nofc][0:3], \
                               bgmid[0:nNode][0:3], compute_buf[0:ns]) \
                       private(xf, yf, zf)
            for (int il = 0; il < ndl; il++) {
                int ill = il + nstrtl;
                double xp = zgmid[ill][0];
                double yp = zgmid[ill][1];
                double zp = zgmid[ill][2];

                #pragma acc loop vector
                for (int it = 0; it < ndt; it++) {
                    int itt = it + nstrtt;

                    int ni0 = f2n[itt][0], ni1 = f2n[itt][1], ni2 = f2n[itt][2];
                    xf[0] = bgmid[ni0][0]; xf[1] = bgmid[ni1][0]; xf[2] = bgmid[ni2][0];
                    yf[0] = bgmid[ni0][1]; yf[1] = bgmid[ni1][1]; yf[2] = bgmid[ni2][1];
                    zf[0] = bgmid[ni0][2]; zf[1] = bgmid[ni1][2]; zf[2] = bgmid[ni2][2];

                    tempa1[il][it] = face_integral2(xf, yf, zf, xp, yp, zp);
                }
            }
        }
        if (use_gpu) { PROF_ACC_WAIT(); }
        PROF_TIME_END(_ds_compute, compute_time);

        // ── D2H: only when GPU was used ──
        if (use_gpu) {
            PROF_TIME_START(_ds_d2h);
            {
                void *dev_ptr = acc_deviceptr(compute_buf);
                acc_memcpy_from_device(st_lf->a1, dev_ptr, ns * sizeof(double));
            }
            PROF_ACC_WAIT();
            PROF_TIME_END(_ds_d2h, d2h_time);
        }

        // ── Unified post-processing ──
        double duration = get_time() - t_start;

        if (use_gpu) {
            if (!PROF_ENABLED) {
                compute_time = duration;
            }
            record_gpu_usage_dense(acquired_gpu, ns, h2d_time, compute_time, d2h_time);
            release_gpu(acquired_gpu);
            *ds_gpu_time += (h2d_time + compute_time + d2h_time);
        } else {
            *ds_cpu_time += duration;
            record_cpu_task_dense(duration, (uint64_t)ns, eligible_gpu);
        }
    }
}

// ============================================================
// comp_row: 固定行 il，计算该行所有列的值
// 修正点3: compute 计时在 kernel 之前开始（包含 kernel launch）
// 修正点4: 使用 PROF_TIME_START/END
// ============================================================
void comp_row(double* zaa, double* zab,
              int ndl, int ndt, int k, int il,
              double* row,
              int nstrtl, int nstrtt,
              int* skip_cols,
              int nofc,
              double zgmid[][3],
              int f2n[][3],
              double bgmid[][3],
              int use_gpu,
              int id,
              double *lr_cpu_time,
              double *lr_h2d_acc,
              double *lr_compute_acc,
              double *lr_d2h_acc)
{
    double xf[3], yf[3], zf[3];
    int it;

    double row_start = get_time();

    // H2D: update device（增量传输上一轮的 zab/zaa）
    if (use_gpu && k > 0) {
        PROF_TIME_START(_cr_h2d);
        int last = k - 1;
        #pragma acc update device(zab[last*ndt:ndt], \
                                  zaa[last*ndl:ndl])
        PROF_ACC_WAIT();
        PROF_TIME_END(_cr_h2d, *lr_h2d_acc);
    }

    // 修正点3: Compute 计时在 kernel 发射之前开始
    PROF_TIME_START(_cr_compute);
    #pragma acc parallel if(use_gpu) \
        present(zab[0:ndt*kparam], zaa[0:ndl*kparam], \
                zgmid[0:nofc][0:3], f2n[0:nofc][0:3], \
                bgmid[0:nNode][0:3], \
                row[0:ndt], skip_cols[0:ndt])
    {
        int ill_h = il + nstrtl;
        double xp_h = zgmid[ill_h][0];
        double yp_h = zgmid[ill_h][1];
        double zp_h = zgmid[ill_h][2];

        double col_factors[50];
        for (int t = 0; t < k; t++) {
            col_factors[t] = zaa[t * ndl + il];
        }

        #pragma acc loop private(xf, yf, zf) \
                         firstprivate(ndt, k, nstrtt)
        for (it = 0; it < ndt; it++) {
            if (skip_cols[it] != 0) {
                if (k > 0) row[it] = 0.0;
            } else {
                int itt = it + nstrtt;

                int n0 = f2n[itt][0], n1 = f2n[itt][1], n2 = f2n[itt][2];
                xf[0] = bgmid[n0][0]; xf[1] = bgmid[n1][0]; xf[2] = bgmid[n2][0];
                yf[0] = bgmid[n0][1]; yf[1] = bgmid[n1][1]; yf[2] = bgmid[n2][1];
                zf[0] = bgmid[n0][2]; zf[1] = bgmid[n1][2]; zf[2] = bgmid[n2][2];

                double val = face_integral2(xf, yf, zf, xp_h, yp_h, zp_h);

                if (k > 0) {
                    double correction = 0.0;
                    for (int t = 0; t < k; t++) {
                        correction += zab[t * ndt + it] * col_factors[t];
                    }
                    row[it] = val - correction;
                } else {
                    row[it] = val;
                }
            }
        }
    }
    if (use_gpu) {
        PROF_ACC_WAIT();
        PROF_TIME_END(_cr_compute, *lr_compute_acc);
    }

    // D2H: update host
    if (use_gpu) {
        PROF_TIME_START(_cr_d2h);
        #pragma acc update host(row[0:ndt])
        PROF_ACC_WAIT();
        PROF_TIME_END(_cr_d2h, *lr_d2h_acc);
    }

    double row_end = get_time();
    if (!use_gpu) *lr_cpu_time += (row_end - row_start);
}

// ============================================================
// comp_col: 固定列 it，计算该列所有行的值
// 修正点3: compute 计时在 kernel 之前开始（包含 kernel launch）
// 修正点4: 使用 PROF_TIME_START/END
// ============================================================
void comp_col(double* zaa, double* zab,
              int ndl, int ndt, int k, int it,
              double* col,
              int nstrtl, int nstrtt,
              int* skip_rows,
              int nofc,
              double zgmid[][3],
              int f2n[][3],
              double bgmid[][3],
              int use_gpu,
              int id,
              double *lr_cpu_time,
              double *lr_h2d_acc,
              double *lr_compute_acc,
              double *lr_d2h_acc)
{
    double col_start = get_time();

    // H2D: update device（增量传输上一轮的 zab/zaa）
    if (use_gpu && k > 0) {
        PROF_TIME_START(_cc_h2d);
        int last = k - 1;
        #pragma acc update device(zab[last*ndt:ndt], \
                                  zaa[last*ndl:ndl])
        PROF_ACC_WAIT();
        PROF_TIME_END(_cc_h2d, *lr_h2d_acc);
    }

    // 修正点3: Compute 计时在 kernel 发射之前开始
    PROF_TIME_START(_cc_compute);
    #pragma acc parallel if(use_gpu) \
        present(zab[0:ndt*kparam], zaa[0:ndl*kparam], \
                zgmid[0:nofc][0:3], f2n[0:nofc][0:3], \
                bgmid[0:nNode][0:3], \
                col[0:ndl], skip_rows[0:ndl])
    {
        int itt_h = it + nstrtt;
        int n0_h = f2n[itt_h][0], n1_h = f2n[itt_h][1], n2_h = f2n[itt_h][2];
        double xf_h[3], yf_h[3], zf_h[3];
        xf_h[0] = bgmid[n0_h][0]; xf_h[1] = bgmid[n1_h][0]; xf_h[2] = bgmid[n2_h][0];
        yf_h[0] = bgmid[n0_h][1]; yf_h[1] = bgmid[n1_h][1]; yf_h[2] = bgmid[n2_h][1];
        zf_h[0] = bgmid[n0_h][2]; zf_h[1] = bgmid[n1_h][2]; zf_h[2] = bgmid[n2_h][2];

        double row_factors[50];
        for (int t = 0; t < k; t++) {
            row_factors[t] = zab[t * ndt + it];
        }

        #pragma acc loop firstprivate(ndl, k, nstrtl)
        for (int il = 0; il < ndl; il++) {
            if (skip_rows[il] != 0) {
                if (k > 0) col[il] = 0.0;
            } else {
                int ill = il + nstrtl;

                double xp = zgmid[ill][0];
                double yp = zgmid[ill][1];
                double zp = zgmid[ill][2];

                double val = face_integral2(xf_h, yf_h, zf_h, xp, yp, zp);

                if (k > 0) {
                    double correction = 0.0;
                    for (int t = 0; t < k; t++) {
                        correction += zaa[t * ndl + il] * row_factors[t];
                    }
                    col[il] = val - correction;
                } else {
                    col[il] = val;
                }
            }
        }
    }
    if (use_gpu) {
        PROF_ACC_WAIT();
        PROF_TIME_END(_cc_compute, *lr_compute_acc);
    }

    // D2H: update host
    if (use_gpu) {
        PROF_TIME_START(_cc_d2h);
        #pragma acc update host(col[0:ndl])
        PROF_ACC_WAIT();
        PROF_TIME_END(_cc_d2h, *lr_d2h_acc);
    }

    double col_end = get_time();
    if (!use_gpu) *lr_cpu_time += (col_end - col_start);
}

// ============================================================
// Utility functions
// ============================================================

#pragma acc routine seq
int minabsvalloc_d(double* za, int nd) {
    int il = 0;
    double zz = fabs(za[0]);
    for (int it = 1; it < nd; it++) {
        if (fabs(za[it]) < zz) {
            il = it;
            zz = fabs(za[it]);
        }
    }
    return il;
}

#pragma acc routine seq
int maxabsvalloc_d(double* za, int nd) {
    int il = 0;
    double zz = 0.0;
    for (int it = 0; it < nd; it++) {
        if (fabs(za[it]) > zz) {
            il = it;
            zz = fabs(za[it]);
        }
    }
    return il;
}

#pragma acc routine seq
double face_integral2(double xs[], double ys[], double zs[],
                      double x, double y, double z) {
    const double INV_4PI_EPS0 = 8.98755178736818e+09;

    double dx0 = xs[0]-x, dy0 = ys[0]-y, dz0 = zs[0]-z;
    double dx1 = xs[1]-x, dy1 = ys[1]-y, dz1 = zs[1]-z;
    double dx2 = xs[2]-x, dy2 = ys[2]-y, dz2 = zs[2]-z;

    double r0 = sqrt(dx0*dx0 + dy0*dy0 + dz0*dz0);
    double r1 = sqrt(dx1*dx1 + dy1*dy1 + dz1*dz1);
    double r2 = sqrt(dx2*dx2 + dy2*dy2 + dz2*dz2);

    double ux = dx1-dx0, uy = dy1-dy0, uz = dz1-dz0;
    double vx = dx2-dx1, vy = dy2-dy1, vz = dz2-dz1;
    double wx = uy*vz - uz*vy, wy = uz*vx - ux*vz, wz = ux*vy - uy*vx;
    double inv_dw = 1.0 / sqrt(wx*wx + wy*wy + wz*wz);
    wx *= inv_dw; wy *= inv_dw; wz *= inv_dw;

    double zp = -(dx0*wx + dy0*wy + dz0*wz);
    double zpabs = fabs(zp);

    double px0 = dx0+zp*wx, py0 = dy0+zp*wy, pz0 = dz0+zp*wz;
    double px1 = dx1+zp*wx, py1 = dy1+zp*wy, pz1 = dz1+zp*wz;
    double px2 = dx2+zp*wx, py2 = dy2+zp*wy, pz2 = dz2+zp*wz;

    double face_integral = 0.0;

    #define EDGE(pi_x,pi_y,pi_z, pj_x,pj_y,pj_z, ri, rj) \
    {                                                        \
        double xj = sqrt(pj_x*pj_x + pj_y*pj_y + pj_z*pj_z); \
        double inv_xj = 1.0 / xj;                           \
        double ejx = pj_x*inv_xj, ejy = pj_y*inv_xj, ejz = pj_z*inv_xj; \
        double fvx = wy*ejz - wz*ejy;                       \
        double fvy = wz*ejx - wx*ejz;                       \
        double fvz = wx*ejy - wy*ejx;                       \
        double xi = pi_x*ejx + pi_y*ejy + pi_z*ejz;         \
        double yi = pi_x*fvx + pi_y*fvy + pi_z*fvz;         \
        double ddx = xj - xi, ddy = -yi;                    \
        double inv_t = 1.0 / sqrt(ddx*ddx + ddy*ddy);       \
        double l = ddx*inv_t, m = ddy*inv_t;                \
        double d  = l*yi - m*xi;                             \
        double ti = l*xi + m*yi;                             \
        double tj = l*xj;                                    \
        double A = yi*zpabs*ti - xi*(ri)*d;                  \
        double B = yi*(ri)*d + xi*zpabs*ti;                  \
        face_integral += d*log((rj+tj)/(ri+ti))              \
                       - zpabs*(atan2(A,B) + atan2((rj)*d, zpabs*tj)); \
    }

    EDGE(px0,py0,pz0, px1,py1,pz1, r0, r1)
    EDGE(px1,py1,pz1, px2,py2,pz2, r1, r2)
    EDGE(px2,py2,pz2, px0,py0,pz0, r2, r0)

    #undef EDGE

    return fabs(face_integral) * INV_4PI_EPS0;
}

// ============================================================
// Cluster tree functions
// ============================================================

double dist_2cluster(int st_cltl, int st_cltt) {
    double zs = 0.0;
    for (int id = 0; id < resultCTlist[st_cltl].ndim; id++) {
        if (resultCTlist[st_cltl].bmax[id] < resultCTlist[st_cltt].bmin[id]) {
            double d = resultCTlist[st_cltt].bmin[id] - resultCTlist[st_cltl].bmax[id];
            zs += d * d;
        } else if (resultCTlist[st_cltt].bmax[id] < resultCTlist[st_cltl].bmin[id]) {
            double d = resultCTlist[st_cltl].bmin[id] - resultCTlist[st_cltt].bmax[id];
            zs += d * d;
        }
    }
    return sqrt(zs);
}

int create_cluster(int ndpth, int nstrt, int nsize, int ndim, int nson) {
    int st_clt = countCT;
    countCT++;
    resultCTlist[st_clt].nstrt = nstrt;
    resultCTlist[st_clt].nsize = nsize;
    resultCTlist[st_clt].ndim = ndim;
    resultCTlist[st_clt].nnson = nson;
    resultCTlist[st_clt].ndpth = ndpth;
    return st_clt;
}

int create_ctree_ssgeom(int st_clt,
                        double (*zgmid)[3],
                        int (*face2node)[3],
                        int ndpth,
                        int ndscd,
                        int nsrt,
                        int nd,
                        int md,
                        int ndim) {
    int id, il, nson;
    double minsz = 500;
    double zcoef = 1.1;
    double zlmin[ndim], zlmax[ndim];
    ndpth = ndpth + 1;

    if (nd <= minsz) {
        nson = 0;
        st_clt = create_cluster(ndpth, nsrt, nd, ndim, nson);
    } else {
        for (id = 0; id < ndim; id++) {
            zlmin[id] = zgmid[0][id];
            zlmax[id] = zlmin[id];
            for (il = 1; il < nd; il++) {
                double zg = zgmid[il][id];
                if (zg < zlmin[id])       zlmin[id] = zg;
                else if (zlmax[id] < zg)  zlmax[id] = zg;
            }
        }

        double zdiff = zlmax[0] - zlmin[0];
        int ncut = 0;
        for (id = 0; id < ndim; id++) {
            double zidiff = zlmax[id] - zlmin[id];
            if (zidiff > zcoef * zdiff) {
                zdiff = zidiff;
                ncut = id;
            }
        }

        double zlmid = 0.5 * (zlmax[ncut] + zlmin[ncut]);
        int nl = 0;
        int nr = nd - 1;

        while (nl < nr) {
            while (nl < nd && zgmid[nl][ncut] <= zlmid) nl++;
            while (nr >= 0 && zgmid[nr][ncut] > zlmid)  nr--;
            if (nl < nr) {
                for (id = 0; id < ndim; id++) {
                    double nh = zgmid[nl][id];
                    zgmid[nl][id] = zgmid[nr][id];
                    zgmid[nr][id] = nh;
                }
                for (id = 0; id < ndim; id++) {
                    int mh = face2node[nl][id];
                    face2node[nl][id] = face2node[nr][id];
                    face2node[nr][id] = mh;
                }
            }
        }

        if (nl == nd || nl == 0) {
            nson = 0;
            st_clt = create_cluster(ndpth, nsrt, nd, ndim, nson);
        } else {
            nson = 2;
            st_clt = create_cluster(ndpth, nsrt, nd, ndim, nson);

            int nsrt1 = nsrt;
            int nd1 = nl;
            resultCTlist[st_clt].offsets[0] = create_ctree_ssgeom(
                resultCTlist[st_clt].offsets[0], zgmid, face2node,
                ndpth, ndscd, nsrt1, nd1, md, ndim);

            nsrt1 = nsrt + nl;
            nd1 = nd - nl;
            resultCTlist[st_clt].offsets[1] = create_ctree_ssgeom(
                resultCTlist[st_clt].offsets[1], &zgmid[nl], &face2node[nl],
                ndpth, ndscd, nsrt1, nd1, md, ndim);
        }
    }

    resultCTlist[st_clt].ndscd = nd;

    double zeps = 1.0e-5;
    if (resultCTlist[st_clt].nnson > 0) {
        for (id = 0; id < ndim; id++) {
            resultCTlist[st_clt].bmin[id] = resultCTlist[resultCTlist[st_clt].offsets[0]].bmin[id];
            resultCTlist[st_clt].bmax[id] = resultCTlist[resultCTlist[st_clt].offsets[0]].bmax[id];
        }
        for (il = 1; il < resultCTlist[st_clt].nnson; il++) {
            for (id = 0; id < ndim; id++) {
                if (resultCTlist[resultCTlist[st_clt].offsets[il]].bmin[id] < resultCTlist[st_clt].bmin[id])
                    resultCTlist[st_clt].bmin[id] = resultCTlist[resultCTlist[st_clt].offsets[il]].bmin[id];
                if (resultCTlist[st_clt].bmax[id] < resultCTlist[resultCTlist[st_clt].offsets[il]].bmax[id])
                    resultCTlist[st_clt].bmax[id] = resultCTlist[resultCTlist[st_clt].offsets[il]].bmax[id];
            }
        }
    } else {
        for (id = 0; id < ndim; id++) {
            resultCTlist[st_clt].bmin[id] = zgmid[0][id];
            resultCTlist[st_clt].bmax[id] = zgmid[0][id];
        }
        for (id = 0; id < ndim; id++) {
            for (il = 1; il < resultCTlist[st_clt].nsize; il++) {
                if (zgmid[il][id] < resultCTlist[st_clt].bmin[id])
                    resultCTlist[st_clt].bmin[id] = zgmid[il][id];
                if (resultCTlist[st_clt].bmax[id] < zgmid[il][id])
                    resultCTlist[st_clt].bmax[id] = zgmid[il][id];
            }
        }
    }

    double zwdth = 0.0;
    for (id = 0; id < ndim; id++) {
        double d = resultCTlist[st_clt].bmax[id] - resultCTlist[st_clt].bmin[id];
        zwdth += d * d;
    }
    zwdth = sqrt(zwdth);

    for (id = 0; id < ndim; id++) {
        double bdiff = resultCTlist[st_clt].bmax[id] - resultCTlist[st_clt].bmin[id];
        if (bdiff < zeps * zwdth) {
            resultCTlist[st_clt].bmax[id] += 0.5 * (zeps * zwdth - bdiff);
            resultCTlist[st_clt].bmin[id] -= 0.5 * (zeps * zwdth - bdiff);
        }
    }

    zwdth = 0.0;
    for (id = 0; id < ndim; id++) {
        double d = resultCTlist[st_clt].bmax[id] - resultCTlist[st_clt].bmin[id];
        zwdth += d * d;
    }
    resultCTlist[st_clt].zwdth = sqrt(zwdth);

    return st_clt;
}

// ============================================================
// Timing function
// ============================================================

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}