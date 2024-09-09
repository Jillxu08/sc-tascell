#include "mandelbrot_acc.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <omp.h>
// #include <immintrin.h>
// #include <unistd.h>
#ifdef _OPENACC
#include <openacc.h>
#endif

#define EPSILON 1e-6
int TH, MAX_ITERATIONS, th_gpu1, th_gpu2, th_cpu;
int rslt_par[HEIGHT * WIDTH];
// double z_real_seq[HEIGHT][WIDTH], z_imag_seq[HEIGHT][WIDTH];
// double z_real_par[HEIGHT][WIDTH], z_imag_par[HEIGHT][WIDTH];

// GPU 初始化函数
void gpu_initialize() {
    #pragma acc parallel loop collapse(2)
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            double real = (x - WIDTH / 2.0) * 4.0 / WIDTH - 0.5;
            double imag = (y - HEIGHT / 2.0) * 4.0 / HEIGHT;
            double z_real_sq, z_imag_sq;
            int value = in_mandelbrot(real, imag);
            // value = in_mandelbrot(real, imag, z_real_par[y][x], z_imag_par[y][x]);
            // rslt_par[y][x] = value;
        }
    }
}

int in_mandelbrot(double real, double imag) {
    double z_real = 0, z_imag = 0;
    double z_real_sq = 0;
    double z_imag_sq = 0;
    int iterations = 0;

    while (z_real_sq + z_imag_sq <= TH && iterations < MAX_ITERATIONS) {
        z_imag = 2 * z_real * z_imag + imag;
        z_real = z_real_sq - z_imag_sq + real;
        z_real_sq = z_real * z_real;
        z_imag_sq = z_imag * z_imag;
        iterations++;
    }
    return iterations;
}

void mb_acc(int a, int b) {
    // double real, imag;
    // printf('i1=%d, i2=%d \n', a, b);
    #pragma acc parallel loop copyout(rslt_par[a:b-a])
    for (int i = a; i < b; i++) {
        int y = i / WIDTH;
        int x = i % WIDTH; 
        // printf('y=%d, x=%d \n', y, x);
        //define the region of mandelbrot set
        // double real = (x - WIDTH / 2.0) * 4.0 / WIDTH - 0.5;
        // double imag = (y - HEIGHT / 2.0) * 4.0 / HEIGHT;

        // Define the region of the Mandelbrot set with zoom applied
        double real = ZOOM_REAL_MIN + (ZOOM_REAL_MAX - ZOOM_REAL_MIN) * x / WIDTH;
        double imag = ZOOM_IMAG_MIN + (ZOOM_IMAG_MAX - ZOOM_IMAG_MIN) * y / HEIGHT;

        // int value = in_mandelbrot(real, imag);
        double z_real = 0, z_imag = 0;
        double z_real_sq = 0;
        double z_imag_sq = 0;
        int iterations = 0;
        while (z_real_sq + z_imag_sq <= TH && iterations < MAX_ITERATIONS) {
            z_imag = 2 * z_real * z_imag + imag;
            z_real = z_real_sq - z_imag_sq + real;
            z_real_sq = z_real * z_real;
            z_imag_sq = z_imag * z_imag;
            iterations++;}
        rslt_par[i] = iterations;
        // }
    }
}

void mb_acc_cpu(int a, int b) {
    // double real, imag;

    // for (int y = a; y < b; y++) {
    //     #pragma omp simd
    //     for (int x = 0; x < WIDTH; x++) {
    //         double real = (x - WIDTH / 2.0) * 4.0 / WIDTH - 0.5;
    //         double imag = (y - HEIGHT / 2.0) * 4.0 / HEIGHT;
    //         // double z_real_sq, z_imag_sq;
    //         int value = in_mandelbrot(real, imag);
    //         rslt_par[y][x] = value;
    //     }
    // }
    #pragma omp simd
    for (int i = a; i < b; i++) {
        int y = i / WIDTH;
        int x = i % WIDTH; 
        //define the region of mandelbrot set
        // double real = (x - WIDTH / 2.0) * 4.0 / WIDTH - 0.5;
        // double imag = (y - HEIGHT / 2.0) * 4.0 / HEIGHT;
        // Define the region of the Mandelbrot set with zoom applied
        double real = ZOOM_REAL_MIN + (ZOOM_REAL_MAX - ZOOM_REAL_MIN) * x / WIDTH;
        double imag = ZOOM_IMAG_MIN + (ZOOM_IMAG_MAX - ZOOM_IMAG_MIN) * y / HEIGHT;

        // int value = in_mandelbrot(real, imag);
        double z_real = 0, z_imag = 0;
        double z_real_sq = 0;
        double z_imag_sq = 0;
        int iterations = 0;
        while (z_real_sq + z_imag_sq <= TH && iterations < MAX_ITERATIONS) {
            z_imag = 2 * z_real * z_imag + imag;
            z_real = z_real_sq - z_imag_sq + real;
            z_real_sq = z_real * z_real;
            z_imag_sq = z_imag * z_imag;
            iterations++;}
        rslt_par[i] = iterations;
        // }
    }
}

// int in_mandelbrot(double real, double imag, double z_real_sq, double z_imag_sq ) {
//     double z_real = 0, z_imag = 0;
//     // double z_real_sq = 0, z_imag_sq = 0;
//     // z_real_sq = 0, z_imag_sq = 0;
//     int iterations = 0;

//     while (z_real_sq + z_imag_sq <= 4 + EPSILON && iterations < MAX_ITERATIONS) {
//         z_imag = 2 * z_real * z_imag + imag;
//         z_real = z_real_sq - z_imag_sq + real;
//         z_real_sq = z_real * z_real;
//         z_imag_sq = z_imag * z_imag;
//         iterations++;
//     }
//     // printf("z_real_sq=%d, z_imag_sq=%d \n", z_real_sq, z_imag_sq);
//     return iterations;
// }        