#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "copygc.h"


/* $B9=B$BNDj5A(B */

typedef struct _Bintree Bintree;

struct _Bintree {
  desc_t d;
  int key;
  int val;
  Bintree *left, *right;
};

/* $B9=B$BNDj5A(B: $B%]%$%s%?$N0LCV(B */

size_t Bintree_node[] = {offsetof(Bintree, left), offsetof(Bintree, right)};
//offsetof$B$O!DBh0l0z?t$O9=B$BN!"BhFs$O9=B$BN$N%]%$%s%?JQ?t(B

descriptor Bintree_d = DESC(Bintree, Bintree_node);
//$B9=B$BN$N$I$N0LCV$K%]%$%s%?$,$"$k$+$r5-21$7$F$*$/(B


/* $B7WB,%Q%i%a!<%?(B */
static int maxins, maxsearch;


Bintree *newBintree(sht scan0, int k, int v){
  //$B0z?t$K(Bscan0$B$,2C$o$k!#(B
  //$B$=$7$F(BBintree$B$N(Bgetmem$B$,(B0$B$+$i(Bscan0$B$X$HJQ$o$k(B
  Bintree *p = (Bintree *)getmem(scan0, &Bintree_d);
  p->key = k;
  p->val = v;
  return p;
}

/* x $B$,$9$G$KB8:_$9$k>l9g(B */
void insert(sht scan0, Bintree *x, int k, int v){
  //$B0z?t$K(Bscan0$B$rDI2C(B
  Bintree *y = 0;

  CT void scan1(void){
    x = move(x);
    y = move(y);
    scan0();
  }
  //$B$3$3$G(Bscan1$B4X?t$N@k8@(B
  //move$B$7$?8e!"0z?t$H$7$FEO$5$l$?(Bscan0$B4X?t$r<B9T$9$k4X?t(B

  while(1){
    if(x->key == k){
      x->val = v;
      return;
    } else if(k < x->key){
      y = x->left;
      if(!y) {
	x->left = newBintree(scan1,k, v);//$B0z?t$K(Bscan1$B4X?t$NDI2C(B
	return;
      }
    }else{
      y = x->right;
      if(!y) {
	x->right = newBintree(scan1,k, v);//$B0z?t$K(Bscan1$B4X?t$NDI2C(B
	return;
      }
    }
    x = y;
  }
}

int search(sht scan0, Bintree *x, int k, int v0){
  //$B0z?t$K(Bscan0$B$NDI2C!"$7$+$7;H$o$J$$(B
  while(x){
    if(x->key == k){
      return x->val;
    } else if(k < x->key){
      x = x->left;
    }else{
      x = x->right;
    }
  }
  return v0;
}

void randinsert(sht scan0, Bintree *this, int n){
  CT void scan1(void){
    this = move(this);
    scan0();
  }
  //scan1$B4X?t$N@k8@!#$3$l$O!"(Bmove$B$H(Bscan0$B$r<B9T$9$k4X?t(B

  int i, k;
  unsigned short seed[3];
  seed[0] = 3;
  seed[1] = 4;
  seed[2] = 5;
  for(i=0; i<n; i++){
    k = nrand48(seed);
    insert(scan1,this, k, k);//scan1$B4X?t$r(Binsert$B$KEO$9(B
  }
}

void randsearch(sht scan0,Bintree *this, int n){
  CT void scan1(void){
    this = move(this);
    scan0();
  }
  //scan1$B4X?t$N@k8@!#$3$l$O!"(Bmove$B$H(Bscan0$B$r<B9T$9$k4X?t(B
  
  int i, k;
  unsigned short seed[3];
  seed[0] = 8; seed[1] = 9; seed[2] = 10;
  for(i=0; i<n; i++){
    k = nrand48(seed);
    search(scan1,this, k, 0);//scan1$B4X?t$r(Bsearch$B$KEO$9(B
  }
}

/* gc$B%Q%i%a!<%?$N@_Dj(B */
//$B=gHV$K%3%^%s%I%i%$%s0z?t(B3,4,5,6$B$,F~$k(B
void init(int tp, int tosize, int stack_size, int limited_max) {
  gc_params p;
  p.gcv = 1;
  p.gctype = tp;//$B0z?t(B3
  p.tosize = tosize;//$B0z?t(B4
  p.stack_size = stack_size;//$B0z?t(B5
  p.limited_stack_max = limited_max;//$B0z?t(B6
  getmem_init(p);
}

int main(int argc, char *argv[]) {
  struct timeval tp1, tp2;
  int i, gctp, searchskip = 0;
  Bintree *root;

  CT void scan1(void){
    root = move(root);
  }
  //scan1$B4X?t$O(Bmove(root)$B$r$9$k!#(B

  if(argc > 1 && strcmp(argv[1],"-s") == 0){
    argc--; argv++;
    searchskip++;
  }

  //$B%3%^%s%I%i%$%s0z?t$NBeF~!"I=<((B
  maxins = argc > 1 ? atoi(argv[1]) : 100000;//$B0z?t(B1
  maxsearch = argc > 2 ? atoi(argv[2]) : 300000;//$B0z?t(B2

  printf("Bintree=%d, maxins=%d, maxsearch=%d\n", sizeof(Bintree),  maxins, maxsearch);

  gctp = argc > 3 ? atoi(argv[3]) : 0;//$B0z?t(B3

  init(
       gctp < 0 ? 0 : gctp,
       argc > 4 ? atoi(argv[4]) : 0,
       argc > 5 ? atoi(argv[5]) : 0,
       argc > 6 ? atoi(argv[6]) : 0
       );
  //$B$3$3$^$G%3%^%s%I%i%$%s0z?t(B

  root = getmem(scan1, &Bintree_d);//$B%a%b%j3NJ]!)(B
  root->key = 0;
  root->val = 0;
  
  /* time */
  gettimeofday(&tp1, 0); 

  randinsert(scan1,root, maxins);//$B%i%s%@%`A^F~(B

  /* time */
  gettimeofday(&tp2, 0);

  if(gctp>=0)
    gc(scan1);

  if(!searchskip)
    randsearch(scan1,root, maxsearch);//$B%i%s%@%`8!:w(B

  printf("---bintree.c---\nmain\n");
  printf("tp1.tv_sec=%f, tp2.tv_sec=%f\n", tp1.tv_sec, tp2.tv_sec);
  printf("tp1.tv_usec=%f, tp2.tv_usec=%f\n\n", tp1.tv_usec, tp2.tv_usec);

  {
    double ttime = (tp2.tv_sec - tp1.tv_sec) + (tp2.tv_usec - tp1.tv_usec) * 0.000001;
    printf("%f %f %f\n", ttime, gc_ttime, ttime-gc_ttime);
  }
  return 0;
}
