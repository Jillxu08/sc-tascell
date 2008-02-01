#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#define BUFSIZE 1024
#define MAXCMDC 4

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

void mem_error(const char *str){
  fputs(str, stderr); fputc('\n', stderr);
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
    if(c == ' ' || c == '\n')
      *b = c = 0;
    else if (p == 0 && r.c < MAXCMDC)
      r.v[r.c++] = b;
  return r;
}

pthread_mutex_t send_mut = PTHREAD_MUTEX_INITIALIZER;

/* struc cmd -> output */
void send_command(struct cmd cmd){
  int i;
  for(i=0; i<cmd.c-1; i++){
    fputs(cmd.v[i], stdout); fputc(' ', stdout);
  }
  if(cmd.c > 0){
    fputs(cmd.v[cmd.c-1], stdout); fputc('\n', stdout);
  }
}

void proto_error(const char *str, struct cmd cmd){
  int i;
  fputs(str, stderr); fputc('\n', stderr);
  for(i=0; i<cmd.c-1; i++){
    fputs(cmd.v[i], stderr); fputc(' ', stderr);
  }
  if(cmd.c > 0){
    fputs(cmd.v[cmd.c-1], stderr); fputc('\n', stderr);
  }
  exit(1);
}

void read_to_eol(){
  int c;
  while((c = getc(stdin)) != EOF) if (c == '\n') break;
}
void write_eol(){ putc('\n', stdout); }
void flush_send(){ fflush(stdout); }

struct task;
struct thread_data;

void do_task_body(struct thread_data *, void *);
void send_task_body(struct thread_data *, void *);
void *recv_task_body(struct thread_data *);
void send_rslt_body(struct thread_data *, void *);
void recv_rslt_body(struct thread_data *, void *);

enum task_stat {
  TASK_ALLOCATED, TASK_INITIALIZED, TASK_STARTED, TASK_DONE,
  TASK_NONE, TASK_SUSPENDED
};

enum task_home_stat {
  TASK_HOME_ALLOCATED, TASK_HOME_INITIALIZED,
  /* $B7k2LBT$A$O(B? => task $B$N$[$&$G$o$+$k(B? */
  TASK_HOME_DONE
};

struct task {
  enum task_stat stat;
  struct task* next;
  struct task* prev;
  void *body;
  int ndiv;
  char rslt_head[256];
};

struct task_home {
  enum task_home_stat stat;
  int id;
  struct task_home* next;
  void *body;
  char task_head[256];
};

struct thread_data {
  struct task_home * volatile req;
  int id;
  int w_rack;
  int w_none;
  int ndiv;
  struct task *task_free;
  struct task *task_top;
  struct task_home* treq_free;
  struct task_home* treq_top;
  struct task_home* sub;
  pthread_mutex_t mut;
  pthread_cond_t cond;
  pthread_cond_t cond_r;
  char id_str[32];
  char buf[BUFSIZE];
};

struct thread_data thread_data[64];

/*
  $B%j%b!<%H$K(B treq $B$7$F$b$i$C$?(B task (copy)
    - treq $B$^$($K(B allocate
    x $B$H$-$I$-!$%j%9%H$+$i(B DONE $B$H$J$C$?$b$N$r=|$/(B
      ($B$=$l$J$i!$(Brack $B$rF@$k$^$G(B DONE $B$K$7$J$$$h$&$K$9$k(B
       active $B$J%9%l%C%I?t$OJL4IM}$9$l$P$h$$(B?)
    x  $B$H$-$I$-$G$J$/$F!$(Brack $B$rF@$?$H$-$G$h$$(B?
    - rslt $B$rAw$C$?$i<+J,$G>C$($k!%(B

  $BJ,3d$7$F:n$C$?(B task (home/orig) => task_home
    - thread_data $B$N(B sub $B$+$i$N%j%9%H$H$J$j!$(Bid $B$O=EJ#$7$J$$$h$&$KIU$1$k!%(B
    x $B:G=i$+$i!$(BSTARTED$B!$$9$0$K(B treq $B85$KAw$k!%(B
    o treq $B$N;~E@$G(B ALLOCATED $B$K$7$F$O(B?
    - rslt $B$,$-$?$i!$(BDONE $B$K$7$F!$(Brack $B$rJV$9(B
    - DONE $B$K$J$C$F$$$?$i!$J,3d85(Btask $B$,%^!<%8$7$F>C5n(B

  ?? task_home $B$r$=$N$^$^F1$8%N!<%I$G=hM}$9$k%1!<%9$b$"$k$N$+(B??
 */
/*
    treq <task_head> <treq_head>
      <task_head>  $B%?%9%/JVEz@h(B
      <treq_head>  $BMW5aAw?.@h(B

    task <ndiv> <rslt_head> <task_head>
      <ndiv>       $BJ,3d2s?t(B $BIi2Y$N%5%$%:$NL\0B(B (sp2$B$,;R$NH=CG$K;H$&(B)
      <rslt_head>  $B7k2LJVEz@h(B
      <task_head>  $B%?%9%/Aw?.@h(B

    rslt <rslt_head>
      <rslt_head>  $B7k2LAw?.@h(B

    rack <task_head>
      <task_head>  rack$BAw?.@h(B
      (w_rack $B%+%&%s%?$r;H$&$Y$-(B)

    none <task_head>
      <task_head>  (no)$B%?%9%/Aw?.@h(B
 */
/*
   [ prev  ] -> [ prev  ] -> [ prev  ]  -> 
<- [ next  ] <- [ next  ] <- [ next  ] <- 
 */

void flush_treq_with_none_1(struct thread_data *thr){
  /* thr->mut $B$H(B send_mut $B$O(B lock $B:Q(B */
  struct cmd rcmd;
  struct task_home *hx;
  while(hx = thr->treq_top){
    rcmd.c = 2;
    rcmd.v[0] = "none";
    rcmd.v[1] = hx->task_head;
    send_command(rcmd);
    thr->treq_top = hx->next;
    hx->next = thr->treq_free;
    thr->treq_free = hx;
  }
}

void flush_treq_with_none(struct thread_data *thr){
  /* treq $B$,$?$^$C$F$$$?$i!$(Bnone $B$rAw$k!%(B
     thr->mut $B$O(B lock $B:Q(B */
  if(thr->treq_top){
    pthread_mutex_lock(&send_mut);
    flush_treq_with_none_1(thr);
    flush_send();
    pthread_mutex_unlock(&send_mut);
  }
}

void recv_exec_send(struct thread_data *thr, char *treq_head){
  /* thr->mut $B$O(B lock $B:Q(B */
  struct cmd rcmd;
  struct task *tx;
  long delay;
  int old_ndiv;
  /* $B:G=i$K(Bx$B$r$H$k(B

     $B%?%9%/$r$H$m$&$H$7$F$$$k4V$K!$(Brslt $B$,FO$$$?$i9)IW$9$k$N$+(B?
     $BC1$K!$(Brslt $BBT$A$K$D$$$F$O!$(Bwait $B$7$J$$$G!$Dj4|E*$K$_$k!%(B
     $B$_$?$H$-$K!$$J$1$l$P!$(Btask $BBT$A$K$7$F$7$^$&$H$$$&$N$,4JC1(B?
     $B$@$C$F(B task $BBT$A$K$7$?$i(B treq $B$r$@$9$N$@$7!%(B
     $B$3$3$G$O!$(Btask $BBT$A$N$9$k>l9g(B

     task $B$K$D$$$F$O!$(Btreq $B;~E@$G>l=j$r:n$C$F$*$/(B => $B%N!<%IFbJBNs$K$b(B

     $BF1;~%9%l%C%I?t$OD6$($J$$$h$&$K$O!$JLES%;%^%U%)$G(B
  */

  /* $B$"$H$G$H$I$/(B none $B$,FO$/$^$GBT$D(B */
  while(thr->w_none > 0) {
    pthread_cond_wait(&thr->cond, &thr->mut);
    /* rslt $B$,E~Ce$7$F$$$?$i(B */
    if(thr->sub && thr->sub->stat == TASK_HOME_DONE)
      return;
  }

  /* allocate */
  tx = thr->task_free;
  if(!tx)
    mem_error("not enough task memory");
  thr->task_top = tx;
  thr->task_free = tx->prev;

  delay = 2 * 1000 * 1000;
  do{

    /* $B:G=i$K(B treq $B$,$?$^$C$F$$$?$i!$(Bnone $B$rAw$k(B */
    flush_treq_with_none(thr);

    tx->stat = TASK_ALLOCATED;
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

      /* rslt $B$,E~Ce$7$F$$$?$i(B $BFCJL$K@h$K$5$;$k(B */
      if(tx->stat != TASK_INITIALIZED
	 && thr->sub && thr->sub->stat == TASK_HOME_DONE){
	if(tx->stat == TASK_NONE)
	  goto Lnone;
	thr->w_none++;
	goto Lnone;
      }
    } while(tx->stat == TASK_ALLOCATED);

    if(tx->stat == TASK_NONE){
      struct timespec t1;
      struct timeval now;
      long nsec;
      gettimeofday(&now, 0);
      nsec = now.tv_usec * 1000;
      /* nsec += 10 * 1000 * 1000; */
      nsec += delay;
      delay += delay;
      if(delay > 40 * 1000 * 1000)
	delay = 40 * 1000 * 1000;
      t1.tv_nsec = (nsec > 999999999) ? nsec - 999999999 : nsec;
      t1.tv_sec = now.tv_sec + ((nsec > 999999999) ? 1 : 0);
      pthread_cond_timedwait(&thr->cond_r, &thr->mut, &t1);

      if(thr->sub && thr->sub->stat == TASK_HOME_DONE)
	goto Lnone;
    }
  }while(tx->stat != TASK_INITIALIZED);

  /*
    none, task, rslt $B$rF1;~$KBT$F$J$$$+(B?
    rack $B$r;H$&8B$j$G$O!$(Btreq $BCf$K(B rslt $B$,FO$$$?>l9g$O$=$N8e(B
    $B$+$J$i$:(B none $B$,La$C$F$/$k!%(B
    none $B$rBT$?$:$K!$(Bw_none $B$r(B inc $B$7$F!$(Bnone $B$,FO$$$?$H$-$K$O(B
    "struct task $B$N=q$-9~$_(B/signal"$B$;$:$K(B dec $B$@$1$9$l$P$h$5$=$&!%(B

    rack $B$r;H$o$J$$$J$i!$(Brslt $B$N8e!$(Btask $B$,FO$/$+$b!%(B
    $B8=:_=hM}Cf$N(B task $B$,(B task_top $B$G$J$/$F$b$h$$$J$iLdBj$J$$!%(B
    $B$=$N$?$a$K$O!$(Btask_top $B$G$J$$$H$-$KJ,$+$k$h$&$K$9$k$H$H$b$K(B
    treq $B;~$K(B thread$BHV9f$K!$(Btask$BHV9f$r2C$($kI,MW$,$"$k$H;W$&!%(B
   */

  /* TASK_INITIALIZED */
  tx->stat = TASK_STARTED;
  old_ndiv = thr->ndiv;
  thr->ndiv = tx->ndiv;

  /* $B$3$3$K(B $B%;%^%U%)(B? */

  pthread_mutex_unlock(&thr->mut);
  do_task_body(thr, tx->body);
  pthread_mutex_lock(&thr->mut);

  /* task $B$N=hM}40N;8e$O!$$=$N(B task_home $B$K(B send_rslt $B$9$k(B */
  rcmd.c = 2;
  rcmd.v[0] = "rslt";
  rcmd.v[1] = tx->rslt_head;
  pthread_mutex_lock(&send_mut);
  send_command(rcmd);
  /* body $B$G$O$J$/!$(Bdo_task_body $B$N(B return value $B$K$9$k$N$O$I$&$+(B? */
  send_rslt_body(thr, tx->body);
  /* $B$^$?$O!$(Bx->body.h.class->send_rslt_body(x->body); */
  write_eol();
  /* $B:G8e$K$b(B treq $B$,$?$^$C$F$$$?$i!$(Bnone $B$rAw$k(B */
  flush_treq_with_none_1(thr);
  flush_send();
  pthread_mutex_unlock(&send_mut);
  thr->w_rack++;
  thr->ndiv = old_ndiv;
 Lnone:
  thr->task_free = tx;
  thr->task_top = tx->next;
}

void *worker(void *arg){
  struct thread_data *thr = arg;
  pthread_mutex_lock(&thr->mut);
  for(;;)
    recv_exec_send(thr, "any");
  pthread_mutex_unlock(&thr->mut);
}

unsigned int num_thrs = 1;

void recv_task(struct cmd cmd){
  struct task *tx;
  struct thread_data *thr;
  unsigned int id;
  size_t len;
  if(cmd.c < 4)
    proto_error("wrong task", cmd);
  /* id $B$r(B <task_head> $B$K4^$a$k(B */
  id = atoi(cmd.v[3]);
  if(! (id<num_thrs))
    proto_error("wrong task_head", cmd);
  thr = thread_data+id;
  pthread_mutex_lock(&thr->mut);
  tx = thr->task_top;
  len = strlen(cmd.v[2]);
  if(len > 254)
    proto_error("too long rslt_head for task", cmd);
  strncpy(tx->rslt_head, cmd.v[2], len+1);
  tx->ndiv = atoi(cmd.v[1]);
  tx->body = recv_task_body(thr);
  read_to_eol();
  /* task $B$r<u$1<h$C$?8e!$%N!<%IFb$KBT$C$F$$$k%9%l%C%I(B($B%o!<%+(B)$B$r5/$3$9(B */
  tx->stat = TASK_INITIALIZED;
  /* $B%9%l%C%I?t$N>e8B$rD6$($k$J$i!$8e$G(B($B$I$l$+$N%9%l%C%I$,=*$o$k$H$-(B)
     signal $B$9$k$Y$-$+(B?
     $B$=$l$h$j$O!$JL$K%;%^%U%)$G>e8B$r4IM}$9$k$[$&$,4JC1(B  */
  pthread_cond_broadcast(&thr->cond); /* pthread_cond_signal? */
  pthread_mutex_unlock(&thr->mut);
}

void recv_none(struct cmd cmd){
  struct thread_data *thr;
  unsigned int id;
  size_t len;
  if(cmd.c < 2)
    proto_error("wrong none", cmd);
  /* id $B$r(B <task_head> $B$K4^$a$k(B */
  id = atoi(cmd.v[1]);
  if(! (id<num_thrs))
    proto_error("wrong task_head", cmd);
  thr = thread_data+id;
  pthread_mutex_lock(&thr->mut);
  if(thr->w_none > 0)
    thr->w_none--;
  else
    thr->task_top->stat = TASK_NONE;
  pthread_cond_broadcast(&thr->cond); /* pthread_cond_signal? */
  pthread_mutex_unlock(&thr->mut);
}

/*
  rack $B$K$D$$$F(B
    rslt $B$K$O(B <rack_head> $B$O$J$$(B => <task_head> $B$r;H$&(B
    treq $BCf$J$i!$(Brack $B$rJV$5$J$$$H$7$F$$$k$,(B? (sp00b)
      rack $B$rJV$7$?$H$-$K$O!$%o!<%+$K7k2L$r<h$j9~$^$;$h$&$H;n$_$k(B

    treq $BCf$K(B rslt $B$r$b$i$C$?$i!$$9$0$K(B rack $B$rJV$5$:!$(B
    $B$=$N(B treq $B$OI,$:(B none $B$5$l$k$O$:$@$+$i!$(Bnone $B$KBP1~$7$F(Brack $B$rJV$9!%(B
    $B$J$<(B?  sp.memo $B$K=q$$$?(B SP: $B0J9_(B $B$N$h$&$K!$(BFIFO$B@-$,$J$$$H!$(Btreq $B$r(B
           rack $B$,DI$$1[$7$F$7$^$&$3$H$,$"$k$+$i!%(B
    => $B$H$j$"$($:$O!$(BFIFO$B@-$r2>Dj$7$F$=$N$^$^(B
 */
void recv_rslt(struct cmd cmd){
  struct cmd rcmd;
  struct thread_data *thr;
  struct task_home *hx;
  unsigned int tid, sid;
  char *b;
  if(cmd.c < 2)
    proto_error("wrong rslt", cmd);
  b = cmd.v[1];
  tid = atoi(b);
  b = strchr(b, ':');
  if(!b)
    proto_error("wrong rslt_head", cmd);
  sid = atoi(b+1);
  thr = thread_data+tid;
  pthread_mutex_lock(&thr->mut);
  /*
  hx = thr->sub;
  while(hx){
    fprintf(stderr,
	    "hx %d %d %s (%d,%d)\n", hx, hx->id, hx->task_head, tid, sid);
    hx = hx->next;
  }
  */
  hx = thr->sub;
  while(hx && hx->id != sid)
    hx = hx->next;
  if(!hx)
    proto_error("wrong rslt_head", cmd);
  recv_rslt_body(thr, hx->body);
  read_to_eol();
  /* rack $B$rJV$9(B $B$b$C$H8e$N$[$&$,$h$$(B? */
  rcmd.c = 2;
  rcmd.v[0] = "rack"; rcmd.v[1] = hx->task_head;
  pthread_mutex_lock(&send_mut);
  send_command(rcmd);
  flush_send();
  pthread_mutex_unlock(&send_mut);
  /* hx $BCf$K5-O?$5$l$?(B task_head $B$K(B rack $B$r8e$GAw$k$J$i!$(B
     $B$3$3$G$O$J$$$,!$$^$@(B free $B$5$l$?$/$J$$$N$G!$$D$J$.$J$*$9$+$b(B  */
  hx->stat = TASK_HOME_DONE;
#if 1
  if(hx == thr->sub){
    pthread_cond_broadcast(&thr->cond_r);
    pthread_cond_broadcast(&thr->cond);
  }
#endif
  pthread_mutex_unlock(&thr->mut);
}

/*
  worker $B$,$7$P$i$/<u$1<h$C$F$/$l$J$/$F$b!$(B
  treq $B$O<u$1<h$C$F$*$/$[$&$,$h$5$=$&!%(B
  $BJ#?t$N(B treq $B$r%j%9%H$K$7$F<u$1<h$i$;$F$b$h$$$N$G$O(B?
  treq $B$O(B task_home $B$N7A$K$7$F$*$$$F$b$h$$$+$b!%(B
  task_home $B$O(B $B%9%?%C%/%"%m%1!<%H$N$D$b$j$@$C$?$,(B...
 */

int try_treq(struct cmd cmd, unsigned int id){
  struct task_home *hx;
  struct thread_data *thr;
  size_t len;
  int avail = 0;


  thr = thread_data+id;
  pthread_mutex_lock(&thr->mut);
  if(thr->task_top && thr->task_top->stat == TASK_STARTED && thr->w_rack == 0)
    avail = 1;

  /*
  fprintf(stderr, "try_treq id:%d, task_top:%ld stat=%d(%d) wrack:%d \n", 
	  id, thr->task_top, thr->task_top->stat, TASK_STARTED, thr->w_rack);
  */

  if(avail){
    hx = thr->treq_free;
    if(!hx)
      mem_error("not enough task_home memory");
    thr->treq_free = hx->next;
    hx->next = thr->treq_top;
    hx->stat = TASK_HOME_ALLOCATED;
    len = strlen(cmd.v[1]);
    if(len > 254)
      proto_error("too long task_head for treq", cmd);
    strncpy(hx->task_head, cmd.v[1], len+1);
    thr->treq_top = hx;
    thr->req = hx;
  }
  pthread_mutex_unlock(&thr->mut);
  return avail;
}

/*
  treq $B$r$?$a$F$*$/$+LdBj(B
    $B<+J,$b(B treq $BCf$N$H$-$O!$(B
      none $B$rJV$9$5$J$$$H%G%C%I%m%C%/$N4m81!%(B
      $B=g=x$,$D$1$i$l$F%k!<%W$7$J$1$l$P!$$^$?$;$F$b$h$$!%(B
    $B<+J,$OF0$$$F$$$k$,EO$9$b$N$,$J$$$H$-(B
      none $B$rJV$9$H2?EY$b$$$C$F$/$k$+$b!%(B
      none $B$rJV$5$J$$$H!$B>$K$$$1$P$h$$$N$KBT$D$3$H$K$J$k$+$b!%(B
      $B0lEY(B none $B$G5qH]$5$l$?$i3P$($F$*$-!$(Btreq $B<u$1IU$1$^$9$H(B
      $B$$$&@kEA$,$/$k$^$G$O$=$N%N!<%I$K$O(B treq $B$7$J$$$H$+$O(B...
 */

void recv_treq(struct cmd cmd){
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
  struct cmd rcmd;
  unsigned int id;
  if(cmd.c < 3)
    proto_error("wrong treq", cmd);
  /* id $B$r(B <task_head> $B$K4^$a$k(B */
  if(strcmp(cmd.v[2], "any") == 0){
    for(id=0;id<num_thrs;id++)
      if(try_treq(cmd, id)) break;
    if(id != num_thrs)
      return;
  }else{
    id = atoi(cmd.v[2]);
    if(! (id<num_thrs))
      proto_error("wrong task_head", cmd);
    if(try_treq(cmd, id))
      return;
  }
  /* treq $B$G$-$J$+$C$?>l9g(B */

  /* none $B$rJV$9(B */
  rcmd.c = 2;
  rcmd.v[0] = "none"; rcmd.v[1] = cmd.v[1];
  pthread_mutex_lock(&send_mut);
  send_command(rcmd);
  flush_send();
  pthread_mutex_unlock(&send_mut);
}

void recv_rack(struct cmd cmd){
  struct task *tx;
  struct thread_data *thr;
  unsigned int id;
  size_t len;
  if(cmd.c < 2)
    proto_error("wrong rack", cmd);
  /* id $B$r(B <task_head> $B$K4^$a$k(B */
  id = atoi(cmd.v[1]);
  if(! (id<num_thrs))
    proto_error("wrong task_head", cmd);
  thr = thread_data+id;
  pthread_mutex_lock(&thr->mut);
  thr->w_rack--;
  pthread_mutex_unlock(&thr->mut);
}

main(int argc, char *argv[]){
  int i, j;
  struct cmd cmd;

  /* $B$3$3$G(B thread_data $B$N=i4|2=(B, task $B$N(B $BAPJ}8~(Blist $B$b(B */
  num_thrs = 1;
  for(i=0; i<num_thrs; i++){
    struct thread_data *thr = thread_data+i;
    struct task *tx;
    struct task_home *hx;
    thr->id = i;
    thr->w_rack = 0;
    thr->w_none = 0;
    thr->ndiv = 0;
    pthread_mutex_init(&thr->mut, 0);
    pthread_cond_init(&thr->cond, 0);
    pthread_cond_init(&thr->cond_r, 0);
    sprintf(thr->id_str, "%d", i);

    tx = (struct task *)malloc(sizeof(struct task) * 512);
    thr->task_top = 0;
    thr->task_free = tx;
    for(j=0; j<511; j++){
      tx[j].prev = &tx[j+1];
      tx[j+1].next = &tx[j];
    }
    tx[0].next = 0;
    tx[511].prev = 0;
    
    hx = (struct task_home *)malloc(sizeof(struct task_home) * 512);
    thr->treq_free = hx;
    thr->treq_top = 0;
    thr->sub = 0;
    for(j=0; j<511; j++)
      hx[j].next = &hx[j+1];
    hx[511].next = 0;

    /* $B$3$3$G%o!<%+$r(Bfork 
     $BF1$8%N!<%IFb$G(B($B6&M-%a%b%j$G(B)$B$G$b$d$j$/$j$G$-$k$+(B?  */
    systhr_create(worker, thr);
  }

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
      proto_error("wrong cmd", cmd);
    }
  exit(0);
}

#define FF int (*_bk)(), struct thread_data *_thr
#define FA _bk, _thr

#define DO_TWO(type1, struct1, work1, work2, put_task1, get_rslt1)	\
do{									\
  type1 struct1[1];							\
  int spawned = 0;							\
  {									\
    int (*_bk2)() = _bk;						\
    int _bk(){								\
      if(spawned)							\
	return 0;							\
      _bk2();								\
      if(_thr->treq_top){						\
	(put_task1);							\
	spawned = 1;							\
	make_and_send_task(_thr, struct1);				\
	return 1;							\
      }									\
      return 0;								\
    }									\
    if(_thr->req)							\
      handle_req(FA);							\
    (work1);								\
  }									\
  if(spawned){								\
    wait_rslt(_thr);							\
    (get_rslt1);							\
  } else {								\
    (work2);								\
  }									\
}while(0)

#define DO_MANY_BEGIN(type1, struct1,			\
inum1, o_inum1, o_inum2, n_inum1, n_inum2)		\
do{							\
  int inum1 = (o_inum1);				\
  int einum1 = (o_inum2);				\
  type1 *struct1;					\
  int spawned = 0;					\
  int (*_bk2)() = _bk;					\
  int _bk(){						\
    if(!spawned)					\
      _bk2();						\
    while(inum1 + 1 < einum1 && _thr->treq_top){	\
      int n_inum2 = einum1;				\
      int n_inum1 = ((inum1+1)+n_inum2)/2;		\
      einum1 = n_inum1;					\
      struct1 = (type1 *)malloc(sizeof (type1));

#define DO_MANY_BODY(struct1, inum1)		\
      spawned++;				\
      make_and_send_task(_thr, struct1);	\
    }						\
    return 0;					\
  }						\
  if(_thr->req)					\
    handle_req(FA);				\
  for (; inum1 < einum1; inum1++)

#define DO_MANY_FINISH(struct1)			\
  while(spawned-- > 0){				\
    struct1 = wait_rslt(_thr);

#define DO_MANY_END(struct1)			\
    free(struct1);				\
  }						\
}while(0)

#define DO_INI_FIN(init1, body1, fin1)		\
do{						\
  { init1 ; }					\
  {						\
    int (*_bk2)() = _bk;			\
    int _bk(){					\
      { fin1 ; }				\
      _bk2();					\
      { init1 ; }				\
      return 0;					\
    }						\
    { body1 ; }					\
  }						\
  { fin1 ; }					\
}while(0)

void handle_req(FF){
  pthread_mutex_lock(&_thr->mut);
  if(_thr->req){
    _bk();
    /* $B$3$3$G(B $B;D$C$F$$$?$i!$(Bsend none $B$9$kBe$o$j$K!$(B
       STARTED $B$G$J$/$J$C$?$i(B none $B$9$k<j$b!%(B
       _thr->req  != 0 $B$N$^$^$K$9$k(B */
    _thr->req = _thr->treq_top;
  }
  pthread_mutex_unlock(&_thr->mut);
}

void make_and_send_task(struct thread_data *thr, void *body){
  struct cmd tcmd;
  struct task_home *hx=thr->treq_top;
  thr->treq_top = hx->next;
  hx->next = thr->sub;
  thr->sub = hx;
  hx->body = body;
  hx->id = hx->next ? hx->next->id + 1 : 0;
  hx->stat = TASK_HOME_INITIALIZED;
  tcmd.c = 3;
  tcmd.v[0] = "task";
  /* $B%:%k$J$N$GCm0U(B */
  sprintf(thr->buf, "%d %s:%d", ++thr->ndiv, thr->id_str, hx->id);
  tcmd.v[1] = thr->buf;
  tcmd.v[2] = hx->task_head;
  pthread_mutex_lock(&send_mut);
  send_command(tcmd);
  send_task_body(thr, body);
  write_eol();
  flush_send();
  pthread_mutex_unlock(&send_mut);
}

void *wait_rslt(struct thread_data *thr){
  void *body;
  struct timespec t1;
  struct timeval now;
  long nsec;
  struct task_home *sub;
  pthread_mutex_lock(&thr->mut);
  sub = thr->sub;
  while(sub->stat != TASK_HOME_DONE){
#if 1
#if 0
    flush_treq_with_none(thr);
    thr->task_top->stat = TASK_SUSPENDED;
#endif
    gettimeofday(&now, 0);
    nsec = now.tv_usec * 1000;
    nsec += 5 * 000 * 000;
    /* nsec += 10 * 1000 * 1000; */
    t1.tv_nsec = (nsec > 999999999) ? nsec - 999999999 : nsec;
    t1.tv_sec = now.tv_sec + ((nsec > 999999999) ? 1 : 0);
    pthread_cond_timedwait(&thr->cond_r, &thr->mut, &t1);
#if 0
    thr->task_top->stat = TASK_STARTED;
#endif
    if(sub->stat == TASK_HOME_DONE)
      break;
#endif
    /* fprintf(stderr, "sub %d\n", sub); */

    recv_exec_send(thr, sub->task_head);
  }
  body = sub->body;
  thr->sub = sub->next;
  sub->next = thr->treq_free;
  thr->treq_free = sub;
  /* fprintf(stderr, "nsub %d\n", thr->sub); */
  pthread_mutex_unlock(&thr->mut);
  return body;
}




/* $B6&DL(B? */

void send_int(int n){ fprintf(stdout, " %d", n); }
void recv_int(int *n){ fscanf(stdin, " %ld", n); }

/* $B$3$3$+$i>e$r%i%$%V%i%j$K$9$k(B */


/* $B$3$3$+$i2<$@$1$r(B app $B$G$O=q$/(B */

struct task_pen {
  int r; 
  int k;
  int i0, i1, i2;
  int a[12];
  int b[70];
};

void send_task_body(struct thread_data *thr, void *x0){
  struct task_pen *x = x0;
  int i;
  send_int(x->k);
  if(x->k < 0) return;
  send_int(x->i0);
  send_int(x->i1);
  send_int(x->i2);
  for(i=0;i < 12; i++)
    send_int(x->a[i]);
  for(i=0;i < 70; i++)
    send_int(x->b[i]);
}

void *recv_task_body(struct thread_data *thr){
  int i;
  struct task_pen *x = (struct task_pen *) malloc(sizeof(struct task_pen));
  recv_int(&x->k);
  if(x->k < 0) return x;
  recv_int(&x->i0);
  recv_int(&x->i1);
  recv_int(&x->i2);
  for(i=0;i < 12; i++)
    recv_int(&x->a[i]);
  for(i=0;i < 70; i++)
    recv_int(&x->b[i]);
  return x;
}

void send_rslt_body(struct thread_data *thr, void *x0){
  struct task_pen *x = x0;
  send_int(x->r);
  free(x);
}

void recv_rslt_body(struct thread_data *thr, void *x0){
  struct task_pen *x = x0;
  int r;
  recv_int(&x->r);
}

int ps[63][5] = {
  {1,1,1,1},{7,7,7,7},
  {7,1,1,1},{4,1,1,1},{1,1,1,4},{1,1,1,7},
  {7,7,7,1},{7,7,6,1},{1,6,7,7},{1,7,7,7},
  {6,1,1,1},{5,1,1,1},{1,1,1,5},{1,1,1,6},
  {7,1,6,7},{7,7,1,6},{7,6,1,7},{6,1,7,7},
  {7,7,1,1},{1,1,5,7},{1,1,7,7},{7,5,1,1},
  {2,5,1,1},{1,1,5,2},{1,6,7,1},{1,7,6,1},
  {7,1,1,5},{5,1,1,7},{7,6,1,1},{1,1,6,7},
  {7,1,1,6},{5,1,1,6},{6,1,1,5},{6,1,1,7},
  {1,7,1,6},{7,1,5,1},{1,5,1,7},{6,1,7,1},
  {7,1,1,7},{5,1,1,5},{1,7,7,1},{1,6,6,1},
  {6,1,1,6},
  {1,6,1,1},{1,1,5,1},{1,5,1,1},{1,1,6,1},
  {7,1,6,1},{1,6,1,6},{6,1,6,1},{1,6,1,7},
  {1,4,1,1},{1,1,7,1},{1,7,1,1},{1,1,4,1},
  {7,1,7,7},{7,6,1,6},{6,1,6,7},{7,7,1,7},
  {7,1,7,1},{1,5,1,6},{1,7,1,7},{6,1,5,1}
};

int pos[13] = { 0, 2, 10, 18, 22, 26, 30, 38, 42, 43, 51, 59, 63};

int 
search(FF, int k, int j0, int j1, int j2, struct task_pen *tsk)
{
  int s = 0;
  DO_MANY_BEGIN(struct task_pen, st, p, j1, j2, i1, i2){
    *st = *tsk;
    st->k = k;
    st->i0 = j0;
    st->i1 = i1;
    st->i2 = i2;
  }
  DO_MANY_BODY(st, p){
    int ap = tsk->a[p], i;
    for(i = pos[ap]; i < pos[ap+1]; i++)
      /* examine the "i"-th (piece, direction)
	 at the first empty location "k" */
      {
	int *pss = ps[i];
	int kk=k,l;
	/* room available? */
	for(l=0;l<4;l++) if((kk += pss[l]) >= 70 || tsk->b[kk] !=0) goto Ln;

	DO_INI_FIN({
	  tsk->b[kk=k] = p+'A'; for(l=0;l<4;l++) tsk->b[kk += pss[l]] = p+'A';
	  tsk->a[p] = tsk->a[j0]; tsk->a[j0] = ap;
	},{ 
	  for(kk=k; kk<70; kk++) if( tsk->b[kk] == 0 ) break;
	  if(kk == 70)
	    s += 1;
	  else if((kk+7 >= 70 || tsk->b[kk+7] != 0) && 
		  (tsk->b[kk+1] != 0 || 
		   (kk+8 >= 70 || tsk->b[kk+8] != 0) && tsk->b[kk+2] != 0))
	    ;
	  else
	    s += search(FA, kk, j0 + 1, j0 + 1, 12, tsk);
	},{
	  ap = tsk->a[j0]; tsk->a[j0] = tsk->a[p]; tsk->a[p] = ap; 
	  tsk->b[kk=k] = 0; for(l=0;l<4;l++) tsk->b[kk += pss[l]] = 0;
	});
      Ln:
	continue;
      }
  }
  DO_MANY_FINISH(st){
    s += st->r;
  }
  DO_MANY_END(st);
  return s;
}

void do_task_body(struct thread_data *_thr, void *x0){
  int _bk(){ return 0; }
  struct task_pen *x = x0;
  int i;
  int k = x->k;
  int i0 = x->i0;
  int i1 = x->i1;
  int i2 = x->i2;
  fprintf(stderr, "start %d %d %d %d\n", k, i0, i1, i2);
  if(k < 0){
    for(i=0;i<12;i++) x->a[i] = i % 12;
    for(i=0;i<70;i++) x->b[i] = 0;
    for(i=6;i<70;i+=7) x->b[i] = '\n'; /* right side wall */
    x->r = search(FA, 0, 0, 0, 12, x);  
  }else{
    x->r = search(FA, k, i0, i1, i2, x);
    fprintf(stderr, "end %d\n", x->r);
  }
}
