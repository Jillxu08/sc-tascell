#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

#define BUFSIZE 1024
#define MAXCMDC 16



int systhr_create(void * (*start_func)(void *), void *arg){
  int status = 0;
  pthread_t tid;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  status = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  if(status == 0) status = pthread_create(&tid, &attr, start_func, arg);
  if(status != 0) status = pthread_create(&tid, 0,     start_func, arg);
  return status;
}

void proto_error(const char *str){
  fputs(str, stderr);
  exit(1);
}

void mem_error(const char *str){
  fputs(str, stderr);
  exit(1);
}

struct cmd {
  int  c;
  char *v[MAXCMDC];
};


char buf[BUFSIZE];

/* input -> struc cmd */
struct cmd recv_command(){
  struct cmd r;
  char p, c, *b = buf;
  fgets(b, BUFSIZE, stdin);
  r.c = 0;
  for(p = 0, c = *b ; c ; p = c, c= *++b)
    if(c == ' ' || c = '\n')
      *b = c = 0;
    else if (p == 0 && r.c < MAXCMDC)
      r.v[r.c++] = b;
  return r;
}

pthread_mutex_t send_mut = PTHREAD_MUTEX_INITIALIZER;

/* struc cmd -> output */
struct cmd send_command(struct cmd cmd){
  int i;
  for(i=0; i<cmd.c-1; i++){
    fputs(cmd.v[i], stdout); fputc(' ', stdout);
  }
  if(cmd.c > 0){
    fputs(cmd.v[cmd.c-1], stdout); fputc('\n', stdout);
  }
}

void read_to_eol(){
  int c;
  while((c = getc(stdin)) != EOF) if (c == '\n') break;
}
void write_eol(){ putc('\n', stdout); }
viod flush_send(){ fflush(stdout); }

enum task_stat {
  TASK_ALLOCATED, TASK_INITIALIZED, TASK_STARTED, TASK_DONE
};

enum task_home_stat {
  TASK_HOME_STARTED, TASK_HOME_DONE
};

#define FF int (*_bk)(), struct task *_x
#define FA _bk, _x
#define TASK_BODY(tp) (*((TP *)_x->body))

/* $BL$JQ99(B  $B$^$@(B SP $B$r<+J,$G$d$k$h$&$K$J$C$F$$$J$$(B */
#define DO_TWO(work1, work2, output_work2, input_work2)			\
do{									\
  int spawned = 0;							\
  {									\
    int (*_bk2)() = _bk;						\
    int _bk(){								\
      if(_bk2()) return 1;						\
      if(!spawned){							\
	spawned = 1;							\
	puts("task");							\
	(output_work2);							\
	putchar('\n');							\
	fflush(stdout);							\
	return 1;							\
      }									\
      return 0;								\
    }									\
    if(req){ req = 0; if(!_bk()){ printf("none\n"); fflush(stdout); }}	\
    (work1);								\
  }									\
  if(spawned){								\
    char *buf;								\
    printf("rqst\n"); fflush(stdout);					\
    while((buf= recv_command(), strncmp(buf,"rslt",4) != 0)){		\
      if(strncmp(buf,"task",4) == 0)					\
	read_exec_print(1, 0);						\
      else if(strncmp(buf,"subt",4) == 0)				\
	read_exec_print(1, 1);						\
    }									\
    (input_work2); read_to_eol();					\
  } else {								\
    (work2);								\
  }									\
}while(0)

struct thread_data;

struct task {
  volatile int req;
  enum task_stat stat;
  struct task* next;
  struct task* prev;
  struct thread_data *thr;
  void *body;
  int ndiv;
  char rslt_head[256];
};

struct task_home {
  // pthread_mutex_t mut;
  enum task_home_stat stat;
  int id;
  struct task_home* next;
  void *body;
  int ndiv;
};

struct thread_data {
  int id;
  char id_str[32];
  struct task_home* sub;
  struct task *task_top;
  pthread_mutex_t mut;
  pthread_cond_t cond;
  char buf[BUFSIZE];
};

struct thread_data thread_data[64];

/*
  $B%j%b!<%H$K(B treq $B$7$F$b$i$C$?(B task (copy)
    - task_root $B$+$i$N%j%9%H$H$J$j!$(Bid $B$O=EJ#$7$J$$$h$&$KIU$1$k!%(B
      (free$B%j%9%H$KLa$9$H$-!$(Bid $B$r$=$N$^$^$K!$:FMxMQ$9$l$P$h$$(B?)
    - treq $B$^$($K(B allocate
    x $B$H$-$I$-!$%j%9%H$+$i(B DONE $B$H$J$C$?$b$N$r=|$/(B
      ($B$=$l$J$i!$(Brack $B$rF@$k$^$G(B DONE $B$K$7$J$$$h$&$K$9$k(B
       active $B$J%9%l%C%I?t$OJL4IM}$9$l$P$h$$(B?)
    -  $B$H$-$I$-$G$J$/$F!$(Brack $B$rF@$?$H$-$G$h$$(B?

  $BJ,3d$7$F:n$C$?(B task (home/orig) => task_home
    x $BJ,3d85(Btask $B$N(B sub $B$+$i$N%j%9%H$H$J$j!$(Bid $B$O=EJ#$7$J$$$h$&$KIU$1$k!%(B
      ($BJ,3d85(Btask$B$O!$%j%b!<%H$K(B treq $B$7$F$b$i$C$?(B task (copy) $B$N$_(B)
    o thread_data $B$N(B sub $B$+$i$N%j%9%H$H$J$j!$(Bid $B$O=EJ#$7$J$$$h$&$KIU$1$k!%(B
    - $B:G=i$+$i!$(BSTARTED$B!$$9$0$K(B treq $B85$KAw$k!%(B
    - rslt $B$,$-$?$i!$(BDONE $B$K$7$F!$(Brack $B$rJV$9(B
    - DONE $B$K$J$C$F$$$?$i!$J,3d85(Btask $B$,%^!<%8$7$F>C5n(B
 */



/*
  task $B$N=hM}40N;8e$O!$$=$N(B task_home $B$K(B send_rslt $B$9$k(B
  ?? task_home $B$r$=$N$^$^F1$8%N!<%I$G=hM}$9$k%1!<%9$b$"$k$N$+(B??

  task_home $B$N=i4|2=8e$O(B req $B85(B $B$K(B send_task $B$9$k$N$+(B?
 */
/*
    treq <task_head> <treq_head>
      <task_head>  $B%?%9%/JVEz@h(B
      <treq_head>  $BMW5aAw?.@h(B

    task <ndiv> <rslt_head> <task_head>
      <ndiv>       $BJ,3d2s?t(B $BIi2Y$N%5%$%:$NL\0B(B (sp2$B$,;R$NH=CG$K;H$&(B)
      <rslt_head>  $B7k2LJVEz@h(B
      <task_head>  $B%?%9%/Aw?.@h(B

    rslt <rack_head> <rslt_head>
      <rack_head>  rack$BJVEz@h(B  ; <task_head> $B$G$h$$(B
      <rslt_head>  $B7k2LAw?.@h(B

    rack <rack_head>
      <rack_head>  rack$BAw?.@h(B

    treq $B$NAw$j@h$,FCDj$N(Btask $B$G$"$k$J$i!$$=$N(B task $B$,$9$G$K$J$/(B
    $B$J$C$F$$$?$i!$(Bnone $B$r$+$($9$H$G$-$k$N$G!$(Brack $B$OITMW(B?
    => No, task $B$N(B id $B$,$b$7F1$8$J$i6hJL$G$-$J$$(B...
    rack $B$OI,MW$@$,!$(Brack $B$,$b$I$C$F$/$k$^$G!$$=$N(B id $B$N(B task 
    $B$r>C$5$J$1$l$P!$(Bid $B$,F1$8$K$J$C$F$7$^$&$3$H$O$J$$!%(B
    -> $B7k6I$O(B w_rack $B%+%&%s%?$r;H$&$Y$-(B
 */

/*
  (st.head.nextt = _x->head.subt),
  (st.head.sid = _x->head.sid + 1),
  (_x->head.subt = &st),
 */

void recv_exec_send(struct thread_data *thr, char *treq_head){
  struct cmd cmd;
  struct task *x;
  int bk(){ return 0; }
  /* $B:G=i$K(Bx$B$r$H$k(B

     $B%?%9%/$r$H$m$&$H$7$F$$$k4V$K!$(Brslt $B$,FO$$$?$i9)IW$9$k$N$+(B?
     $BC1$K!$(Brslt $BBT$A$K$D$$$F$O!$(Bwait $B$7$J$$$G!$Dj4|E*$K$_$k!%(B
     $B$_$?$H$-$K!$$J$1$l$P!$(Btask $BBT$A$K$7$F$7$^$&$H$$$&$N$,4JC1(B?
     $B$@$C$F(B task $BBT$A$K$7$?$i(B treq $B$r$@$9$N$@$7!%(B
     $B$3$3$G$O!$(Btask $BBT$A$N$9$k>l9g(B

     task $B$K$D$$$F$O!$(Btreq $B;~E@$G>l=j$r:n$C$F$*$/(B => $B%N!<%IFbJBNs$K$b(B

     $BF1;~%9%l%C%I?t$OD6$($J$$$h$&$K$O!$JLES%;%^%U%)$G(B
  */
  pthread_mutex_lock(&thr->mut);
  x = thr->task_top = thr->task_top->prev;
  x->stat = TASK_ALLOCATED;
  if(!x)
    mem_error("not enough task memory");
  rcmd.c = 3;
  rcmd.v[0] = "treq";
  rcmd.v[1] = thr->id_str;
  rcmd.v[2] = treq_head;
  pthread_mutex_lock(&send_mut);
  send_command(rcmd);
  flush_send();
  pthread_mutex_unlock(&send_mut);
  /* recv_task $B$G=i4|2=$5$l$k$N$rBT$D(B */
  do{
    pthread_cond_wait(&thr->cond, &thr->mut);
  } while(x->stat != TASK_INITIALIZED);
  x->stat = TASK_STARTED;
  pthread_mutex_unlock(&thr->mut);

  /* $B$3$3$K(B $B%;%^%U%)(B? */

  do_work(bk, x);
  /* rslt */

  /* $B0J2<$G(B thr $B$N(B lock $B$,$$$k$+(B? */
  rcmd.c = 3;
  rcmd.v[0] = "rslt";
  rcmd.v[1] = thr->id_str;
  rcmd.v[2] = x->rslt_head;
  pthread_mutex_lock(&send_mut);
  send_command(rcmd);

  send_rslt_body(x->body);
  /* x->body.h.class->send_rslt_body(x->body); */

  write_eol();
  flush_send();
  pthread_mutex_unlock(&send_mut);
}

void *worker(void *arg){
  for(;;)
    recv_exec_send(arg, "any");
}

unsinged int num_thrs = 1;

void recv_task(struct cmd cmd){
  struct task *tx;
  struct thread_data *thr;
  unsinged int id;
  size_t len;
  if(cmd.c < 4)
    proto_error("wrong task");
  /* id $B$r(B <task_head> $B$K4^$a$k(B */
  id = atoi(cmd.v[3]);
  if(! id<num_thrs)
    proto_error("wrong task_head");
  thr=thread_data+id;
  pthread_mutex_lock(&thr->mut);
  tx = thr->task_top;
  tx->req = 0;
  len = strlen(cmd.v[2])
  if(len > 254)
    proto_error("too long rslt_head for task");
  strncpy(tx->rslt_head, cmd.v[2], len+1);
  tx->ndiv = atoi(cmd.v[1]);
  tx->body = recv_task_body();
  read_to_eol();
  /* task $B$r<u$1<h$C$?8e!$%N!<%IFb$KBT$C$F$$$k%9%l%C%I(B($B%o!<%+(B)$B$r5/$3$9(B */
  tx->stat = TASK_INITIALIZED;
  /* $B%9%l%C%I?t$N>e8B$rD6$($k$J$i!$8e$G(B($B$I$l$+$N%9%l%C%I$,=*$o$k$H$-(B)
     signal $B$9$k$Y$-$+(B?
     $B$=$l$h$j$O!$JL$K%;%^%U%)$G>e8B$r4IM}$9$k$[$&$,$i3Z$G$O(B?  */
  pthread_cond_broadcast(&thr->cond); /* pthread_cond_signal? */
  pthread_mutex_unlock(&thr->mut);
}

/* $B0J2<$O$^$@!$(Btask $B$+$i(B thread $B$X$NJQ99$,=*$o$C$F$$$J$$(B */
void recv_rslt(struct cmd cmd){
  struct cmd rcmd;
  struct task *tx;
  struct task_home *hx;
  int tid, sid;
  char *b;
  if(cmd.c < 3)
    proto_error("wrong rslt");
  b = cmd.v[2]
  tid = atoi(b);
  b = strchr(b, ':');
  if(!b)
    proto_error("wrong rslt_head");
  sid = atoi(b);
  pthread_mutex_lock(&task_mut);
  tx = task_root;
  pthread_mutex_unlock(&task_mut);
  /* id $B$r(B <task_head> $B$K4^$a$k(B */
  while(tx && tx->id != tid)
    tx = tx->next;
  if(!tx)
    proto_error("wrong rslt_head");
  hx = tx->sub;
  while(hx && hx->id != sid)
    hx = hx->next;
  if(!hx)
    proto_error("wrong rslt_head");
  recv_rslt_body(hx->body);
  read_to_eol();
  /* mut $B$O(B task_mut $B$K$7$F$b$h$$(B? */
  pthread_mutex_lock(&hx->mut);
  hx->stat = TASK_HOME_DONE;
  pthread_mutex_unlock(&hx->mut);
  /* rack $B$rJV$9(B $B$b$C$H8e$N$[$&$,$h$$(B? */
  rcmd.c = 2;
  rcmd.v[0] = "rack"; rcmd.v[1] = cmd.v[1];
  pthread_mutex_lock(&send_mut);
  send_command(rcmd);
  flush_send();
  pthread_mutex_unlock(&send_mut);
}

void recv_treq(struct cmd cmd){
  struct task *tx;
  int id;
  /*
    task id $B$,;XDj$5$l$F$$$k>l9g$H!$(Bany $B$N>l9g$,$"$k!%(B
    any $B$N>l9g$O!$(Bany $B$G$H$C$F$-$?$b$N$N$[$&$,Bg$-$$$N$G$O(B?
    => $B$=$b$=$b!$$9$Y$F$N(B task $B$,F0$$$F$$$k$o$1$G$O$J$$!%(B
       $B$9$/$J$/$H$b!$(Brslt $B$^$A$G!$JL(B task $B$rN)$A>e$2$?(B task $B$K(B
       req $B$7$F$b$@$a!%(B
    * thread $BC10L$G(B req $B@h$r$b$D$J$i$=$l$O$=$l$G(B OK
    * task $BC10L$G(B req $B@h$r$b$D$J$i!$(B
      + task $B$N(B req $B@h$r8+$D$1$i$l$k$+$H$$$&LdBj(B
      + $B:#F0$$$F$$$k$b$N$K(B req $B$9$Y$-(B
        regulation ($B%;%^%U%)(B)$B$G!$Dd;_Cf$N(B task $B$K(B req $B$7$F$b$b$i$($J$$$+$b(B
         -> any $B$@$C$?$iBP>]$+$i$O$:$9!%(B
            $B$^$?$O!$(B $BFCDj$N(Btask $B$J$i!$0l;~E*$K(B regu $B$r$O$:$9(B?
            regu $B$7$J$$$J$i$7$J$$$G!$$=$l$O!$?J$^$J$$$b$N$,=P$k!%(B
            $B0lEY!$(Bregu$B$r$O$:$7$F$b!$(Btask_send$B8e$K(B regu $B$r3NG'(B
         -> $B$A$g$C$HM>J,$K(B thread $B$r;H$C$F$b!$%?%9%/J,3d$,$=$NJ,(B
            $B?J$_$,0-$/$J$k$J$i!$7k6I!$CY1d1#JC$K$J$i$J$$2DG=@-$,9b$$!%(B
            $BCY1d1#JC$N$?$a$K$O!$(Bthread $B$,L2$C$F$$$F$b(B task $B$,J,3d2DG=(B
            $B$@$H$h$$!%?2$kA0$KJ,3d$7$F$*$/$H$+(B...
            -> $B%9%l%C%I?t(B1$B$G@h$KJ,3d$9$k$N$O$"$^$j0UL#$O$J$$$+$b!%(B
                 => $B$b$0$i$J$$8z2L$O$"$k(B?
               $B$=$l$K@h$KJ,3d$9$k$0$i$$$J$i!$%9%l%C%I?t$rG\$G$h$$(B?
    */


}

main(int argc, char *argv[]){
  struct cmd cmd;

  /* $B$3$3$G(B thread_data $B$N=i4|2=(B, task $B$N(B $BAPJ}8~(Blist $B$b(B */

  /* $B$3$3$G%o!<%+$r(Bfork 
     $BF1$8%N!<%IFb$G(B($B6&M-%a%b%j$G(B)$B$G$b$d$j$/$j$G$-$k$+(B?  */
  systhr_create(worker, thread_data+0);
  
  while(cmd = recv_command(), (cmd.c > 0 && strcmp(cmd.v[0],"exit") != 0))
    if(strcmp(cmd.v[0],"task") == 0)
      recv_task(cmd);
    else if(strcmp(cmd.v[0],"rslt") == 0)
      recv_rslt(cmd);
    else if(strcmp(cmd.v[0],"treq") == 0)
      recv_treq(cmd);
    else if(strcmp(cmd.v[0],"none") == 0)
      recv_none(cmd);
    else if(strcmp(cmd.v[0],"rack") == 0)
      recv_rack(cmd);
    else {
      proto_error("wrong cmd");
    }
  exit(0);
}

/* $B6&DL(B? */

void send_int(int n){ printf(" %d", n); }
void recv_int(int *n){ scanf(" %ld", n); }

#define TASK_COMMON  struct { int req; int task_type; struct task *t; } h; 

struct task_common {
  TASK_COMMON
};

/* $B$3$3$+$i2<$@$1$r(B app $B$G$O=q$/(B */

/* 
  (def (task task-fib)
    (def n int)
    (def r int)
    (def (send-task) n) ?
    (def (send-rslt) r) ?
    (def (recv-rslt) r) ?

  (def (task task-fib)
    (def n int :in)
    (def r int :out))
 */

struct task_fib {
  TASK_COMMON
  int n;
  int r;
};

void *recv_task_body(){
  struct task_fib *x = (struct task_fib *) malloc(sizeof(struct task_fib));
  recv_int(&x->n);
  return x;
}

void send_rslt_body(void *x0){
  struct task_fib *x = x0;
  send_int(x->r);
}

void recv_rslt_body(void *x0){
  struct task_fib *x = x0;
  recv_int(&x->r);
}

void send_task_body(void *x0){
  struct task_fib *x = x0;
  send_int(x->n);
}

/*
  (def-WF (fib n) (fn int int) ....)
  (def (fib n) (w-fn int int) ....)

  (def (fib _bk _x n) (fn int (ptr (fn int)) (prt (struct task)) int) ....)
 */

int fib(FF, int n){
  if(n <= 2){
    return 1;
  }else{
    int s1, s2;
    DO_TWO(struct task_fib, st,
           (s1 = fib(FA, n-1)),
	   (s2 = fib(FA, n-2)),
	   (st.n = n-2),
	   (s2 = st.r));
    return s1+s2;
  }
}

void do_work(FF)
{
  int n = TASK_BODY(struct task_fib).n;
  int r = fib(FA, n);
  TASK_BODY(struct task_fib).r = r;
}
