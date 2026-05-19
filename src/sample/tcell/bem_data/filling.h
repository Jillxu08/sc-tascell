// #include <stdio.h>
// #include <stdlib.h>
// #include <math.h>
// #include <time.h>
// #include <sys/time.h>
// #include <mkl.h>
// #include <cublas_v2.h>
// extern cublasHandle_t handle;
#ifndef FILLING_H
#define FILLING_H

#ifdef __STDC__
#include <stdint.h>
#else
// 手动定义 uint64_t
typedef unsigned long long uint64_t;
#endif

extern struct cluster* resultCTlist;
extern int countCT;
extern double (*zgmid)[3];
extern double (*bgmid)[3];
extern int (*f2n)[3];
extern int nofc;
extern int nNode;
// extern int denseB;
extern int th_ds, th_lr, th_minsz, num_gpus;
// extern int m, n, p, q; // m: number of DS calls to GPU, n: number of DS calls to CPU
// extern int dense, low_rank;

// extern pthread_mutex_t gpu_lock;

struct leafmtx{
  int ltmtx;                         //kind of the matrix; 1:rk 2:full
  int kt;                            //rank of the partition
  int nstrtl,ndl;                    //the coordination of the first element of partition
  int nstrtt,ndt;                    //the length & width of the partition
  double *a1;
  double *a2;                    //the elements of the partition, a2 is row, a1 is col
};  

struct cluster{
  int ndim;
  int nstrt,nsize,ndpth,nnson;
  int ndscd;             //number of descendants
  double bmin[3];
  double bmax[3];
  double zwdth;          //width of the cluster
  int offsets[2];
  int nnsons;    
  long nnnd;
};

// ============================================================
// Gantt图事件记录结构
// ============================================================
#define GANTT_MAX_EVENTS_PER_GPU 65536
#define TIMELINE_NUM_BINS 100

typedef struct {
    double start_time;     // 任务开始时间（相对于wall_start）
    double end_time;       // 任务结束时间
    int task_type;         // 0: dense, 1: low-rank
    int worker_id;         // 执行该任务的worker
    int ndl, ndt;          // 块尺寸
    int kt;                // 低秩块的秩（仅LR有效）
} gantt_event_t;

// ============================================================
// CPU任务统计结构
// ============================================================
typedef struct {
    // Dense CPU 统计
    uint64_t cpu_ds_tasks;
    uint64_t cpu_ds_fallback_tasks;   // GPU eligible但fallback的
    uint64_t cpu_ds_direct_tasks;     // 尺寸低于阈值直接CPU的
    double   cpu_ds_total_time;
    double   cpu_ds_min_time;
    double   cpu_ds_max_time;
    uint64_t cpu_ds_total_elements;
    
    // Low-rank CPU 统计
    uint64_t cpu_lr_tasks;
    uint64_t cpu_lr_fallback_tasks;
    uint64_t cpu_lr_direct_tasks;
    double   cpu_lr_total_time;
    double   cpu_lr_min_time;
    double   cpu_lr_max_time;
    uint64_t cpu_lr_total_elements;
    
    // CPU端ACA时间分解
    double   cpu_lr_entry_compute_time;   // face_integral2 元素计算时间
    double   cpu_lr_blas_time;            // cblas_dnrm2等BLAS运算时间
    
    // CPU端Dense时间分解
    double   cpu_ds_entry_compute_time;   // face_integral2 元素计算时间
} cpu_task_stats_t;

// ============================================================
// 任务到达率/处理率时间线结构
// ============================================================
typedef struct {
    int arrive_count[TIMELINE_NUM_BINS];      // 每个时间窗口的GPU候选任务到达数
    int gpu_success_count[TIMELINE_NUM_BINS];  // 每个时间窗口的GPU成功获取数
    int gpu_fail_count[TIMELINE_NUM_BINS];     // 每个时间窗口的GPU获取失败数
    int ds_arrive[TIMELINE_NUM_BINS];          // Dense到达
    int lr_arrive[TIMELINE_NUM_BINS];          // LR到达
} timeline_stats_t;


// ========== 矩阵填充函数 ==========
void fill_sub_leafmtx(struct leafmtx *st_lf, double znrmmat, int id, 
                      int *gpu_lr_cnt, int *cpu_lr_cnt, 
                      int *gpu_ds_cnt, int *cpu_ds_cnt, 
                      uint64_t *gpu_lr_elem, uint64_t *cpu_lr_elem, 
                      uint64_t *gpu_ds_elem, uint64_t *cpu_ds_elem, 
                      uint64_t *pen_ds_gpu_subm, uint64_t *pen_ds_gpu_elem, 
                      uint64_t *pen_lr_gpu_subm, uint64_t *pen_lr_gpu_elem, 
                      double *ds_cpu_time, double *lr_cpu_time, 
                      double *ds_gpu_time, double *lr_gpu_time);

// ========== ACA算法 ==========
int acaplus(double* zaa, double* zab, int ndl, int ndt, int nstrtl, int nstrtt, 
            int kmax, double eps, double znrmmat, double pACA_EPS, 
            double* pa_ref, double* pb_ref, int* lrow_done, int* lcol_done, 
            int nofc, double zgmid[][3], int f2n[][3], double bgmid[][3], 
            int id, int use_gpu, int acquired_gpu_id,
            double *lr_cpu_time, double *lr_gpu_time);

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
              double *lr_d2h_acc);
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
              double *lr_d2h_acc);

void adotsub_dsm(double* zr, double* zaa, double* zab, int it, int ndl, int ndt, int mdl, int mdt);
void adot_dsm(double* zau, double* zab, double* zu, int im, int ndl, int ndt, int mdl, int mdt);


// void cross_product(double* u, double* v, double* w);
int create_ctree_ssgeom(int st_clt,double (*zgmid)[3],int (*face2node)[3],int ndpth,int ndscd,int nsrt,int nd,int md,int ndim);

double dist_2cluster(int st_cltl,int st_cltt);
// double dot_product(double* v, double* u, int n);

// double entry_ij(int i, int j);
double face_integral2(double xs[], double ys[], double zs[], double x, double y, double z);
// int max(int a, int b);
static inline int imax(int a, int b);
int minabsvalloc_d(double* za, int nd);
int maxabsvalloc_d(double* za, int nd);
void data_transfer();
double get_time();

// ========== 多GPU动态分配管理 ==========
void initialize_multi_gpu();
int try_acquire_any_gpu(int worker_id);
void release_gpu(int gpu_id);

// ========== GPU使用统计 ==========
// record_gpu_usage_dense / record_gpu_usage_lr 为 static 函数，仅在 filling.c 内部使用
void print_gpu_usage_report();
void cleanup_multi_gpu();

// GPU configuration functions
void set_num_gpus(int n);           // 设置每个进程使用的GPU数量
int get_num_gpus(void);             // 获取当前使用的GPU数量
int get_gpus_per_process(void);     // 获取每个进程可用的最大GPU数量
int get_total_hw_gpus(void);        // 获取硬件总GPU数量
int get_gpu_start_id(void);         // 获取该进程的起始GPU ID
void set_gpu_report_filename(const char *filename);          // 设置GPU使用报告文件名
void set_job_id(const char *jid);                            // 设置Job ID（用于输出文件名标注）

int get_physical_gpu_id(int logical_gpu_id); // 获取物理GPU ID映射
void print_gpu_usage_summary();                         // 打印GPU使用摘要
int get_num_gpus(void);
#endif // FILLING_H


// // filling.h - Enhanced Version with Runtime Profiling Support
// // ============================================================
// // 所有 profiling 数据结构始终声明（无需编译期宏）
// // 运行时通过 PROF_ENABLED 控制是否收集数据
// // ============================================================
// #ifndef FILLING_H
// #define FILLING_H

// #ifdef __STDC__
// #include <stdint.h>
// #else
// typedef unsigned long long uint64_t;
// #endif

// extern struct cluster* resultCTlist;
// extern int countCT;
// extern double (*zgmid)[3];
// extern double (*bgmid)[3];
// extern int (*f2n)[3];
// extern int nofc;
// extern int nNode;
// extern int th_ds, th_lr, th_minsz, num_gpus;

// struct leafmtx{
//   int ltmtx;
//   int kt;
//   int nstrtl,ndl;
//   int nstrtt,ndt;
//   double *a1;
//   double *a2;
// };  

// struct cluster{
//   int ndim;
//   int nstrt,nsize,ndpth,nnson;
//   int ndscd;
//   double bmin[3];
//   double bmax[3];
//   double zwdth;
//   int offsets[2];
//   int nnsons;    
//   long nnnd;
// };

// // ============================================================
// // Profiling 数据结构（始终声明，运行时控制是否收集）
// // ============================================================

// #define GANTT_MAX_EVENTS_PER_GPU 200000
// #define TIMELINE_NUM_BINS 100

// typedef struct {
//     double start_time;
//     double end_time;
//     int task_type;      // 0: dense, 1: lowrank
//     int worker_id;
//     int ndl;
//     int ndt;
//     int kt;
// } gantt_event_t;

// typedef struct {
//     int arrive_count[TIMELINE_NUM_BINS];
//     int gpu_success_count[TIMELINE_NUM_BINS];
//     int gpu_fail_count[TIMELINE_NUM_BINS];
//     int ds_arrive[TIMELINE_NUM_BINS];
//     int lr_arrive[TIMELINE_NUM_BINS];
// } timeline_stats_t;

// typedef struct {
//     int cpu_ds_tasks;
//     int cpu_ds_fallback_tasks;
//     int cpu_ds_direct_tasks;
//     double cpu_ds_total_time;
//     uint64_t cpu_ds_total_elements;
//     double cpu_ds_min_time;
//     double cpu_ds_max_time;
//     int cpu_lr_tasks;
//     int cpu_lr_fallback_tasks;
//     int cpu_lr_direct_tasks;
//     double cpu_lr_total_time;
//     uint64_t cpu_lr_total_elements;
//     double cpu_lr_min_time;
//     double cpu_lr_max_time;
// } cpu_task_stats_t;

// // ========== 矩阵填充函数 ==========
// void fill_sub_leafmtx(struct leafmtx *st_lf, double znrmmat, int id, 
//                       int *gpu_lr_cnt, int *cpu_lr_cnt, 
//                       int *gpu_ds_cnt, int *cpu_ds_cnt, 
//                       uint64_t *gpu_lr_elem, uint64_t *cpu_lr_elem, 
//                       uint64_t *gpu_ds_elem, uint64_t *cpu_ds_elem, 
//                       uint64_t *pen_ds_gpu_subm, uint64_t *pen_ds_gpu_elem, 
//                       uint64_t *pen_lr_gpu_subm, uint64_t *pen_lr_gpu_elem, 
//                       double *ds_cpu_time, double *lr_cpu_time, 
//                       double *ds_gpu_time, double *lr_gpu_time);

// // ========== ACA算法 ==========
// int acaplus(double* zaa, double* zab, int ndl, int ndt, int nstrtl, int nstrtt, 
//             int kmax, double eps, double znrmmat, double pACA_EPS, 
//             double* pa_ref, double* pb_ref, int* lrow_done, int* lcol_done, 
//             int nofc, double zgmid[][3], int f2n[][3], double bgmid[][3], 
//             int id, int use_gpu, int acquired_gpu_id,
//             double *lr_cpu_time, double *lr_gpu_time);

// void comp_row(double* zaa, double* zab,
//               int ndl, int ndt, int k, int il,
//               double* row,
//               int nstrtl, int nstrtt,
//               int* skip_cols,
//               int nofc, double zgmid[][3], int f2n[][3], double bgmid[][3],
//               int use_gpu, int id,
//               double *lr_cpu_time,
//               double *lr_h2d_acc, double *lr_compute_acc, double *lr_d2h_acc);
// void comp_col(double* zaa, double* zab,
//               int ndl, int ndt, int k, int it,
//               double* col,
//               int nstrtl, int nstrtt,
//               int* skip_rows,
//               int nofc, double zgmid[][3], int f2n[][3], double bgmid[][3],
//               int use_gpu, int id,
//               double *lr_cpu_time,
//               double *lr_h2d_acc, double *lr_compute_acc, double *lr_d2h_acc);

// void adotsub_dsm(double* zr, double* zaa, double* zab, int it, int ndl, int ndt, int mdl, int mdt);
// void adot_dsm(double* zau, double* zab, double* zu, int im, int ndl, int ndt, int mdl, int mdt);

// int create_ctree_ssgeom(int st_clt,double (*zgmid)[3],int (*face2node)[3],int ndpth,int ndscd,int nsrt,int nd,int md,int ndim);
// double dist_2cluster(int st_cltl,int st_cltt);
// double face_integral2(double xs[], double ys[], double zs[], double x, double y, double z);
// static inline int imax(int a, int b);
// int minabsvalloc_d(double* za, int nd);
// int maxabsvalloc_d(double* za, int nd);
// void data_transfer();
// double get_time();

// // ========== 多GPU动态分配管理 ==========
// void initialize_multi_gpu();
// int try_acquire_any_gpu(int worker_id);
// void release_gpu(int gpu_id);

// // ========== GPU使用统计 ==========
// void print_gpu_usage_report();
// void cleanup_multi_gpu();

// void set_num_gpus(int n);
// int get_num_gpus(void);
// int get_gpus_per_process(void);
// int get_total_hw_gpus(void);
// int get_gpu_start_id(void);
// void set_gpu_report_filename(const char *filename);
// int get_physical_gpu_id(int logical_gpu_id);
// void print_gpu_usage_summary();

// void start_wall_clock();
// void stop_wall_clock();
// void set_wall_clock_time(double t);
// double get_wall_clock_time();
// void set_job_id(const char *jid);

// int get_gpu_acquire_attempts();
// int get_gpu_acquire_failures();
// int get_direct_cpu_ds_tasks();
// int get_direct_cpu_lr_tasks();
// int get_eligible_ds_tasks();
// int get_eligible_lr_tasks();

// // double dot_product(double* v, double* u, int n);
// // void cross_product(double* u, double* v, double* w);
// // int max(int a, int b);
// // int min(int a, int b);   

// #endif // FILLING_H