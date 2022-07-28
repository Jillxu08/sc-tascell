//矩阵乘法

# include <stdio.h>
# include <stdlib.h>
# include <time.h>
# include <sys/time.h>

#define MATRIX_SIZE 1000

double get_wall_time(){
  struct timeval time;
  if (gettimeofday(&time,NULL)){ 
    return 0;
  }
  return (double)time.tv_sec + (double)time.tv_usec * .000001;
}



//生成随机矩阵
void matgen(float* a, int n)
{
    int i, j;

    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n; j++)
        {

            a[i * n + j] = (float)rand() / RAND_MAX + (float)rand() / (RAND_MAX * RAND_MAX);

        }
    }
}

// void
// mm (float *a, float *b, float *c, int n)
// {
//   int i;
//   int j;
//   int k;
//   double t = 0;
//   {
//     i = 0;
//     for (; i < n; i++)
//       {
//         j = 0;
//         for (; j < n; j++)
//           {
//             {
//               k = 0;
//               for (; k < n; k++)
//                 {
//                   t += a[i * n + k] * b[k * n + j];
//                 }
//             }
//             c[i * n + j] = t;
//           }
//       }
//   }
// }


int main()
{

    //定义矩阵
    float *a, *b, *c;

    int n = MATRIX_SIZE;

    //分配内存
    a = (float*)malloc(sizeof(float)* n * n); 
    b = (float*)malloc(sizeof(float)* n * n); 
    c = (float*)malloc(sizeof(float)* n * n); 
    
    //设置随机数种子
    srand(0);

    //随机生成矩阵
    matgen(a, n);
    matgen(b, n);
    


    // clock_t final_time = max_end - min_start;
    double start = get_wall_time();

    // mm (a, b, c, n);
      //CPU矩阵乘法，存入矩阵c
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        { 
            double t = 0;

            for (int k = 0; k < n; k++)
            { 

                t += a[i * n + k] * b[k * n + j]; 

            } 

            c[i * n + j] = t;
        }
    }

    double end = get_wall_time();

    double usetime = end - start;

    printf("usetime = %lf\n", usetime);


//    //output
//     for (int i = 0; i < n; i++)
//     {
//         for (int j = 0; j < n; j++)
//         { 
//             printf("%f ", c[i * n + j]);
//         } 
//         printf("\n");
//     }
    
    return 0;

}



