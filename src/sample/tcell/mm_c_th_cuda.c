#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <cublas_v2.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include "mm_c_th_acc.h"
// #include <cblas.h>

extern int dgemm_(char *transa, char *transb, int *m, int *
                  n, int *k, double *alpha, double *a, int *lda, double *b, int *
                  ldb, double *beta, double *c, int *ldc);

double c[N][N];
double a[N][N];
double b[N][N];
double T_b[N][N];
// double * C[4];
void mm_sub(int c_r, int c_c, int n, int id)
{
    int i,j,k,k1;
    // double time1, time2,time_dgemm;
    int N_Z = N;
    double alpha = 1.0f;
    double beta = 0.0f;
    // fprintf(stderr, "M0=%d\n",);
    double *A, *B;
    double *mm = (double *)malloc(sizeof(double) * n * n);
    if (!mm){
        printf("out of memory");
        exit(99);
    }
    // fprintf(stderr, "c_r=%d, c_c=%d, n=%d, id=%d\n", c_r, c_c, n, id);

    A = &a[c_r][0];  //th*N
    B = &T_b[c_c][0];   //N*th
    // 2*th*N add th*th <= memory
    // printf("A[0]=%f,A[1]=%f,A[2]=%f,A[3]=%f\n,A[4]=%f,A[5]=%f,A[6]=%f,A[7]=%f\n,B[0]=%f,B[1]=%f,B[2]=%f,B[3]=%f\n,B[4]=%f,B[5]=%f,B[6]=%f,B[7]=%f\n,mm[0]=%f,mm[1]=%f,mm[2]=%f,mm[3]=%f\n,mm[4]=%f,mm[5]=%f,mm[6]=%f,mm[7]=%f\n\n\n", A[0],A[1],A[2],A[3],A[4],A[5],A[6],A[7], B[0], B[1], B[2],B[3], B[4],B[5],B[6],B[7],mm[0], mm[1], mm[2],mm[3],mm[4],mm[5],mm[6],mm[7]);
    // time1=get_time();
    // printf("0: c_r=%d, c_c=%d, n=%d, id=%d\n", c_r, c_c, n, id);
    // fprintf(stderr,"0: c_r=%d, c_c=%d, n=%d, id=%d\n", c_r, c_c, n, id);
    // dgemm_("N", "T", &n, &n, &N_Z, &alpha, A, &n, B, &N_Z, &beta, mm, &n);
    dgemm_("T", "N", &n, &n, &N_Z, &alpha, B, &N_Z, A, &N_Z, &beta, mm, &n); 
                // fprintf(stderr, "N_Z1=%d\n",N_Z);
    // fprintf(stderr,"1: c_r=%d, c_c=%d, n=%d, id=%d\n", c_r, c_c, n, id);
    // time2=get_time();
    // time_dgemm = time2 - time1;
    // fprintf(stderr,"A[0]=%f,A[1]=%f,A[2]=%f,A[3]=%f\n,A[4]=%f,A[5]=%f,A[6]=%f,A[7]=%f\n,B[0]=%f,B[1]=%f,B[2]=%f,B[3]=%f\n,B[4]=%f,B[5]=%f,B[6]=%f,B[7]=%f\n,mm[0]=%f,mm[1]=%f,mm[2]=%f,mm[3]=%f\n,mm[4]=%f,mm[5]=%f,mm[6]=%f,mm[7]=%f\n\n\n", A[0],A[1],A[2],A[3],A[4],A[5],A[6],A[7], B[0], B[1], B[2],B[3], B[4],B[5],B[6],B[7],mm[0], mm[1], mm[2],mm[3],mm[4],mm[5],mm[6],mm[7]);
    // printf("time_dgemm=%f\n",time_dgemm);
 
     for (i=0; i<n; i++)
    {
        for (j=0; j<n; j++)
        {
            c[i+c_r][j+c_c] = mm[i*n+j];
        }
    }
    free(mm);
}

void mm_sub_cpu(int c_r, int c_c, int n)
{
    int i,j,k;
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n; j++)
        {
            double sum = 0.0;
            for (k = 0; k < N; k++)
            {
                sum += a[i+c_r][k] * T_b[j+c_c][k];
            }
            c[i+c_r][j+c_c] = sum;
        }
    }
}

double get_time(){
  struct timeval time;
  if (gettimeofday(&time,NULL)){
    return 0;
  }
  return (double)time.tv_sec + (double)time.tv_usec * .000001;
}






    // for (k = 0; k < n; k++)
        // {
        //     for(k1 = 0; k1 < n; k1++)
        //     {
                

                // for (i = 0; i < 1; i++)
                // {
                //     for (j = 0; j < N; j++)
                //     {
                //         A[i*N+j] = a[k+c_r][j];
                //     }
                // }

                // for (i = 0; i < N; i++)
                // {
                //     for (j = 0; j < 1; j++)
                //     {
                //         B[i*1+j] = b[i][k1+c_c];
                //     }
                // }
                // fprintf(stderr, "N_Z=%d\n",N_Z);

                // c[N][N]; a[N][N]; b[N][N];    
                // A [n, N]   B[N,n]  C[n, n]



// // openacc
// void mm_sub(int c_r, int c_c, int n, int id)
// {
//     int i,j,k;
//     double t1, t2, t, t3, t_c;
//     double * cc=C[id];

//     #pragma acc data copyin(a[c_r:n][0:N]), copyin(T_b[c_c:n][0:N]), copyout(cc[0:n*n])
//     #pragma acc parallel loop collapse(2) 
//         for (i = 0; i < n; i++)
//         {
//             for (j = 0; j < n; j++)
//             {
//                 double sum = 0.0;
//                 for (k = 0; k < N; k++)
//                 {
//                     sum += a[i+c_r][k] * T_b[j+c_c][k];
//                 }
//                 cc[i*n+j] = sum;
//             }
//         }

//     for (i=0; i<n; i++)
//     {
//         for (j=0; j<n; j++)
//         {
//             c[i+c_r][j+c_c] = cc[i*n+j];
//         }
//     }
// }


// void initC(int ngpu)
// {
//     int i;

//     for (i=0; i<ngpu; i++)
//     {
//         C[i] = (double *)malloc(th * th * sizeof(double));
//     }
// }