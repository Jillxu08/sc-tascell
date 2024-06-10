// flag for different subparts
// temporal blcoking -> gpu performance 
// points: 27=3*3*3, 3d; 25

#include "stencil3d_acc.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#ifdef _OPENACC
#include <openacc.h>
#endif

// double delT = 0.1 / (N * N);
double (*a)[N+2][N+2];
double (*b)[N+2][N+2];

void sgpu(int x, int y, int z, int n, int id)
{
    // #ifdef _OPENACC
    //     printf("Nimber of device: %d\n", \
    //         acc_get_num_devices(acc_device_not_host));
    // #else
    //     printf("OpenACC is not supported.\n");
    // #endif

    // int device_type = acc_get_device_type();
    // printf("a_in1[0][1]=%f, address is %ld\n",a[0][1], &a[0][1]);

    #pragma acc data copyin(a[x-1:n+2][y-1:n+2][z-1:n+2]) copyout(b[x:n][y:n][z:n])
    {
        #pragma acc parallel
        for(int i=x; i < (x+n); i++){
            for(int j=y; j < (y+n); j++)
                for(int k=z; k < (z+n); k++){
                b[j][i][k] = delT*(a[i-1][j-1][k-1] + a[i][j-1][k-1] + a[i+1][j-1][k-1] +
                                    a[i-1][j][k-1] + a[i][j][k-1] + a[i+1][j][k-1] +
                                    a[i-1][j+1][k-1] + a[i][j+1][k-1] + a[i+1][j+1][k-1] +
                                    a[i-1][j-1][k] + a[i][j-1][k] + a[i+1][j-1][k] +
                                    a[i-1][j][k] + a[i][j][k] + a[i+1][j][k] +
                                    a[i-1][j+1][k] + a[i][j+1][k] + a[i+1][j+1][k] +
                                    a[i-1][j-1][k+1] + a[i][j-1][k+1] + a[i+1][j-1][k+1] +
                                    a[i-1][j][k+1] + a[i][j][k+1] + a[i+1][j][k+1] +
                                    a[i-1][j+1][k+1] + a[i][j+1][k+1] + a[i+1][j+1][k+1]);

                // if (device_type == acc_device_nvidia) {
                //     printf("Running on NVIDIA GPU\n");
                //     } else if (device_type == acc_device_host) {
                //         printf("Running on CPU\n");
                //     } else {
                //         printf("Unknown device type\n");
                //     }
    }}}
}

















// void scpu(int x, int y, int n)
// {
//     int i,j;
//     for(j=(1+x);j<=(x+n) && j!=N;j++){
//         for(i=(1+y);i<=(y+n) && i!=N;i++){
//             b[j][i] = a[j][i] + delT * N * N * (a[j+1][i] + a[j-1][i] + a[j][i+1] + a[j][i-1] - 4*a[j][i]);
//         }
//     }
// }


