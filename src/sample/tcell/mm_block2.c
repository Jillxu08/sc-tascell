//tasks:
// 1.without structure
// 2.add threshold
// 3.evaluate performance
// 4.tascell

#include <stdio.h>
#include <stdlib.h>
#include<math.h>
#include<time.h>
#include<sys/time.h>

#define N 2048
#define th 10

double a[N][N],b[N][N];  
double c[N][N],d[N][N];

double get_wall_time();
void createarr(double temp[][N]);
void matMul(int n);
void block_recursive(int a_r, int a_c, int b_r, int b_c, int c_r, int c_c, int len);
void print(double c[N][N]);
void printarray(double a[N][N]);

int main()
{
    int i,j;
    int a_r,a_c,b_r,b_c,c_r,c_c;
    int len=N;
    double start1, end1;
    double start2, end2;
    double elapsed1, elapsed2;

    a_r=a_c=0;
    b_r=b_c=0;
    c_r=c_c=0;

    printf("mm_block2's result is as follows:\n");
    printf("-------------------------------------------------------------------------\n\n"); 
    printf("Matrix size = %2d\n", N);
    printf("threshold = %2d\n", th);
    
    srand(0);
    createarr(a);
    createarr(b);

    start1 = get_wall_time();
    matMul(len);
    end1 = get_wall_time();
    elapsed1 = ((double) (end1 - start1));
    printf("naive_time = %5.4f\t\t",elapsed1);

    start2 = get_wall_time();
    block_recursive(a_r,a_c,b_r,b_c,c_r,c_c,len);
    end2 = get_wall_time();
    elapsed2 = ((double) (end2 - start2));
    printf("Block_time = %5.4f\n",elapsed2);

    printf("-------------------------------------------------------------------------\n\n");

    int flag = 0;
    for (i=0; i < len; i++){
        for (j = 0; j < len; j++){
            if (fabs (c[i][j] - d[i][j]) > 1.0E-6){
                printf("THE ANSWER IS WRONG!\n");
                flag = 1;
                break;
            }}
        if (flag == 1){
           break;
        }}
    if (flag == 0){
        printf("THE ANSWER IS RIGHT!\n");
    }
    // printf("-------------------------------------------------------------------------\n\n");
    // printf("-------------Input Matrices a and b--------------\n\n");
    // printarray(a);
    // printarray(b);
    // printf("-------------Output Matrices c and d--------------\n\n");
    // print(c);
    // print(d);
    return 0;
}

void block_recursive(int a_r, int a_c, int b_r, int b_c, int c_r, int c_c, int len){
    int n=len;
    int i;
    // if (n==1)
    // {
    //     c[c_r][c_c]+=a[a_r][a_c]*b[b_r][b_c];
    // }
    
   
    if (n<=th)
    { 
        int i_a,i_c,j_b,j_c,k_a,k_b;
        double sum;
        for (i_a=a_r, i_c=c_r; i_a<a_r+n; i_a++, i_c++)
        { 
            for (j_b=b_c, j_c=c_c; j_b<b_c+n; j_b++, j_c++)
            {
                sum = 0.0;
                for (k_a=a_c, k_b=b_r; k_a<a_c+n; k_a++, k_b++)
                {
                    sum += a[i_a][k_a] * b[k_b][j_b];
                }
                c[i_c][j_c] += sum;
            }
        }
    }
    
    else{
        int sa1_r, sa1_c, sb1_r, sb1_c, sc1_r, sc1_c;
        int sa2_r, sa2_c, sb2_r, sb2_c, sc2_r, sc2_c;
        int sa3_r, sa3_c, sb3_r, sb3_c, sc3_r, sc3_c;
        int sa4_r, sa4_c, sb4_r, sb4_c, sc4_r, sc4_c;
       
        // 矩阵A 进行分块后的各个子块下标
        sa1_r = a_r;
        sa1_c = a_c;
        sa2_r = a_r;
        sa2_c = a_c+n/2;
        sa3_c = a_c;
        sa3_r = a_r+n/2;
        sa4_r = a_r+n/2;
        sa4_c = a_c+n/2;

        // 矩阵B 进行分块后的各个子块下标
        sb1_r = b_r;
        sb1_c = b_c;
        sb2_r = b_r;
        sb2_c = b_c+n/2;
        sb3_c = b_c;
        sb3_r = b_r+n/2;
        sb4_r = b_r+n/2;
        sb4_c = b_c+n/2;
 
        // 矩阵temp 进行分块后的各个子块下标
        sc1_r=c_r;
        sc1_c=c_c;
        sc2_r=c_r;
        sc2_c=c_c+n/2;
        sc3_c=c_c;
        sc3_r=c_r+n/2;
        sc4_r=c_r+n/2;
        sc4_c=c_c+n/2;
// 将矩阵分为四块  分别求解。采用下标的方式进行分块，可以省去复制矩阵所产生的时间
// 若要复制矩阵则会产生 O(n*n)的时间复杂度
        block_recursive(sa1_r,sa1_c,sb1_r,sb1_c,sc1_r,sc1_c,n/2);
        block_recursive(sa2_r,sa2_c,sb3_r,sb3_c,sc1_r,sc1_c,n/2);
        block_recursive(sa1_r,sa1_c,sb2_r,sb2_c,sc2_r,sc2_c,n/2);
        block_recursive(sa2_r,sa2_c,sb4_r,sb4_c,sc2_r,sc2_c,n/2);
        block_recursive(sa3_r,sa3_c,sb1_r,sb1_c,sc3_r,sc3_c,n/2);
        block_recursive(sa4_r,sa4_c,sb3_r,sb3_c,sc3_r,sc3_c,n/2);
        block_recursive(sa3_r,sa3_c,sb2_r,sb2_c,sc4_r,sc4_c,n/2);
        block_recursive(sa4_r,sa4_c,sb4_r,sb4_c,sc4_r,sc4_c,n/2);

    }
}

void createarr(double temp[][N]){
    int i,j;
    
    for (i = 0; i < N; ++i)
    {
        for (j = 0; j < N; ++j)
        {
            temp[i][j]=(double)rand()/RAND_MAX;}}}

void print(double c[N][N]){
    int i,j;
    printf("\n====================================\n");
    for (i = 0; i < N; ++i)
    {
        for (j = 0; j < N; ++j)
        {
            printf("%f ", c[i][j]);
        }
        printf("\n");
    }
    printf("===================================\n");
}

void printarray(double a[N][N]){
    int i,j;
    printf("-----------------------\n");
    for (i = 0; i < N; ++i)
    {
        for (j = 0; j < N; ++j)
        {
            printf("%f \t", a[i][j]);
        }
        printf("\n");
    }
    printf("-----------------------\n");
}

double get_wall_time(){
  struct timeval time;
  if (gettimeofday(&time,NULL)){
    return 0;
  }
  return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

void matMul(int n)
{
    int i = 0, j = 0, k = 0;
    double sum;
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n; j++)
        {
            sum = 0.0;
            for (k = 0; k < n; k++)
            {
                sum += a[i][k] * b[k][j];
            }
            d[i][j] = sum;
        }
    }
}


