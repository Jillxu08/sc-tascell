#define N 8
// 65536, 32768, 16384, 8192, 4096, 2048
#define th_gpu 8
#define th_cpu 2
#define delT (1/27.0)
#define It 10
#define BLOCK_SIZE 2

extern double (*a)[N+2][N+2];
extern double (*b)[N+2][N+2];
extern int it;

void sgpu(int x, int y, int z, int n, int id);
// void sgpu_N(int x, int y, int n, int id);
void scpu(int x, int y, int n);


// void printpoint(int p, int q, double *M);
// ------ Max matrix size ----------
// ITO CPU memory 384gb;
// copy for gpu is 16gb;
// # of matrix[N*N] is 38;

// (384-16)gb = 368 * 10^9 bytes
// 1double = 8 bytes
// N^2= [368 * 10^9 / 4 /8]= 1.15 * 10^10  (# of matrix is 4)
// N = 107238
// 2^15	= 65536

// # of matrix is 3, the N_max=123827, 2^15.
// # of matrix is 5, the N_max=95916, 2^15.

// --------- Max th_gpu ------------
// ITO perGPU memory is 16gb.
// 3 matrix 
// 16gb = 16 * 10^9 bytes
// 1double = 8 bytes

// 16gb = 2 * 10^9 double 
// th * N * 2 + th * th <= 2 * 10^9
// th_max_min=13804, 2^13=8192

// N=65536, th_max=13804, 2^13=8192.
// N=32768, th_max=22673, 2^14=16384.
// N=16384, th_max=31244.
