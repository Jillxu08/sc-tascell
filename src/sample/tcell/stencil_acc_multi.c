// flag for different subparts
// temporal blcoking -> gpu performance 
// points: 27=3*3*3, 3d; 25

#include "stencil_acc_multi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <omp.h>
#include <immintrin.h>
#include <unistd.h>
#ifdef _OPENACC
#include <openacc.h>
#endif

int th_gpu1, th_gpu2;
int th_cpu;
int It;
int BLOCK_LEVEL;

double (* restrict a)[N+2];
double (* restrict b)[N+2];
double (* restrict c)[N+2];
double (* restrict d)[N+2];
double (* restrict material)[N+2];
double * A [64];
double * B [64];
// double * M [64];

void init_gpu(int n)
{   
    char *name;
    name = (char *)malloc(20 * sizeof(int));
    int len;
    gethostname(name, &len);
    printf("my processor is %s\n",name);
    
    for(int k=0; k < 64 ; k++){
        A[k] = (double (*))malloc((th_cpu+2*BLOCK_LEVEL) * (th_cpu+2*BLOCK_LEVEL) * sizeof(double));
        B[k] = (double (*))malloc((th_cpu+2*BLOCK_LEVEL) * (th_cpu+2*BLOCK_LEVEL) * sizeof(double));
        // M[k] = (double (*))malloc((th_cpu+2*BLOCK_LEVEL) * (th_cpu+2*BLOCK_LEVEL) * sizeof(double));
    }
    #pragma acc data copyin(a[0:n+2][0:n+2], b[0:n+2][0:n+2])
    {
        #pragma acc parallel loop collapse(2)
        for(int j=1; j < (1+n); j++){
            for(int i=1; i < (1+n); i++){
                b[j][i] = delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]);
                // if(a[j][i] > MAX_TEMP){
                //     b[j][i] = a[j][i] - 0.1 * (a[j][i] - delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]));
                // }
                // else
                // {
                //     b[j][i] = delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]);
                // }
                // b[j][i] = delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]);
                // b[j][i] = delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] 
                // + a[j-1][i-1] + a[j-1][i+1] + a[j+1][i-1] + a[j+1][i+1] + a[j][i]);
    }}}
}

void cpu(int x, int y, int n, int id, int it)
{   
    for(int j=x; j < (x+n); j++){
        for(int i=y; i < (y+n); i++){
            b[j][i] = delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]);  
            // if(a[j][i] > MAX_TEMP)
            //     {
            //         // fprintf(stderr,"----------  ------------\n");
            //         b[j][i] = delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i] + a[j-1][i-1] + a[j-1][i+1] + a[j+1][i-1] + a[j+1][i+1]);
            //     }
            //     else{
            //         b[j][i] = delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]);  
            //     }
            // b[j][i] = delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] 
            // + a[j-1][i-1] + a[j-1][i+1] + a[j+1][i-1] + a[j+1][i+1] + a[j][i]);
    }}
}

void seq_cpu(int j, int i, int it)
{     
    // double conductivity;
    // // 根据材料类型选择热传导系数
    // if (material[j][i] == 0) {
    // conductivity = CONDUCTIVITY_0;
    // } else if (material[j][i] == 1) {
    // conductivity = CONDUCTIVITY_1;
    // } else if (material[j][i] == 2) {
    // conductivity = CONDUCTIVITY_2;
    // } else {
    // // 默认传导系数（错误情况）
    // conductivity = 0.0;
    // }
    d[j][i] = delT*(c[j+1][i] + c[j-1][i] + c[j][i+1] + c[j][i-1] + c[j][i]);  
    // d[j][i] = c[j][i] - 0.1 * (c[j][i] - delT*(c[j+1][i] + c[j-1][i] + c[j][i+1] + c[j][i-1] + c[j][i]));
    // if(c[j][i] > MAX_TEMP)
    //     {
    // //         // fprintf(stderr,"---------- seq_cpu ------------\n");
    // //         // d[j][i] = delT1*(c[j+1][i] + c[j-1][i] + c[j][i+1] + c[j][i-1] + c[j][i]+ c[j-1][i-1] + c[j-1][i+1] + c[j+1][i-1] + c[j+1][i+1]);
    //         d[j][i] = c[j][i] - 0.1 * (c[j][i] - delT*(c[j+1][i] + c[j-1][i] + c[j][i+1] + c[j][i-1] + c[j][i]));
    //     }
    //     else if{
    //         d[j][i] = delT*(c[j+1][i] + c[j-1][i] + c[j][i+1] + c[j][i-1] + c[j][i]);  
    //     }
        // fprintf(stderr,"seq_cpu: new_b[%d][%d]=%f(%p), c[%d][%d]=%f(%p), c[%d][%d]=%f, c[%d][%d]=%f, c[%d][%d]=%f, c[%d][%d]=%f\n", j, i, d[j][i], &d[j][i], j+1, i, c[j+1][i], &c[j+1][i], j-1, i, c[j-1][i], j, i+1, c[j][i+1], j, i-1, c[j][i-1], j, i, c[j][i]);
}

void gpu(int x, int y, int n, int id, int it)
{
    // fprintf(stderr,"---------- nn= %d ------------\n", it);
    #pragma acc data copyin(a[x-1:n+2][y-1:n+2]) copyout(b[x:n][y:n])
    {
        #pragma acc parallel loop collapse(2)
        for(int j=x; j < (x+n); j++){        
            for(int i=y; i < (y+n); i++){
                b[j][i] = delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]);
                // if(a[j][i] > MAX_TEMP)
                // {
                //     b[j][i] = a[j][i] - 0.1 * (a[j][i] - delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]));
                // }
                // else{
                //     b[j][i] = delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]);  
                // // b[j][i] = delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] 
                // // + a[j-1][i-1] + a[j-1][i+1] + a[j+1][i-1] + a[j+1][i+1] + a[j][i]);
                // }
            }
        }
    }
}

void sgpu_tb(int x, int y, int n, int id, int it)
{
    int blockEnd;
    int x_s, x_e, y_s, y_e, len_x, len_y;
    int xx_s, yy_s, xx_e, yy_e;
    int nn = n+2*BLOCK_LEVEL;
    // int nn = th_gpu1+2*BLOCK_LEVEL;
    // fprintf(stderr,"---------- nn= %d ------------\n", nn);
    
    double (* new_b)[nn];
    new_b = (double (*)[nn])malloc(nn * nn * sizeof(double));

    // double (* new_b)[nn] = B[id];
    // fprintf(stderr,"th_gpu1+2*BLOCK_LEVEL=%d\n",th_gpu1+2*BLOCK_LEVEL);

    // new_b = (double (*)[nn])malloc(nn * nn * sizeof(double));
    // for(int j=0; j < th_gpu+2*BLOCK_LEVEL; j++) {
    //     for(int i=0 ; i < th_gpu+2*BLOCK_LEVEL; i++) {
    //         new_b[j][i] = b[j][i]; 
    //     }}

    xx_s= (x - BLOCK_LEVEL < 0) ? 0 : x - BLOCK_LEVEL; 
    yy_s= (y - BLOCK_LEVEL < 0) ? 0 : y - BLOCK_LEVEL;
    xx_e = (x + n - 1 + BLOCK_LEVEL > N + 1) ? N + 1 : x + n - 1 + BLOCK_LEVEL;
    yy_e = (y + n - 1 + BLOCK_LEVEL > N + 1) ? N + 1 : y + n - 1 + BLOCK_LEVEL;
    len_x = xx_e-xx_s+1;
    len_y = yy_e-yy_s+1;
    // fprintf(stderr,"---------- 1 ------------\n");

    if(it+BLOCK_LEVEL < It){
        blockEnd = it + BLOCK_LEVEL;} 
        else {
            blockEnd = It;} 
     
    x_s =  x - BLOCK_LEVEL; 
    y_s =  y - BLOCK_LEVEL;
    x_e =  x + n - 1 + BLOCK_LEVEL;
    y_e =  y + n - 1 + BLOCK_LEVEL;

    int nb_xx_s, nb_yy_s,  nb_xx_e, nb_yy_e, nb_len_x, nb_len_y;
    nb_xx_s= (x - BLOCK_LEVEL < 0) ? BLOCK_LEVEL-1 : 1; 
    nb_yy_s= (y - BLOCK_LEVEL < 0) ? BLOCK_LEVEL-1 : 1;
    nb_xx_e = (x + n - 1 + BLOCK_LEVEL > N + 1) ? n+BLOCK_LEVEL : n-1+2*BLOCK_LEVEL;
    nb_yy_e = (y + n - 1 + BLOCK_LEVEL > N + 1) ? n+BLOCK_LEVEL : n-1+2*BLOCK_LEVEL;
    nb_len_x = nb_xx_e-nb_xx_s+1;
    nb_len_y = nb_yy_e-nb_yy_s+1;
    // fprintf(stderr,"---------- 2 ------------\n");
    
    // Allocate memory and copy data to the GPU
    // int gpu_id = id;  // 假设要使用的 GPU ID 为 1
    // acc_set_device_num(gpu_id, acc_device_nvidia);
    // fprintf(stderr,"x=%d, y=%d:\n xx_s=%d, yy_s=%d,  xx_e=%d, yy_e=%d, len_x=%d, len_y=%d;\n nb_xx_s=%d, nb_yy_s=%d,  nb_xx_e=%d, nb_yy_e=%d, nb_len_x=%d, nb_len_y=%d\n", 
    //                 x, y, xx_s, yy_s,  xx_e, yy_e, len_x, len_y, nb_xx_s, nb_yy_s,  nb_xx_e, nb_yy_e, nb_len_x, nb_len_y);
    #pragma acc data copyin(x, y, n, nb_xx_s, nb_yy_s, nb_xx_e, nb_yy_e, nb_len_x, nb_len_y, xx_s, yy_s, xx_e, yy_e,len_x, len_y, a[xx_s:len_x][yy_s:len_y]) create(new_b[nb_xx_s:nb_len_x][nb_yy_s:nb_len_y]) copyout(b[x:n][y:n])
    {
        // fprintf(stderr,"---------- 3 ------------\n");
        for (int iter = it; iter < blockEnd-1; iter++) {
            int j_x_s = (x_s < 1) ? 1 : x_s+1;
            int j_x_e = (x_e > N ) ? N+1 : x_e;
            int i_y_s = (y_s < 1) ? 1 : y_s+1; 
            int i_y_e = (y_e > N ) ? N+1 : y_e;
            // fprintf(stderr,"---------- 4 ------------\n");
            // double conductivity;

            #pragma acc kernels present(nb_xx_s, nb_yy_s, nb_xx_e, nb_yy_e, nb_len_x, xx_s, yy_s, xx_e, yy_e, len_x, len_y, a[xx_s:len_x][yy_s:len_y], new_b[nb_xx_s:nb_len_x][nb_yy_s:nb_len_y]) 
            // #pragma acc kernels loop, gang(32), worker(16)
            for(int j=j_x_s; j < j_x_e; j++) {
                // #pragma acc loop gang(16), worker(32)
                for(int i=i_y_s ; i < i_y_e; i++) {
                    // double conductivity;
                    // // double conductivity = 0.1;
                    // // // 根据材料类型选择热传导系数
                    // if (material[j][i] == 0) {
                    //     conductivity = CONDUCTIVITY_0;
                    // } 
                    // else if (material[j][i] == 1) {
                    //     conductivity = CONDUCTIVITY_1;
                    // } 
                    // else if (material[j][i] == 2) {
                    //     conductivity = CONDUCTIVITY_2;
                    // } 
                    // else {
                    // // 默认传导系数（错误情况）
                    //     conductivity = 0.0;
                    // }
                    // printf('1\n');
                    new_b[j-j_x_s+nb_xx_s][i-i_y_s+nb_yy_s] = delT * (a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]);
                    // new_b[j-j_x_s+nb_xx_s][i-i_y_s+nb_yy_s] = a[j][i] - 0.1 * (a[j][i] - delT * (a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]));
                    // if(a[j][i] > MAX_TEMP){
                    // //     // printf("---------- sgpu_tb ------------\n");
                    // //     // Apply different update rule if temperature exceeds threshold
                    // //     // new_b[j-j_x_s+nb_xx_s][i-i_y_s+nb_yy_s] =  delT1 * (a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i] + a[j-1][i-1] + a[j-1][i+1] + a[j+1][i-1] + a[j+1][i+1]);
                    //     new_b[j-j_x_s+nb_xx_s][i-i_y_s+nb_yy_s] = a[j][i] - 0.1 * (a[j][i] - delT * (a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]));
                    //     // new_b[j-j_x_s+nb_xx_s][i-i_y_s+nb_yy_s] = 25.0;
                    // }
                    // else{
                    // //     // 5 points stencil computation
                    //     new_b[j-j_x_s+nb_xx_s][i-i_y_s+nb_yy_s] = delT * (a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]);
                    // }
                    // printf("else: %f(%p)=new_b[%d][%d], a[%d][%d]=%f(%p), a[%d][%d]=%f, a[%d][%d]=%f, a[%d][%d]=%f, a[%d][%d]=%f\n", new_b[j-j_x_s+nb_xx_s][i-i_y_s+nb_yy_s], &new_b[j-j_x_s+nb_xx_s][i-i_y_s+nb_yy_s], j-j_x_s+nb_xx_s, i-i_y_s+nb_yy_s, j+1, i, a[j+1][i], &a[j+1][i], j-1, i, a[j-1][i], j, i+1, a[j][i+1], j, i-1, a[j][i-1], j, i, a[j][i]);
                }
            }

            #pragma acc kernels present(nb_xx_s, nb_yy_s, nb_xx_e, nb_yy_e, nb_len_x, xx_s, yy_s, len_x, len_y, a[xx_s:len_x][yy_s:len_y], new_b[nb_xx_s:nb_len_x][nb_yy_s:nb_len_y]) 
            // #pragma acc kernels loop
            for(int j=j_x_s; j < j_x_e; j++) {
                // #pragma acc loop gang(16), worker(32)
                for(int i=i_y_s ; i < i_y_e; i++) {
                    a[j][i] = new_b[j-j_x_s+nb_xx_s][i-i_y_s+nb_yy_s];               
                    // printf("new_b[%d][%d]=%f, a[%d][%d]=%f\n", j, i, new_b[j][i], j, i, a[j][i]);
                }}
            x_s++;
            y_s++;
            x_e--;
            y_e--;
        }

        // final iter
        #pragma acc kernels present(xx_s, yy_s, len_x, len_y, a[xx_s:len_x][yy_s:len_y], b[x:n][y:n]) 
        for(int j=x; j < (x+n); j++){
            for(int i=y; i < (y+n); i++){
                // double conductivity;
                // // 根据材料类型选择热传导系数
                // if (material[j][i] == 0) {
                // conductivity = CONDUCTIVITY_0;
                // } else if (material[j][i] == 1) {
                // conductivity = CONDUCTIVITY_1;
                // } else if (material[j][i] == 2) {
                // conductivity = CONDUCTIVITY_2;
                // } else {
                // // 默认传导系数（错误情况）
                // conductivity = 0.0;
                // }
                b[j][i] =  delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]);
                // b[j][i] = a[j][i] - 0.1 * (a[j][i] - delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]));
                // if(a[j][i] > MAX_TEMP){
                // //     // printf("----------last sgpu_tb ------------\n");
                // //     // b[j][i] = delT1 * (a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i] + a[j-1][i-1] + a[j-1][i+1] + a[j+1][i-1] + a[j+1][i+1]);
                //     b[j][i] = a[j][i] - 0.1 * (a[j][i] - delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]));
                // }
                // else{
                //     b[j][i] = delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j][i]);
                // //         // + a[j-1][i-1] + a[j-1][i+1] + a[j+1][i-1] + a[j+1][i+1] );
                // }
                // printf("else: b[%d][%d]=%f(%p), a[%d][%d]=%f(%p), a[%d][%d]=%f, a[%d][%d]=%f, a[%d][%d]=%f, a[%d][%d]=%f\n", j, i, b[j][i], &b[j][i], j+1, i, a[j+1][i], &a[j+1][i], j-1, i, a[j-1][i], j, i+1, a[j][i+1], j, i-1, a[j][i-1], j, i, a[j][i]);
            }}      
    }
    // free(new_b);
}
void scpu_tb(int x, int y, int n, int id, int it)
{
    int blockEnd;
    int x_s, x_e, y_s, y_e;
    int xx_s, yy_s, xx_e, yy_e;
    int nn = n+2*BLOCK_LEVEL;

    double (* new_a)[nn]=A[id];
    double (* new_b)[nn]=B[id];
    // double (* new_m)[nn]=M[id];
    // double (* new_a)[nn];
    // double (* new_b)[nn];
    // new_a = (double (*)[nn])malloc(nn * nn * sizeof(double));
    // new_b = (double (*)[nn])malloc(nn * nn * sizeof(double));

    x_s =  x - BLOCK_LEVEL; 
    y_s =  y - BLOCK_LEVEL;
    x_e =  x + n - 1 + BLOCK_LEVEL;
    y_e =  y + n - 1 + BLOCK_LEVEL;

    xx_s= (x_s < 0) ? 0 : x_s; 
    yy_s= (y_s < 0) ? 0 : y_s;
    xx_e = (x_e > N + 1) ? N + 1 : x_e;
    yy_e = (y_e > N + 1) ? N + 1 : y_e;

    if(it+BLOCK_LEVEL < It){
        blockEnd = it + BLOCK_LEVEL;} 
        else {
            blockEnd = It;}
    
    int n_xx_s, n_yy_s,  n_xx_e, n_yy_e;
    n_xx_s= (x_s < 0) ? BLOCK_LEVEL-1 : 0; 
    n_yy_s= (y_s < 0) ? BLOCK_LEVEL-1 : 0;
    n_xx_e = (x_e > N + 1) ? th_cpu+BLOCK_LEVEL : th_cpu-1+2*BLOCK_LEVEL;
    n_yy_e = (y_e > N + 1) ? th_cpu+BLOCK_LEVEL : th_cpu-1+2*BLOCK_LEVEL;

    // for(int j=xx_s; j < xx_e+1; j++) {
    //     #pragma omp simd
    //     for(int i=yy_s ; i < yy_e+1; i++) {
    //         new_a[j-xx_s+n_xx_s][i-yy_s+n_yy_s] = a[j][i]; 
    //         new_b[j-xx_s+n_xx_s][i-yy_s+n_yy_s] = b[j][i];
    //         // fprintf(stderr,"new_a[%d][%d]=%f(%p)\n", j-xx_s+n_xx_s, i-yy_s+n_yy_s, new_a[j-xx_s+n_xx_s][i-yy_s+n_yy_s],&new_a[j-xx_s+n_xx_s][i-yy_s+n_yy_s]);
    //     }}

    // fprintf(stderr,"x=%d, y=%d:\n xx_s=%d, yy_s=%d,  xx_e=%d, yy_e=%d, n=%d\n", 
    //                 x, y, xx_s, yy_s,  xx_e, yy_e, n);
    #pragma omp simd
    for(int j=xx_s; j < xx_e+1; j++) {
        memcpy(&new_a[j-xx_s+n_xx_s][n_yy_s], &a[j][yy_s], (yy_e+1-yy_s) * sizeof(double));
        memcpy(&new_b[j-xx_s+n_xx_s][n_yy_s], &b[j][yy_s], (yy_e+1-yy_s) * sizeof(double));
        // memcpy(&new_m[j-xx_s+n_xx_s][n_yy_s], &material[j][yy_s], (yy_e+1-yy_s) * sizeof(double));
    }
    // fprintf(stderr,"new_a[%d][%d]=%f\n");
    // fprintf(stderr,"x=%d, y=%d:\n xx_s=%d, yy_s=%d,  xx_e=%d, yy_e=%d, n=%d\n", 
                // x, y, xx_s, yy_s,  xx_e, yy_e, n);
    for(int iter = it; iter < blockEnd-1; iter++) {  
        int j_x_s = (x_s < 1) ? BLOCK_LEVEL : n_xx_s+1;
        int j_x_e = (x_e > N ) ? th_cpu+BLOCK_LEVEL : n_xx_e;
        int i_y_s = (y_s < 1) ? BLOCK_LEVEL : n_yy_s+1; 
        int i_y_e = (y_e > N ) ? th_cpu+BLOCK_LEVEL : n_yy_e;
        // fprintf(stderr,"x=%d, y=%d:\n xx_s=%d, yy_s=%d,  xx_e=%d, yy_e=%d, j_x_s=%d, j_x_e=%d, i_y_s=%d, i_y_e=%d, n=%d\n", 
        //             x, y, xx_s, yy_s,  xx_e, yy_e, j_x_s, j_x_e, i_y_s, i_y_e, n);
        // #pragma omp parallel for collapse(2) simd
        for(int j=j_x_s; j < j_x_e; j++) {
            #pragma omp simd
            for(int i=i_y_s; i < i_y_e; i++) {
                // double conductivity;          
                // if (new_m[j][i] == 0) {
                // conductivity = CONDUCTIVITY_0;
                // } else if (new_m[j][i] == 1) {
                // conductivity = CONDUCTIVITY_1;
                // } else if (new_m[j][i] == 2) {
                // conductivity = CONDUCTIVITY_2;
                // } else {
                // // 默认传导系数（错误情况）
                // conductivity = 0.0;
                // }
                // fprintf(stderr,"material[%d][%d]=%f\n", j+xx_s-n_xx_s, i+yy_s-n_yy_s, material[j+xx_s-n_xx_s][i+yy_s-n_yy_s]);
                new_b[j][i] =  delT * (new_a[j+1][i] + new_a[j-1][i] + new_a[j][i+1] + new_a[j][i-1] + new_a[j][i]);
                // fprintf(stderr,"material[%d][%d]=%f, new_b[%d][%d]=%f\n", j+xx_s-n_xx_s, i+yy_s-n_yy_s, material[j+xx_s-n_xx_s][i+yy_s-n_yy_s], j, i, new_b[j][i]);
                // new_b[j][i] = new_a[j][i] - 0.1 * (new_a[j][i] - delT * (new_a[j+1][i] + new_a[j-1][i] + new_a[j][i+1] + new_a[j][i-1] + new_a[j][i]));
                // if(new_a[j][i] > MAX_TEMP){
                // //     // fprintf(stderr,"---------- scpu_tb ------------\n");
                // //     // new_b[j][i] = delT1 * (new_a[j+1][i] + new_a[j-1][i] + new_a[j][i+1] + new_a[j][i-1] + new_a[j][i] + new_a[j-1][i-1] + new_a[j-1][i+1] + new_a[j+1][i-1] + new_a[j+1][i+1]);
                //     new_b[j][i] = new_a[j][i] - 0.1 * (new_a[j][i] - delT * (new_a[j+1][i] + new_a[j-1][i] + new_a[j][i+1] + new_a[j][i-1] + new_a[j][i]));
                // //     // fprintf(stderr,"if: new_b[j][i]=%f, new_a[j+1][i]=%f, new_a[j-1][i]=%f, new_a[j][i+1]=%f, new_a[j][i-1]=%f, new_a[j][i]=%f\n", new_b[j][i], new_a[j+1][i], new_a[j-1][i], new_a[j][i+1], new_a[j][i-1], new_a[j][i]);
                // }
                // else
                // {
                //     new_b[j][i] = delT * (new_a[j+1][i] + new_a[j-1][i] + new_a[j][i+1] + new_a[j][i-1] + new_a[j][i]);
                // //     // fprintf(stderr,"else: new_b[%d][%d]=%f(%p), new_a[%d][%d]=%f(%p), new_a[%d][%d]=%f, new_a[%d][%d]=%f, new_a[%d][%d]=%f, new_a[%d][%d]=%f\n", j, i, new_b[j][i], &new_b[j][i], j+1, i, new_a[j+1][i], &new_a[j+1][i], j-1, i, new_a[j-1][i], j, i+1,  new_a[j][i+1], j, i-1, new_a[j][i-1], j, i, new_a[j][i]);
                // //             // + new_a[j-1][i-1] + new_a[j-1][i+1] + new_a[j+1][i-1] + new_a[j+1][i+1] + new_a[j][i]);
                // }
        }}
        double (* tmp)[nn];
        tmp=new_a;
        new_a=new_b;
        new_b=tmp;
        // for(int j=j_x_s; j < j_x_e; j++) {
        //     for(int i=i_y_s; i < i_y_e; i++) {
        //         new_a[j][i] = new_b[j][i];
        //         // fprintf(stderr,"new_b[%d][%d]=%f, new_a[%d][%d]=%f\n", j, i, new_b[j][i], j, i, new_a[j][i]);
        //     }}
        n_xx_s++;
        n_yy_s++;
        n_xx_e--;
        n_yy_e--;
    }

    // #pragma omp parallel for collapse(2) simd
    int tx = x-BLOCK_LEVEL;
    int ty = y-BLOCK_LEVEL;

    for(int j=x; j < (x+n); j++){
        #pragma omp simd
        for(int i=y; i < (y+n); i++){
            // double conductivity;
            // // 根据材料类型选择热传导系数
            // if (material[j][i] == 0) {
            // conductivity = CONDUCTIVITY_0;
            // } else if (material[j][i] == 1) {
            // conductivity = CONDUCTIVITY_1;
            // } else if (material[j][i] == 2) {
            // conductivity = CONDUCTIVITY_2;
            // } else {
            // // 默认传导系数（错误情况）
            // conductivity = 0.0;
            // }
            b[j][i] =  delT * (new_a[j-tx+1][i-ty] + new_a[j-tx-1][i-ty] + new_a[j-tx][i-ty+1] + new_a[j-tx][i-ty-1] + new_a[j-tx][i-ty]);
            // b[j][i] = new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL] - 0.1 * (new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL] -  delT * (new_a[j-x+BLOCK_LEVEL+1][i-y+BLOCK_LEVEL] + new_a[j-x+BLOCK_LEVEL-1][i-y+BLOCK_LEVEL] + new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL+1] + new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL-1] + new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL]));
            // if(new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL] > MAX_TEMP)
            // {
            // //     // fprintf(stderr,"---------- last scpu_tb ------------\n");
            // //     // b[j][i] = delT1 * (new_a[j-x+BLOCK_LEVEL+1][i-y+BLOCK_LEVEL] + new_a[j-x+BLOCK_LEVEL-1][i-y+BLOCK_LEVEL] + new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL+1] + new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL-1] + new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL]
            // //     //    + new_a[j-x+BLOCK_LEVEL-1][i-y+BLOCK_LEVEL-1] + new_a[j-x+BLOCK_LEVEL-1][i-y+BLOCK_LEVEL+1] + new_a[j-x+BLOCK_LEVEL+1][i-y+BLOCK_LEVEL-1] + new_a[j-x+BLOCK_LEVEL+1][i-y+BLOCK_LEVEL+1]);
            //     b[j][i] = new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL] - 0.1 * (new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL] -  delT * (new_a[j-x+BLOCK_LEVEL+1][i-y+BLOCK_LEVEL] + new_a[j-x+BLOCK_LEVEL-1][i-y+BLOCK_LEVEL] + new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL+1] + new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL-1] + new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL]));
            // }
            // else
            // {
            //     b[j][i] = delT * (new_a[j-x+BLOCK_LEVEL+1][i-y+BLOCK_LEVEL] + new_a[j-x+BLOCK_LEVEL-1][i-y+BLOCK_LEVEL] + new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL+1] + new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL-1] + new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL]);
            // //        // + new_a[j-x+BLOCK_LEVEL-1][i-y+BLOCK_LEVEL-1] + new_a[j-x+BLOCK_LEVEL-1][i-y+BLOCK_LEVEL+1] + new_a[j-x+BLOCK_LEVEL+1][i-y+BLOCK_LEVEL-1] + new_a[j-x+BLOCK_LEVEL+1][i-y+BLOCK_LEVEL+1] + new_a[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL]);
            // //     // fprintf(stderr,"b[%d][%d]=%f, new_a[%d][%d]=%f, new_a[%d][%d]=%f, new_a[%d][%d]=%f, new_a[%d][%d]=%f, new_a[%d][%d]=%f\n", j, i, b[j][i], j+1, i, new_a[j+1][i], j-1, i, new_a[j-1][i], j, i+1,  new_a[j][i+1], j, i-1, new_a[j][i-1], j, i, new_a[j][i]);            
            // }
        }}
    // free(new_a);
    // free(new_b);
    // free(tmp);
}

// with temporal blocking
void cpu_tb(int x, int y, int n, int it)
{
    int blockEnd;
    int x_s, x_e, y_s, y_e;
    int xx_s, yy_s, xx_e, yy_e;
  
    int nn = th_cpu+2*BLOCK_LEVEL;
    double (* new_c)[nn];
    double (* new_d)[nn];
    // double (* tmp)[nn];
    new_c = (double (*)[nn])malloc(nn * nn * sizeof(double));
    new_d = (double (*)[nn])malloc(nn * nn * sizeof(double));
    // tmp = (double (*)[nn])malloc(nn * nn * sizeof(double));

    x_s =  x - BLOCK_LEVEL; 
    y_s =  y - BLOCK_LEVEL;
    x_e =  x + n - 1 + BLOCK_LEVEL;
    y_e =  y + n - 1 + BLOCK_LEVEL;

    xx_s= (x_s < 0) ? 0 : x_s; 
    yy_s= (y_s < 0) ? 0 : y_s;
    xx_e = (x_e > N + 1) ? N + 1 : x_e;
    yy_e = (y_e > N + 1) ? N + 1 : y_e;
    // len_x = xx_e-xx_s+1;
    // len_y = yy_e-yy_s+1;

    if(it+BLOCK_LEVEL < It){
        blockEnd = it + BLOCK_LEVEL;} 
        else {
            blockEnd = It;}
    
    int n_xx_s, n_yy_s,  n_xx_e, n_yy_e;
    n_xx_s= (x_s < 0) ? BLOCK_LEVEL-1 : 0; 
    n_yy_s= (y_s < 0) ? BLOCK_LEVEL-1 : 0;
    n_xx_e = (x_e > N + 1) ? th_cpu+BLOCK_LEVEL : th_cpu-1+2*BLOCK_LEVEL;
    n_yy_e = (y_e > N + 1) ? th_cpu+BLOCK_LEVEL : th_cpu-1+2*BLOCK_LEVEL;
    // n_len_x = n_xx_e-n_xx_s+1;
    // n_len_y = n_yy_e-n_yy_s+1;
    // fprintf(stderr,"x=%d, y=%d:\n xx_s=%d, yy_s=%d,  xx_e=%d, yy_e=%d, len_x=%d, len_y=%d;\n n_xx_s=%d, n_yy_s=%d,  n_xx_e=%d, n_yy_e=%d, n_len_x=%d, n_len_y=%d\n", 
    //         x, y, xx_s, yy_s,  xx_e, yy_e, len_x, len_y, n_xx_s, n_yy_s,  n_xx_e, n_yy_e, n_len_x, n_len_y);
    
    for(int j=xx_s; j < xx_e+1; j++) {
        for(int i=yy_s ; i < yy_e+1; i++) {
            new_c[j-xx_s+n_xx_s][i-yy_s+n_yy_s] = c[j][i]; 
            // printf("new_c[%d][%d]=%f, c[%d][%d]=%f \n", j-xx_s+n_xx_s, i-yy_s+n_yy_s, new_c[j-xx_s+n_xx_s][i-yy_s+n_yy_s], j, i, c[j][i] );  
        }}

    for(int iter = it; iter < blockEnd-1; iter++) {  
        // fprintf(stderr,"x=%d, y=%d:\n xx_s=%d, yy_s=%d,  xx_e=%d, yy_e=%d, len_x=%d, len_y=%d;\n n_xx_s=%d, n_yy_s=%d,  n_xx_e=%d, n_yy_e=%d, n_len_x=%d, n_len_y=%d\n", 
            // x, y, xx_s, yy_s,  xx_e, yy_e, len_x, len_y, n_xx_s, n_yy_s,  n_xx_e, n_yy_e, n_len_x, n_len_y);
        int j_x_s = (x_s < 1) ? BLOCK_LEVEL : n_xx_s+1;
        int j_x_e = (x_e > N ) ? th_cpu+BLOCK_LEVEL : n_xx_e;
        int i_y_s = (y_s < 1) ? BLOCK_LEVEL : n_yy_s+1; 
        int i_y_e = (y_e > N ) ? th_cpu+BLOCK_LEVEL : n_yy_e;
        // fprintf(stderr,"j_x_s=%d, j_x_e=%d, i_y_s=%d, i_y_e=%d\n", j_x_s, j_x_e, i_y_s, i_y_e);
        for(int j=j_x_s; j < j_x_e; j++) {
            for(int i=i_y_s; i < i_y_e; i++) {
                new_d[j][i] = delT * (new_c[j+1][i] + new_c[j-1][i] + new_c[j][i+1] + new_c[j][i-1] + new_c[j][i]);
                // if(new_c[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL]>MAX_TEMP)
                // {
                //     new_d[j][i] =  new_c[j+1][i] - 0.1 * (new_c[j+1][i] -  delT * (new_c[j+1][i] + new_c[j-1][i] + new_c[j][i+1] + new_c[j][i-1] + new_c[j][i]));
                // }
                // else
                // {
                //     new_d[j][i] = delT * (new_c[j+1][i] + new_c[j-1][i] + new_c[j][i+1] + new_c[j][i-1] + new_c[j][i]);
                // }
                // new_d[j][i] = delT * (new_c[j+1][i] + new_c[j-1][i] + new_c[j][i+1] + new_c[j][i-1] 
                //             + new_c[j-1][i-1] + new_c[j-1][i+1] + new_c[j+1][i-1] + new_c[j+1][i+1] + new_c[j][i]);
            }}

        // tmp=new_c;
        // new_c=new_d;
        // new_d=tmp;
        for(int j=j_x_s; j < j_x_e; j++) {
            for(int i=i_y_s; i < i_y_e; i++) {
                new_c[j][i] = new_d[j][i];
            }}

        n_xx_s++;
        n_yy_s++;
        n_xx_e--;
        n_yy_e--;
    }

    for(int j=x; j < (x+n); j++){
        for(int i=y; i < (y+n); i++){
            d[j][i] = delT * (new_c[j-x+BLOCK_LEVEL+1][i-y+BLOCK_LEVEL] + new_c[j-x+BLOCK_LEVEL-1][i-y+BLOCK_LEVEL] + new_c[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL+1] + new_c[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL-1] + new_c[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL]);
            // d[j][i] = delT * (new_c[j-x+BLOCK_LEVEL+1][i-y+BLOCK_LEVEL] + new_c[j-x+BLOCK_LEVEL-1][i-y+BLOCK_LEVEL] + new_c[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL+1] + new_c[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL-1] 
            //         + new_c[j-x+BLOCK_LEVEL-1][i-y+BLOCK_LEVEL-1] + new_c[j-x+BLOCK_LEVEL-1][i-y+BLOCK_LEVEL+1] + new_c[j-x+BLOCK_LEVEL+1][i-y+BLOCK_LEVEL-1] + new_c[j-x+BLOCK_LEVEL+1][i-y+BLOCK_LEVEL+1] + new_c[j-x+BLOCK_LEVEL][i-y+BLOCK_LEVEL]);
        }}
    free(new_c);
    free(new_d);
}


// void printarray(int p, int q, double arr[p][q]){
//     int i, j;
//     for (i=0; i<p; i++){
//         for (j=0; j<q; j++){
//             // printf("%f ", arr[i][j]);
//             printf("a[%d][%d]=%f ", i, j, arr[i][j]);
//         }
//         printf("\n");
//     }
// }

// double get_time(){
//   struct timeval time;
//   if (gettimeofday(&time,NULL)){
//     return 0;
//   }
//   return (double)time.tv_sec + (double)time.tv_usec * .000001;
// }



// // with temporal blocking
// void scpu_tb(int x, int y, int it)
// {
//     int blockEnd;
//     int x_s, x_e, y_s, y_e, len_x, len_y;
//     int xx_s, yy_s, xx_e, yy_e;
//     // double new_c[N+2][N+2];
//     // double new_d[N+2][N+2];
//     int n = th_gpu;

//     // compute the range of data using tb
//     // printf("..no error...\n");
//     x_s =  x - BLOCK_LEVEL; 
//     y_s =  y - BLOCK_LEVEL;
//     x_e =  x + n - 1 + BLOCK_LEVEL;
//     y_e =  y + n - 1 + BLOCK_LEVEL;
//     // printf("..no error1...\n");

//     xx_s= (x - BLOCK_LEVEL < 0) ? 0 : x - BLOCK_LEVEL; 
//     yy_s= (y - BLOCK_LEVEL < 0) ? 0 : y - BLOCK_LEVEL;
//     xx_e = (x + n - 1 + BLOCK_LEVEL > N + 1) ? N + 1 : x + n - 1 + BLOCK_LEVEL;
//     yy_e = (y + n - 1 + BLOCK_LEVEL > N + 1) ? N + 1 : y + n - 1 + BLOCK_LEVEL;
//     len_x = xx_e-xx_s+1;
//     len_y = yy_e-yy_s+1;

//     if(it+BLOCK_LEVEL < It){
//         blockEnd = it + BLOCK_LEVEL;} 
//         else {
//             blockEnd = It;}
    
//     for(int j=xx_s; j < xx_e+1; j++) {
//         for(int i=yy_s ; i < yy_e+1; i++) {
//             new_c[j][i] = c[j][i]; 
//         }}
   
//     for(int iter = it; iter < blockEnd-1; iter++) {
//         int j_x_s = (x_s < 1) ? 1 : x_s+1;
//         int j_x_e = (x_e > N ) ? N+1 : x_e;
//         int i_y_s = (y_s < 1) ? 1 : y_s+1; 
//         int i_y_e = (y_e > N ) ? N+1 : y_e;
//         // printf("j_x_s=%d, j_x_e=%d, i_y_s=%d, i_y_e=%d\n", j_x_s, j_x_e, i_y_s, i_y_e);

//         for(int j=j_x_s; j < j_x_e; j++) {
//             for(int i=i_y_s ; i < i_y_e; i++) {
//                 new_d[j][i] = delT * (new_c[j+1][i] + new_c[j-1][i] + new_c[j][i+1] + new_c[j][i-1] 
//                             + new_c[j-1][i-1] + new_c[j-1][i+1] + new_c[j+1][i-1] + new_c[j+1][i+1] + new_c[j][i]);
//             }}
//         for(int j=j_x_s; j < j_x_e; j++) {
//             for(int i=i_y_s ; i < i_y_e; i++) {
//                 new_c[j][i] = new_d[j][i];               
//             }}
//         x_s++;
//         y_s++;
//         x_e--;
//         y_e--;
//     }

//     for(int j=x; j < (x+n); j++){
//         for(int i=y; i < (y+n); i++){
//             d[j][i] = delT * (new_c[j+1][i] + new_c[j-1][i] + new_c[j][i+1] + new_c[j][i-1] 
//                     + new_c[j-1][i-1] + new_c[j-1][i+1] + new_c[j+1][i-1] + new_c[j+1][i+1] + new_c[j][i]);
//         }}

//     // printf("-----------after iter, d-----------\n");    
//     // int i, j;
//     // for (i=0; i<N+2; i++){
//     //     for (j=0; j<N+2; j++){
//     //         // printf("%f ", arr[i][j]);
//     //         printf("%f ", d[i][j]);
//     //     }
//     //     printf("\n");
//     // }
// }


// // with temporal blocking
// void sgpu_tb(int x, int y, int n, int id, int it)
// {
//     int blockEnd;
//     int x_s, x_e, y_s, y_e, len_x, len_y;
//     int xx_s, yy_s, xx_e, yy_e;
//     // printf("---------- 0 ------------\n");

//     xx_s= (x - BLOCK_LEVEL < 0) ? 0 : x - BLOCK_LEVEL; 
//     yy_s= (y - BLOCK_LEVEL < 0) ? 0 : y - BLOCK_LEVEL;
//     xx_e = (x + n - 1 + BLOCK_LEVEL > N + 1) ? N + 1 : x + n - 1 + BLOCK_LEVEL;
//     yy_e = (y + n - 1 + BLOCK_LEVEL > N + 1) ? N + 1 : y + n - 1 + BLOCK_LEVEL;
//     len_x = xx_e-xx_s+1;
//     len_y = yy_e-yy_s+1;
//     // printf("---------- 1 ------------\n");

//     if(it+BLOCK_LEVEL < It){
//         blockEnd = it + BLOCK_LEVEL;} 
//         else {
//             blockEnd = It;} 
            
//     x_s =  x - BLOCK_LEVEL; 
//     y_s =  y - BLOCK_LEVEL;
//     x_e =  x + n - 1 + BLOCK_LEVEL;
//     y_e =  y + n - 1 + BLOCK_LEVEL;


//     int nb_xx_s, nb_yy_s,  nb_xx_e, nb_yy_e, nb_len_x, nb_len_y;
//     nb_xx_s= (x - BLOCK_LEVEL < 0) ? BLOCK_LEVEL-1 : 0; 
//     nb_yy_s= (y - BLOCK_LEVEL < 0) ? BLOCK_LEVEL-1 : 0;
//     nb_xx_e = (x + n - 1 + BLOCK_LEVEL > N + 1) ? th_gpu+BLOCK_LEVEL : th_gpu-1+2*BLOCK_LEVEL;
//     nb_yy_e = (y + n - 1 + BLOCK_LEVEL > N + 1) ? th_gpu+BLOCK_LEVEL : th_gpu-1+2*BLOCK_LEVEL;
//     nb_len_x = nb_xx_e-nb_xx_s+1;
//     nb_len_y = nb_yy_e-nb_yy_s+1;


//     for(int j=0; j < th_gpu+2*BLOCK_LEVEL; j++) {
//         for(int i=0 ; i < th_gpu+2*BLOCK_LEVEL; i++) {
//             new_b[j][i] = b[j][i]; 
//         }}

//     // printf("---------- 2 ------------\n");
    
//     // Allocate memory and copy data to the GPU
//     // #pragma acc data copyin(x, y, n, xx_s, yy_s, xx_e, yy_e,len_x, len_y, a[xx_s:len_x][yy_s:len_y]) create(new_b[xx_s:len_x][yy_s:len_y]) copyout(b[x:n][y:n])
    
//      #pragma acc data copyin(x, y, n, xx_s, yy_s, xx_e, yy_e,len_x, len_y, a[xx_s:len_x][yy_s:len_y]) create(new_b[nb_xx_s:nb_len_x][nb_yy_s:nb_len_y]) copyout(b[x:n][y:n]){
//         for (int iter = it; iter < blockEnd-1; iter++) {
//             int j_x_s = (x_s < 1) ? 1 : x_s+1;
//             int j_x_e = (x_e > N ) ? N+1 : x_e;
//             int i_y_s = (y_s < 1) ? 1 : y_s+1; 
//             int i_y_e = (y_e > N ) ? N+1 : y_e;
//             // printf("j_x_s=%d, j_x_e=%d, i_y_s=%d, i_y_e=%d\n", j_x_s, j_x_e, i_y_s, i_y_e);
//             // printf("in_OpenACC: xx_s=%d, yy_s=%d, x_s=%d, x_e=%d, y_s=%d, y_e=%d, len_x=%d, len_y=%d\n",  xx_s, yy_s, x_s, x_e, y_s, y_e, len_x, len_y);
//             // printf("----------------------\n");
//             // #pragma acc loop independent
//             #pragma acc kernels present(xx_s, yy_s, xx_e, yy_e, len_x, len_y, a[xx_s:len_x][yy_s:len_y], new_b[xx_s:len_x][yy_s:len_y]) 
//             // #pragma acc kernels loop, gang(32), worker(16)
//             for(int j=j_x_s; j < j_x_e; j++) {
//                 // #pragma acc loop gang(16), worker(32)
//                 for(int i=i_y_s ; i < i_y_e; i++) {
//                     new_b[j][i] = delT * (a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j-1][i-1] + a[j-1][i+1] + a[j+1][i-1] + a[j+1][i+1] + a[j][i]);
//                     // new_b[j][i] = delT * (a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] + a[j-1][i-1] + a[j-1][i+1] + a[j+1][i-1] + a[j+1][i+1] + a[j][i]);
                   
//                     // printf("after b: new_b[%d][%d]=%f(%p), a[%d][%d]=%f(%p), a[%d][%d]=%f(%p), a[%d][%d]=%f(%p), a[%d][%d]=%f(%p), a[%d][%d]=%f(%p), a[%d][%d]=%f(%p), a[%d][%d]=%f(%p).\n",  
//                     //         j, i, new_b[j][i], &new_b[j][i], 
//                     //         j, i, a[j][i], &a[j][i], 
//                     //         j+1, i, a[j+1][i], &a[j+1][i], 
//                     //         j-1, i, a[j-1][i], &a[j-1][i], 
//                     //         j, i+1, a[j][i+1], &a[j][i+1], 
//                     //         j, i-1, a[j][i-1], &a[j][i-1], 
//                     //         j-1, i-1, a[j-1][i-1], &a[j-1][i-1], 
//                     //         j-1, i+1, a[j-1][i+1], &a[j-1][i+1], 
//                     //         j+1, i-1, a[j+1][i-1], &a[j+1][i-1], 
//                     //         j+1, i+1, a[j+1][i+1], &a[j+1][i+1]
//                     //         );
//                     // printf("Difference: a-b = %d, b[%d][%d]=%f(%p), b[%d][%d]=%f(%p),a[%d][%d]=%f(%p), a[%d][%d]=%f(%p)\n", &a[xx_s][yy_s]-&new_b[x][y], x,y, new_b[x][y],&new_b[x][y], x+n-1,y+n-1, new_b[x+n-1][y+n-1],&new_b[x+n-1][y+n-1],xx_s,yy_s, a[xx_s][yy_s],&a[xx_s][yy_s], xx_e, yy_e, a[xx_e][yy_e],&a[xx_e][yy_e]); 
//                     }}
      
//             #pragma acc kernels present(xx_s, yy_s, len_x, len_y, a[xx_s:len_x][yy_s:len_y], new_b[xx_s:len_x][yy_s:len_y]) 
//             // #pragma acc kernels loop
//             for(int j=j_x_s; j < j_x_e; j++) {
//                 // #pragma acc loop gang(16), worker(32)
//                 for(int i=i_y_s ; i < i_y_e; i++) {
//                     a[j][i] = new_b[j][i];               
//                     // printf("new_b[%d][%d]=%f, a[%d][%d]=%f\n", j, i, new_b[j][i], j, i, a[j][i]);
//                 }}
//             x_s++;
//             y_s++;
//             x_e--;
//             y_e--;
//         }

//         // final iter
//         #pragma acc kernels present(xx_s, yy_s, len_x, len_y, a[xx_s:len_x][yy_s:len_y], b[x:n][y:n]) 
//         for(int j=x; j < (x+n); j++){
//             for(int i=y; i < (y+n); i++){
//                 b[j][i] = delT*(a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] 
//                 + a[j-1][i-1] + a[j-1][i+1] + a[j+1][i-1] + a[j+1][i+1] + a[j][i]);
//             }}

//         // // Use the updated data on the host 
//         // #pragma acc kernels present(x, y, n, xx_s, yy_s, len_x, len_y, new_b[xx_s:len_x][yy_s:len_y], b[x:n][y:n]) 
//         // for(int j=x; j < x+n; j++) {
//         //     for(int i=y ; i < y+n; i++) {
//         //         b[j][i] = new_b[j][i];               
//         //         // printf("b[%d][%d]=%f, a[%d][%d]=%f\n", j, i, b[j][i], j, i, a[j][i]);
//         //     }}        
//     }
// }