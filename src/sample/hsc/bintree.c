#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "copygc.h"


/* ��¤����� */

typedef struct _Bintree Bintree;

struct _Bintree {
  desc_t d;
  int key;
  int val;
  Bintree *left, *right;
};

/* ��¤�����: �ݥ��󥿤ΰ��� */

size_t Bintree_node[] = {offsetof(Bintree, left), offsetof(Bintree, right)};
//offsetof�ϡ��������Ϲ�¤�Ρ�����Ϲ�¤�ΤΥݥ����ѿ�

descriptor Bintree_d = DESC(Bintree, Bintree_node);
//��¤�ΤΤɤΰ��֤˥ݥ��󥿤����뤫�򵭲����Ƥ���


/* ��¬�ѥ�᡼�� */
static int maxins, maxsearch;


Bintree *newBintree(sht scan0, int k, int v){
  //������scan0���ä�롣
  //������Bintree��getmem��0����scan0�ؤ��Ѥ��
  Bintree *p = (Bintree *)getmem(scan0, &Bintree_d);
  p->key = k;
  p->val = v;
  return p;
}

/* x �����Ǥ�¸�ߤ����� */
void insert(sht scan0, Bintree *x, int k, int v){
  //������scan0���ɲ�
  Bintree *y = 0;

  CT void scan1(void){
    x = move(x);
    y = move(y);
    scan0();
  }
  //������scan1�ؿ������
  //move�����塢�����Ȥ����Ϥ��줿scan0�ؿ���¹Ԥ���ؿ�

  while(1){
    if(x->key == k){
      x->val = v;
      return;
    } else if(k < x->key){
      y = x->left;
      if(!y) {
	x->left = newBintree(scan1,k, v);//������scan1�ؿ����ɲ�
	return;
      }
    }else{
      y = x->right;
      if(!y) {
	x->right = newBintree(scan1,k, v);//������scan1�ؿ����ɲ�
	return;
      }
    }
    x = y;
  }
}

int search(sht scan0, Bintree *x, int k, int v0){
  //������scan0���ɲá��������Ȥ�ʤ�
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
  //scan1�ؿ������������ϡ�move��scan0��¹Ԥ���ؿ�

  int i, k;
  unsigned short seed[3];
  seed[0] = 3;
  seed[1] = 4;
  seed[2] = 5;
  for(i=0; i<n; i++){
    k = nrand48(seed);
    insert(scan1,this, k, k);//scan1�ؿ���insert���Ϥ�
  }
}

void randsearch(sht scan0,Bintree *this, int n){
  CT void scan1(void){
    this = move(this);
    scan0();
  }
  //scan1�ؿ������������ϡ�move��scan0��¹Ԥ���ؿ�
  
  int i, k;
  unsigned short seed[3];
  seed[0] = 8; seed[1] = 9; seed[2] = 10;
  for(i=0; i<n; i++){
    k = nrand48(seed);
    search(scan1,this, k, 0);//scan1�ؿ���search���Ϥ�
  }
}

/* gc�ѥ�᡼�������� */
//���֤˥��ޥ�ɥ饤�����3,4,5,6������
void init(int tp, int tosize, int stack_size, int limited_max) {
  gc_params p;
  p.gcv = 1;
  p.gctype = tp;//����3
  p.tosize = tosize;//����4
  p.stack_size = stack_size;//����5
  p.limited_stack_max = limited_max;//����6
  getmem_init(p);
}

int main(int argc, char *argv[]) {
  struct timeval tp1, tp2;
  int i, gctp, searchskip = 0;
  Bintree *root;

  CT void scan1(void){
    root = move(root);
  }
  //scan1�ؿ���move(root)�򤹤롣

  if(argc > 1 && strcmp(argv[1],"-s") == 0){
    argc--; argv++;
    searchskip++;
  }

  //���ޥ�ɥ饤�������������ɽ��
  maxins = argc > 1 ? atoi(argv[1]) : 100000;//����1
  maxsearch = argc > 2 ? atoi(argv[2]) : 300000;//����2

  printf("Bintree=%d, maxins=%d, maxsearch=%d\n", sizeof(Bintree),  maxins, maxsearch);

  gctp = argc > 3 ? atoi(argv[3]) : 0;//����3

  init(
       gctp < 0 ? 0 : gctp,
       argc > 4 ? atoi(argv[4]) : 0,
       argc > 5 ? atoi(argv[5]) : 0,
       argc > 6 ? atoi(argv[6]) : 0
       );
  //�����ޤǥ��ޥ�ɥ饤�����

  root = getmem(scan1, &Bintree_d);//������ݡ�
  root->key = 0;
  root->val = 0;
  
  /* time */
  gettimeofday(&tp1, 0); 

  randinsert(scan1,root, maxins);//����������

  /* time */
  gettimeofday(&tp2, 0);

  if(gctp>=0)
    gc(scan1);

  if(!searchskip)
    randsearch(scan1,root, maxsearch);//�����ม��

  printf("---bintree.c---\nmain\n");
  printf("tp1.tv_sec=%f, tp2.tv_sec=%f\n", tp1.tv_sec, tp2.tv_sec);
  printf("tp1.tv_usec=%f, tp2.tv_usec=%f\n\n", tp1.tv_usec, tp2.tv_usec);

  {
    double ttime = (tp2.tv_sec - tp1.tv_sec) + (tp2.tv_usec - tp1.tv_usec) * 0.000001;
    printf("%f %f %f\n", ttime, gc_ttime, ttime-gc_ttime);
  }
  return 0;
}
