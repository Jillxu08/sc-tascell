#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define WIDTH 800
#define HEIGHT 600
#define MAX_ITERATIONS 1000

int is_in_mandelbrot(double real, double imag) {
    double z_real = 0, z_imag = 0;
    double z_real_sq = 0, z_imag_sq = 0;
    int iterations = 0;

    while (z_real_sq + z_imag_sq <= 4 && iterations < MAX_ITERATIONS) {
        z_imag = 2 * z_real * z_imag + imag;
        z_real = z_real_sq - z_imag_sq + real;
        z_real_sq = z_real * z_real;
        z_imag_sq = z_imag * z_imag;
        iterations++;
    }

    return iterations;
}

int main() {
    FILE *fp = fopen("mandelbrot.ppm", "wb");
    fprintf(fp, "P6\n%d %d\n255\n", WIDTH, HEIGHT);

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            double real = (x - WIDTH / 2.0) * 4.0 / WIDTH;
            double imag = (y - HEIGHT / 2.0) * 4.0 / HEIGHT;
            int value = is_in_mandelbrot(real, imag);

            unsigned char r, g, b;
            if (value == MAX_ITERATIONS) {
                r = g = b = 0;  // 黒（集合に属する点）
            } else {
                // 集合に属さない点の色付け
                value = (int)(value * 255.0 / MAX_ITERATIONS);
                r = value;
                g = value;
                b = 255;
            }

            fputc(r, fp);
            fputc(g, fp);
            fputc(b, fp);
        }
    }

    fclose(fp);
    return 0;
}