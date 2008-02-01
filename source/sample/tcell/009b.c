#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#define BUFSIZE 1024
#define MAXCMDC 4

// #define TEST_PART
// #define TEST_MSG
// #define TEST_MSG_SEND
// #define TEST_MSG_LOCK_THREAD
// #define TEST_MSG_LOCK_SEND
// #define TEST_MSG_LOCK_QUEUE
// #define TEST_MSG_ALLOC
// #define PRINT_TIME

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

enum node {OUTSIDE, INSIDE, ANY};

struct cmd {
  int  c;
  enum node node;
  char *v[MAXCMDC];
};

struct cmd_list {
  struct cmd cmd;
  void *body;
  struct cmd_list *next;
};

struct cmd_list cmd_queue[512];
char cmd_v_buf[512][4][256];
struct cmd_list *cmd_in;
struct cmd_list *cmd_out;

char buf[BUFSIZE];

int divisibility_flag = 0;

/* input -> struc cmd */
struct cmd recv_command(){
  struct cmd r;
  char p, c, *b = buf;
  fgets(b, BUFSIZE, stdin);
  r.c = 0;
  r.node = OUTSIDE;
  for(p = 0, c = *b ; c ; p = c, c= *++b)
    if(c == ' ' || c == '\n')
      *b = c = 0;
    else if (p == 0 && r.c < MAXCMDC)
      r.v[r.c++] = b;
  return r;
}

void recv_rslt(struct cmd, void *);
void recv_task(struct cmd, void *);
void recv_treq(struct cmd);
void recv_rack(struct cmd);
void recv_none(struct cmd);

pthread_mutex_t send_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_mut = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond_q = PTHREAD_COND_INITIALIZER;

/* struc cmd -> output */
void send_command(struct cmd cmd){
  int i;
#ifdef TEST_MSG_SEND
      fprintf(stderr, "send %s\n", cmd.v[0]);
#endif
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

void enqueue_command(struct cmd cmd, void *body){
  int i;
  size_t len;
  struct cmd_list *q;

#ifdef TEST_MSG
  fprintf(stderr, "node=%d v[0]=%s enqueue_cmd\n",cmd.node, cmd.v[0]);  
#endif
  /* cmd_queue $B$O(Block$B:Q(B */
  q = cmd_in;
  if (cmd_in->next == cmd_out){
    perror("cmd_queue overflow\n");
    exit(0);
  }
  cmd_in = cmd_in->next;
  (*q).cmd.c = cmd.c;
  (*q).cmd.node = cmd.node;
  for(i=0; i<cmd.c; i++){
    len = strlen(cmd.v[i]);
    if(len > 254)
      proto_error("too long cmd", cmd);
    strncpy(q->cmd.v[i], cmd.v[i], len+1);
  }
  q->body = body;
}

void *exec_queue_cmd(void *arg){
  struct cmd cmd;
  void *body;
  for(;;){
    pthread_mutex_lock(&queue_mut);
#ifdef TEST_MSG_LOCK_QUEUE
    fprintf(stderr, "lock queue\n");
#endif
    while (cmd_in == cmd_out){
#ifdef TEST_MSG_LOCK_QUEUE
      fprintf(stderr, "unlock wait queue\n");
#endif
      pthread_cond_wait(&cond_q, &queue_mut);
#ifdef TEST_MSG_LOCK_QUEUE
      fprintf(stderr, "lock wait queue\n");
#endif
    }
    cmd = (*cmd_out).cmd;
    body = cmd_out->body;
#ifdef TEST_MSG_LOCK_QUEUE
      fprintf(stderr, "unlock queue\n");
#endif
    pthread_mutex_unlock(&queue_mut);
#ifdef TEST_MSG
    fprintf(stderr, "node=%d v[0]=%s exec_queue_cmd\n",cmd.node, cmd.v[0]);
#endif
    if(strcmp(cmd.v[0],"task") == 0)
      recv_task(cmd, body);
    else if(strcmp(cmd.v[0],"rslt") == 0)
      recv_rslt(cmd, body);
    else if(strcmp(cmd.v[0],"treq") == 0)
      recv_treq(cmd);
    else if(strcmp(cmd.v[0],"none") == 0)
      recv_none(cmd);
    else if(strcmp(cmd.v[0],"rack") == 0)
      recv_rack(cmd);
    else {
      proto_error("wrong cmd", cmd);
      exit(0);
    }
    cmd_out = cmd_out->next;
  }
}

void read_to_eol(){
  int c;
  while((c = getc(stdin)) != EOF) if (c == '\n') break;
}
void write_eol(){ putc('\n', stdout); }

void flush_send(){
#ifdef TEST_MSG_SEND
  fprintf(stderr, "flush send\n");
#endif
  fflush(stdout);
}

pthread_mutex_t snr_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_snr = PTHREAD_COND_INITIALIZER;
struct cmd_list snr_queue[32];
char snr_v_buf[32][2][256];
struct cmd_list *snr_in;
struct cmd_list *snr_out;
/* snr: send none or rack */

void enqueue_snr(struct cmd cmd){
  int i;
  size_t len;
  struct cmd_list *q;

  q = snr_in;
  if (snr_in->next == snr_out){
    perror("snr_queue overflow\n");
    exit(0);
  }
  snr_in = snr_in->next;
  (*q).cmd.c = cmd.c;
  (*q).cmd.node = cmd.node;
  for(i=0; i<cmd.c; i++){
    len = strlen(cmd.v[i]);
    if(len > 254)
      proto_error("too long cmd", cmd);
    strncpy(q->cmd.v[i], cmd.v[i], len+1);
  }
}

void *send_none_rack(void *arg){
  struct cmd cmd;
  for(;;){
    pthread_mutex_lock(&snr_mut);
    while (snr_in == snr_out){
      pthread_cond_wait(&cond_snr, &snr_mut);
    }
    cmd = (*snr_out).cmd;
    pthread_mutex_unlock(&snr_mut);
    pthread_mutex_lock(&send_mut);
    send_command(cmd);
    flush_send();
    if (strcmp(cmd.v[0],"none") == 0) divisibility_flag = 1;
    pthread_mutex_unlock(&send_mut);
    snr_out = snr_out->next;
  }
}
    
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
  enum node rslt_to;
  char rslt_head[256];
};

struct task_home {
  enum task_home_stat stat;
  int id;
  enum node req_from;
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
  pthread_mutex_t rack_mut;
  pthread_cond_t cond;
  pthread_cond_t cond_r;
  char ndiv_buf[32];
  char id_str[32];
  char buf[BUFSIZE];
};

struct thread_data thread_data[64];
unsigned int num_thrs = 1;

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
    rcmd.node = hx->req_from;
    rcmd.v[0] = "none";
    rcmd.v[1] = hx->task_head;
    if (rcmd.node == INSIDE){
      pthread_mutex_lock(&queue_mut);
#ifdef TEST_MSG_LOCK_QUEUE
      fprintf(stderr, "lock queue id=%d\n", thr->id);
#endif
      enqueue_command(rcmd, NULL);
#ifdef TEST_MSG_LOCK_QUEUE
      fprintf(stderr, "signal queue id=%d\n", thr->id);
      fprintf(stderr, "unlock queue id=%d\n", thr->id);
#endif
      pthread_cond_signal(&cond_q);
      pthread_mutex_unlock(&queue_mut);
    }
    else if(rcmd.node == OUTSIDE){
      send_command(rcmd);
      divisibility_flag = 1;
    }
    else{
      perror("invalid rcmd.node in flush_treq_with_none_1\n");
      fprintf(stderr, "%d\n", rcmd.node);
      exit(0);
    }
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
#ifdef TEST_MSG_LOCK_SEND
    fprintf(stderr, "lock send id=%d\n", thr->id);
#endif
    flush_treq_with_none_1(thr);
    flush_send();
#ifdef TEST_MSG_LOCK_SEND
    fprintf(stderr, "unlock send id=%d\n", thr->id);
#endif
    pthread_mutex_unlock(&send_mut);
  }
}

void recv_exec_send(struct thread_data *thr, char *treq_head, int req_to){
  /* thr->mut $B$O(B lock $B:Q(B */
  struct cmd rcmd;
  struct task *tx;
  long delay;
  int old_ndiv;
#ifdef PRINT_TIME
  struct timeval tp[2];
#endif

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
#ifdef TEST_MSG_LOCK_THREAD
    fprintf(stderr, "unlock wait none in read_exec_send id=%d\n", thr->id);
#endif
    pthread_cond_wait(&thr->cond, &thr->mut);
#ifdef TEST_MSG_LOCK_THREAD
    fprintf(stderr, "lock after wait none in read_exec_send id=%d\n", thr->id);
#endif
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
#ifdef TEST_MSG_LOCK_THREAD
    fprintf(stderr, "unlock id=%d in recv_exec_send\n", thr->id);
#endif
    pthread_mutex_unlock(&thr->mut);
    rcmd.c = 3;
    if (num_thrs >1)
      rcmd.node = req_to;
    else
      rcmd.node = OUTSIDE;
    rcmd.v[0] = "treq";
    rcmd.v[1] = thr->id_str;
    rcmd.v[2] = treq_head;
    
    if (rcmd.node != OUTSIDE){
      pthread_mutex_lock(&queue_mut);
#ifdef TEST_MSG_LOCK_QUEUE
      fprintf(stderr, "lock queue id=%d\n", thr->id);
#endif
      enqueue_command(rcmd, NULL);
#ifdef TEST_MSG_LOCK_QUEUE
      fprintf(stderr, "signal queue id=%d\n", thr->id);
      fprintf(stderr, "unlock queue id=%d\n", thr->id);
#endif      
      pthread_cond_signal(&cond_q);
      pthread_mutex_unlock(&queue_mut);
    }
    else{
      pthread_mutex_lock(&send_mut);
#ifdef TEST_MSG_LOCK_SEND
      fprintf(stderr, "lock send id=%d\n", thr->id);
#endif
      /* $B"-$N(Bsend_command$B$G=PNO$,5M$^$k(B($B%P%C%U%!$,$$$C$Q$$$K$J$k(B)$B$H(B
	 $B%G%C%I%m%C%/$N2DG=@-$,$"$k(B?
	 $B$?$@$7=PNOJ8;z?t$O>/$J$$$N$G5M$^$k3NN($ODc$$(B */
      send_command(rcmd);
      flush_send();
      divisibility_flag = 1;
#ifdef TEST_MSG_LOCK_SEND
      fprintf(stderr, "unlock send id=%d\n", thr->id);
#endif
      pthread_mutex_unlock(&send_mut);
    }
    pthread_mutex_lock(&thr->mut);
#ifdef TEST_MSG_LOCK_THREAD
    fprintf(stderr, "lock id=%d in recv_exec_send\n",thr->id);
#endif  
    /* recv_task $B$G=i4|2=$5$l$k$N$rBT$D(B */
    do{
      /* rslt $B$,E~Ce$7$F$$$?$i(B $BFCJL$K@h$K$5$;$k(B */
      if(tx->stat != TASK_INITIALIZED && thr->sub && thr->sub->stat == TASK_HOME_DONE){
	if(tx->stat == TASK_NONE)
	  goto Lnone;
	thr->w_none++;
	goto Lnone;
      }
      if (tx->stat != TASK_ALLOCATED) break;
#ifdef TEST_MSG_LOCK_THREAD
      fprintf(stderr, "unlock wait task init id=%d\n", thr->id);
#endif
      pthread_cond_wait(&thr->cond, &thr->mut);
#ifdef TEST_MSG_LOCK_THREAD
      fprintf(stderr, "lock after wait task init id=%d\n", thr->id);
#endif
    } while(1);

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
#ifdef TEST_MSG_LOCK_THREAD
      fprintf(stderr, "unlock wait id=%d\n", thr->id);
#endif
      pthread_cond_timedwait(&thr->cond_r, &thr->mut, &t1);
#ifdef TEST_MSG_LOCK_THREAD
      fprintf(stderr, "lock id=%d\n", thr->id);
#endif
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
#ifdef TEST_MSG
  fprintf(stderr, "id=%d TASK_STARTED\n",thr->id);
#endif
  /* $B$3$3$K(B $B%;%^%U%)(B? */
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "unlock id=%d before do_task_body\n",thr->id);
#endif
  pthread_mutex_unlock(&thr->mut);
#ifdef PRINT_TIME
  if (tx->ndiv == 0)
    gettimeofday(tp, 0);
#endif
  do_task_body(thr, tx->body);
#ifdef PRINT_TIME
  if (tx->ndiv == 0){
    gettimeofday(tp+1, 0);
    fprintf(stderr, "%lf\n", tp[1].tv_sec-tp[0].tv_sec+1e-6*(tp[1].tv_usec-tp[0].tv_usec));
  }
#endif
  pthread_mutex_lock(&thr->mut);
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "lock id=%d after do_task_body\n",thr->id);
#endif

  /* task $B$N=hM}40N;8e$O!$$=$N(B task_home $B$K(B send_rslt $B$9$k(B */
  rcmd.c = 2;
  rcmd.node = tx->rslt_to;
  rcmd.v[0] = "rslt";
  rcmd.v[1] = tx->rslt_head;
  if(rcmd.node == INSIDE){
    pthread_mutex_lock(&queue_mut);
#ifdef TEST_MSG_LOCK_QUEUE
    fprintf(stderr, "lock queue id=%d\n", thr->id);
#endif
    enqueue_command(rcmd, tx->body);
#ifdef TEST_MSG_LOCK_QUEUE
    fprintf(stderr, "signal queue id=%d\n", thr->id);
    fprintf(stderr, "unlock queue id=%d\n", thr->id);
#endif   
    pthread_cond_signal(&cond_q); 
    pthread_mutex_unlock(&queue_mut);
    pthread_mutex_lock(&send_mut); /* flush_treq $B$N$?$a(B */
#ifdef TEST_MSG_LOCK_SEND
    fprintf(stderr, "lock send id=%d\n", thr->id);
#endif
  }
  else if(rcmd.node == OUTSIDE){
    pthread_mutex_lock(&send_mut);
#ifdef TEST_MSG_LOCK_SEND
    fprintf(stderr, "lock send id=%d\n", thr->id);
#endif
#ifdef TEST_MSG_LOCK_THREAD
    fprintf(stderr, "unlock before send id=%d\n", thr->id);
#endif
    pthread_mutex_unlock(&thr->mut);
    send_command(rcmd);
    /* body $B$G$O$J$/!$(Bdo_task_body $B$N(B return value $B$K$9$k$N$O$I$&$+(B? */
#ifdef TEST_MSG_SEND
    fprintf(stderr, "id=%d send rslt body\n", thr->id);
#endif
    send_rslt_body(thr, tx->body);
    /* $B$^$?$O!$(Bx->body.h.class->send_rslt_body(x->body); */
    write_eol();
    pthread_mutex_lock(&thr->mut);
#ifdef TEST_MSG_LOCK_THREAD
    fprintf(stderr, "lock after send id=%d\n", thr->id);
#endif
  }
  else{
    perror("invalid rcmd.node in recv_exec_send\n");
    fprintf(stderr, "%d\n", rcmd.node);
    exit(0);
  }
  /* $B:G8e$K$b(B treq $B$,$?$^$C$F$$$?$i!$(Bnone $B$rAw$k(B */
  flush_treq_with_none_1(thr);
  flush_send();
#ifdef TEST_MSG_LOCK_SEND
  fprintf(stderr, "unlock send id=%d\n", thr->id);
#endif
  pthread_mutex_unlock(&send_mut);
  pthread_mutex_lock(&thr->rack_mut);
  thr->w_rack++;
  pthread_mutex_unlock(&thr->rack_mut);
  thr->ndiv = old_ndiv;
 Lnone:
  thr->task_free = tx;
  thr->task_top = tx->next;
}

void *worker(void *arg){
  struct thread_data *thr = arg;
  pthread_mutex_lock(&thr->mut);
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "lock id=%d start worker\n",thr->id);
#endif
  for(;;)
    recv_exec_send(thr, "any", ANY);
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "unlock id=%d finish worker\n",thr->id);
#endif
  pthread_mutex_unlock(&thr->mut);
}



void recv_task(struct cmd cmd, void *body){
  struct task *tx;
  struct thread_data *thr;
  unsigned int id;
  size_t len;
  if(cmd.c < 4)
    proto_error("wrong task", cmd);
  /* id $B$r(B <task_head> $B$K4^$a$k(B */
  id = atoi(cmd.v[3]);
#ifdef TEST_MSG
  fprintf(stderr, "id=%d recv_task\n",id);
#endif  
  if(! (id<num_thrs))
    proto_error("wrong task_head", cmd);
  thr = thread_data+id;
  pthread_mutex_lock(&thr->mut);
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "lock id=%d recv_task\n",thr->id);
#endif
  tx = thr->task_top;
  tx->rslt_to = cmd.node;
  len = strlen(cmd.v[2]);
  if(len > 254)
    proto_error("too long rslt_head for task", cmd);
  strncpy(tx->rslt_head, cmd.v[2], len+1);
  tx->ndiv = atoi(cmd.v[1]);
  if (cmd.node == INSIDE){
    tx->body = body;
  }
  else if(cmd.node == OUTSIDE){
    tx->body =
      recv_task_body(thr); /* $BFI$_=P$7$O%m%C%/3MF@$NA0$N$[$&$,$$$$$+$b(B */
    read_to_eol();
  }
  else {
    perror("invalid cmd.node in recv_task\n");
    fprintf(stderr, "%d\n", cmd.node);
    exit(0);
  }
  /* task $B$r<u$1<h$C$?8e!$%N!<%IFb$KBT$C$F$$$k%9%l%C%I(B($B%o!<%+(B)$B$r5/$3$9(B */

  tx->stat = TASK_INITIALIZED; /* $B$=$b$=$b%m%C%/$,I,MW$J$N$O$3$3$@$1$+$b(B */
  /* treq$B$7$F$+$i(Btask$B$r<u$1<h$k$^$G$N4V$O(Bstat$B$7$+%"%/%;%9$5$l$J$$5$$,$9$k(B */
  
  /* $B%9%l%C%I?t$N>e8B$rD6$($k$J$i!$8e$G(B($B$I$l$+$N%9%l%C%I$,=*$o$k$H$-(B)
     signal $B$9$k$Y$-$+(B?
     $B$=$l$h$j$O!$JL$K%;%^%U%)$G>e8B$r4IM}$9$k$[$&$,4JC1(B  */
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "pthread_cond_broadcast id=%d\n",thr->id);
  fprintf(stderr, "unlock id=%d recv_task fin\n",thr->id);
#endif
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
#ifdef TEST_MSG
  fprintf(stderr, "id=%d recv_none\n",thr->id);
#endif
  pthread_mutex_lock(&thr->mut);
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "lock id=%d recv_none\n",thr->id);
#endif
  if(thr->w_none > 0)
    thr->w_none--;
  else
    thr->task_top->stat = TASK_NONE;
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "pthread_cond_broadcast id=%d\n",thr->id);
  fprintf(stderr, "unlock id=%d recv_none fin\n",thr->id);
#endif
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
void recv_rslt(struct cmd cmd, void *rbody){
  struct cmd rcmd;
  struct thread_data *thr;
  struct task_home *hx;
  unsigned int tid, sid;
  char *b;
  char h_buf[256];

  if(cmd.c < 2)
    proto_error("wrong rslt", cmd);
  b = cmd.v[1];
  tid = atoi(b);
  b = strchr(b, ':');
  if(!b)
    proto_error("wrong rslt_head", cmd);
  sid = atoi(b+1);
  thr = thread_data+tid;
#ifdef TEST_MSG
  fprintf(stderr, "id=%d recv_rslt\n",thr->id);
#endif
  pthread_mutex_lock(&thr->mut);
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "lock id=%d recv_rslt\n",thr->id);
#endif
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
  if(cmd.node == INSIDE){
    hx->body = rbody;
  }
  else if (cmd.node == OUTSIDE){
    recv_rslt_body(thr, hx->body);
    read_to_eol();
  }
  else {
    perror("invalid cmd.node in recv_rslt\n");
    fprintf(stderr, "%d\n", cmd.node);
    exit(0);
  }
  /* rack $B$rJV$9(B $B$b$C$H8e$N$[$&$,$h$$(B? */
  rcmd.c = 2;
  rcmd.node = cmd.node;
  rcmd.v[0] = "rack";
  strncpy(h_buf, hx->task_head, strlen(hx->task_head)+1);
  rcmd.v[1] = h_buf;
  /* hx $BCf$K5-O?$5$l$?(B task_head $B$K(B rack $B$r8e$GAw$k$J$i!$(B
     $B$3$3$G$O$J$$$,!$$^$@(B free $B$5$l$?$/$J$$$N$G!$$D$J$.$J$*$9$+$b(B  */
  hx->stat = TASK_HOME_DONE;
  if(hx == thr->sub){
#ifdef TEST_MSG_LOCK_THREAD
    fprintf(stderr, "pthread_cond_broadcast id=%d\n",thr->id);
#endif
    pthread_cond_broadcast(&thr->cond_r);
    pthread_cond_broadcast(&thr->cond);
  }
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "unlock id=%d recv_rslt fin\n",thr->id);
#endif
  pthread_mutex_unlock(&thr->mut);
  if (cmd.node){
    pthread_mutex_lock(&queue_mut);
#ifdef TEST_MSG_LOCK_QUEUE
    fprintf(stderr, "lock queue in recv_rslt\n");
#endif
    enqueue_command(rcmd, NULL);
#ifdef TEST_MSG_LOCK_QUEUE
    fprintf(stderr, "signal queue in recv_rslt\n");
    fprintf(stderr, "unlock queue in recv_rslt\n");
#endif    
    pthread_cond_signal(&cond_q);
    pthread_mutex_unlock(&queue_mut);
  }
  else{
#if 0
    pthread_mutex_lock(&send_mut);
#ifdef TEST_MSG_LOCK_SEND
    fprintf(stderr, "lock send in recv_rslt\n");
#endif
    send_command(rcmd);
    flush_send();
#ifdef TEST_MSG_LOCK_SEND
    fprintf(stderr, "unlock send in recv_rslt\n", thr->id);
#endif
    pthread_mutex_unlock(&send_mut);
#endif
    /* $B%G%C%I%m%C%/KI;_$N$?$aFI$_=P$7$r$9$k%9%l%C%I$G=q$-9~$_$r$7$J$$$h$&$K$7$F$$$k(B */
    pthread_mutex_lock(&snr_mut);
    enqueue_snr(rcmd);
    pthread_cond_signal(&cond_snr);
    pthread_mutex_unlock(&snr_mut);
  }
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
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "lock id=%d try_treq\n",thr->id);
#endif
  pthread_mutex_lock(&thr->rack_mut);
  if(thr->task_top &&
     (thr->task_top->stat == TASK_STARTED || thr->task_top->stat == TASK_INITIALIZED)
     && thr->w_rack == 0)
    avail = 1;
  pthread_mutex_unlock(&thr->rack_mut);
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
    if (cmd.node != OUTSIDE)
      hx->req_from = INSIDE;
    else
      hx->req_from = OUTSIDE;
    thr->treq_top = hx;
    thr->req = hx;
  }
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "unlock id=%d try_treq fin\n",thr->id);
#endif
  pthread_mutex_unlock(&thr->mut);
#ifdef TEST_MSG
  fprintf(stderr, "id=%d try_treq %d\n",id,avail);
#endif
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
#ifdef TEST_MSG
  fprintf(stderr, "recv_treq\n");
#endif
  if(cmd.c < 3)
    proto_error("wrong treq", cmd);
  /* id $B$r(B <task_head> $B$K4^$a$k(B */
  if(strcmp(cmd.v[2], "any") == 0){
    unsigned int myid = atoi(cmd.v[1]);
    for(id=0;id<num_thrs;id++){
      if(cmd.node != OUTSIDE && (id == myid)) continue;
      if(try_treq(cmd, id)) break;
    }
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

  if (cmd.node == ANY){
    if (atoi(cmd.v[1]) == 0){
      pthread_mutex_lock(&send_mut);
#ifdef TEST_MSG_LOCK_SEND
      fprintf(stderr, "lock send in recv_treq\n");
#endif
      send_command(cmd);
      flush_send();
      divisibility_flag = 1;
#ifdef TEST_MSG_LOCK_SEND
      fprintf(stderr, "unlock send in recv_treq\n");
#endif
      pthread_mutex_unlock(&send_mut);
      return;
    }
    else{
      cmd.node = INSIDE;
    }
  }
  
  /* none $B$rJV$9(B */
  rcmd.c = 2;
  rcmd.node = cmd.node;
  rcmd.v[0] = "none"; rcmd.v[1] = cmd.v[1];
  if(rcmd.node == INSIDE){
    pthread_mutex_lock(&queue_mut);
#ifdef TEST_MSG_LOCK_QUEUE
    fprintf(stderr, "lock queue in recv_treq\n");
#endif
    enqueue_command(rcmd, NULL);
#ifdef TEST_MSG_LOCK_QUEUE
    fprintf(stderr, "signal queue in recv_treq\n");
    fprintf(stderr, "unlock queue in recv_treq\n");
#endif    
    pthread_cond_signal(&cond_q);
    pthread_mutex_unlock(&queue_mut);
  }
  else if (rcmd.node == OUTSIDE){
#if 0
    pthread_mutex_lock(&send_mut);
#ifdef TEST_MSG_LOCK_SEND
    fprintf(stderr, "lock send in recv_treq\n");
#endif
    send_command(rcmd);
    flush_send();
    divisibility_flag = 1;
#ifdef TEST_MSG_LOCK_SEND
    fprintf(stderr, "unlock send in recv_treq\n");
#endif
    pthread_mutex_unlock(&send_mut);
#endif
    /* $B%G%C%I%m%C%/KI;_$N$?$aFI$_=P$7$r$9$k%9%l%C%I$G=q$-9~$_$r$7$J$$$h$&$K$7$F$$$k(B */
    pthread_mutex_lock(&snr_mut);
    enqueue_snr(rcmd);
    pthread_cond_signal(&cond_snr);
    pthread_mutex_unlock(&snr_mut);
  }
  else{
    perror("invalid rcmd.node in recv_treq\n");
    fprintf(stderr, "%d\n", rcmd.node);
    exit(0);
  }
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
#ifdef TEST_MSG
  fprintf(stderr, "id=%d recv_rack\n",id);
#endif
  if(! (id<num_thrs))
    proto_error("wrong task_head", cmd);
  thr = thread_data+id;
  pthread_mutex_lock(&thr->rack_mut);
  thr->w_rack--;
  pthread_mutex_unlock(&thr->rack_mut);
}

main(int argc, char *argv[]){
  int i, j;
  void *dummy;
  struct cmd cmd;

  /* cmd_queue $B$N=i4|2=(B */
  for(i=0; i<511; i++){
    cmd_queue[i].next = &cmd_queue[i+1];
    for(j=0; j<4; j++)
      cmd_queue[i].cmd.v[j] = cmd_v_buf[i][j];
  }
  cmd_queue[511].next = &cmd_queue[0];
  for(j=0; j<4; j++)
    cmd_queue[511].cmd.v[j] = cmd_v_buf[511][j];
  cmd_in = &cmd_queue[0];
  cmd_out = &cmd_queue[0];

  systhr_create(exec_queue_cmd, NULL);

  /* snr_queue $B$N=i4|2=(B */
  for(i=0; i<31; i++){
    snr_queue[i].next = &snr_queue[i+1];
    for(j=0; j<2; j++)
      snr_queue[i].cmd.v[j] = snr_v_buf[i][j];
  }
  snr_queue[31].next = &snr_queue[0];
  for(j=0; j<2; j++)
    snr_queue[31].cmd.v[j] = snr_v_buf[31][j];
  snr_in = &snr_queue[0];
  snr_out = &snr_queue[0];

  systhr_create(send_none_rack, NULL);
  
  if (argc > 1){
    num_thrs = atoi(argv[1]);
  }
  /* $B$3$3$G(B thread_data $B$N=i4|2=(B, task $B$N(B $BAPJ}8~(Blist $B$b(B */
  /* num_thrs = 1;*/
  for(i=0; i<num_thrs; i++){
    struct thread_data *thr = thread_data+i;
    struct task *tx;
    struct task_home *hx;
    thr->id = i;
    thr->w_rack = 0;
    thr->w_none = 0;
    thr->ndiv = 0;
    pthread_mutex_init(&thr->mut, 0);
    pthread_mutex_init(&thr->rack_mut, 0);
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
    if(strcmp(cmd.v[0],"task") == 0){
      recv_task(cmd, dummy);
    }
    else if(strcmp(cmd.v[0],"rslt") == 0){
      recv_rslt(cmd, dummy);
    }
    else if(strcmp(cmd.v[0],"treq") == 0){
      recv_treq(cmd);
    }
    else if(strcmp(cmd.v[0],"none") == 0){
      recv_none(cmd);
    }
    else if(strcmp(cmd.v[0],"rack") == 0){
      recv_rack(cmd);
      divisibility_flag = 1;
    }
    else {
      proto_error("wrong cmd", cmd);
    }
  exit(0);
}

void send_divisible(){
  struct cmd cmd;
  if (pthread_mutex_trylock(&send_mut)) return;
  divisibility_flag = 0;
  cmd.c = 1;
  cmd.v[0] = "dvbl";
  send_command(cmd);
  flush_send();
  pthread_mutex_unlock(&send_mut);
}
  

#define FF int (*_bk)(), struct thread_data *_thr
#define FA _bk, _thr

#define DO_TWO(type1, struct1, work1, work2, put_task1, get_rslt1)	\
do{									\
  type1 struct1[1];							\
  int spawned = 0;							\
  if (divisibility_flag == 1) send_divisible();                         \
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
  if (divisibility_flag == 1) send_divisible(); \
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
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "lock id=%d hundle_req\n",_thr->id);
#endif
#ifdef TEST_MSG
  fprintf(stderr, "id=%d hundle_req\n",_thr->id);
#endif
  if(_thr->req){
    _bk();
    /* $B$3$3$G(B $B;D$C$F$$$?$i!$(Bsend none $B$9$kBe$o$j$K!$(B
       STARTED $B$G$J$/$J$C$?$i(B none $B$9$k<j$b!%(B
       _thr->req  != 0 $B$N$^$^$K$9$k(B */
    _thr->req = _thr->treq_top;
  }
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "unlock id=%d hundle_req fin\n",_thr->id);
#endif
  pthread_mutex_unlock(&_thr->mut);
}

void make_and_send_task(struct thread_data *thr, void *body){
  struct cmd tcmd;
  struct task_home *hx=thr->treq_top;
#ifdef TEST_MSG
  fprintf(stderr, "id=%d make_and_send_task\n",thr->id);
#endif
  thr->treq_top = hx->next;
  hx->next = thr->sub;
  thr->sub = hx;
  hx->body = body;
  hx->id = hx->next ? hx->next->id + 1 : 0;
  hx->stat = TASK_HOME_INITIALIZED;
  tcmd.c = 4;
  tcmd.node = hx->req_from;
  tcmd.v[0] = "task";
  /* $B%:%k$J$N$GCm0U(B */
  sprintf(thr->ndiv_buf, "%d", ++thr->ndiv);
  sprintf(thr->buf, "%s:%d", thr->id_str, hx->id);
  tcmd.v[1] = thr->ndiv_buf;
  tcmd.v[2] = thr->buf;
  tcmd.v[3] = hx->task_head;
  
  if(tcmd.node == INSIDE){
    pthread_mutex_lock(&queue_mut);
#ifdef TEST_MSG_LOCK_QUEUE
    fprintf(stderr, "lock queue id=%d\n", thr->id);
#endif
    enqueue_command(tcmd, body);
#ifdef TEST_MSG_LOCK_QUEUE
    fprintf(stderr, "signal queue id=%d\n", thr->id);
    fprintf(stderr, "unlock queue id=%d\n", thr->id);
#endif
    pthread_cond_signal(&cond_q);
    pthread_mutex_unlock(&queue_mut);
  }
  else if(tcmd.node == OUTSIDE){
    pthread_mutex_lock(&send_mut);
#ifdef TEST_MSG_LOCK_SEND
    fprintf(stderr, "lock send id=%d\n",thr->id);
#endif
#ifdef TEST_MSG_LOCK_THREAD
    fprintf(stderr, "unlock before send id=%d\n",thr->id);
#endif
    pthread_mutex_unlock(&thr->mut);
    send_command(tcmd);
#ifdef TEST_MSG_SEND
    fprintf(stderr, "id=%d send task body\n", thr->id);
#endif
    send_task_body(thr, body);
    write_eol();
    flush_send();
#ifdef TEST_MSG_LOCK_SEND
    fprintf(stderr, "unlock send id=%d\n",thr->id);
#endif
    pthread_mutex_unlock(&send_mut);
    pthread_mutex_lock(&thr->mut);
#ifdef TEST_MSG_LOCK_THREAD
    fprintf(stderr, "lock after send id=%d\n",thr->id);
#endif
  }
  else {
    perror("invalid tcmd.node in make_and_send_task\n");
    fprintf(stderr, "%d\n", tcmd.node);
    exit(0);
  }
}

void *wait_rslt(struct thread_data *thr){
  void *body;
  struct timespec t1;
  struct timeval now;
  long nsec;
  struct task_home *sub;
  pthread_mutex_lock(&thr->mut);
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "lock id=%d wait_rslt\n",thr->id);
#endif
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
#ifdef TEST_MSG_LOCK_THREAD
    fprintf(stderr, "unlock wait id=%d\n",thr->id);
#endif
    pthread_cond_timedwait(&thr->cond_r, &thr->mut, &t1);
#ifdef TEST_MSG_LOCK_THREAD
    fprintf(stderr, "lock after wait in wait_rslt id=%d\n",thr->id);
#endif
#if 0
    thr->task_top->stat = TASK_STARTED;
#endif
    if(sub->stat == TASK_HOME_DONE)
      break;
#endif
    /* fprintf(stderr, "sub %d\n", sub); */

    recv_exec_send(thr, sub->task_head, sub->req_from);
  }
  body = sub->body;
  thr->sub = sub->next;
  sub->next = thr->treq_free;
  thr->treq_free = sub;
  /* fprintf(stderr, "nsub %d\n", thr->sub); */
#ifdef TEST_MSG_LOCK_THREAD
  fprintf(stderr, "unlock id=%d wait_rslt fin\n",thr->id);
#endif
  pthread_mutex_unlock(&thr->mut);
  return body;
}

/*  ********************************************  */
