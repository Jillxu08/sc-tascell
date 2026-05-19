// seconde paper version:Single-process + Single-GPU without MPI 

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
// #include <mkl.h>
#include <cblas.h>
#include <stdint.h>

extern struct cluster* resultCTlist;
extern double (*zgmid)[3];
extern double (*bgmid)[3];
extern int (*f2n)[3];
extern int nofc;
extern int nNode;
extern int th_ds, th_lr;
// #pragma acc declare create(zgmid, bgmid, f2n)
#pragma acc declare create(zgmid[0:nofc][0:3], f2n[0:nofc][0:3], bgmid[0:nNode][0:3])
#pragma acc declare create(resultCTlist)

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
  int ndscd;                         //number of descendants
  double bmin[3];
  double bmax[3];
  double zwdth;
  int offsets[2];
  int nnsons;
  long nnnd;
};

#pragma acc routine seq
double dot_product(double* v, double* u, int n);
#pragma acc routine seq
void cross_product(double* u, double* v, double* w);
#pragma acc routine seq
double face_integral2(double xs[], double ys[], double zs[], double x, double y, double z);
#pragma acc routine seq
double entry_ij(int i, int j);
#pragma acc routine seq
void adotsub_dsm(double* zr, double* zaa, double* zu, int im, int ndl, int ndt, int mdl, int mdt);

int acaplus(double* zaa, double* zab, int ndl, int ndt, int nstrtl, int nstrtt, int kmax, double eps, double znrmmat, double pACA_EPS);
void fill_sub_leafmtx(struct leafmtx *st_lf, double znrmmat);
#pragma acc routine seq 
int minabsvalloc_d(double* za, int nd);
#pragma acc routine seq
int maxabsvalloc_d(double* za, int nd);
// double entry_ij(int i, int j);
// double face_integral2(double xs[], double ys[], double zs[], double x, double y, double z);
void comp_col(double* zaa, double *zab, int ndl, int ndt, int k, int it, double* col, int nstrtl, int nstrtt, int* lrow_done, int use_gpu, int assigned_gpu);
void comp_row(double* zaa, double* zab, int ndl, int ndt, int k, int il, double* row, int nstrtl, int nstrtt, int* lrow_done, int use_gpu, int assigned_gpu);
// void cross_product(double* u, double* v, double* w);
// double dot_product(double* v, double* u, int n);
// void adotsub_dsm(double* zr, double* zaa, double* zu, int it, int ndl, int ndt, int mdl, int mdt);
// void adot_dsm(double* zau, double* zaa, double* zu, int im, int ndl, int ndt, int mdl, int mdt);
#pragma acc routine seq
int max(int a, int b);
#pragma acc routine seq
int min(int a, int b);

int create_ctree_ssgeom(int st_clt,double (*zgmid)[3],int (*face2node)[3],int ndpth,int ndscd,int nsrt,int nd,int md,int ndim);
double dist_2cluster(int st_cltl,int st_cltt);

// struct cluster* resultCTlist;
int countCT;
// double (*zgmid)[3];
// double (*bgmid)[3];
// int (*f2n)[3];

void init_gpu_pool();
void data_transfer();