#include <stdio.h>
#include <stdlib.h>
#define NX 193
#define NY 193
#define ND ((NX+1)*(NY+1))
#define NXNY (NX*NY)

int main(int argc, const char * argv[]) {

  double u[NY+1][NX+1], un[NY+1][NX+1];
  int i,j,k;
  double t=0.0;
  double delT;
  delT = 0.1/(NX*NX);
  for(j=0;j<NY+1;j++){
    for(i=0;i<NX+1;i++){
      u[j][i] = 0;
    }
  }
  for(j=1;j<NY+1;j++){
    u[j][0] = 0.5;
  }
  for(i=0;i<NX+1;i++){
    u[0][i] = 1;
  }
  for(k=0;k<40000;k++){
    for(j=1;j<NY;j++){
      for(i=1;i<NX;i++){
        un[j][i] = u[j][i] + delT*NX*NX*(u[j+1][i] + u[j-1][i] + u[j][i+1] + u[j][i-1] - 4*u[j][i]);
      }
    }
    for(j=1;j<NY;j++){
      for(i=1;i<NX;i++){
        u[j][i] = un[j][i];
      }
    }
  }
  // printf('finish');
  FILE * fp;
  fp = fopen ("u.data","w");
  for(j=0; j<=NY; j++){
    for(i=0; i<=NX; i++){
      fprintf(fp," %.15E %.15E %.15E\n", (double)i/NX, (double)j/NY, u[j][i]);
    }
    fprintf(fp,"\n");
  }
  fclose(fp);
}
