typedef char *(*nestfn_t) (char *, void *);
typedef struct
{
  nestfn_t fun;
  void *fr;
} closure_t;
typedef double Align_t;

char *lw_call (char *esp);

struct func_arg
{
  void *(*func) (char *, void *);
  void *arg;
};

void *thread_origin (void *farg);
struct systhr_create_frame;
struct recv_exec_send_frame;
struct worker_frame;
struct handle_req_frame;
struct handle_req_cncl_frame;
struct handle_exception_frame;
struct handle_cancellation_frame;
struct handle_reqs_frame;
struct wait_rslt_frame;
struct main_frame;
#define _GNU_SOURCE
#include<sched.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<pthread.h>
#include<sys/time.h>
#include<unistd.h>

int connect_to (char *hostname, unsigned short port);

void close_socket (int socket);

int send_char (char, int);

int send_string (char *str, int socket);

int send_fmt_string (int socket, char *fmt_string, ...);

int send_binary (void *src, unsigned long elm_size, unsigned long n_elm,
                 int socket);

int receive_char (int socket);

char *receive_line (char *buf, int maxlen, int socket);

int receive_binary (void *dst, unsigned long elm_size, unsigned long n_elm,
                    int socket);
extern char *receive_buf;
extern char *receive_buf_p;
#include<sys/time.h>

int connect_to (char *hostname, unsigned short port);

void close_socket (int socket);

int send_char (char, int);

int send_string (char *str, int socket);

int send_fmt_string (int socket, char *fmt_string, ...);

int send_binary (void *src, unsigned long elm_size, unsigned long n_elm,
                 int socket);

int receive_char (int socket);

char *receive_line (char *buf, int maxlen, int socket);

int receive_binary (void *dst, unsigned long elm_size, unsigned long n_elm,
                    int socket);
extern char *receive_buf;
extern char *receive_buf_p;
enum addr
{ ANY = -3, PARENT = -4, FORWARD = -5, TERM = -99 };
enum node
{ INSIDE, OUTSIDE };
enum command
{ TASK, RSLT, TREQ, NONE, RACK, BCST, BCAK, STAT, VERB, EXIT, CNCL, WRNG };
static char *cmd_strings[] =
  { "task", "rslt", "treq", "none", "rack", "bcst", "bcak", "stat", "verb",
"exit", "cncl", "wrng", 0 };
enum choose
{ CHS_RANDOM, CHS_ORDER };
static char *choose_strings[] = { "CHS-RANDOM", "CHS-ORDER" };

struct cmd
{
  enum command w;
  int c;
  enum node node;
  enum addr v[4][16];
};

struct cmd_list
{
  struct cmd cmd;
  void *body;
  int task_no;
  struct cmd_list *next;
};
struct task;
struct thread_data;
extern void (*task_doers[256]) (char *, struct thread_data *, void *);
extern void (*task_senders[256]) (void *);
extern void *(*task_receivers[256]) ();
extern void (*rslt_senders[256]) (void *);
extern void (*rslt_receivers[256]) (void *);
struct worker_data;

void worker_init (struct thread_data *);
enum task_stat
{ TASK_ALLOCATED, TASK_INITIALIZED, TASK_STARTED, TASK_DONE, TASK_NONE,
    TASK_SUSPENDED };
static char *task_stat_strings[] =
  { "TASK-ALLOCATED", "TASK-INITIALIZED", "TASK-STARTED", "TASK-DONE",
"TASK-NONE", "TASK-SUSPENDED" };
enum task_home_stat
{ TASK_HOME_ALLOCATED, TASK_HOME_INITIALIZED, TASK_HOME_DONE,
    TASK_HOME_EXCEPTION, TASK_HOME_ABORTED };
static char *task_home_stat_strings[] =
  { "TASK-HOME-ALLOCATED", "TASK-HOME-INITIALIZED", "TASK-HOME-DONE",
"TASK-HOME-EXCEPTION", "TASK-HOME-ABORTED" };
enum exiting_rsn
{ EXITING_NORMAL, EXITING_EXCEPTION, EXITING_CANCEL, EXITING_SPAWN };
static char *exiting_rsn_strings[] =
  { "EXITING-NORMAL", "EXITING-EXCEPTION", "EXITING-CANCEL",
"EXITING-SPAWN" };
enum tcounter
{ TCOUNTER_INIT, TCOUNTER_EXEC, TCOUNTER_SPWN, TCOUNTER_WAIT, TCOUNTER_EXCP,
    TCOUNTER_EXCP_WAIT, TCOUNTER_ABRT, TCOUNTER_ABRT_WAIT, TCOUNTER_TREQ_BK,
    TCOUNTER_TREQ_ANY };
static char *tcounter_strings[] =
  { "TCOUNTER-INIT", "TCOUNTER-EXEC", "TCOUNTER-SPWN", "TCOUNTER-WAIT",
"TCOUNTER-EXCP", "TCOUNTER-EXCP-WAIT", "TCOUNTER-ABRT", "TCOUNTER-ABRT-WAIT", "TCOUNTER-TREQ-BK",
"TCOUNTER-TREQ-ANY" };
enum event
{ EV_SEND_TASK, EV_STRT_TASK, EV_RSLT_TASK, EV_EXCP_TASK, EV_ABRT_TASK,
    EV_SEND_CNCL };
static char *ev_strings[] =
  { "EV-SEND-TASK", "EV-STRT-TASK", "EV-RSLT-TASK", "EV-EXCP-TASK",
"EV-ABRT-TASK", "EV-SEND-CNCL" };
enum obj_type
{ OBJ_NULL, OBJ_INT, OBJ_ADDR, OBJ_PADDR };

union aux_data_body
{
  long aux_int;
  enum addr aux_addr[16];
  enum addr *aux_paddr;
};

struct aux_data
{
  enum obj_type type;
  union aux_data_body body;
};

struct task
{
  enum task_stat stat;
  struct task *next;
  struct task *prev;
  int task_no;
  void *body;
  int ndiv;
  int cancellation;
  enum node rslt_to;
  enum addr rslt_head[16];
};

struct task_home
{
  enum task_home_stat stat;
  int id;
  int exception_tag;
  int msg_cncl;
  enum addr waiting_head[16];
  struct task *owner;
  struct task_home *eldest;
  int task_no;
  enum node req_from;
  enum addr task_head[16];
  struct task_home *next;
  void *body;
};

struct thread_data
{
  int id;
  pthread_t pthr_id;
  struct task_home *req;
  int req_cncl;
  int w_rack;
  int w_none;
  int ndiv;
  double probability;
  int last_treq;
  enum choose last_choose;
  double random_seed1;
  double random_seed2;
  unsigned short random_seed_probability[3];
  struct task *task_free;
  struct task *task_top;
  struct task_home *treq_free;
  struct task_home *treq_top;
  struct task_home *sub;
  pthread_mutex_t mut;
  pthread_mutex_t rack_mut;
  pthread_cond_t cond;
  pthread_cond_t cond_r;
  void *wdptr;
  int w_bcak;
  enum exiting_rsn exiting;
  int exception_tag;
  enum tcounter tcnt_stat;
  double tcnt[10];
  struct timeval tcnt_tp[10];
  struct aux_data tc_aux;
  int ev_cnt[6];
  FILE *fp_tc;
  char dummy[1111];
};
enum DATA_FLAG
{ DATA_NONE, DATA_REQUESTING, DATA_EXIST };

struct dhandler_arg
{
  enum node data_to;
  enum addr head[16];
  struct cmd dreq_cmd;
  struct cmd dreq_cmd_fwd;
  int start;
  int end;
};

void make_and_send_task (struct thread_data *thr, int task_no, void *body,
                         int eldest_p);

void *wait_rslt (char *esp, struct thread_data *thr, int stback);

void broadcast_task (struct thread_data *thr, int task_no, void *body);

void proto_error (char const *str, struct cmd *pcmd);

void read_to_eol (void);

void init_data_flag (int);

void guard_task_request (struct thread_data *);

int guard_task_request_prob (struct thread_data *, double);

void recv_rslt (struct cmd *, void *);

void recv_task (struct cmd *, void *);

void recv_treq (struct cmd *);

void recv_rack (struct cmd *);

void recv_none (struct cmd *);

void recv_back (struct cmd *);

void print_task_list (struct task *task_top, char *name);

void print_task_home_list (struct task_home *treq_top, char *name);

void print_thread_status (struct thread_data *thr);

void print_status (struct cmd *);

void set_verbose_level (struct cmd *);

void recv_exit (struct cmd *);

void recv_bcst (struct cmd *);

void recv_bcak (struct cmd *);

void recv_cncl (struct cmd *);

void initialize_tcounter (struct thread_data *);

void tcounter_start (struct thread_data *, enum tcounter);

void tcounter_end (struct thread_data *, enum tcounter);

enum tcounter tcounter_change_state (struct thread_data *, enum tcounter,
                                     enum obj_type, void *);

void initialize_evcounter (struct thread_data *);

int evcounter_count (struct thread_data *, enum event, enum obj_type, void *);

void show_counters ();

int serialize_cmdname (char *buf, enum command w);

int deserialize_cmdname (enum command *buf, char *str);

int serialize_arg (char *buf, enum addr *arg);

enum addr deserialize_addr (char *str);

int deserialize_arg (enum addr *buf, char *str);

int serialize_cmd (char *buf, struct cmd *pcmd);

int deserialize_cmd (struct cmd *pcmd, char *str);

int copy_address (enum addr *dst, enum addr *src);

int address_equal (enum addr *adr1, enum addr *adr2);

struct runtime_option
{
  int num_thrs;
  char *sv_hostname;
  unsigned short port;
  char *node_name;
  char *initial_task;
  int auto_exit;
  int affinity;
  int always_flush_accepted_treq;
  int verbose;
  char *timechart_file;
};
extern struct runtime_option option;
#define NDEBUG
#include<assert.h>
struct thread_data threads[128];
unsigned int num_thrs;
double random_seed1 = 0.2403703;
double random_seed2 = 3.638732;
struct timeval tp_strt;

int
my_random (int max, double *pseed1, double *pseed2)
{
  *pseed1 = *pseed1 * 3.0 + *pseed2;
  *pseed1 -= (int) *pseed1;
  return max ** pseed1;
}

double
my_random_double (double *pseed1, double *pseed2)
{
  *pseed1 = *pseed1 * 3.0 + *pseed2;
  *pseed1 -= (int) *pseed1;
  return *pseed1;
}

double
my_random_probability (struct thread_data *thr)
{
  double d;
  d = erand48 ((*thr).random_seed_probability);
  return d;
}

struct runtime_option option;

struct systhr_create_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  pthread_attr_t attr;
  pthread_t tid;
  int status;
  void *arg;
  void *(*start_func) (char *, void *);
  pthread_t *p_tid;
};

int
systhr_create (char *esp, pthread_t * p_tid,
               void *(*start_func) (char *, void *), void *arg)
{
  struct func_arg *farg;
  struct func_arg *farg2;
  int status;
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct systhr_create_frame *efp;
LGOTO:;
  efp = (struct systhr_create_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct systhr_create_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  status = 0;
  if (!p_tid)
    p_tid = &efp->tid;
  else;
  pthread_attr_init (&efp->attr);
  status = pthread_attr_setscope (&efp->attr, PTHREAD_SCOPE_SYSTEM);
  if (status == 0)
    status = (farg =
              (struct func_arg *) malloc (sizeof (struct func_arg)),
              farg->func = start_func, farg->arg =
              arg, pthread_create (p_tid, &efp->attr, thread_origin, farg));
  else
    status = (farg2 =
              (struct func_arg *) malloc (sizeof (struct func_arg)),
              farg2->func = start_func, farg2->arg =
              arg, pthread_create (p_tid, 0, thread_origin, farg2));
  return status;
}

void
mem_error (char const *str)
{
  fputs (str, stderr);
  fputc ('\n', stderr);
  exit (1);
}

int
get_universal_real_time ()
{
  struct timeval now;
  gettimeofday (&now, 0);
  return 1000 * 1000 * now.tv_sec + now.tv_usec;
}

double
diff_timevals (struct timeval *tp1, struct timeval *tp2)
{
  return (*tp1).tv_sec - (*tp2).tv_sec + 1.0E-6 * ((*tp1).tv_usec -
                                                   (*tp2).tv_usec);
}

void
proto_error (char const *str, struct cmd *pcmd)
{
  int i;
  char buf[1280];
  serialize_cmd (buf, pcmd);
  fprintf (stderr, "(%d): %s> %s\n", get_universal_real_time (), str, buf);
}

pthread_mutex_t send_mut;
int sv_socket;

void
read_to_eol (void)
{
  int c;
  while (EOF != (c = receive_char (sv_socket)))
    {
      if (c == '\n')
        break;
      else;
    }
}

void
write_eol (void)
{
  send_char ('\n', sv_socket);
}

void
flush_send (void)
{
  if (sv_socket < 0)
    fflush (stdout);
  else;
}

char buf[1280];
struct cmd cmd_buf;

struct cmd *
read_command ()
{
  char *b;
  char *cp;
  b = buf;
  cp = NULL;
  cp = receive_line (b, 1280, sv_socket);
  if (cp)
    {
      cmd_buf.node = OUTSIDE;
      if (option.verbose >= 1)
        fprintf (stderr, "(%d): RECEIVED> %s", get_universal_real_time (), b);
      else;
      deserialize_cmd (&cmd_buf, b);
      return &cmd_buf;
    }
  else
    {
      if (option.verbose >= 1)
        fprintf (stderr, "(%d): RECEIVED> (failed)",
                 get_universal_real_time ());
      else;
      return NULL;
    }
}

char send_buf[1280];

void
send_out_command (struct cmd *pcmd, void *body, int task_no)
{
  int ret;
  enum command w;
  int i;
  struct thread_data *thr;
  FILE *fp;
  w = (*pcmd).w;
  pthread_mutex_lock (&send_mut);
  serialize_cmd (send_buf, pcmd);
  send_string (send_buf, sv_socket);
  write_eol ();
  if (body)
    if (w == TASK || w == BCST)
      {
        (task_senders[task_no]) (body);
        write_eol ();
      }
    else if (w == RSLT)
      {
        (rslt_senders[task_no]) (body);
        write_eol ();
      }
    else;
  else;
  flush_send ();
  pthread_mutex_unlock (&send_mut);
  if (w == RSLT && option.auto_exit)
    {
      {
        i = 0;
        for (; i < num_thrs; i++)
          {
            thr = threads + i;
            tcounter_change_state (thr, TCOUNTER_INIT, OBJ_NULL, 0);
            if ((*thr).fp_tc)
              {
                fp = (*thr).fp_tc;
                (*thr).fp_tc = 0;
                fclose (fp);
              }
            else;
          }
      }
      show_counters ();
      exit (0);
    }
  else;
}

void
proc_cmd (struct cmd *pcmd, void *body)
{
  enum command w;
  w = (*pcmd).w;
  switch (w)
    {
    case TASK:
      recv_task (pcmd, body);
      break;
    case RSLT:
      recv_rslt (pcmd, body);
      break;
    case TREQ:
      recv_treq (pcmd);
      break;
    case NONE:
      recv_none (pcmd);
      break;
    case RACK:
      recv_rack (pcmd);
      break;
    case BCST:
      recv_bcst (pcmd);
      break;
    case CNCL:
      recv_cncl (pcmd);
      break;
    case BCAK:
      recv_bcak (pcmd);
      break;
    case STAT:
      print_status (pcmd);
      break;
    case VERB:
      set_verbose_level (pcmd);
      break;
    case EXIT:
      recv_exit (pcmd);
      break;
    default:
      proto_error ("wrong cmd", pcmd);
      break;
    }
}

void
send_command (struct cmd *pcmd, void *body, int task_no)
{
  if ((*pcmd).node == INSIDE)
    {
      if (option.verbose >= 3)
        if (option.verbose >= 4 || TREQ != (*pcmd).w && NONE != (*pcmd).w)
          proto_error ("INSIDE", pcmd);
        else;
      else;
      proc_cmd (pcmd, body);
    }
  else
    {
      if (option.verbose >= 1)
        {
          proto_error ("OUTSIDE", pcmd);
        }
      else;
      send_out_command (pcmd, body, task_no);
    }
}

void
flush_treq_with_none_1 (struct thread_data *thr, struct task_home **p_hx)
{
  struct task_home *hx;
  struct cmd rcmd;
  hx = *p_hx;
  rcmd = (struct cmd)
  {
  .c = 1,.w = NONE,.node = (*hx).req_from};
  copy_address ((rcmd.v)[0], (*hx).task_head);
  send_command (&rcmd, 0, 0);
  *p_hx = (*hx).next;
  (*hx).next = (*thr).treq_free;
  (*thr).treq_free = hx;
}

void
guard_task_request (struct thread_data *thr)
{
  flush_treq_with_none_1 (thr, &(*thr).treq_top);
}

int
guard_task_request_prob (struct thread_data *thr, double prob)
{
  if (prob >= my_random_probability (thr))
    {
      flush_treq_with_none_1 (thr, &(*thr).treq_top);
      return 1;
    }
  else
    return 0;
}

void
flush_treq_with_none (struct thread_data *thr, enum addr *rslt_head,
                      enum node rslt_to)
{
  struct task_home **pcur_hx;
  struct task_home *hx;
  int flush;
  int ignored;
  int flushed_any_treq;
  enum addr *flushed_stealing_back_head;
  char buf0[1280];
  char buf1[1280];
  pcur_hx = &(*thr).treq_top;
  ignored = 0;
  flushed_any_treq = 0;
  flushed_stealing_back_head = 0;
  while (hx = *pcur_hx)
    {
      if (option.always_flush_accepted_treq
          || TERM == ((*hx).waiting_head)[0])
        {
          if (option.verbose >= 2)
            {
              flushed_any_treq++;
            }
          else;
          flush = 1;
        }
      else if (rslt_head && (*hx).req_from == rslt_to
               && address_equal ((*hx).waiting_head, rslt_head))
        {
          if (option.verbose >= 2)
            {
              flushed_stealing_back_head = rslt_head;
              rslt_head = 0;
            }
          else;
          flush = 1;
        }
      else
        {
          if (option.verbose >= 2)
            {
              ignored++;
            }
          else;
          flush = 0;
        }
      if (flush)
        flush_treq_with_none_1 (thr, pcur_hx);
      else
        {
          pcur_hx = &(*hx).next;
        }
    }
  if (option.verbose >= 2)
    if (flushed_any_treq > 0 || ignored > 0 || flushed_stealing_back_head)
      fprintf (stderr,
               "(%d): (Thread %d) flushed %d any %s and ignored %d stealing-back treqs in flush-treq-with-none\n",
               get_universal_real_time (), (*thr).id, flushed_any_treq,
               flushed_stealing_back_head
               ? (serialize_arg (buf1, flushed_stealing_back_head),
                  sprintf (buf0, "and stealing-back from %s", buf1),
                  buf0) : "", ignored);
    else;
  else;
}

struct task *
allocate_task (struct thread_data *thr)
{
  struct task *tx;
  tx = (*thr).task_free;
  (*tx).stat = TASK_ALLOCATED;
  if (!tx)
    mem_error ("Not enough task memory");
  else;
  (*thr).task_top = tx;
  (*thr).task_free = (*tx).prev;
  return tx;
}

void
deallocate_task (struct thread_data *thr)
{
  struct task *tx;
  tx = (*thr).task_top;
  (*thr).task_free = tx;
  (*thr).task_top = (*tx).next;
  return;
}

void
timeval_plus_nsec_to_timespec (struct timespec *pts_dst,
                               struct timeval *ptv_src, long diff)
{
  long nsec;
  nsec = diff + 1000 * (*ptv_src).tv_usec;
  (*pts_dst).tv_nsec = nsec > 999999999 ? nsec - 999999999 : nsec;
  (*pts_dst).tv_sec = (*ptv_src).tv_sec + (nsec > 999999999 ? 1 : 0);
}

int
send_treq_to_initialize_task (struct thread_data *thr, enum addr *treq_head,
                              enum node req_to)
{
  struct cmd rcmd;
  long delay;
  struct task *tx;
  struct timespec t_until;
  struct timeval now;
  delay = 1000;
  tx = (*thr).task_top;
  rcmd.c = 2;
  rcmd.node = req_to;
  rcmd.w = TREQ;
  (rcmd.v)[0][0] = (*thr).id;
  if (req_to != ANY && (*thr).sub)
    {
      (rcmd.v)[0][1] = (*(*thr).sub).id;
      (rcmd.v)[0][2] = TERM;
    }
  else
    (rcmd.v)[0][1] = TERM;
  copy_address ((rcmd.v)[1], treq_head);
  do
    {
      flush_treq_with_none (thr, 0, 0);
      (*tx).stat = TASK_ALLOCATED;
      {
        pthread_mutex_unlock (&(*thr).mut);
        send_command (&rcmd, 0, 0);
        pthread_mutex_lock (&(*thr).mut);
      }
      while (1)
        {
          if ((*tx).stat != TASK_INITIALIZED && (*thr).sub
              && ((*(*thr).sub).stat == TASK_HOME_DONE
                  || (*(*thr).sub).stat == TASK_HOME_EXCEPTION
                  || (*(*thr).sub).stat == TASK_HOME_ABORTED))
            {
              if ((*tx).stat != TASK_NONE)
                ((*thr).w_none)++;
              else;
              return 0;
            }
          else;
          if ((*tx).stat != TASK_ALLOCATED)
            break;
          else;
          pthread_cond_wait (&(*thr).cond, &(*thr).mut);
        }
      if ((*tx).stat == TASK_NONE)
        {
          if (1)
            {
              gettimeofday (&now, 0);
              timeval_plus_nsec_to_timespec (&t_until, &now, delay);
              pthread_cond_timedwait (&(*thr).cond_r, &(*thr).mut, &t_until);
              delay += delay;
              if (delay > 100 * 1000 * 1000)
                delay = 100 * 1000 * 1000;
              else;
            }
          else;
          if ((*thr).sub
              && ((*(*thr).sub).stat == TASK_HOME_DONE
                  || (*(*thr).sub).stat == TASK_HOME_EXCEPTION
                  || (*(*thr).sub).stat == TASK_HOME_ABORTED))
            return 0;
          else;
        }
      else;
    }
  while ((*tx).stat != TASK_INITIALIZED);
  return 1;
}

struct recv_exec_send_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  struct cmd rcmd;
  int reason;
  enum exiting_rsn rsn;
  double old_probability;
  int old_ndiv;
  struct task *tx;
  int ifexp_result;
  int ifexp_result2;
  int ifexp_result3;
  int ifexp_result4;
  int ifexp_result5;
  int ifexp_result6;
  enum node req_to;
  enum addr *treq_head;
  struct thread_data *thr;
};

void
recv_exec_send (char *esp, struct thread_data *thr, enum addr *treq_head,
                enum node req_to)
{
  int ifexp_result6;
  int ifexp_result5;
  int ifexp_result4;
  int ifexp_result3;
  int ifexp_result2;
  int ifexp_result;
  struct task *tx;
  int old_ndiv;
  double old_probability;
  enum exiting_rsn rsn;
  int reason;
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct recv_exec_send_frame *efp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct recv_exec_send_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct recv_exec_send_frame) + sizeof (Align_t) +
                   -1) / sizeof (Align_t));
      *((char **) esp) = 0;
    LGOTO:switch ((*efp).call_id)
        {
        case 0:
          goto L_CALL;
        }
      goto L_CALL;
    }
  else;
  efp = (struct recv_exec_send_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct recv_exec_send_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  while ((*thr).w_none > 0)
    {
      pthread_cond_wait (&(*thr).cond, &(*thr).mut);
      {
        if ((*thr).sub)
          {
            {
              if ((*(*thr).sub).stat == TASK_HOME_DONE)
                ifexp_result3 = 1;
              else
                {
                  {
                    if ((*(*thr).sub).stat == TASK_HOME_EXCEPTION)
                      ifexp_result5 = 1;
                    else
                      {
                        if ((*(*thr).sub).stat == TASK_HOME_ABORTED)
                          ifexp_result6 = 1;
                        else
                          ifexp_result6 = 0;
                        ifexp_result5 = ifexp_result6;
                      }
                    if (ifexp_result5)
                      ifexp_result4 = 1;
                    else
                      ifexp_result4 = 0;
                  }
                  ifexp_result3 = ifexp_result4;
                }
              if (ifexp_result3)
                ifexp_result2 = 1;
              else
                ifexp_result2 = 0;
            }
            ifexp_result = ifexp_result2;
          }
        else
          ifexp_result = 0;
        if (ifexp_result)
          return;
        else;
      }
    }
  tx = allocate_task (thr);
  if (send_treq_to_initialize_task (thr, treq_head, req_to))
    {
      tcounter_change_state (thr, TCOUNTER_EXEC, OBJ_NULL, 0);
      evcounter_count (thr, EV_STRT_TASK, OBJ_PADDR, (*tx).rslt_head);
      (*tx).stat = TASK_STARTED;
      (*tx).cancellation = 0;
      old_ndiv = (*thr).ndiv;
      old_probability = (*thr).probability;
      (*thr).ndiv = (*tx).ndiv;
      (*thr).probability = 1.0;
      pthread_mutex_unlock (&(*thr).mut);
      if (option.verbose >= 1)
        fprintf (stderr, "(%d): (Thread %d) start %d<%p> (body=%p).\n",
                 get_universal_real_time (), (*thr).id, (*tx).task_no, tx,
                 (*tx).body);
      else;
      {
        new_esp = esp;
        while ((task_doers[(*tx).task_no]) (new_esp, thr, (*tx).body),
               __builtin_expect ((efp->tmp_esp = *((char **) esp)) != 0, 0))
          {
            efp->reason = reason;
            efp->rsn = rsn;
            efp->old_probability = old_probability;
            efp->old_ndiv = old_ndiv;
            efp->tx = tx;
            efp->ifexp_result = ifexp_result;
            efp->ifexp_result2 = ifexp_result2;
            efp->ifexp_result3 = ifexp_result3;
            efp->ifexp_result4 = ifexp_result4;
            efp->ifexp_result5 = ifexp_result5;
            efp->ifexp_result6 = ifexp_result6;
            efp->req_to = req_to;
            efp->treq_head = treq_head;
            efp->thr = thr;
            efp->call_id = 0;
            return;
          L_CALL:;
            reason = efp->reason;
            rsn = efp->rsn;
            old_probability = efp->old_probability;
            old_ndiv = efp->old_ndiv;
            tx = efp->tx;
            ifexp_result = efp->ifexp_result;
            ifexp_result2 = efp->ifexp_result2;
            ifexp_result3 = efp->ifexp_result3;
            ifexp_result4 = efp->ifexp_result4;
            ifexp_result5 = efp->ifexp_result5;
            ifexp_result6 = efp->ifexp_result6;
            req_to = efp->req_to;
            treq_head = efp->treq_head;
            thr = efp->thr;
            new_esp = esp + 1;
          }
      }
      rsn = (*thr).exiting;
      (*thr).exiting = EXITING_NORMAL;
      switch (rsn)
        {
        case EXITING_NORMAL:
          if (option.verbose >= 1)
            fprintf (stderr, "(%d): (Thread %d) end %d<%p> (body=%p).\n",
                     get_universal_real_time (), (*thr).id, (*tx).task_no, tx,
                     (*tx).body);
          else;
          evcounter_count (thr, EV_RSLT_TASK, OBJ_PADDR, (*tx).rslt_head);
          reason = 0;
          break;
        case EXITING_EXCEPTION:
          if (option.verbose >= 1)
            fprintf (stderr,
                     "(%d): (Thread %d) end %d<%p> (body=%p) with exception %d.\n",
                     get_universal_real_time (), (*thr).id, (*tx).task_no, tx,
                     (*tx).body, (*thr).exception_tag);
          else;
          evcounter_count (thr, EV_EXCP_TASK, OBJ_PADDR, (*tx).rslt_head);
          reason = 1;
          break;
        case EXITING_CANCEL:
          if (option.verbose >= 1)
            fprintf (stderr, "(%d): (Thread %d) aborted %d<%p> (body=%p).\n",
                     get_universal_real_time (), (*thr).id, (*tx).task_no, tx,
                     (*tx).body);
          else;
          evcounter_count (thr, EV_ABRT_TASK, OBJ_PADDR, (*tx).rslt_head);
          reason = 2;
          break;
        default:
          fprintf (stderr,
                   "(%d) Warn: Thread %d ended with unexpected reason.\n",
                   get_universal_real_time (), (*thr).id);
          reason = 0;
        }
      (efp->rcmd).w = RSLT;
      (efp->rcmd).c = 3;
      (efp->rcmd).node = (*tx).rslt_to;
      copy_address (((efp->rcmd).v)[0], (*tx).rslt_head);
      ((efp->rcmd).v)[1][0] = reason;
      ((efp->rcmd).v)[1][1] = TERM;
      ((efp->rcmd).v)[2][0] = (*thr).exception_tag;
      ((efp->rcmd).v)[2][1] = TERM;
      send_command (&efp->rcmd, (*tx).body, (*tx).task_no);
      pthread_mutex_lock (&(*thr).rack_mut);
      ((*thr).w_rack)++;
      pthread_mutex_unlock (&(*thr).rack_mut);
      pthread_mutex_lock (&(*thr).mut);
      (*thr).ndiv = old_ndiv;
      (*thr).probability = old_probability;
    }
  else;
  (*tx).stat = TASK_DONE;
  flush_treq_with_none (thr, (*tx).rslt_head, (*tx).rslt_to);
  deallocate_task (thr);
}

void
worker_setaffinity (int n)
{
  cpu_set_t mask;
  CPU_ZERO (&mask);
  CPU_SET (n, &mask);
  if (-1 == sched_setaffinity (0, sizeof (mask), &mask))
    {
      perror ("Failed to set CPU affinity");
      exit (-1);
    }
  else;
  if (option.verbose >= 1)
    fprintf (stderr, "Bind worker to core %d\n", n);
  else;
}

struct worker_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  struct thread_data *thr;
  void *arg;
};

void *
worker (char *esp, void *arg)
{
  struct thread_data *thr;
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct worker_frame *efp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct worker_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct worker_frame) + sizeof (Align_t) +
                   -1) / sizeof (Align_t));
      *((char **) esp) = 0;
    LGOTO:switch ((*efp).call_id)
        {
        case 0:
          goto L_CALL2;
        }
      goto L_CALL2;
    }
  else;
  efp = (struct worker_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct worker_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  thr = arg;
  (*thr).wdptr = malloc (sizeof (struct thread_data));
  if (option.affinity)
    worker_setaffinity ((*thr).id);
  else;
  worker_init (thr);
  pthread_mutex_lock (&(*thr).mut);
  while (1)
    {
      if ((*thr).tcnt_stat != TCOUNTER_INIT)
        tcounter_change_state (thr, TCOUNTER_TREQ_ANY, OBJ_NULL, 0);
      else;
      {
        new_esp = esp;
        while (recv_exec_send (new_esp, thr, (enum addr[2])
                               {
                               ANY, TERM}, INSIDE),
               __builtin_expect ((efp->tmp_esp = *((char **) esp)) != 0, 0))
          {
            efp->thr = thr;
            efp->arg = arg;
            efp->call_id = 0;
            return (void *) 0 - 1;
          L_CALL2:;
            thr = efp->thr;
            arg = efp->arg;
            new_esp = esp + 1;
          }
      }
    }
  pthread_mutex_unlock (&(*thr).mut);
}

void
recv_task (struct cmd *pcmd, void *body)
{
  struct task *tx;
  struct thread_data *thr;
  enum addr id;
  int task_no;
  size_t len;
  if ((*pcmd).c < 4)
    proto_error ("wrong-task", pcmd);
  else;
  task_no = ((*pcmd).v)[3][0];
  if ((*pcmd).node == OUTSIDE)
    {
      body = (task_receivers[task_no]) ();
      read_to_eol ();
    }
  else;
  id = ((*pcmd).v)[2][0];
  if (!(id < num_thrs))
    proto_error ("wrong task-head", pcmd);
  else;
  thr = threads + id;
  pthread_mutex_lock (&(*thr).mut);
  tx = (*thr).task_top;
  (*tx).rslt_to = (*pcmd).node;
  copy_address ((*tx).rslt_head, ((*pcmd).v)[1]);
  (*tx).ndiv = ((*pcmd).v)[0][0];
  (*tx).task_no = task_no;
  (*tx).body = body;
  (*tx).stat = TASK_INITIALIZED;
  pthread_cond_broadcast (&(*thr).cond);
  pthread_mutex_unlock (&(*thr).mut);
}

void
recv_none (struct cmd *pcmd)
{
  struct thread_data *thr;
  enum addr id;
  size_t len;
  if ((*pcmd).c < 1)
    proto_error ("Wrong none", pcmd);
  else;
  id = ((*pcmd).v)[0][0];
  if (!(id < num_thrs))
    proto_error ("Wrong task-head", pcmd);
  else;
  thr = threads + id;
  pthread_mutex_lock (&(*thr).mut);
  if ((*thr).w_none > 0)
    ((*thr).w_none)--;
  else
    (*(*thr).task_top).stat = TASK_NONE;
  pthread_cond_broadcast (&(*thr).cond);
  pthread_mutex_unlock (&(*thr).mut);
}

struct task_home *
search_task_home_by_id (int id, struct task_home *hx)
{
  while (hx && (*hx).id != id)
    {
      hx = (*hx).next;
    }
  return hx;
}

int
set_cancelled (struct thread_data *thr, struct task *owner,
               struct task_home *eldest)
{
  struct task_home *cur;
  int count;
  count = 0;
  {
    cur = (*thr).sub;
    for (; cur; cur = (*cur).next)
      {
        if ((*cur).owner == owner && (*cur).stat == TASK_HOME_INITIALIZED
            && (*cur).msg_cncl == 0)
          {
            (*cur).msg_cncl = 1;
            count++;
          }
        else;
        if (eldest && eldest == (*cur).eldest)
          break;
        else;
      }
  }
  if (count)
    (*thr).req_cncl = 1;
  else;
  return count;
}

void
recv_rslt (struct cmd *pcmd, void *body)
{
  struct cmd rcmd;
  struct thread_data *thr;
  struct task_home *hx;
  enum addr tid;
  int sid;
  int reason;
  int exception_tag;
  if ((*pcmd).c < 2)
    proto_error ("Wrong rslt", pcmd);
  else;
  tid = ((*pcmd).v)[0][0];
  if (!(tid < num_thrs))
    proto_error ("wrong rslt-head", pcmd);
  else;
  sid = ((*pcmd).v)[0][1];
  if (TERM == sid)
    proto_error ("Wrong rslt-head (no task-home-id)", pcmd);
  else;
  thr = threads + tid;
  reason = ((*pcmd).v)[1][0];
  exception_tag = ((*pcmd).v)[2][0];
  pthread_mutex_lock (&(*thr).mut);
  if (!(hx = search_task_home_by_id (sid, (*thr).sub)))
    proto_error ("Wrong rslt-head (specified task not exists)", pcmd);
  else;
  if ((*pcmd).node == OUTSIDE)
    {
      (rslt_receivers[(*hx).task_no]) ((*hx).body);
      read_to_eol ();
    }
  else if ((*pcmd).node == INSIDE)
    {
      (*hx).body = body;
    }
  else
    {
      proto_error ("Wrong cmd.node", pcmd);
    }
  rcmd.c = 1;
  rcmd.node = (*pcmd).node;
  rcmd.w = RACK;
  copy_address ((rcmd.v)[0], (*hx).task_head);
  if (reason == 0)
    {
      (*hx).stat = TASK_HOME_DONE;
    }
  else if (reason == 1)
    {
      (*hx).stat = TASK_HOME_EXCEPTION;
      (*hx).exception_tag = exception_tag;
      ((*(*hx).owner).cancellation)++;
      set_cancelled (thr, (*hx).owner, (*hx).eldest);
    }
  else if (reason == 2)
    {
      (*hx).stat = TASK_HOME_ABORTED;
    }
  else;
  if (hx == (*thr).sub)
    {
      pthread_cond_broadcast (&(*thr).cond_r);
      pthread_cond_broadcast (&(*thr).cond);
    }
  else;
  pthread_mutex_unlock (&(*thr).mut);
  send_command (&rcmd, 0, 0);
}

struct task *
have_task (struct thread_data *thr, enum addr *task_spec, enum node task_from)
{
  struct task *tx;
  tx = (*thr).task_top;
  while (tx)
    {
      if (((*tx).stat == TASK_INITIALIZED || (*tx).stat == TASK_SUSPENDED
           || (*tx).stat == TASK_STARTED) && (*tx).rslt_to == task_from
          && address_equal ((*tx).rslt_head, task_spec))
        return tx;
      else;
      tx = (*tx).next;
    }
  return 0;
}

int
try_treq (struct cmd *pcmd, enum addr id)
{
  enum addr *from_addr;
  enum addr *dest_addr;
  struct task_home *hx;
  struct thread_data *thr;
  int fail_reason;
  int avail;
  char from_str[1280];
  char buf1[1280];
  char rsn_str[1280];
  from_addr = ((*pcmd).v)[0];
  dest_addr = ((*pcmd).v)[1];
  fail_reason = 0;
  avail = 0;
  thr = threads + id;
  pthread_mutex_lock (&(*thr).mut);
  pthread_mutex_lock (&(*thr).rack_mut);
  if ((*thr).w_rack > 0)
    {
      fail_reason = 1;
    }
  else if (!(*thr).task_top)
    {
      fail_reason = 2;
    }
  else if (dest_addr[0] == ANY)
    if (!
        ((*(*thr).task_top).stat == TASK_STARTED
         || (*(*thr).task_top).stat == TASK_INITIALIZED))
      fail_reason = 3;
    else;
  else if (!have_task (thr, from_addr, (*pcmd).node))
    fail_reason = 4;
  else;
  if (!fail_reason && dest_addr[0] == ANY)
    if ((*thr).probability < my_random_probability (thr))
      fail_reason = 5;
    else;
  else;
  avail = !fail_reason;
  pthread_mutex_unlock (&(*thr).rack_mut);
  if (option.verbose >= 2)
    if (!avail)
      {
        serialize_arg (from_str, from_addr);
        switch (fail_reason)
          {
          case 1:
            sprintf (rsn_str, "w-rack=%d", (*thr).w_rack);
            break;
          case 2:
            strcpy (rsn_str, "of having no task");
            break;
          case 3:
            sprintf (rsn_str, "the task is %s",
                     task_stat_strings[(*(*thr).task_top).stat]);
            break;
          case 4:
            serialize_arg (buf1, from_addr);
            sprintf (rsn_str, "%s is already finished", buf1);
            break;
          case 5:
            sprintf (rsn_str, "of probability (%lf)", (*thr).probability);
            break;
          default:
            strcpy (rsn_str, "Unexpected reason");
            break;
          }
        fprintf (stderr, "(%d): Thread %d refused treq from %s because %s.\n",
                 get_universal_real_time (), id, from_str, rsn_str);
      }
    else;
  else;
  if (avail)
    {
      hx = (*thr).treq_free;
      if (!hx)
        mem_error ("Not enough task-home memory");
      else;
      (*thr).treq_free = (*hx).next;
      (*hx).next = (*thr).treq_top;
      (*hx).stat = TASK_HOME_ALLOCATED;
      if (dest_addr[0] == ANY)
        ((*hx).waiting_head)[0] = TERM;
      else
        copy_address ((*hx).waiting_head, from_addr);
      copy_address ((*hx).task_head, ((*pcmd).v)[0]);
      if ((*pcmd).node != OUTSIDE)
        (*hx).req_from = INSIDE;
      else
        (*hx).req_from = OUTSIDE;
      (*thr).treq_top = hx;
      (*thr).req = hx;
    }
  else;
  pthread_mutex_unlock (&(*thr).mut);
  return avail;
}

int
choose_treq (enum addr from_addr)
{
  struct thread_data *thr;
  if (0 <= from_addr)
    {
      thr = threads + from_addr;
      (*thr).last_choose = (1 + (*thr).last_choose) % 2;
      if (CHS_RANDOM == (*thr).last_choose)
        return my_random (num_thrs, &(*thr).random_seed1,
                          &(*thr).random_seed2);
      else if (CHS_ORDER == (*thr).last_choose)
        {
          (*thr).last_treq = (1 + (*thr).last_treq) % num_thrs;
          return (*thr).last_treq;
        }
      else
        return 0;
    }
  else if (PARENT == from_addr)
    return my_random (num_thrs, &random_seed1, &random_seed2);
  else
    return 0;
}

void
recv_treq (struct cmd *pcmd)
{
  struct cmd rcmd;
  enum addr dst0;
  int myid;
  int start_id;
  int d;
  int id;
  char buf1[1280];
  char buf12[1280];
  if ((*pcmd).c < 2)
    proto_error ("Wrong treq", pcmd);
  else;
  dst0 = ((*pcmd).v)[1][0];
  if (dst0 == ANY)
    {
      myid = ((*pcmd).v)[0][0];
      start_id = choose_treq (myid);
      {
        d = 0;
        for (; d < num_thrs; d++)
          {
            id = (d + start_id) % num_thrs;
            if ((*pcmd).node != OUTSIDE && id == myid)
              continue;
            else;
            if (try_treq (pcmd, id))
              {
                if (option.verbose >= 2)
                  fprintf (stderr, "(%d): treq(any) %d->%d... accepted.\n",
                           get_universal_real_time (), myid, id);
                else;
                break;
              }
            else;
            if (option.verbose >= 4)
              fprintf (stderr, "(%d): treq(any) %d->%d... refused.\n",
                       get_universal_real_time (), myid, id);
            else;
          }
      }
      if (d < num_thrs)
        return;
      else;
    }
  else
    {
      if (!(0 <= dst0 && dst0 < num_thrs))
        proto_error ("Wrong task-head", pcmd);
      else;
      if (try_treq (pcmd, dst0))
        {
          if (option.verbose >= 2)
            {
              fprintf (stderr,
                       "(%d): treq %s->%d (stealing back)... accepted.\n",
                       get_universal_real_time (),
                       (serialize_arg (buf1, ((*pcmd).v)[0]), buf1), dst0);
            }
          else;
          return;
        }
      else;
      if (option.verbose >= 2)
        {
          fprintf (stderr, "(%d): treq %s->%d (stealing back)... refused.\n",
                   get_universal_real_time (),
                   (serialize_arg (buf12, ((*pcmd).v)[0]), buf12), dst0);
        }
      else;
    }
  if (dst0 == ANY && (*pcmd).node == INSIDE && ((*pcmd).v)[0][0] == 0)
    {
      (*pcmd).node = OUTSIDE;
      send_command (pcmd, 0, 0);
      return;
    }
  else;
  rcmd.c = 1;
  rcmd.node = (*pcmd).node;
  rcmd.w = NONE;
  copy_address ((rcmd.v)[0], ((*pcmd).v)[0]);
  send_command (&rcmd, 0, 0);
}

void
recv_rack (struct cmd *pcmd)
{
  struct task *tx;
  struct thread_data *thr;
  enum addr id;
  size_t len;
  if ((*pcmd).c < 1)
    proto_error ("Wrong rack", pcmd);
  else;
  id = ((*pcmd).v)[0][0];
  if (!(id < num_thrs))
    proto_error ("Wrong task-head", pcmd);
  else;
  thr = threads + id;
  pthread_mutex_lock (&(*thr).rack_mut);
  ((*thr).w_rack)--;
  pthread_mutex_unlock (&(*thr).rack_mut);
}

void
recv_bcst (struct cmd *pcmd)
{
  struct cmd rcmd;
  int task_no;
  void *body;
  if ((*pcmd).c < 2)
    proto_error ("wrong-task", pcmd);
  else;
  task_no = ((*pcmd).v)[1][0];
  body = (task_receivers[task_no]) ();
  read_to_eol ();
  free (body);
  rcmd.c = 1;
  rcmd.node = (*pcmd).node;
  rcmd.w = BCAK;
  copy_address ((rcmd.v)[0], ((*pcmd).v)[0]);
  send_command (&rcmd, 0, task_no);
}

void
recv_cncl (struct cmd *pcmd)
{
  enum addr *from_addr;
  enum addr dst0;
  struct thread_data *thr;
  struct task *tx;
  if ((*pcmd).c < 2)
    proto_error ("Wrong cncl", pcmd);
  else;
  from_addr = ((*pcmd).v)[0];
  dst0 = ((*pcmd).v)[1][0];
  if (!(0 <= dst0 && dst0 < num_thrs))
    proto_error ("Wrong cncl-head", pcmd);
  else;
  thr = threads + dst0;
  pthread_mutex_lock (&(*thr).mut);
  if (tx = have_task (thr, from_addr, (*pcmd).node))
    {
      ((*tx).cancellation)++;
      set_cancelled (thr, tx, 0);
      if (option.verbose >= 1)
        fprintf (stderr, "Task %p of worker %d is cancelled by cncl\n", tx,
                 dst0);
      else;
    }
  else;
  pthread_mutex_unlock (&(*thr).mut);
}

void
recv_bcak (struct cmd *pcmd)
{
  struct thread_data *thr;
  enum addr id;
  if ((*pcmd).c < 1)
    proto_error ("wrong-task", pcmd);
  else;
  id = ((*pcmd).v)[0][0];
  thr = threads + id;
  pthread_mutex_lock (&(*thr).mut);
  (*thr).w_bcak = 0;
  pthread_cond_broadcast (&(*thr).cond);
  pthread_mutex_unlock (&(*thr).mut);
}

void
node_to_string (char *buf3, enum node node2)
{
  switch (node2)
    {
    case INSIDE:
      strcpy (buf3, "INSIDE");
      break;
    case OUTSIDE:
      strcpy (buf3, "OUTSIDE");
      break;
    default:
      strcpy (buf3, "wrong-value");
      break;
    }
}

void
addr_to_string (char *buf4, enum addr addr2)
{
  switch (addr2)
    {
    case ANY:
      strcpy (buf4, "ANY");
      break;
    case PARENT:
      strcpy (buf4, "PARENT");
      break;
    case TERM:
      strcpy (buf4, "TERM");
      break;
    default:
      sprintf (buf4, "%d", addr2);
      break;
    }
}

void
print_task_list (struct task *task_top, char *name)
{
  struct task *cur;
  char buf1[1280];
  char buf2[1280];
  fprintf (stderr, "%s= {", name);
  {
    cur = task_top;
    for (; cur; cur = (*cur).next)
      {
        fprintf (stderr,
                 "{stat=%s, task-no=%d, body=%p, ndiv=%d, cancellation=%d, rslt-to=%s, rslt-head=%s}, ",
                 task_stat_strings[(*cur).stat], (*cur).task_no, (*cur).body,
                 (*cur).ndiv, (*cur).cancellation,
                 (node_to_string (buf1, (*cur).rslt_to), buf1),
                 (serialize_arg (buf2, (*cur).rslt_head), buf2));
      }
  }
  fprintf (stderr, "}, ");
  return;
}

void
print_task_home_list (struct task_home *treq_top, char *name)
{
  struct task_home *cur;
  char buf0[1280];
  char buf1[1280];
  char buf2[1280];
  fprintf (stderr, "%s= {", name);
  {
    cur = treq_top;
    for (; cur; cur = (*cur).next)
      {
        fprintf (stderr,
                 "{stat=%s, id=%d, exception-tag=%d, msg-cncl=%d, waiting=%s, owner=%p, eldest=%p(%d), task-no=%d, body=%p, req-from=%s, task-head=%s}, ",
                 task_home_stat_strings[(*cur).stat], (*cur).id,
                 (*cur).exception_tag, (*cur).msg_cncl,
                 (serialize_arg (buf0, (*cur).waiting_head), buf0),
                 (*cur).owner, (*cur).eldest,
                 (*cur).eldest ? (*(*cur).eldest).id : 0, (*cur).task_no,
                 (*cur).body, (node_to_string (buf1, (*cur).req_from), buf1),
                 (serialize_arg (buf2, (*cur).task_head), buf2));
      }
  }
  fprintf (stderr, "}, ");
  return;
}

void
print_thread_status (struct thread_data *thr)
{
  fprintf (stderr, "*** Worker %d ***\n", (*thr).id);
  fprintf (stderr, "req=%p, ", (*thr).req);
  fprintf (stderr, "w-rack=%d, ", (*thr).w_rack);
  fprintf (stderr, "w-none=%d, ", (*thr).w_none);
  fprintf (stderr, "ndiv=%d, ", (*thr).ndiv);
  fprintf (stderr, "exiting=%s, ", exiting_rsn_strings[(*thr).exiting]);
  fprintf (stderr, "exception-tag=%d, ", (*thr).exception_tag);
  fprintf (stderr, "probability=%lf, ", (*thr).probability);
  fprintf (stderr, "last-treq=%d, ", (*thr).last_treq);
  fprintf (stderr, "last-choose=%s, ", choose_strings[(*thr).last_choose]);
  fprintf (stderr, "random-seed(1,2)=(%f,%f), ", (*thr).random_seed1,
           (*thr).random_seed2);
  fprintf (stderr, "\n");
  print_task_list ((*thr).task_top, "tasks");
  fprintf (stderr, "\n");
  print_task_home_list ((*thr).treq_top, "treq-top");
  fprintf (stderr, "\n");
  print_task_home_list ((*thr).sub, "sub");
  fprintf (stderr, "\n");
  return;
}

void
print_status (struct cmd *pcmd)
{
  int i;
  fprintf (stderr, "worker-name: %s\n",
           option.node_name ? option.node_name : "Unnamed");
  fprintf (stderr, "num-thrs: %d\n", num_thrs);
  fprintf (stderr, "verbose-level: %d\n", option.verbose);
  {
    i = 0;
    for (; i < num_thrs; i++)
      {
        print_thread_status (&threads[i]);
      }
  }
  return;
}

void
set_verbose_level (struct cmd *pcmd)
{
  if ((*pcmd).c < 1)
    proto_error ("Wrong verb", pcmd);
  else;
  option.verbose = ((*pcmd).v)[0][0];
  return;
}

void
recv_exit (struct cmd *pcmd)
{
  fprintf (stderr, "Received \"exit\"... terminate.\n");
  exit (0);
}

struct handle_req_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  int ifexp_result7;
  int ifexp_result8;
  struct thread_data *_thr;
  closure_t *_bk;
};

void
handle_req (char *esp, closure_t * _bk, struct thread_data *_thr)
{
  int ifexp_result8;
  int ifexp_result7;
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct handle_req_frame *efp;
  char *argp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct handle_req_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct handle_req_frame) + sizeof (Align_t) +
                   -1) / sizeof (Align_t));
      *((char **) esp) = 0;
    LGOTO:switch ((*efp).call_id)
        {
        case 0:
          goto L_CALL3;
        }
      goto L_CALL3;
    }
  else;
  efp = (struct handle_req_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct handle_req_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  {
    if ((*_thr).req)
      {
        if (!(*(*_thr).task_top).cancellation)
          ifexp_result8 = 1;
        else
          ifexp_result8 = 0;
        ifexp_result7 = ifexp_result8;
      }
    else
      ifexp_result7 = 0;
    if (ifexp_result7)
      {
        (*_thr).exiting = EXITING_SPAWN;
        tcounter_change_state (_thr, TCOUNTER_SPWN, OBJ_NULL, 0);
        {
          argp =
            (char *) ((Align_t *) esp +
                      (sizeof (char *) + sizeof (Align_t) +
                       -1) / sizeof (Align_t));
          *((closure_t **) argp) = _bk;
          efp->ifexp_result7 = ifexp_result7;
          efp->ifexp_result8 = ifexp_result8;
          efp->_thr = _thr;
          efp->_bk = _bk;
          efp->argp = argp;
          efp->tmp_esp = argp;
          efp->call_id = 0;
          return;
        L_CALL3:;
          ifexp_result7 = efp->ifexp_result7;
          ifexp_result8 = efp->ifexp_result8;
          _thr = efp->_thr;
          _bk = efp->_bk;
        }
        tcounter_change_state (_thr, TCOUNTER_EXEC, OBJ_NULL, 0);
        (*_thr).exiting = EXITING_NORMAL;
        (*_thr).req = (*_thr).treq_top;
      }
    else;
  }
}

int
send_cncl_for_flagged_subtasks (struct thread_data *thr)
{
  struct cmd rcmd;
  struct task_home *cur;
  int count;
  count = 0;
  rcmd.w = CNCL;
  rcmd.c = 2;
  (rcmd.v)[0][0] = (*thr).id;
  (rcmd.v)[0][2] = TERM;
  {
    cur = (*thr).sub;
    for (; cur; cur = (*cur).next)
      {
        if ((*cur).msg_cncl == 1 && (*cur).stat == TASK_HOME_INITIALIZED)
          {
            rcmd.node = (*cur).req_from;
            (rcmd.v)[0][1] = (*cur).id;
            copy_address ((rcmd.v)[1], (*cur).task_head);
            send_command (&rcmd, 0, 0);
            (*cur).msg_cncl = 2;
            evcounter_count (thr, EV_SEND_CNCL, OBJ_PADDR, (*cur).task_head);
            count++;
          }
        else;
      }
  }
  return count;
}

struct handle_req_cncl_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  struct thread_data *_thr;
  closure_t *_bk;
};

void
handle_req_cncl (char *esp, closure_t * _bk, struct thread_data *_thr)
{
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct handle_req_cncl_frame *efp;
LGOTO:;
  efp = (struct handle_req_cncl_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct handle_req_cncl_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  if ((*_thr).req_cncl)
    {
      if (option.verbose >= 1)
        fprintf (stderr, "(%d): (Thread %d) detected cncl message request\n",
                 get_universal_real_time (), (*_thr).id);
      else;
      send_cncl_for_flagged_subtasks (_thr);
      (*_thr).req_cncl = 0;
    }
  else;
}

struct handle_exception_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  int excep;
  struct thread_data *_thr;
  closure_t *_bk;
};

void
handle_exception (char *esp, closure_t * _bk, struct thread_data *_thr,
                  int excep)
{
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct handle_exception_frame *efp;
  char *argp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct handle_exception_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct handle_exception_frame) + sizeof (Align_t) +
                   -1) / sizeof (Align_t));
      *((char **) esp) = 0;
    LGOTO:switch ((*efp).call_id)
        {
        case 0:
          goto L_CALL4;
        }
      goto L_CALL4;
    }
  else;
  efp = (struct handle_exception_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct handle_exception_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  (*_thr).exiting = EXITING_EXCEPTION;
  (*_thr).exception_tag = excep;
  tcounter_change_state (_thr, TCOUNTER_EXCP, OBJ_INT,
                         (void *) ((long) excep));
  {
    argp =
      (char *) ((Align_t *) esp +
                (sizeof (char *) + sizeof (Align_t) + -1) / sizeof (Align_t));
    *((closure_t **) argp) = _bk;
    efp->excep = excep;
    efp->_thr = _thr;
    efp->_bk = _bk;
    efp->argp = argp;
    efp->tmp_esp = argp;
    efp->call_id = 0;
    return;
  L_CALL4:;
    excep = efp->excep;
    _thr = efp->_thr;
    _bk = efp->_bk;
  }
  perror ("Unexpectedly returned from backtracking for an exception.");
}

struct handle_cancellation_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  struct thread_data *_thr;
  closure_t *_bk;
};

void
handle_cancellation (char *esp, closure_t * _bk, struct thread_data *_thr)
{
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct handle_cancellation_frame *efp;
  char *argp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct handle_cancellation_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct handle_cancellation_frame) +
                   sizeof (Align_t) + -1) / sizeof (Align_t));
      *((char **) esp) = 0;
    LGOTO:switch ((*efp).call_id)
        {
        case 0:
          goto L_CALL5;
        }
      goto L_CALL5;
    }
  else;
  efp = (struct handle_cancellation_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct handle_cancellation_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  if ((*(*_thr).task_top).cancellation)
    {
      if (option.verbose >= 1)
        fprintf (stderr,
                 "(%d): (Thread %d) detected cancellation flag (%d)\n",
                 get_universal_real_time (), (*_thr).id,
                 (*(*_thr).task_top).cancellation);
      else;
      (*_thr).exiting = EXITING_CANCEL;
      tcounter_change_state (_thr, TCOUNTER_ABRT, OBJ_INT,
                             (void *) ((long) (*(*_thr).task_top).
                                       cancellation));
      pthread_mutex_unlock (&(*_thr).mut);
      {
        argp =
          (char *) ((Align_t *) esp +
                    (sizeof (char *) + sizeof (Align_t) +
                     -1) / sizeof (Align_t));
        *((closure_t **) argp) = _bk;
        efp->_thr = _thr;
        efp->_bk = _bk;
        efp->argp = argp;
        efp->tmp_esp = argp;
        efp->call_id = 0;
        return;
      L_CALL5:;
        _thr = efp->_thr;
        _bk = efp->_bk;
      }
      perror ("Unexpectedly returned from backtracking for abortion.");
    }
  else;
}

struct handle_reqs_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  struct thread_data *_thr;
  closure_t *_bk;
};

void
handle_reqs (char *esp, closure_t * _bk, struct thread_data *_thr)
{
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct handle_reqs_frame *efp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct handle_reqs_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct handle_reqs_frame) + sizeof (Align_t) +
                   -1) / sizeof (Align_t));
      *((char **) esp) = 0;
    LGOTO:switch ((*efp).call_id)
        {
        case 0:
          goto L_CALL6;
        case 1:
          goto L_CALL7;
        case 2:
          goto L_CALL8;
        }
      goto L_CALL6;
    }
  else;
  efp = (struct handle_reqs_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct handle_reqs_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  pthread_mutex_lock (&(*_thr).mut);
  {
    new_esp = esp;
    while (handle_req (new_esp, _bk, _thr),
           __builtin_expect ((efp->tmp_esp = *((char **) esp)) != 0, 0))
      {
        efp->_thr = _thr;
        efp->_bk = _bk;
        efp->call_id = 0;
        return;
      L_CALL6:;
        _thr = efp->_thr;
        _bk = efp->_bk;
        new_esp = esp + 1;
      }
  }
  {
    new_esp = esp;
    while (handle_req_cncl (new_esp, _bk, _thr),
           __builtin_expect ((efp->tmp_esp = *((char **) esp)) != 0, 0))
      {
        efp->_thr = _thr;
        efp->_bk = _bk;
        efp->call_id = 1;
        return;
      L_CALL7:;
        _thr = efp->_thr;
        _bk = efp->_bk;
        new_esp = esp + 1;
      }
  }
  {
    new_esp = esp;
    while (handle_cancellation (new_esp, _bk, _thr),
           __builtin_expect ((efp->tmp_esp = *((char **) esp)) != 0, 0))
      {
        efp->_thr = _thr;
        efp->_bk = _bk;
        efp->call_id = 2;
        return;
      L_CALL8:;
        _thr = efp->_thr;
        _bk = efp->_bk;
        new_esp = esp + 1;
      }
  }
  pthread_mutex_unlock (&(*_thr).mut);
}

void
make_and_send_task (struct thread_data *thr, int task_no, void *body,
                    int eldest_p)
{
  struct cmd tcmd;
  struct task_home *hx;
  hx = (*thr).treq_top;
  (*thr).treq_top = (*hx).next;
  (*hx).next = (*thr).sub;
  (*thr).sub = hx;
  (*hx).task_no = task_no;
  (*hx).body = body;
  (*hx).id = (*hx).next ? (*(*hx).next).id + 1 : 0;
  (*hx).owner = (*thr).task_top;
  (*hx).eldest = eldest_p ? hx : (*(*hx).next).eldest;
  (*hx).msg_cncl = 0;
  (*hx).stat = TASK_HOME_INITIALIZED;
  tcmd.c = 4;
  tcmd.node = (*hx).req_from;
  tcmd.w = TASK;
  (tcmd.v)[0][0] = ++(*thr).ndiv;
  (tcmd.v)[0][1] = TERM;
  (tcmd.v)[1][0] = (*thr).id;
  (tcmd.v)[1][1] = (*hx).id;
  (tcmd.v)[1][2] = TERM;
  copy_address ((tcmd.v)[2], (*hx).task_head);
  (tcmd.v)[3][0] = task_no;
  (tcmd.v)[3][1] = TERM;
  evcounter_count (thr, EV_SEND_TASK, OBJ_PADDR, (*hx).task_head);
  send_command (&tcmd, body, task_no);
}

struct wait_rslt_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  struct timespec t_until;
  struct timeval now;
  enum tcounter tcnt_stat_w;
  enum tcounter tcnt_stat;
  struct task_home *sub;
  void *body;
  int ifexp_result23;
  int ifexp_result24;
  int ifexp_result13;
  int ifexp_result14;
  int ifexp_result15;
  int ifexp_result16;
  int ifexp_result9;
  int ifexp_result10;
  int ifexp_result11;
  int ifexp_result12;
  int ifexp_result19;
  int ifexp_result20;
  int ifexp_result21;
  int ifexp_result22;
  enum tcounter ifexp_result17;
  enum tcounter ifexp_result18;
  int stback;
  struct thread_data *thr;
};

void *
wait_rslt (char *esp, struct thread_data *thr, int stback)
{
  enum tcounter ifexp_result18;
  enum tcounter ifexp_result17;
  int ifexp_result22;
  int ifexp_result21;
  int ifexp_result20;
  int ifexp_result19;
  int ifexp_result12;
  int ifexp_result11;
  int ifexp_result10;
  int ifexp_result9;
  int ifexp_result16;
  int ifexp_result15;
  int ifexp_result14;
  int ifexp_result13;
  int ifexp_result24;
  int ifexp_result23;
  void *body;
  struct task_home *sub;
  enum tcounter tcnt_stat;
  enum tcounter tcnt_stat_w;
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct wait_rslt_frame *efp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct wait_rslt_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct wait_rslt_frame) + sizeof (Align_t) +
                   -1) / sizeof (Align_t));
      *((char **) esp) = 0;
    LGOTO:switch ((*efp).call_id)
        {
        case 0:
          goto L_CALL9;
        }
      goto L_CALL9;
    }
  else;
  efp = (struct wait_rslt_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct wait_rslt_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  pthread_mutex_lock (&(*thr).mut);
  tcnt_stat = (*thr).tcnt_stat;
  sub = (*thr).sub;
  {
    if ((*sub).stat != TASK_HOME_DONE)
      {
        {
          if ((*sub).stat != TASK_HOME_EXCEPTION)
            {
              if ((*sub).stat != TASK_HOME_ABORTED)
                ifexp_result12 = 1;
              else
                ifexp_result12 = 0;
              ifexp_result11 = ifexp_result12;
            }
          else
            ifexp_result11 = 0;
          if (ifexp_result11)
            ifexp_result10 = 1;
          else
            ifexp_result10 = 0;
        }
        ifexp_result9 = ifexp_result10;
      }
    else
      ifexp_result9 = 0;
    if (ifexp_result9)
      {
        goto loop_start;
        while (1)
          {
            if ((*sub).stat != TASK_HOME_DONE)
              {
                {
                  if ((*sub).stat != TASK_HOME_EXCEPTION)
                    {
                      if ((*sub).stat != TASK_HOME_ABORTED)
                        ifexp_result16 = 1;
                      else
                        ifexp_result16 = 0;
                      ifexp_result15 = ifexp_result16;
                    }
                  else
                    ifexp_result15 = 0;
                  if (ifexp_result15)
                    ifexp_result14 = 1;
                  else
                    ifexp_result14 = 0;
                }
                ifexp_result13 = ifexp_result14;
              }
            else
              ifexp_result13 = 0;
            if (!ifexp_result13)
              goto loop_end;
            else;
          loop_start:;
            {
              if (tcnt_stat == TCOUNTER_EXEC)
                ifexp_result17 = TCOUNTER_WAIT;
              else
                {
                  if (tcnt_stat == TCOUNTER_ABRT)
                    ifexp_result18 = TCOUNTER_ABRT_WAIT;
                  else
                    ifexp_result18 = TCOUNTER_EXCP_WAIT;
                  ifexp_result17 = ifexp_result18;
                }
              tcnt_stat_w = ifexp_result17;
            }
            tcounter_change_state (thr, tcnt_stat_w, OBJ_NULL, 0);
            (*(*thr).task_top).stat = TASK_SUSPENDED;
            if ((*thr).exiting == EXITING_EXCEPTION)
              {
                set_cancelled (thr, (*thr).task_top, (*sub).eldest);
                send_cncl_for_flagged_subtasks (thr);
              }
            else;
            if (OUTSIDE == (*sub).req_from)
              {
                gettimeofday (&efp->now, 0);
                timeval_plus_nsec_to_timespec (&efp->t_until, &efp->now,
                                               1000);
                pthread_cond_timedwait (&(*thr).cond_r, &(*thr).mut,
                                        &efp->t_until);
              }
            else;
            {
              if ((*sub).stat == TASK_HOME_DONE)
                ifexp_result19 = 1;
              else
                {
                  {
                    if ((*sub).stat == TASK_HOME_EXCEPTION)
                      ifexp_result21 = 1;
                    else
                      {
                        if ((*sub).stat == TASK_HOME_ABORTED)
                          ifexp_result22 = 1;
                        else
                          ifexp_result22 = 0;
                        ifexp_result21 = ifexp_result22;
                      }
                    if (ifexp_result21)
                      ifexp_result20 = 1;
                    else
                      ifexp_result20 = 0;
                  }
                  ifexp_result19 = ifexp_result20;
                }
              if (ifexp_result19)
                break;
              else;
            }
            if (stback)
              {
                tcounter_change_state (thr, TCOUNTER_TREQ_BK, OBJ_ADDR,
                                       (*sub).task_head);
                {
                  new_esp = esp;
                  while (recv_exec_send
                         (new_esp, thr, (*sub).task_head, (*sub).req_from),
                         __builtin_expect ((efp->tmp_esp =
                                            *((char **) esp)) != 0, 0))
                    {
                      efp->tcnt_stat_w = tcnt_stat_w;
                      efp->tcnt_stat = tcnt_stat;
                      efp->sub = sub;
                      efp->body = body;
                      efp->ifexp_result23 = ifexp_result23;
                      efp->ifexp_result24 = ifexp_result24;
                      efp->ifexp_result13 = ifexp_result13;
                      efp->ifexp_result14 = ifexp_result14;
                      efp->ifexp_result15 = ifexp_result15;
                      efp->ifexp_result16 = ifexp_result16;
                      efp->ifexp_result9 = ifexp_result9;
                      efp->ifexp_result10 = ifexp_result10;
                      efp->ifexp_result11 = ifexp_result11;
                      efp->ifexp_result12 = ifexp_result12;
                      efp->ifexp_result19 = ifexp_result19;
                      efp->ifexp_result20 = ifexp_result20;
                      efp->ifexp_result21 = ifexp_result21;
                      efp->ifexp_result22 = ifexp_result22;
                      efp->ifexp_result17 = ifexp_result17;
                      efp->ifexp_result18 = ifexp_result18;
                      efp->stback = stback;
                      efp->thr = thr;
                      efp->call_id = 0;
                      return (void *) 0 - 1;
                    L_CALL9:;
                      tcnt_stat_w = efp->tcnt_stat_w;
                      tcnt_stat = efp->tcnt_stat;
                      sub = efp->sub;
                      body = efp->body;
                      ifexp_result23 = efp->ifexp_result23;
                      ifexp_result24 = efp->ifexp_result24;
                      ifexp_result13 = efp->ifexp_result13;
                      ifexp_result14 = efp->ifexp_result14;
                      ifexp_result15 = efp->ifexp_result15;
                      ifexp_result16 = efp->ifexp_result16;
                      ifexp_result9 = efp->ifexp_result9;
                      ifexp_result10 = efp->ifexp_result10;
                      ifexp_result11 = efp->ifexp_result11;
                      ifexp_result12 = efp->ifexp_result12;
                      ifexp_result19 = efp->ifexp_result19;
                      ifexp_result20 = efp->ifexp_result20;
                      ifexp_result21 = efp->ifexp_result21;
                      ifexp_result22 = efp->ifexp_result22;
                      ifexp_result17 = efp->ifexp_result17;
                      ifexp_result18 = efp->ifexp_result18;
                      stback = efp->stback;
                      thr = efp->thr;
                      new_esp = esp + 1;
                    }
                }
                tcounter_change_state (thr, tcnt_stat_w, OBJ_NULL, 0);
              }
            else
              pthread_cond_wait (&(*thr).cond_r, &(*thr).mut);
          }
      loop_end:;
      }
    else;
  }
  if ((*sub).stat == TASK_HOME_EXCEPTION)
    {
      (*thr).exiting = EXITING_EXCEPTION;
      (*thr).exception_tag = (*sub).exception_tag;
      ((*(*sub).owner).cancellation)--;
      tcnt_stat = TCOUNTER_EXCP;
    }
  else;
  tcounter_change_state (thr, tcnt_stat, OBJ_NULL, 0);
  {
    if ((*sub).stat == TASK_HOME_EXCEPTION)
      ifexp_result23 = 1;
    else
      {
        if ((*sub).stat == TASK_HOME_ABORTED)
          ifexp_result24 = 1;
        else
          ifexp_result24 = 0;
        ifexp_result23 = ifexp_result24;
      }
    if (ifexp_result23)
      {
        free (body);
        body = 0;
      }
    else
      body = (*sub).body;
  }
  (*thr).sub = (*sub).next;
  (*sub).next = (*thr).treq_free;
  (*thr).treq_free = sub;
  (*(*thr).task_top).stat = TASK_STARTED;
  pthread_mutex_unlock (&(*thr).mut);
  return body;
}

int tcell_bcst_wait_bcak = 1;

void
broadcast_task (struct thread_data *thr, int task_no, void *body)
{
  struct cmd bcmd;
  bcmd.c = 2;
  bcmd.node = OUTSIDE;
  bcmd.w = BCST;
  (bcmd.v)[0][0] = (*thr).id;
  (bcmd.v)[0][1] = TERM;
  (bcmd.v)[1][0] = task_no;
  (bcmd.v)[1][1] = TERM;
  send_command (&bcmd, body, task_no);
  if (tcell_bcst_wait_bcak)
    {
      pthread_mutex_lock (&(*thr).mut);
      (*thr).w_bcak = 1;
      while ((*thr).w_bcak)
        {
          pthread_cond_wait (&(*thr).cond, &(*thr).mut);
        }
      pthread_mutex_unlock (&(*thr).mut);
    }
  else;
}

void
usage (int argc, char **argv)
{
  fprintf (stderr,
           "Usage: %s [-s hostname] [-p port-num] [-n n-threads] [-i initial-task-parms] [-a] [-v verbosity] [-T timechart-prefix]\n",
           argv[0]);
  exit (1);
}

void
set_option (int argc, char **argv)
{
  int i;
  int ch;
  FILE *fp;
  char buf5[256];
  char *command2;
  option.sv_hostname = 0;
  option.port = 9865;
  option.num_thrs = 1;
  option.node_name = 0;
  option.initial_task = 0;
  option.auto_exit = 0;
  option.affinity = 0;
  option.always_flush_accepted_treq = 0;
  option.verbose = 0;
  option.timechart_file = 0;
  while (-1 != (ch = getopt (argc, argv, "n:s:p:N:i:xafP:v:T:h")))
    {
      switch (ch)
        {
        case 'n':
          option.num_thrs = atoi (optarg);
          break;
        case 's':
          option.sv_hostname = strcmp ("stdout", optarg) ? optarg : 0;
          break;
        case 'p':
          option.port = atoi (optarg);
          break;
        case 'N':
          if (option.node_name)
            free (option.node_name);
          else;
          option.node_name =
            (char *) malloc ((1 + strlen (optarg)) * sizeof (char));
          strcpy (option.node_name, optarg);
          break;
        case 'i':
          if (option.initial_task)
            free (option.initial_task);
          else;
          option.initial_task =
            (char *) malloc ((1 + strlen (optarg)) * sizeof (char));
          strcpy (option.initial_task, optarg);
          option.auto_exit = 1;
          break;
        case 'x':
          option.auto_exit = 1;
          break;
        case 'a':
          if (0 == option.affinity)
            fprintf (stderr, "setaffinity enabled.\n");
          else;
          option.affinity = 1;
          break;
        case 'f':
          option.always_flush_accepted_treq = 1;
          break;
        case 'v':
          option.verbose = atoi (optarg);
          break;
        case 'T':
          command2 = "hostname -s";
          if ((fp = popen (command2, "r")) == NULL)
            {
              fprintf (stderr, "popen error!\n");
              exit (EXIT_FAILURE);
            }
          else;
          fgets (buf5, 256, fp);
          strtok (buf5, "\n");
          (void) pclose (fp);
          {
            option.timechart_file = optarg;
            strcat (option.timechart_file, buf5);
          }
          break;
        case 'h':
          usage (argc, argv);
          break;
        default:
          fprintf (stderr, "Unknown option: %c\n", ch);
          usage (argc, argv);
          break;
        }
    }
  return;
}

void
initialize_task_list (struct task *tlist, int len, struct task **p_top,
                      struct task **p_free)
{
  int i;
  *p_top = 0;
  *p_free = tlist;
  {
    i = 0;
    for (; i < len - 1; i++)
      {
        (tlist[i]).prev = &tlist[i + 1];
        (tlist[i + 1]).next = &tlist[i];
      }
  }
  (tlist[0]).next = 0;
  (tlist[len - 1]).prev = 0;
  {
    i = 0;
    for (; i < len; i++)
      {
        (tlist[i]).rslt_to = TERM;
        ((tlist[i]).rslt_head)[0] = TERM;
      }
  }
  return;
}

void
initialize_task_home_list (struct task_home *hlist, int len,
                           struct task_home **p_top,
                           struct task_home **p_free)
{
  int i;
  *p_top = 0;
  *p_free = hlist;
  {
    i = 0;
    for (; i < len - 1; i++)
      {
        (hlist[i]).next = &hlist[i + 1];
        (hlist[len - 1]).next = 0;
      }
  }
  return;
}

void
set_aux_data (struct aux_data *paux, enum obj_type aux_type, void *aux_body)
{
  (*paux).type = aux_type;
  switch (aux_type)
    {
    case OBJ_INT:
      (*paux).body.aux_int = (long) aux_body;
      break;
    case OBJ_ADDR:
      copy_address ((*paux).body.aux_addr, (enum addr *) aux_body);
      break;
    case OBJ_PADDR:
      (*paux).body.aux_paddr = (enum addr *) aux_body;
      break;
    }
}

void
print_aux_data (FILE * fp, struct aux_data *paux)
{
  char buf6[1280];
  switch ((*paux).type)
    {
    case OBJ_INT:
      fprintf (fp, " %d", (*paux).body.aux_int);
      break;
    case OBJ_ADDR:
      serialize_arg (buf6, (*paux).body.aux_addr);
      fputc (' ', fp);
      fputs (buf6, fp);
      break;
    case OBJ_PADDR:
      serialize_arg (buf6, (*paux).body.aux_paddr);
      fputc (' ', fp);
      fputs (buf6, fp);
      break;
    }
}

void
initialize_tcounter (struct thread_data *thr)
{
  int i;
  struct timeval tp;
  (*thr).tcnt_stat = TCOUNTER_INIT;
  gettimeofday (&tp, 0);
  {
    i = 0;
    for (; i < 10; i++)
      {
        ((*thr).tcnt)[i] = 0;
        ((*thr).tcnt_tp)[i] = tp;
      }
  }
  (*thr).tc_aux.type = OBJ_NULL;
}

void
tcounter_start (struct thread_data *thr, enum tcounter tcnt_stat)
{
  struct timeval tp;
  gettimeofday (&tp, 0);
  ((*thr).tcnt_tp)[tcnt_stat] = tp;
}

void
tcounter_end (struct thread_data *thr, enum tcounter tcnt_stat)
{
  struct timeval tp;
  gettimeofday (&tp, 0);
  ((*thr).tcnt)[tcnt_stat] +=
    diff_timevals (&tp, &((*thr).tcnt_tp)[tcnt_stat]);
  ((*thr).tcnt_tp)[tcnt_stat] = tp;
}

enum tcounter
tcounter_change_state (struct thread_data *thr, enum tcounter tcnt_stat,
                       enum obj_type aux_type, void *aux_body)
{
  struct timeval tp;
  enum tcounter tcnt_stat0;
  double tcnt0;
  double tcnt;
  struct timeval *tp0;
  tcnt_stat0 = (*thr).tcnt_stat;
  if (tcnt_stat0 != tcnt_stat)
    {
      tcnt0 = ((*thr).tcnt)[tcnt_stat0];
      gettimeofday (&tp, 0);
      tcnt = tcnt0 + diff_timevals (&tp, &((*thr).tcnt_tp)[tcnt_stat0]);
      ((*thr).tcnt)[tcnt_stat0] = tcnt;
      ((*thr).tcnt_tp)[tcnt_stat] = tp;
      (*thr).tcnt_stat = tcnt_stat;
      if ((*thr).fp_tc)
        {
          tp0 = &((*thr).tcnt_tp)[tcnt_stat0];
          if (tcnt_stat0 == TCOUNTER_INIT && (*thr).id == 0)
            tp_strt = tp;
          else;
          fprintf ((*thr).fp_tc, "%s %lf %lf", tcounter_strings[tcnt_stat0],
                   diff_timevals (tp0, &tp_strt), diff_timevals (&tp,
                                                                 &tp_strt));
          fputc (' ', (*thr).fp_tc);
          print_aux_data ((*thr).fp_tc, &(*thr).tc_aux);
          fputc ('\n', (*thr).fp_tc);
          set_aux_data (&(*thr).tc_aux, aux_type, aux_body);
        }
      else;
    }
  else;
  return tcnt_stat0;
}

void
initialize_evcounter (struct thread_data *thr)
{
  int i;
  {
    i = 0;
    for (; i < 6; i++)
      {
        ((*thr).ev_cnt)[i] = 0;
      }
  }
}

int
evcounter_count (struct thread_data *thr, enum event ev,
                 enum obj_type aux_type, void *aux_body)
{
  struct timeval tp;
  struct aux_data aux;
  (((*thr).ev_cnt)[ev])++;
  if ((*thr).fp_tc)
    {
      gettimeofday (&tp, 0);
      fprintf ((*thr).fp_tc, "%s %lf", ev_strings[ev],
               diff_timevals (&tp, &tp_strt));
      fputc (' ', (*thr).fp_tc);
      set_aux_data (&aux, aux_type, aux_body);
      print_aux_data ((*thr).fp_tc, &aux);
      fputc ('\n', (*thr).fp_tc);
    }
  else;
  return ((*thr).ev_cnt)[ev];
}

void
show_counters ()
{
  int i;
  int j;
  struct thread_data *thr;
  {
    i = 0;
    for (; i < num_thrs; i++)
      {
        fprintf (stderr, "*** Worker %d ***\n", i);
        thr = threads + i;
        {
          j = 0;
          for (; j < 10; j++)
            {
              fprintf (stderr, "%s: %lf\n", tcounter_strings[j],
                       ((*thr).tcnt)[j]);
            }
        }
        {
          j = 0;
          for (; j < 6; j++)
            {
              fprintf (stderr, "%s: %ld\n", ev_strings[j],
                       ((*thr).ev_cnt)[j]);
            }
        }
      }
  }
  return;
}

struct main_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  char *header;
  char *p_dst;
  char *p_src;
  struct thread_data *thr2;
  int len;
  char *fname;
  double q;
  double r;
  struct task_home *hx;
  struct task *tx;
  struct thread_data *thr;
  pthread_mutexattr_t m_attr;
  struct cmd *pcmd;
  int j;
  int i;
  int ifexp_result25;
  int ifexp_result26;
  char **argv;
  int argc;
};

int
main (int argc, char **argv)
{
  int ifexp_result26;
  int ifexp_result25;
  int i;
  int j;
  struct cmd *pcmd;
  struct thread_data *thr;
  struct task *tx;
  struct task_home *hx;
  double r;
  double q;
  char *fname;
  int len;
  struct thread_data *thr2;
  char *p_src;
  char *p_dst;
  char *header;
  char estack[262144];
  char *esp = estack;
  char *new_esp;
  struct main_frame *efp;
LGOTO:;
  efp = (struct main_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct main_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  fprintf (stderr,
           "compile-time options: VERBOSE=1 PROFILE=1 NF-TYPE=LW-SC USE-AFFINITY=SCHED\n");
  set_option (argc, argv);
  {
    if (option.sv_hostname)
      ifexp_result25 = connect_to (option.sv_hostname, option.port);
    else
      ifexp_result25 = -1;
    sv_socket = ifexp_result25;
  }
  pthread_mutexattr_init (&efp->m_attr);
  pthread_mutex_init (&send_mut, &efp->m_attr);
  num_thrs = option.num_thrs;
  {
    i = 0;
    for (; i < num_thrs; i++)
      {
        thr = threads + i;
        (*thr).req = 0;
        (*thr).req_cncl = 0;
        (*thr).id = i;
        (*thr).w_rack = 0;
        (*thr).w_none = 0;
        (*thr).w_bcak = 0;
        (*thr).ndiv = 0;
        (*thr).probability = 1.0;
        (*thr).last_treq = i;
        (*thr).last_choose = CHS_RANDOM;
        (*thr).exiting = EXITING_NORMAL;
        (*thr).exception_tag = 0;
        {
          r = sqrt (0.5 + i);
          q = sqrt (r + i);
          r -= (int) r;
          (*thr).random_seed1 = r;
          (*thr).random_seed2 = q;
        }
        ((*thr).random_seed_probability)[0] = 3 + 3 * i;
        ((*thr).random_seed_probability)[1] = 4 + 3 * i;
        ((*thr).random_seed_probability)[2] = 5 + 3 * i;
        pthread_mutex_init (&(*thr).mut, &efp->m_attr);
        pthread_mutex_init (&(*thr).rack_mut, &efp->m_attr);
        pthread_cond_init (&(*thr).cond, 0);
        pthread_cond_init (&(*thr).cond_r, 0);
        tx = (struct task *) malloc (sizeof (struct task) * (4 * 65536));
        initialize_task_list (tx, 4 * 65536, &(*thr).task_top,
                              &(*thr).task_free);
        hx =
          (struct task_home *) malloc (sizeof (struct task_home) *
                                       (4 * 65536));
        initialize_task_home_list (hx, 4 * 65536, &(*thr).treq_top,
                                   &(*thr).treq_free);
        (*thr).sub = 0;
        if (option.timechart_file)
          {
            len = strlen (option.timechart_file) + 10;
            fname = malloc (sizeof (char) * len);
            snprintf (fname, len, "%s-%04d.dat", option.timechart_file,
                      (*thr).id);
            (*thr).fp_tc = fopen (fname, "w");
            if (!(*thr).fp_tc)
              perror ("Failed to open timechart-file for writing");
            else;
            free (fname);
          }
        else
          (*thr).fp_tc = 0;
      }
  }
  {
    i = 0;
    for (; i < num_thrs; i++)
      {
        thr2 = threads + i;
        initialize_evcounter (thr2);
        initialize_tcounter (thr2);
        {
          new_esp = esp;
          while (systhr_create (new_esp, &(*thr2).pthr_id, worker, thr2),
                 __builtin_expect ((efp->tmp_esp = *((char **) esp)) != 0, 0))
            {
              char *goto_fr;
              *((char **) esp) = 0;
              efp->header = header;
              efp->p_dst = p_dst;
              efp->p_src = p_src;
              efp->thr2 = thr2;
              efp->len = len;
              efp->fname = fname;
              efp->q = q;
              efp->r = r;
              efp->hx = hx;
              efp->tx = tx;
              efp->thr = thr;
              efp->pcmd = pcmd;
              efp->j = j;
              efp->i = i;
              efp->ifexp_result25 = ifexp_result25;
              efp->ifexp_result26 = ifexp_result26;
              efp->argv = argv;
              efp->argc = argc;
              goto_fr = lw_call (efp->tmp_esp);
              if ((char *) goto_fr == (char *) efp)
                goto LGOTO;
              else;
              header = efp->header;
              p_dst = efp->p_dst;
              p_src = efp->p_src;
              thr2 = efp->thr2;
              len = efp->len;
              fname = efp->fname;
              q = efp->q;
              r = efp->r;
              hx = efp->hx;
              tx = efp->tx;
              thr = efp->thr;
              pcmd = efp->pcmd;
              j = efp->j;
              i = efp->i;
              ifexp_result25 = efp->ifexp_result25;
              ifexp_result26 = efp->ifexp_result26;
              argv = efp->argv;
              argc = efp->argc;
              new_esp = esp + 1;
            }
        }
      }
  }
  if (option.initial_task)
    {
      header = "task 0 0 0 ";
      receive_buf =
        (char *) malloc ((3 + strlen (option.initial_task) + strlen (header))
                         * sizeof (char));
      receive_buf_p = receive_buf;
      strcpy (receive_buf, header);
      {
        p_src = option.initial_task, p_dst = receive_buf + strlen (header);
        for (; *p_src; (p_src++, p_dst++))
          {
            if (' ' == *p_src)
              ifexp_result26 = '\n';
            else
              ifexp_result26 = *p_src;
            *p_dst = ifexp_result26;
          }
      }
      *p_dst++ = '\n';
      *p_dst++ = '\n';
      *p_dst = 0;
      sleep (1);
      if (option.verbose >= 1)
        fprintf (stderr, "%s", receive_buf);
      else;
    }
  else;
  while (1)
    {
      pcmd = read_command ();
      if (pcmd)
        {
          proc_cmd (pcmd, 0);
        }
      else
        while (1)
          {
            sleep (2147483647);
          }
    }
  exit (0);
}
