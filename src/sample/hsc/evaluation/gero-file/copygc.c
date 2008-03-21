#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "copygc.h"

#ifndef TOSIZE
#define TOSIZE   (5*1024*1024)
#endif

#ifndef ROOTSIZEMAX
#define ROOTSIZEMAX (8*1024)
#endif

#ifndef GC_STACK_SIZE
#define GC_STACK_SIZE (params.tosize/(sizeof(double)))
#endif

#ifndef GC_LIMITED_STACK_SIZE
#define GC_LIMITED_STACK_SIZE 256
#endif

#ifndef GC_LIMITED_STACK_MAX
#define GC_LIMITED_STACK_MAX 32
#endif

#define IN_FROM(p) ((unsigned)((char *)(p) - old_memory) < params.tosize)
//p����Â������������������̂��Aparams.tosize��菬�����@�@�H�H
#define IN_TOSP(p) ((unsigned)((char *)(p) - new_memory) < params.tosize)
//p����V���������������������̂��Aparams.tosize��菬�����@�H�H
#define FWPTR(p) (*((void **)(p)))
//p���A�|�C���^�̃|�C���^�ɂ������̂̃|�C���^�@�@�H�H

#if 0
#define MEMCPY(d,s,sz) memcpy((d),(s),(sz))
#else
#define MEMCPY(d,s,sz) \
  { long *_des=(long *)(d), *_src=(long *)(s),  \
         *_til=(long *)(((char *)_src)+(sz)); \
    do{ *(_des++) = *(_src++); } while( _src < _til); }
//MEMCPY�́Asrc����des�ւƃR�s�[����֐�
//�����炭�R�s�[GC�̃R�s�[�����̂��ƁB
//sz�́A�R�s�[���郁�����̗�
#endif

/* util */

static void error(char *s)//�G���[����
{
  fprintf(stderr, "ERROR: %s\n", s);
  exit(EXIT_FAILURE);
}

static void *mymalloc(size_t size)
{
  void *p = malloc(size);
  //malloc�̕Ԃ�l:�m�ۂ����������u���b�N���w���|�C���^
  //���s(�������s��)�Ȃ�NULL�B
  if(p == NULL)
    error("Not enough memory.");//�������s��
  return p;
}

static void *myrealloc(void *p, size_t size) {
  void *q = realloc(p, size);
  //p�̈ʒu�̃��������Asize�̑傫���ɂ��ĐV���ɃR�s�[����B
  //�R�s�[���q�ɂȂ�B

  if(q == NULL)
    error("Not enough memory.");
  return q;
}

/* root + heap area */

gc_params params;

static int allocated_size;
static char *old_memory, *new_memory;//�����^�ւ̃|�C���^
static char *old_memory_end, *new_memory_end;

/* gc */

static char *b;

/* *link = move(*link); */

void *move(void *vp)
     //move�֐��B�����N��H����́B
     //*vp����́A�|�C���^��Ԃ�
{
  char *p = vp;
  if(!IN_FROM(p))//((unsigned)((char *)(p) - old_memory) > params.tosize)
    return vp;
  if(IN_TOSP(FWPTR(p)))//((unsigned)((char *)(p) - new_memory) < params.tosize)
    return FWPTR(p);//p��(*((void **)(p)))�ɂ��ĕԂ�


  //�ǂ���ł��Ȃ����
  {
    desc_t d = *(desc_t *)p;//desc_t�͍\����
    
    char *np = b, *nb = np + d->asize;
    //b�͕�����Bgc_breadth_first�̒��ŏo�Ă���Bnew_memory�����B
    //d��p�R��
    
    if(nb >= new_memory_end)
      error("buffer overrun.");
    //���炭�A�O�̃|�C���^�ɍ��m�ۂ��������������̂�
    //�������̗ʂ��I�[�o�[�����Ƃ��ɏo��G���[�B
    
    MEMCPY(np, p, d->size);//p����np�ւƁAd->size�������R�s�[����B
    b = nb;//nb��b�ցB
    FWPTR(p) = np;//(*((void **)(p)))��np����
    return np;
  }
}

/* BREADTH_FIRST_GC */

void gc_breadth_first(sht scan) {
  //�����͊֐��Bgc()�֐��̒��Ŏg���B
  int i;
  char *tmp;
  desc_t d;
  char *p;
  void **link;
  char *s;

  if(params.gcv)
    printf("BREADTH_FIRST_GC start\n");
  //params.gcv�̓\�[�X�������ς��Ȃ������1�B
  
  s = b = new_memory;//new_memory��s,b�ɑ��
  //new_memory�̏����l�́H -> �R�}���h���C����������������֐�
  //init(bt0.c�̒�)�����getmem_init�Ō��߂�B�R�}���h���C�������ɂ���Č��肷��B
  //b�̓O���[�o���ϐ�
  scan();
  //�X�L����(���̊֐��̈����̂��) scan1()�Ȃ�Amove�֐������s����B
  //move�̕Ԃ�l�́A�ړ��O�̃������ւ̃|�C���^�̈ʒu
  //void scan1(){root=move(root);}
  
  while(s < b) {
    //s�͊J�n�ʒu�Ab�͊m�ۂ��܂������Ō�̎w���Ă���_
    d = *(desc_t *)s;
    p = (char *)s;
    for(i = 0; i < d->fli_len; i++) {
      //d�̒��̃p�����[�^�̉񐔂���������
      //p��d->fli[i]�����������̂�link�Ƃ���B
      link = (void **)(p + d->fli[i]);
      *link = move(*link);//move����B
      //�悭������Ȃ����Alink��move����B
    }
    s += d->asize;//s��1�P�ʕ������i�߂�
  }
  allocated_size = b - new_memory;//�m�ۂ��ꂽ�������̃T�C�Y
  tmp = new_memory;
  new_memory = old_memory;
  old_memory = tmp;//new_memory��old_memory�̓���ւ�
  tmp = new_memory_end;
  new_memory_end = old_memory_end;
  old_memory_end = tmp;//new_memory_end��old_memory_end�̓���ւ�
  if(params.gcv)  //params.gcv�̓\�[�X�������ς��Ȃ������1�B
    printf("GC complete (%d)\n\n", allocated_size);//GC�����A�m�ۂ����ʂ�����
}

/* */

double gc_ttime;

void gc(sht scan){
  //gc�֐�
  //�����͊֐�scan
  //���炭�AGC�ɂ����������Ԃ𒲂ׂ�B
  struct timeval tp1, tp2;
  gettimeofday(&tp1, 0);
  
  switch(params.gctype){
    //params.gctype�̓R�}���h���C���̑�O�����B0�ȏ�̐������w��B
    //�����������0�ƂȂ�B
  case 0: gc_breadth_first(scan); break;//params.gctype��0�Ȃ��gc_breadth_first���s���B
  }
  gettimeofday(&tp2, 0); 
  gc_ttime += ((tp2.tv_sec - tp1.tv_sec) + (tp2.tv_usec - tp1.tv_usec) * 0.000001);
  //�����Ď��Ԍv��
  printf("gc(scan)\n");
  printf("tp1.tv_sec=%f, tp2.tv_sec=%f\n", tp1.tv_sec, tp2.tv_sec);
  printf("tp1.tv_usec=%f, tp2.tv_usec=%f\n", tp1.tv_usec, tp2.tv_usec);
}

/* memory allocation interface */

void getmem_init(gc_params p) {
  //�R�}���h���C������w�肵���AGC�̃p�����[�^�������ƂȂ�֐�
  char *mp;
  static int called = 0;

  if(called)//"called"�܂�u�Ăяo���ꂽ�v�Ƃ������ƁB�����炭���̊֐��͈�x���肵���g��Ȃ��B
    return;
  called=1;//"called"�ɂ���B
  gc_ttime = 0.0;
  params = p;//gc_params�^�̃O���[�o���ϐ�params�B
  if(params.tosize == 0)//params.tosize�̓R�}���h���C������4�Ŏw��\�B
    params.tosize = TOSIZE;//5*1024*1024
  
  params.tosize += 3;
  params.tosize -= (params.tosize & 3);
  //3�������̂��A����2�r�b�g�̒l��0�ɂ���B
  
  if(params.stack_size == 0)//�R�}���h���C������5
    params.stack_size = GC_STACK_SIZE;
  if(params.limited_stack_max == 0)//�R�}���h���C������6
    params.limited_stack_max = GC_LIMITED_STACK_MAX;
  //��������w��o����B�w�肪���������Ƃ��̓f�t�H���g�̒l��^����B
  
  printf("tosize=%d, stack=%d, limit=%d\n",params.tosize,params.stack_size,params.limited_stack_max);
  //�����̒l���o�͂���B
  
  old_memory = mymalloc(params.tosize);
  //params.tosize�������������m�ۂ��A�m�ۂ����u���b�N�̐擪�̈ʒu������
  old_memory_end = old_memory + params.tosize;
  //�m�ۂ����������̍Ō�̈ʒu������
  new_memory = mymalloc(params.tosize);
  new_memory_end = new_memory + params.tosize;
  //old_memory�Ɠ������Ƃ����B
  allocated_size = 0;
/*  
  for(mp = new_memory; mp < new_memory_end; mp+= 4096)
    *mp = 1;
  for(mp = old_memory; mp < old_memory_end; mp+= 4096)
    *mp = 1;*/
  //4096���ɋ�؂���1������B�܂��Ӗ���������Ȃ��B
}

void *try_getmem(desc_t d) {
  //getmem�֐��̒��Ŏg���B
  //�������������邩�ǂ����Atry����֐�
  //�����͍\����
  size_t size;
  char *p;

  size = d->asize;
  if(allocated_size + size > params.tosize)//GC���K�v�ȏꍇ
      return 0;

  p = old_memory + allocated_size;//�m�ۂ��I�������|�C���^���ڂ��B
  allocated_size += size;//���܂Ŋm�ۂ������v�����v�Z�B
  memset(p, 0, size);//p�̎w���I�u�W�F�N�g�̐擪size�����ɁA0��������B
  *(desc_t *)p = d;
  return p;
}

void *getmem(sht scan, desc_t d) {
  //�֐�scan�ƍ\���̂������ɂƂ�
  //�������𓾂�֐��H
  void *p;
  p = try_getmem(d);
  if(p == 0) {//try_getmem(d)�̕Ԃ�l��0�A���Ȃ킿�������s���Ȃ��GC�����s����B
    gc(scan);
    p = try_getmem(d);
    if(p == 0)//GC�����s���Ă�������������Ȃ���΁A�������s���Ƃ����\�����o���B
      error("getmem: Not enough memory.");
  }
  return p;//�������m�ۏI��
}
