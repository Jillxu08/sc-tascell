#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>

#define total   4       //15

void strassenMul(double* X, double* Y, double* Z, int m);
void matMul(double* A, double* B, double* C, int n);
void matAdd(double* A, double* B, double* C, int m);
void matSub(double* A, double* B, double* C, int m);

enum array { x11, x12, x21, x22, y11, y12, y21, y22,
    P1, P2, P3, P4, P5, P6, P7, C11, C12, C21, C22,
    S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11, S12, S13, S14, arrs };

int idx = 0;

int main()
{
    int N;
    int count = 0;
    int i, j;
    clock_t start, end;
    double elapsed;

    double tnaive[total];
    double tstrassen[total];
    double *X, *Y, *Z, *W, *A, *B, *C;

    printf("-------------------------------------------------------------------------\n\n");
    for (count = 0; count < total; count++)
    {
        N = (int)pow(2, count);
        printf("Matrix size = %2d\t",N);
        X = malloc(N*N*sizeof(double));
        Y = malloc(N*N*sizeof(double));
        Z = malloc(N*N*sizeof(double));
        W = malloc(N*N*sizeof(double));
        if (X==NULL || Y==NULL || Z==NULL || W==NULL) {
            printf("Out of memory (1)\n");
            return 1;
        }
        srand((unsigned)time(NULL));
        for (i=0; i<N*N; i++)
        {
            X[i] = rand()/(RAND_MAX + 1.);
            Y[i] = rand()/(RAND_MAX + 1.);
        }
        start = clock();
        matMul(X, Y, W, N);
        end = clock();
        elapsed = ((double) (end - start))*100/ CLOCKS_PER_SEC;
        tnaive[count] = elapsed;
        printf("naive = %5.4f\t\t",tnaive[count]);

        start = clock();
        strassenMul(X, Y, Z, N);
        free(W); 
        free(Z); 
        free(Y); 
        free(X); 
        end = clock();
        elapsed = ((double) (end - start))*100/ CLOCKS_PER_SEC;
        tstrassen[count] = elapsed;
        printf("strassen = %5.4f\n",tstrassen[count]);
    }
    printf("-------------------------------------------------------------------\n\n\n");

    while (tnaive[idx+1] <= tstrassen[idx+1] && idx < 14) idx++;

    printf("Optimum input size to switch from normal multiplication to Strassen's is above %d\n\n", idx);

    printf("Please enter the size of array as a power of 2\n");
    scanf("%d",&N);
    A = malloc(N*N*sizeof(double));
    B = malloc(N*N*sizeof(double));
    C = malloc(N*N*sizeof(double));
    if (A==NULL || B==NULL || C==NULL) {
        printf("Out of memory (2)\n");
        return 1;
    }
    srand((unsigned)time(NULL));
    for (i=0; i<N*N; i++)
    {
        A[i] = rand()/(RAND_MAX + 1.);
        B[i] = rand()/(RAND_MAX + 1.);
    }

    printf("------------------- Input Matrices A and B ---------------------------\n\n");
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
            printf("%5.4f  ",A[i*N+j]);
        printf("\n");
    }
    printf("\n");

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
            printf("%5.4f  ",B[i*N+j]);
        printf("\n");
    }
    printf("\n------- Output matrix by Strassen's method after optimization -----------\n\n");

    strassenMul(A, B, C, N);

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
            printf("%5.4f  ",C[i*N+j]);
        printf("\n");
    }
    free(C); 
    free(B); 
    free(A); 
    return(0);
}

void strassenMul(double *X, double *Y, double *Z, int m)
{
    int row = 0, col = 0;
    int n = m/2;
    int i = 0, j = 0;
    double *arr[arrs];                      // each matrix mem ptr

    if (m <= idx)
    {
        matMul(X, Y, Z, m);
        return;
    }
    if (m == 1)
    {
        *Z = *X * *Y;
        return;
    }

    for (i=0; i<arrs; i++) {                // memory for arrays
        arr[i] = malloc(n*n*sizeof(double));
        if (arr[i] == NULL) {
            printf("Out of memory (1)\n");
            exit (1);                       // brutal
        }
    }

    for (row = 0, i = 0; row < n; row++, i++)
    {
        for (col = 0, j = 0; col < n; col++, j++)
        {
            arr[x11][i*n+j] = X[row*m+col];
            arr[y11][i*n+j] = Y[row*m+col];
        }
        for (col = n, j = 0; col < m; col++, j++)
        {
            arr[x12][i*n+j] = X[row*m+col];
            arr[y12][i*n+j] = Y[row*m+col];
        }
    }

    for (row = n, i = 0; row < m; row++, i++)
    {
        for (col = 0, j = 0; col < n; col++, j++)
        {
            arr[x21][i*n+j] = X[row*m+col];
            arr[y21][i*n+j] = Y[row*m+col];
        }
        for (col = n, j = 0; col < m; col++, j++)
        {
            arr[x22][i*n+j] = X[row*m+col];
            arr[y22][i*n+j] = Y[row*m+col];
        }
    }

    // Calculating P1
    matAdd(arr[x11], arr[x22], arr[S1], n);
    matAdd(arr[y11], arr[y22], arr[S2], n);
    strassenMul(arr[S1], arr[S2], arr[P1], n);

    // Calculating P2
    matAdd(arr[x21], arr[x22], arr[S3], n);
    strassenMul(arr[S3], arr[y11], arr[P2], n);

    // Calculating P3
    matSub(arr[y12], arr[y22], arr[S4], n);
    strassenMul(arr[x11], arr[S4], arr[P3], n);

    // Calculating P4
    matSub(arr[y21], arr[y11], arr[S5], n);
    strassenMul(arr[x22], arr[S5], arr[P4], n);

    // Calculating P5
    matAdd(arr[x11], arr[x12], arr[S6], n);
    strassenMul(arr[S6], arr[y22], arr[P5], n);

    // Calculating P6
    matSub(arr[x21], arr[x11], arr[S7], n);
    matAdd(arr[y11], arr[y12], arr[S8], n);
    strassenMul(arr[S7], arr[S8], arr[P6], n);

    // Calculating P7
    matSub(arr[x12], arr[x22], arr[S9], n);
    matAdd(arr[y21], arr[y22], arr[S10], n);
    strassenMul(arr[S9], arr[S10], arr[P7], n);

    // Calculating C11
    matAdd(arr[P1], arr[P4], arr[S11], n);
    matSub(arr[S11], arr[P5], arr[S12], n);
    matAdd(arr[S12], arr[P7], arr[C11], n);

    // Calculating C12
    matAdd(arr[P3], arr[P5], arr[C12], n);

    // Calculating C21
    matAdd(arr[P2], arr[P4], arr[C21], n);

    // Calculating C22
    matAdd(arr[P1], arr[P3], arr[S13], n);
    matSub(arr[S13], arr[P2], arr[S14], n);
    matAdd(arr[S14], arr[P6], arr[C22], n);

    for (row = 0, i = 0; row < n; row++, i++)
    {
        for (col = 0, j = 0; col < n; col++, j++)
            Z[row*m+col] = arr[C11][i*n+j];
        for (col = n, j = 0; col < m; col++, j++)
            Z[row*m+col] = arr[C12][i*n+j];
    }
    for (row = n, i = 0; row < m; row++, i++)
    {
        for (col = 0, j = 0; col < n; col++, j++)
            Z[row*m+col] = arr[C21][i*n+j];
        for (col = n, j = 0; col < m; col++, j++)
            Z[row*m+col] = arr[C22][i*n+j];
    }

    for (i=0; i<arrs; i++)
        free (arr[i]);
}

void matMul(double *A, double *B, double *C, int n)
{
    int i = 0, j = 0, k = 0, row = 0, col = 0;
    double sum;
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n; j++)
        {
            sum = 0.0;
            for (k = 0; k < n; k++)
            {
                sum += A[i*n+k] * B[k*n+j];
            }
            C[i*n+j] = sum;
        }
    }
}

void matAdd(double *A, double *B, double *C, int m)
{
    int row = 0, col = 0;
    for (row = 0; row < m; row++)
        for (col = 0; col < m; col++)
            C[row*m+col] = A[row*m+col] + B[row*m+col];
}

void matSub(double *A, double *B, double *C, int m)
{
    int row = 0, col = 0;
    for (row = 0; row < m; row++)
        for (col = 0; col < m; col++)
            C[row*m+col] = A[row*m+col] - B[row*m+col];
}