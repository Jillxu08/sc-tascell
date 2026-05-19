// #include <stdio.h>
// #include <stdlib.h>
// #include <math.h>
// #include <time.h>
// #include <sys/time.h>
// #include <mkl.h>
// #include <cublas_v2.h>
// extern cublasHandle_t handle;
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
extern int denseB;
extern int th_ds, th_lr;
// extern int m, n, p, q; // m: number of DS calls to GPU, n: number of DS calls to CPU
// extern int dense, low_rank;

extern pthread_mutex_t gpu_lock;

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

int acaplus(double* zaa, double* zab, int ndl, int ndt, int nstrtl, int nstrtt, int kmax, double eps, double znrmmat, double pACA_EPS, double* pa_ref, double* pb_ref, int* lrow_done, int* lcol_done, int nofc, double zgmid[][3], int f2n[][3], double bgmid[][3], int id, int use_gpu, double *lr_cpu_time, double *lr_gpu_time);
// int acaplus(double* zaa, double* zab, int ndl, int ndt, int nstrtl, int nstrtt, int kmax, double eps, double znrmmat, double pACA_EPS);
void adotsub_dsm(double* zr, double* zaa, double* zu, int it, int ndl, int ndt, int mdl, int mdt, double* zau);
void adot_dsm(double* zau, double* zaa, double* zu, int im, int ndl, int ndt, int mdl, int mdt);

// void comp_col(double* zaa, double *zab, int ndl, int ndt, int k, int it, double* col, int nstrtl, int nstrtt, int* lrow_done, double* zau, double zgmid[][3]);
void comp_col(double* zaa, double* zab, int ndl, int ndt, int k, int it, double* col, int nstrtl, int nstrtt, int* lrow_done, double* zau, int nofc, double zgmid[][3], int f2n[][3], double bgmid[][3], int use_gpu, int id, double *lr_cpu_time, double *lr_gpu_time);
// void comp_row(double* zaa, double* zab, int ndl, int ndt, int k, int il, double* row, int nstrtl, int nstrtt, int* lrow_done, double* zau);
void comp_row(double* zaa, double* zab, int ndl, int ndt, int k, int il, double* row, int nstrtl, int nstrtt, int* lrow_done, double* zau, int nofc, double zgmid[][3], int f2n[][3], double bgmid[][3], int use_gpu, int id, double *lr_cpu_time, double *lr_gpu_time);
void cross_product(double* u, double* v, double* w);
int create_ctree_ssgeom(int st_clt,double (*zgmid)[3],int (*face2node)[3],int ndpth,int ndscd,int nsrt,int nd,int md,int ndim);

double dist_2cluster(int st_cltl,int st_cltt);
double dot_product(double* v, double* u, int n);

double entry_ij(int i, int j);

void fill_sub_leafmtx(struct leafmtx *st_lf, double znrmmat, int id, int *gpu_lr_cnt, int *cpu_lr_cnt, int *gpu_ds_cnt, int *cpu_ds_cnt, uint64_t *gpu_lr_elem, uint64_t *cpu_lr_elem, uint64_t *gpu_ds_elem, uint64_t *cpu_ds_elem, uint64_t *pen_ds_gpu_subm, uint64_t *pen_ds_gpu_elem, uint64_t *pen_lr_gpu_subm, uint64_t *pen_lr_gpu_elem, double *ds_cpu_time, double *lr_cpu_time, double *ds_gpu_time, double *lr_gpu_time);
// void fill_sub_leafmtx(struct leafmtx *st_lf, double znrmmat, int id);
double face_integral2(double xs[], double ys[], double zs[], double x, double y, double z);

int max(int a, int b);
// int min(int a, int b);
int minabsvalloc_d(double* za, int nd);
int maxabsvalloc_d(double* za, int nd);


// double simple_dnrm2(int n, const double *x, int incx, int use_gpu);
void data_transfer();
void init_gpu_pool();
void check_gpu_usage();

// GPU设备管理
void assign_gpu_to_process();
int get_assigned_gpu();

int try_acquire_gpu_lock();
void release_gpu_lock();

void print_call_stats();
// void init_cublas();
// void cleanup_cublas();
double get_time1();
void get_time(struct timespec *ts);
// double diff_time(struct timespec *start, struct timespec *end);

double diff_time(const struct timespec *start, const struct timespec *end);