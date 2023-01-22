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
	
    for (i_a=a_r, i_c=c_r; i_a<a_r+n; i_a++, i_c++)
    {
        for (j_b=b_c, j_c=c_c; j_b<b_c+n; j_b++, j_c++)
        {
            sum = 0.0;
            for (k_a=a_c, k_b=b_r; k_a<a_c+n; k_a++, k_b++)
            {
                sum += a[i_a][k_a] * T_b[j_b][k_b];
            } 
            c[id][i_c][j_c] += sum;
        }
    }
}