#include <openacc.h>
#include "mm_c_th_acc.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <cublas_v2.h>


double c[N][N];
double a[N][N];
double b[N][N];
double T_b[N][N];
double x[1][N];
double y[N][1];
// double C[4][th][th];
double * C[4];
// extern void cublasDgemm(cublasHandle_t, cublasOperation_t,cublasOperation_t,int, int, int, const double,const double,const double,const double, double, int);


void mm_sub(int c_r, int c_c, int n, int id)
{
    int i,j,k;
    double t1, t2, t, t3, t_c;
    double * cc=C[id];

    t1 = get_time(); 
    // #pragma omp target device(id) map(to:a[c_r:n][:N],T_b[c_c:n][:N]) map(from:cc[:n*n])
    // #pragma omp teams distribute parallel for private(j,k)  
    // #pragma omp distribute parallel for private(j,k)  
    #pragma acc data copyin(a[c_r:n][0:N]), copyin(T_b[c_c:n][0:N]), copyout(cc[0:n*n])
    #pragma acc parallel loop collapse(2) 
        for (i = 0; i < n; i++)
        {
            for (j = 0; j < n; j++)
            {
                double sum = 0.0;
                for (k = 0; k < N; k++)
                {
                    sum += a[i+c_r][k] * T_b[j+c_c][k];
                }
                cc[i*n+j] = sum;
            }
        }
    
    // #pragma acc data copyin(a[c_r:n][0:N]), copyin(T_b[c_c:n][0:N]), copyout(cc[0:n*n])
    // {
       
    //     for (i = 0; i < n; i++)
    //     {
    //         #pragma acc kernels
    //         for (j = 0; j < N; j++)
    //         {
    //             x[1][j] = a[i+c_r][j];
    //             y[j][1] = T_b[i+c_c][j];
    //         }
    //         double sum[n][n] = 0.0;
    //         #pragma acc host_data use_device(x,y)
    //         {
    //             cublasHandle_t handle;
    //             cublasCreat(&handle);
    //             double alpha = 1.0;
    //             double beta = 0.0;
    //             cublasDgemm(handle,CUBLAS_OP_N,CUBLAS_OP_N,1,1,N,&alpha,a,1,T_b,N,&beta,sum,1);
                // for (k = 0; k < N; k++)
                // {
                //     sum += a[i+c_r][k] * T_b[j+c_c][k];
                // }
                // cc[i*n+j] = sum;
                
            // }
        // }
    // }


    t2 = get_time(); 
    t=t2-t1;
    printf("t=%f\n", t);

    for (i=0; i<n; i++)
    {
        for (j=0; j<n; j++)
        {
            c[i+c_r][j+c_c] = cc[i*n+j];
        }
    }
    // t3 = get_time(); 
    // t_c=t3-t1;
    // printf("t_c=%f\n", t_c);
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

void initC(int ngpu)
{
    int i;

    for (i=0; i<ngpu; i++)
    {
        C[i] = (double *)malloc(th * th * sizeof(double));
    }
}

double get_time(){
  struct timeval time;
  if (gettimeofday(&time,NULL)){
    return 0;
  }
  return (double)time.tv_sec + (double)time.tv_usec * .000001;
}