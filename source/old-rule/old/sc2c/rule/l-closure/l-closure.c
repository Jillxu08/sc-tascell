//L-closure$B$r%5%]!<%H$9$k(BC$B8@8l$+$iB~$N(BC$B8@8l$X$NJQ49Nc$G$9!%(B

//$B!z!z(BL-closure$B$r%5%]!<%H$9$k(BC$B8@8l$N%W%m%0%i%`Nc!z!z(B:

int h(int i, lightweight int (*g)(int)){
  return g(g(i));
}

int foo(int a){
  int x = 0;
  lightweight int g1(int b){
    x++;
    printf("g1(%ld)=%ld \n", b, a+b);
    return a+b;
  }
  return h(a+1, g1) + x;
}

main(){
  printf("%ld\n", foo(1));
}

//$B!z!zB~$N(BC$B8@8l$X$NJQ49Nc!z!z(B:

#include <stddef.h>


typedef char *(*nestfn_t)(char *, void *);
typedef struct { nestfn_t fn; void *fr; } closure_t;

#define SPECIAL(tp)	(((tp)0)-1)

typedef double align_t;
#define ALIGNED_ADD(base, size)	\
  ((char *)(((align_t *)(base)) + ((size)+sizeof(align_t)-1)/sizeof(align_t)))
#define ALIGNED_SUB(base, size)	\
  ((char *)(((align_t *)(base)) - ((size)+sizeof(align_t)-1)/sizeof(align_t)))

#define MREF(tp, p)	(*(tp *)(p))

#define PUSH_ARG(tp, v, argp) \
  (MREF(tp, (argp)) = (v), (argp) = ALIGNED_ADD((argp), (sizeof(tp))))
#define POP_ARG(tp, argp) \
  ((argp) = ALIGNED_SUB((argp), (sizeof(tp))), MREF(tp, (argp)))


int h(char *esp, int i, closure_t *g)
{
  char *new_esp;
  char *argp;
  struct frame {
    char *tmp_esp;
    char *argp;
    int callid;
    int i;
    closure_t *g;
  } *efp;
  int tmp_res1;
  if(((size_t)esp) & 1){
    esp -= 1;
    efp = (struct frame *) esp;
    esp =  ALIGNED_ADD (esp, sizeof(struct frame));
    switch(efp->callid){
    case 0: goto L_call0;
    case 1: goto L_call1;
    }
    goto L_call0;
  }
  efp = (struct frame *) esp;
  esp =  ALIGNED_ADD (esp, sizeof(struct frame));
  MREF(char *, esp) = 0; // $B4X?t$N@hF,$G$N$_(B
  {
    argp = ALIGNED_ADD (esp, sizeof(char *));
    /// g->fn $B$r$I$3$+!"(Bargp $B$KJ]B8$7$J$$$H$$$1$J$$(B
    // $B$3$N;~E@$G!"(Bg->fn, g->fr $B$O$^$@@55,2=$5$l$F$$$J$$(B
    // g $B$=$N$b$N$r5-21$9$k$7$+$J$$(B
    PUSH_ARG(int, i, argp);
    MREF(closure_t *, argp) = g;
    efp->i = i;
    efp->g = g;
    efp->callid = 0;
    efp->tmp_esp = efp->argp = argp;
    return SPECIAL(int);
  L_call0:
    i = efp->i;
    g = efp->g;
    tmp_res1 = MREF(int, efp->argp);
  }
  {
    argp = ALIGNED_ADD (esp, sizeof(char *));
    PUSH_ARG(int, tmp_res1, argp);
    MREF(closure_t *, argp) = g;
    efp->i = i;
    efp->g = g;
    efp->callid = 1;
    efp->tmp_esp = efp->argp = argp;
    return SPECIAL(int);
  L_call1:
    i = efp->i;
    g = efp->g;
    tmp_res1 = MREF(int, efp->argp);
  }
  return tmp_res1;
}



struct foo_frame {
  char *tmp_esp;
  char *argp;
  int callid;
  int a;
  int x;
  int tmp_arg1;
  closure_t *tmp_arg2;
  closure_t g1;
};

char *g1_0_in_foo(char *esp, void *xfp0)
{
  // leaf $B$N>l9g$O4JC1$K$J$k!#$3$3$G$O$=$&!#(B
  struct foo_frame *xfp = xfp0;
  char *parmp = esp;
  int b = POP_ARG(int, parmp);
  xfp->x++;
  printf("g1(%ld)=%ld \n", b, xfp->a+b);
  MREF(int, esp) = xfp->a+b;
  return 0;
}

int foo(char *esp, int a)
{
  char *new_esp;
  char *argp;
  struct foo_frame *efp;
  int x = 0;
  int tmp_arg1;
  closure_t *tmp_arg2;
  int tmp_res1;
  if(((size_t)esp) & 1){
    esp -= 1;
    efp = (struct foo_frame *) esp;
    esp =  ALIGNED_ADD (esp, sizeof(struct foo_frame));
    MREF(char *, esp) = 0;
    switch(efp->callid){
    case 0: goto L_call0;
    }
    goto L_call0;
  }
  efp = (struct foo_frame *) esp;
  esp =  ALIGNED_ADD (esp, sizeof(struct foo_frame));
  MREF(char *, esp) = 0; // $B4X?t$N@hF,$G$N$_(B
  new_esp = esp;
  tmp_arg1 = a+1;
  tmp_arg2 = &(efp->g1);
  while ((tmp_res1 = h(new_esp, tmp_arg1, tmp_arg2)) == SPECIAL(int) &&
	 (efp->tmp_esp = MREF(char *, esp)) != 0){
    // $B%9%?%C%/$K$*$-$C$Q$J$7$K$G$-$J$$$+$i!"$+$D!"F~$l;R4X?t$K8+$;$k$?$a(B
    // $B%G!<%?$N%;!<%V(B
    efp->a = a;
    efp->x = x;
    // tmp_arg1, tmp_arg2 $B$rJ]B8$9$k$H(B foo$B$G(Bh$B$N8F$S=P$78e$K$b(B
    // $B@8$-$F$$$kJQ?t$,A}$($F9%$^$7$/$J$$!#(B
    efp->g1.fn = g1_0_in_foo;
    efp->g1.fr = (void *)efp;
    efp->callid = 0;
    return SPECIAL(int);
  L_call0:
    // $B%9%?%C%/$K$*$-$C$Q$J$7$K$G$-$J$$$+$iJ]B8$7$?%G!<%?$NI|3h!"$+$D!"(B
    // $BF~$l;R4X?t$NI{:nMQ$NH?1G$N$?$a!"%G!<%?$r%m!<%I(B
    a = efp->a;
    x = efp->x;
    new_esp = esp + 1;
    tmp_arg1 = efp->tmp_arg1;  // $B>C$9$HCY$/$J$k(B???
    tmp_arg2 = efp->tmp_arg2;
  }
  return tmp_res1 + x;
}


void lw_call(char *esp){
  char *tmp_esp;
  closure_t *clos = MREF(closure_t *, esp);
  char *new_esp = esp;
  while(tmp_esp = (clos->fn) (new_esp, clos->fr)){
    lw_call(tmp_esp);
    new_esp = esp + 1;
  }
}

main(){
  char estack[8192];
  char *esp = estack;
  char *new_esp;
  struct frame {
    char *tmp_esp;
    char *argp;
    int callid;
    int tmp_arg1;
  } *efp;
  int tmp_arg1;
  int tmp_res1;
  efp = (struct frame *) esp;
  esp =  ALIGNED_ADD (esp, sizeof(struct frame));
  MREF(char *, esp) = 0;
  new_esp = esp;
  tmp_arg1 = 1;
  while ((tmp_res1 = foo(new_esp, tmp_arg1)) == SPECIAL(int) &&
	 (efp->tmp_esp = MREF(char *, esp)) != 0){
    MREF(char *, esp) = 0;
    lw_call (efp->tmp_esp);
    new_esp = esp + 1;
    tmp_arg1 = efp->tmp_arg1;  // $B>C$9$HCY$/$J$k(B???
  }
  printf("%ld\n", tmp_res1);
}


