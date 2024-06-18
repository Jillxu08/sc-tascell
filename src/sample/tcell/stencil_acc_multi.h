#define N 65536L
// // 131072L, 65536L, 32768, 16384, 8192, 4096, 2048
// #define th_gpu1 4096 // th_gpu > BLOCK_LEVEL
// #define th_gpu2 1024
// #define th_cpu 128
#define delT (1/5.0)
#define MAX_TEMP 25.0 // Temperature threshold
// #define It 500
// #define BLOCK_LEVEL 30

extern int th_gpu1, th_gpu2;
extern int th_cpu;
extern int It, it;
extern int BLOCK_LEVEL;

extern double (* restrict a)[N+2];
extern double (* restrict b)[N+2];
extern double (* restrict c)[N+2];
extern double (* restrict d)[N+2];

double get_time();
void printarray(int p, int q, double arr[p][q]);
void init_gpu(int n);
void gpu(int x, int y, int n, int id, int it);
void sgpu_tb(int x, int y, int n, int id, int it);
void scpu_tb(int x, int y, int n, int id, int it);
// void scpu(int x, int y, int n, int id, int it);
void cpu_tb(int x, int y, int n, int it);
void cpu(int x, int y, int n, int id, int it);
void seq_cpu(int j, int i, int it);

// issues:
// 1.只有gpu函数，用2个core时， 
// ------------------- time -----------------
// tascell:worker_id=0, it=0, n=16, x=1, y=1 
// tascell:worker_id=0, it=0, n=16, x=1, y=17 
// srun: error: ng0008: task 0: Segmentation fault (core dumped)



// ------ size ----------
// system G
// 1 node: 2cpus, 4gpus, 64cores, 512g
// 1 cpu: 32cores, 
// 1 gpu: 80gb;
// 16gb = 16 * 10^9 bytes
// 1double = 8 bytes
// N_max = 2^16 = 65,536; th_max = 2^15 = 32,768;






//  a[th+2B]*[th+2B], new_b[th+2B]*[th+2B], b[th][th]
// if B = 1, th_max = 9440; if B = 300, th_max = 8840; 
// if th=8192, B_max = 620
// 80GB = (th+2B)^2 * 8字节 + (th+2B)^2 * 8字节 + th^2 * 8字节
//      = 24th^2 + 64Bth + 64B^2
// 3th^2 + 8Bth + 8B^2 <= 10 * 1024 * 1024 * 1024=10^10 bytes=10,737,418,240

// if B=1, th < 59824, th_max = 2^15 = 32768;
// if B=100, th < 59692, th_max = 2^15 = 32768;
// if B=200, th < 59558, th_max = 2^15 = 32768;
// if B=300, th < 59425, th_max = 2^15 = 32768;
// if B=600, th < 59023, th_max = 2^15 = 32768;
// if B=1000, th < 58485, th_max = 2^15 = 32768;


// 512GB-80GB = (N+2)^2 * 4 * 8
// 1.35 * 10^10 = N^2 + 2 * N + 4
// N = 116,189, N_max = 2^16 = 65,536

// --------- Max th_gpu ------------


