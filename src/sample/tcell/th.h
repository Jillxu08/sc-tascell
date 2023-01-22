#define N 1024

extern double c[36][N][N];
extern double a[N][N];
extern double b[N][N];

void mm_sub(int a_r, int a_c, int b_r, int b_c, int c_r, int c_c, int n, int id);