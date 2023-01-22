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

#define MATRIX_SIZE 16
#define ROW 8    //指定 行数
#define COL 8      //指定 列数 

// int ROW = COL = MATRIX_SIZE
double a[ROW][COL],b[ROW][COL];  //matrix a and matrix b
double c[ROW][COL];                      // c = a * b 

//保存一个矩阵的第一个元素的位置，即左上角元素的下标
//如果加上一个长度就可以知道整个矩阵了
//the name of struct is subarr
typedef struct {   //这里没有指定一个矩阵的长度，在分块时应该加入长度，否则不知道子块矩阵的大小
    int str,stc;    //str行下标  ； strc列下标
}subarr;

// 两矩阵arr、brr相加减 保存在temp中
double get_wall_time();

//分治法 求矩阵相乘 ，sa,sb分别为矩阵a,b参加运算的首元素
void block_recursive(subarr sa,subarr sb,subarr sc,int len){
    int n=len;
    // int **temp;
    int i;
    // 长度为1 则直接相乘
    if (n==1)
    {
        c[sc.str][sc.stc]+=a[sa.str][sa.stc]*b[sb.str][sb.stc];
    }else{
         // 这里都是对下标进行初始化
         // sa,sb,sc代表输入矩阵A,B,temp参加运算的首元素下标，因为进行分块后只进行特定子块的运算
         //标号1，2，3，4 分别代表第一、二、三、四个子块
        subarr sa1,sb1, sc1;
        subarr sa2,sb2, sc2;
        subarr sa3, sb3,sc3;
        subarr sa4, sb4, sc4;
        // 矩阵A 进行分块后的各个子块下标
        sa1.str=sa.str;
        sa1.stc=sa.stc;
        sa2.str=sa.str;
        sa2.stc=sa.stc+n/2;
        sa3.stc=sa.stc;
        sa3.str=sa.str+n/2;
        sa4.str=sa.str+n/2;
        sa4.stc=sa.stc+n/2;
        // 矩阵B 进行分块后的各个子块下标
        sb1.str=sb.str;
        sb1.stc=sb.stc;
        sb2.str=sb.str;
        sb2.stc=sb.stc+n/2;
        sb3.stc=sb.stc;
        sb3.str=sb.str+n/2;
        sb4.str=sb.str+n/2;
        sb4.stc=sb.stc+n/2;
        // 矩阵temp 进行分块后的各个子块下标
        sc1.str=sc1.stc=0;
        sc2.str=0;
        sc2.stc=n/2;
        sc3.stc=0;
        sc3.str=n/2;
        sc4.str=n/2;
        sc4.stc=n/2;
// 将矩阵分为四块  分别求解。采用下标的方式进行分块，可以省去复制矩阵所产生的时间
// 若要复制矩阵则会产生 O(n*n)的时间复杂度
        block_recursive(sa1,sb1,sc1,n/2);

        block_recursive(sa2,sb3,sc1,n/2);

        block_recursive(sa1,sb2,sc2,n/2);
        block_recursive(sa2,sb4,sc2,n/2);
        block_recursive(sa3,sb1,sc3,n/2);
        block_recursive(sa4,sb3,sc3,n/2);
        block_recursive(sa3,sb2,sc4,n/2);
        block_recursive(sa4,sb4,sc4,n/2);
        


    }
    // return 0;

}

//为矩阵初始化 即赋值
void createarr(double temp[][COL]){
    int i,j;
    for (i = 0; i < ROW; ++i)
    {
        for (j = 0; j < COL; ++j)
        {
            temp[i][j]=(int)rand()%5;
            

        }

    }

}
// 打印C矩阵
void print(){
    int i,j;
    printf("\n====================================\n");
    for (i = 0; i < ROW; ++i)
    {
        for (j = 0; j < COL; ++j)
        {
            printf("%f\t", c[i][j]);
        }
        printf("\n");
    }
    printf("===================================\n");
}
// 打印矩阵
void printarray(double a[ROW][COL]){
    int i,j;
    printf("-----------------------\n");
    for (i = 0; i < ROW; ++i)
    {
        for (j = 0; j < COL; ++j)
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

int main()
{
    int i,j;
    subarr sa,sb,sc;
    int len;
    //初始化各个下标
    sa.str=sa.stc=0;
    sb.str=sb.stc=0;
    sc.str=sc.stc=0;
    // 长度赋值，因为在subarr结构里没有长度的定义
    len=ROW;

    // 给矩阵A，B 复制初始化
    createarr(a);
    createarr(b);
    //  进行运算
    block_recursive(sa,sb,sc,len);
    // 打印矩阵A,B,C
    printarray(a);
    printarray(b);
    print();
    return 0;
}
