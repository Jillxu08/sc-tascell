#define WIDTH 8000
#define HEIGHT 8000
// 65536, 32768,8 16384, 8192, 4096, 2048
// #define th_gpu1 0s
// #define th_gpu2 -1
// #define th_cpu 2048
// New zoom parameters (for example, zoom into the region around -0.75 + 0.186i)
// #define ZOOM_REAL_MIN -2.5
// #define ZOOM_REAL_MAX 1.5
// #define ZOOM_IMAG_MIN -2
// #define ZOOM_IMAG_MAX 2
#define ZOOM_REAL_MIN -0.15
#define ZOOM_REAL_MAX 0.1
#define ZOOM_IMAG_MIN -0.7
#define ZOOM_IMAG_MAX -0.45

// #define ZOOM_REAL_MIN -0.367
// #define ZOOM_REAL_MAX -0.362
// #define ZOOM_IMAG_MIN 0.656
// #define ZOOM_IMAG_MAX 0.660

extern int th_gpu1, th_gpu2, th_cpu, MAX_ITERATIONS, TH;
extern int rslt_par[HEIGHT * WIDTH];
// extern double z_real_seq[HEIGHT][WIDTH], z_imag_seq[HEIGHT][WIDTH], z_real_par[HEIGHT][WIDTH], z_imag_par[HEIGHT][WIDTH];
// extern int th_cpu, th_worker;

// void initC(int ngpu);
// double get_time();
// void init_gpu();
int in_mandelbrot(double real, double imag);
void hsv_to_rgb(double h, double s, double v, unsigned char *r, unsigned char *g, unsigned char *b);
// in_mandelbrot(double real, double imag, double z_real_sq, double z_imag_sq);
void mb_acc(int a, int b);
void mb_acc_cpu(int a, int b);
void gpu_initialize();
// void gpu_trans();
