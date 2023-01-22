#include <omp.h>
#include "th.h"

double c[36][N][N];
double a[N][N];
double b[N][N];
double T_b[N][N];

void mm_sub(int a_r, int a_c, int b_r, int b_c, int c_r, int c_c, int n, int id)
{
    int i_a, i_c, j_b, j_c, k_a, k_b;
    double sum;

	for (int k_b=b_r; k_b<b_r+n; k_b++)
	{	
		for (int j_b=b_c; j_b<b_c+n; j_b++)
			T_b[j_b][k_b]= b[k_b][j_b];
	}
	
	#pragma omp target enter data map(to:a[0:N][0:N],T_b[0:N][0:N],c[36][0:N][0:N], a_r, a_c, b_r, b_c, c_r, c_c, n, id) 
    #pragma omp target teams distribute parallel for private(k_b,j_c)
	for (i_a=a_r; i_a<a_r+n; i_a++)
	{	
		int t=c_r+i_a-a_r;
		j_c=c_c;
		for (j_b=b_c; j_b<b_c+n; j_b++)
		{			
			sum = 0.0;
			k_b=b_r;
			for (k_a=a_c; k_a<a_c+n; k_a++)
			{
				sum += a[i_a][k_a] * T_b[j_b][k_b];
				k_b++;
			} 
				c[id][t][j_c] += sum; 
			j_c++;
		}
	}
	#pragma omp target update from(c[36][0:N][0:N])
    {}  
    #pragma omp target exit data map(release: a[0:N][0:N],T_b[0:N][0:N])        
	{} 
}
