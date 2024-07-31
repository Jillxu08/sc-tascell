// #include <stdio.h>
// #include <stdlib.h>
// #include <math.h>

// #define WIDTH 800
// #define HEIGHT 600
// #define MAX_ITERATIONS 1000

// int is_in_mandelbrot(double real, double imag) {
//     double z_real = 0, z_imag = 0;
//     double z_real_sq = 0, z_imag_sq = 0;
//     int iterations = 0;

//     while (z_real_sq + z_imag_sq <= 4 && iterations < MAX_ITERATIONS) {
//         z_imag = 2 * z_real * z_imag + imag;
//         z_real = z_real_sq - z_imag_sq + real;
//         z_real_sq = z_real * z_real;
//         z_imag_sq = z_imag * z_imag;
//         iterations++;
//     }

//     return iterations;
// }

// int main() {
//     FILE *fp = fopen("mandelbrot.ppm", "wb");
//     fprintf(fp, "P6\n%d %d\n255\n", WIDTH, HEIGHT);

//     for (int y = 0; y < HEIGHT; y++) {
//         for (int x = 0; x < WIDTH; x++) {
//             double real = (x - WIDTH / 2.0) * 4.0 / WIDTH;
//             double imag = (y - HEIGHT / 2.0) * 4.0 / HEIGHT;
//             int value = is_in_mandelbrot(real, imag);

//             unsigned char r, g, b;
//             if (value == MAX_ITERATIONS) {
//                 r = g = b = 0;  // 黒（集合に属する点）
//             } else {
//                 // 集合に属さない点の色付け
//                 value = (int)(value * 255.0 / MAX_ITERATIONS);
//                 r = value;
//                 g = value;
//                 b = 255;
//             }

//             fputc(r, fp);
//             fputc(g, fp);
//             fputc(b, fp);
//         }
//     }

//     fclose(fp);
//     return 0;
// }

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define WIDTH 800
#define HEIGHT 600
#define MAX_ITERATIONS 1000

int in_mandelbrot(double real, double imag, double *z_real_sq, double *z_imag_sq) {
    double z_real = 0, z_imag = 0;
    *z_real_sq = 0, *z_imag_sq = 0;
    int iterations = 0;

    while (*z_real_sq + *z_imag_sq <= 4 && iterations < MAX_ITERATIONS) {
        z_imag = 2 * z_real * z_imag + imag;
        z_real = *z_real_sq - *z_imag_sq + real;
        *z_real_sq = z_real * z_real;
        *z_imag_sq = z_imag * z_imag;
        iterations++;
    }

    return iterations;
}

void hsv_to_rgb(double h, double s, double v, unsigned char *r, unsigned char *g, unsigned char *b) {
    int i;
    double f, p, q, t;

    if (s == 0) {
        *r = *g = *b = (unsigned char)(v * 255);
        return;
    }

    h /= 60;
    i = (int)h;
    f = h - i;
    p = v * (1 - s);
    q = v * (1 - s * f);
    t = v * (1 - s * (1 - f));

    switch (i) {
        case 0: *r = (unsigned char)(v * 255); *g = (unsigned char)(t * 255); *b = (unsigned char)(p * 255); break;
        case 1: *r = (unsigned char)(q * 255); *g = (unsigned char)(v * 255); *b = (unsigned char)(p * 255); break;
        case 2: *r = (unsigned char)(p * 255); *g = (unsigned char)(v * 255); *b = (unsigned char)(t * 255); break;
        case 3: *r = (unsigned char)(p * 255); *g = (unsigned char)(q * 255); *b = (unsigned char)(v * 255); break;
        case 4: *r = (unsigned char)(t * 255); *g = (unsigned char)(p * 255); *b = (unsigned char)(v * 255); break;
        default: *r = (unsigned char)(v * 255); *g = (unsigned char)(p * 255); *b = (unsigned char)(q * 255); break;
    }
}

int main() {
    FILE *fp = fopen("mandelbrot.ppm", "wb");
    fprintf(fp, "P6\n%d %d\n255\n", WIDTH, HEIGHT);

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            double real = (x - WIDTH / 2.0) * 4.0 / WIDTH;
            double imag = (y - HEIGHT / 2.0) * 4.0 / HEIGHT;
            double z_real_sq, z_imag_sq;
            int value = is_in_mandelbrot(real, imag, &z_real_sq, &z_imag_sq);

            unsigned char r, g, b;
            if (value == MAX_ITERATIONS) {
                r = g = b = 0;  // 黑色（集合中点）
            } else {
                // 使用平滑着色方法
                double smooth = value + 1 - log(log(sqrt(z_real_sq + z_imag_sq))) / log(2.0);
                double hue = 360.0 * smooth / MAX_ITERATIONS;
                hsv_to_rgb(hue, 1.0, 1.0, &r, &g, &b);
            }

            fputc(r, fp);
            fputc(g, fp);
            fputc(b, fp);
        }
    }

    fclose(fp);
    return 0;
}
