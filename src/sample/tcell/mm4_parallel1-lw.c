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
struct do_mm_start_task_frame;
struct _bk_in_do_mm_start_task_frame;
struct do_BlockTask_task_frame;
struct _bk_in_do_BlockTask_task_frame;
struct do_matAddTask_task_frame;
struct _bk_in_do_matAddTask_task_frame;
struct BlockMul_frame;
struct BlockMul_parallel_frame;
struct do_many_bk_in_BlockMul_parallel_frame;
struct matAdd_parallel_frame;
struct do_many_bk2_in_matAdd_parallel_frame;
#include <pthread.h>
#include <stdio.h>
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
#include "sendrecv.h"
#include <stdlib.h>

void handle_reqs (char *esp, closure_t *, struct thread_data *);

void handle_exception (char *esp, closure_t *, struct thread_data *, long);
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

void BlockMul_parallel (char *esp, closure_t * _bk, struct thread_data *_thr,
                        double *(*param)[3], int n, int i1, int i2);

void BlockMul (char *esp, closure_t * _bk, struct thread_data *_thr,
               double *X, double *Y, double *Z, int m);

void matAdd_parallel (char *esp, closure_t * _bk, struct thread_data *_thr,
                      double *A, double *B, double *C, int i1, int i2);

void matMul (double *A, double *B, double *C, int n);

void matAdd (double *A, double *B, double *C, int m);

double get_wall_time (void);

/* Error!
rule::no-match
*/

struct mm_start
{
  char _dummy_[1000];
};

struct BlockTask
{
  double *(*param)[3];
  int n;
  int i1;
  int i2;
  char _dummy_[1000];
};

struct matAddTask
{
  double *A;
  double *B;
  double *C;
  int i1;
  int i2;
  char _dummy_[1000];
};

struct _bk_in_do_mm_start_task_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  int ifexp_result;
  int ifexp_result2;
  struct do_mm_start_task_frame *xfp;
};

struct do_mm_start_task_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  int flag;
  double *C;
  double *B;
  double *A;
  double *W;
  double *Z;
  double *Y;
  double *X;
  double elapsed2;
  double elapsed;
  double end2;
  double start2;
  double end;
  double start;
  int j;
  int i;
  int N;
  int ifexp_result3;
  int ifexp_result4;
  int ifexp_result5;
  int ifexp_result6;
  int ifexp_result7;
  int ifexp_result8;
  struct mm_start *pthis;
  struct thread_data *_thr;
  closure_t _bk;
};

char *
_bk_in_do_mm_start_task (char *esp, void *xfp0)
{
  int ifexp_result2;
  int ifexp_result;
  char *new_esp;
  struct _bk_in_do_mm_start_task_frame *efp;
  struct do_mm_start_task_frame *xfp = xfp0;
  size_t esp_flag = (size_t) esp & 3;
  char *parmp = (char *) ((size_t) esp ^ esp_flag);
LGOTO:;
  efp = (struct _bk_in_do_mm_start_task_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct _bk_in_do_mm_start_task_frame) +
               sizeof (Align_t) + -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  {
    if ((*xfp->_thr).exiting == EXITING_EXCEPTION)
      ifexp_result = 1;
    else
      {
        if ((*xfp->_thr).exiting == EXITING_CANCEL)
          ifexp_result2 = 1;
        else
          ifexp_result2 = 0;
        ifexp_result = ifexp_result2;
      }
    if (ifexp_result)
      {
        fprintf (stderr,
                 "Error: Exception is thrown but this program is complied invalidating exception support.\n");
        exit (99);
      }
    else;
  }
  {
    *((int *) efp) = 0;
    return 0;
  }
  return 0;
}

void
do_mm_start_task (char *esp, struct thread_data *_thr, struct mm_start *pthis)
{
  int ifexp_result8;
  int ifexp_result7;
  int ifexp_result6;
  int ifexp_result5;
  int ifexp_result4;
  int ifexp_result3;
  int N;
  int i;
  int j;
  double start;
  double end;
  double start2;
  double end2;
  double elapsed;
  double elapsed2;
  double *X;
  double *Y;
  double *Z;
  double *W;
  double *A;
  double *B;
  double *C;
  int flag;
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct do_mm_start_task_frame *efp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct do_mm_start_task_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct do_mm_start_task_frame) + sizeof (Align_t) +
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
  efp = (struct do_mm_start_task_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct do_mm_start_task_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  fprintf (stderr, "mm4_parallel1\'s result is as follows:   \n");
  fprintf (stderr,
           "--------------------------worker=1-----------------------------------------------\n");
  N = 2048;
  fprintf (stderr, "Matrix size = %2d    ", N);
  X = malloc (sizeof (double) * (N * N));
  Y = malloc (sizeof (double) * (N * N));
  Z = malloc (sizeof (double) * (N * N));
  W = malloc (sizeof (double) * (N * N));
  {
    if (X == NULL)
      ifexp_result3 = 1;
    else
      {
        {
          if (Y == NULL)
            ifexp_result5 = 1;
          else
            {
              {
                if (Z == NULL)
                  ifexp_result7 = 1;
                else
                  {
                    if (W == NULL)
                      ifexp_result8 = 1;
                    else
                      ifexp_result8 = 0;
                    ifexp_result7 = ifexp_result8;
                  }
                if (ifexp_result7)
                  ifexp_result6 = 1;
                else
                  ifexp_result6 = 0;
              }
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
      {
        fprintf (stderr, "Out of memory (1)\n");
        exit (1);
      }
    else;
  }
  srand (0);
  {
    i = 0;
    for (; i < N * N; i++)
      {
        X[i] = rand () / (RAND_MAX + 1.0);
        Y[i] = rand () / (RAND_MAX + 1.0);
      }
  }
  start = get_wall_time ();
  matMul (X, Y, W, N);
  end = get_wall_time ();
  elapsed = end - start;
  fprintf (stderr, "naive = %5.4f    ", elapsed);
  start2 = get_wall_time ();
  {
    new_esp = esp;
    while (BlockMul (new_esp, &efp->_bk, _thr, X, Y, Z, N),
           __builtin_expect ((efp->tmp_esp = *((char **) esp)) != 0, 0))
      {
        efp->flag = flag;
        efp->C = C;
        efp->B = B;
        efp->A = A;
        efp->W = W;
        efp->Z = Z;
        efp->Y = Y;
        efp->X = X;
        efp->elapsed2 = elapsed2;
        efp->elapsed = elapsed;
        efp->end2 = end2;
        efp->start2 = start2;
        efp->end = end;
        efp->start = start;
        efp->j = j;
        efp->i = i;
        efp->N = N;
        efp->ifexp_result3 = ifexp_result3;
        efp->ifexp_result4 = ifexp_result4;
        efp->ifexp_result5 = ifexp_result5;
        efp->ifexp_result6 = ifexp_result6;
        efp->ifexp_result7 = ifexp_result7;
        efp->ifexp_result8 = ifexp_result8;
        efp->pthis = pthis;
        efp->_thr = _thr;
        efp->_bk.fun = _bk_in_do_mm_start_task;
        efp->_bk.fr = (void *) efp;
        efp->call_id = 0;
        return;
      L_CALL:;
        flag = efp->flag;
        C = efp->C;
        B = efp->B;
        A = efp->A;
        W = efp->W;
        Z = efp->Z;
        Y = efp->Y;
        X = efp->X;
        elapsed2 = efp->elapsed2;
        elapsed = efp->elapsed;
        end2 = efp->end2;
        start2 = efp->start2;
        end = efp->end;
        start = efp->start;
        j = efp->j;
        i = efp->i;
        N = efp->N;
        ifexp_result3 = efp->ifexp_result3;
        ifexp_result4 = efp->ifexp_result4;
        ifexp_result5 = efp->ifexp_result5;
        ifexp_result6 = efp->ifexp_result6;
        ifexp_result7 = efp->ifexp_result7;
        ifexp_result8 = efp->ifexp_result8;
        pthis = efp->pthis;
        _thr = efp->_thr;
        new_esp = esp + 1;
      }
  }
  end2 = get_wall_time ();
  elapsed2 = end2 - start2;
  fprintf (stderr, "BlockMul = %5.4f  \n", elapsed2);
  flag = 0;
  {
    i = 0;
    for (; i < N; i++)
      {
        {
          j = 0;
          for (; j < N; j++)
            {
              if (fabs (Z[i * N + j] - W[i * N + j]) > 1.0E-6)
                {
                  fprintf (stderr, "THE ANSWER IS WRONG!\n");
                  flag = 1;
                  break;
                }
              else;
            }
        }
        if (flag == 1)
          break;
        else;
      }
  }
  if (flag == 0)
    fprintf (stderr, "THE ANSWER IS RIGHT!\n");
  else;
  free (W);
  free (Z);
  free (Y);
  free (X);
  fprintf (stderr,
           "-------------------------------------------------------------------------\n\n\n");
mm_start_exit:return;
}

struct _bk_in_do_BlockTask_task_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  int ifexp_result9;
  int ifexp_result10;
  struct do_BlockTask_task_frame *xfp;
};

struct do_BlockTask_task_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  struct BlockTask *pthis;
  struct thread_data *_thr;
  closure_t _bk;
};

char *
_bk_in_do_BlockTask_task (char *esp, void *xfp0)
{
  int ifexp_result10;
  int ifexp_result9;
  char *new_esp;
  struct _bk_in_do_BlockTask_task_frame *efp;
  struct do_BlockTask_task_frame *xfp = xfp0;
  size_t esp_flag = (size_t) esp & 3;
  char *parmp = (char *) ((size_t) esp ^ esp_flag);
LGOTO:;
  efp = (struct _bk_in_do_BlockTask_task_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct _bk_in_do_BlockTask_task_frame) +
               sizeof (Align_t) + -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  {
    if ((*xfp->_thr).exiting == EXITING_EXCEPTION)
      ifexp_result9 = 1;
    else
      {
        if ((*xfp->_thr).exiting == EXITING_CANCEL)
          ifexp_result10 = 1;
        else
          ifexp_result10 = 0;
        ifexp_result9 = ifexp_result10;
      }
    if (ifexp_result9)
      {
        fprintf (stderr,
                 "Error: Exception is thrown but this program is complied invalidating exception support.\n");
        exit (99);
      }
    else;
  }
  {
    *((int *) efp) = 0;
    return 0;
  }
  return 0;
}

void
do_BlockTask_task (char *esp, struct thread_data *_thr,
                   struct BlockTask *pthis)
{
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct do_BlockTask_task_frame *efp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct do_BlockTask_task_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct do_BlockTask_task_frame) +
                   sizeof (Align_t) + -1) / sizeof (Align_t));
      *((char **) esp) = 0;
    LGOTO:switch ((*efp).call_id)
        {
        case 0:
          goto L_CALL2;
        }
      goto L_CALL2;
    }
  else;
  efp = (struct do_BlockTask_task_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct do_BlockTask_task_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  {
    new_esp = esp;
    while (BlockMul_parallel
           (new_esp, &efp->_bk, _thr, (*pthis).param, (*pthis).n, (*pthis).i1,
            (*pthis).i2), __builtin_expect ((efp->tmp_esp =
                                             *((char **) esp)) != 0, 0))
      {
        efp->pthis = pthis;
        efp->_thr = _thr;
        efp->_bk.fun = _bk_in_do_BlockTask_task;
        efp->_bk.fr = (void *) efp;
        efp->call_id = 0;
        return;
      L_CALL2:;
        pthis = efp->pthis;
        _thr = efp->_thr;
        new_esp = esp + 1;
      }
  }
BlockTask_exit:return;
}

struct _bk_in_do_matAddTask_task_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  int ifexp_result11;
  int ifexp_result12;
  struct do_matAddTask_task_frame *xfp;
};

struct do_matAddTask_task_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  struct matAddTask *pthis;
  struct thread_data *_thr;
  closure_t _bk;
};

char *
_bk_in_do_matAddTask_task (char *esp, void *xfp0)
{
  int ifexp_result12;
  int ifexp_result11;
  char *new_esp;
  struct _bk_in_do_matAddTask_task_frame *efp;
  struct do_matAddTask_task_frame *xfp = xfp0;
  size_t esp_flag = (size_t) esp & 3;
  char *parmp = (char *) ((size_t) esp ^ esp_flag);
LGOTO:;
  efp = (struct _bk_in_do_matAddTask_task_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct _bk_in_do_matAddTask_task_frame) +
               sizeof (Align_t) + -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  {
    if ((*xfp->_thr).exiting == EXITING_EXCEPTION)
      ifexp_result11 = 1;
    else
      {
        if ((*xfp->_thr).exiting == EXITING_CANCEL)
          ifexp_result12 = 1;
        else
          ifexp_result12 = 0;
        ifexp_result11 = ifexp_result12;
      }
    if (ifexp_result11)
      {
        fprintf (stderr,
                 "Error: Exception is thrown but this program is complied invalidating exception support.\n");
        exit (99);
      }
    else;
  }
  {
    *((int *) efp) = 0;
    return 0;
  }
  return 0;
}

void
do_matAddTask_task (char *esp, struct thread_data *_thr,
                    struct matAddTask *pthis)
{
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct do_matAddTask_task_frame *efp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct do_matAddTask_task_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct do_matAddTask_task_frame) +
                   sizeof (Align_t) + -1) / sizeof (Align_t));
      *((char **) esp) = 0;
    LGOTO:switch ((*efp).call_id)
        {
        case 0:
          goto L_CALL3;
        }
      goto L_CALL3;
    }
  else;
  efp = (struct do_matAddTask_task_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct do_matAddTask_task_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  {
    new_esp = esp;
    while (matAdd_parallel
           (new_esp, &efp->_bk, _thr, (*pthis).A, (*pthis).B, (*pthis).C,
            (*pthis).i1, (*pthis).i2), __builtin_expect ((efp->tmp_esp =
                                                          *((char **) esp)) !=
                                                         0, 0))
      {
        efp->pthis = pthis;
        efp->_thr = _thr;
        efp->_bk.fun = _bk_in_do_matAddTask_task;
        efp->_bk.fr = (void *) efp;
        efp->call_id = 0;
        return;
      L_CALL3:;
        pthis = efp->pthis;
        _thr = efp->_thr;
        new_esp = esp + 1;
      }
  }
matAddTask_exit:return;
}

struct BlockMul_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  double *(*param)[3];
  double *arr[arrs];
  int j;
  int i;
  int n;
  int col;
  int row;
  int m;
  double *Z;
  double *Y;
  double *X;
  struct thread_data *_thr;
  closure_t *_bk;
};

void
BlockMul (char *esp, closure_t * _bk, struct thread_data *_thr, double *X,
          double *Y, double *Z, int m)
{
  int row;
  int col;
  int n;
  int i;
  int j;
  double *(*param)[3];
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct BlockMul_frame *efp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct BlockMul_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct BlockMul_frame) + sizeof (Align_t) +
                   -1) / sizeof (Align_t));
      *((char **) esp) = 0;
    LGOTO:switch ((*efp).call_id)
        {
        case 0:
          goto L_CALL4;
        case 1:
          goto L_CALL5;
        case 2:
          goto L_CALL6;
        case 3:
          goto L_CALL7;
        case 4:
          goto L_CALL8;
        }
      goto L_CALL4;
    }
  else;
  efp = (struct BlockMul_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct BlockMul_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  row = 0;
  col = 0;
  n = m / 2;
  i = 0;
  j = 0;
  if (m == 1)
    {
      *Z = *X ** Y;
      return;
    }
  else;
  {
    i = 0;
    for (; i < arrs; i++)
      {
        (efp->arr)[i] = (double *) malloc (sizeof (double) * (n * n));
        if ((efp->arr)[i] == NULL)
          {
            fprintf (stderr, "Out of memory (1) \n");
            exit (1);
          }
        else;
      }
  }
  {
    row = 0, i = 0;
    for (; row < n; (row++, i++))
      {
        {
          col = 0, j = 0;
          for (; col < n; (col++, j++))
            {
              (efp->arr)[x11][i * n + j] = X[row * m + col];
              (efp->arr)[y11][i * n + j] = Y[row * m + col];
            }
        }
        {
          col = n, j = 0;
          for (; col < m; (col++, j++))
            {
              (efp->arr)[x12][i * n + j] = X[row * m + col];
              (efp->arr)[y12][i * n + j] = Y[row * m + col];
            }
        }
      }
  }
  {
    row = n, i = 0;
    for (; row < m; (row++, i++))
      {
        {
          col = 0, j = 0;
          for (; col < n; (col++, j++))
            {
              (efp->arr)[x21][i * n + j] = X[row * m + col];
              (efp->arr)[y21][i * n + j] = Y[row * m + col];
            }
        }
        {
          col = n, j = 0;
          for (; col < m; (col++, j++))
            {
              (efp->arr)[x22][i * n + j] = X[row * m + col];
              (efp->arr)[y22][i * n + j] = Y[row * m + col];
            }
        }
      }
  }
  param = (double *(*)[3]) malloc (sizeof (double *[3]) * 8);
  param[0][0] = (efp->arr)[x11];
  param[0][1] = (efp->arr)[y11];
  param[0][2] = (efp->arr)[P1];
  param[1][0] = (efp->arr)[x12];
  param[1][1] = (efp->arr)[y21];
  param[1][2] = (efp->arr)[P2];
  param[2][0] = (efp->arr)[x11];
  param[2][1] = (efp->arr)[y12];
  param[2][2] = (efp->arr)[P3];
  param[3][0] = (efp->arr)[x12];
  param[3][1] = (efp->arr)[y22];
  param[3][2] = (efp->arr)[P4];
  param[4][0] = (efp->arr)[x21];
  param[4][1] = (efp->arr)[y11];
  param[4][2] = (efp->arr)[P5];
  param[5][0] = (efp->arr)[x22];
  param[5][1] = (efp->arr)[y21];
  param[5][2] = (efp->arr)[P6];
  param[6][0] = (efp->arr)[x21];
  param[6][1] = (efp->arr)[y12];
  param[6][2] = (efp->arr)[P7];
  param[7][0] = (efp->arr)[x22];
  param[7][1] = (efp->arr)[y22];
  param[7][2] = (efp->arr)[P8];
  {
    new_esp = esp;
    while (BlockMul_parallel (new_esp, _bk, _thr, param, n, 0, 8),
           __builtin_expect ((efp->tmp_esp = *((char **) esp)) != 0, 0))
      {
        efp->param = param;
        efp->j = j;
        efp->i = i;
        efp->n = n;
        efp->col = col;
        efp->row = row;
        efp->m = m;
        efp->Z = Z;
        efp->Y = Y;
        efp->X = X;
        efp->_thr = _thr;
        efp->_bk = _bk;
        efp->call_id = 0;
        return;
      L_CALL4:;
        param = efp->param;
        j = efp->j;
        i = efp->i;
        n = efp->n;
        col = efp->col;
        row = efp->row;
        m = efp->m;
        Z = efp->Z;
        Y = efp->Y;
        X = efp->X;
        _thr = efp->_thr;
        _bk = efp->_bk;
        new_esp = esp + 1;
      }
  }
  {
    new_esp = esp;
    while (matAdd_parallel
           (new_esp, _bk, _thr, (efp->arr)[P1], (efp->arr)[P2],
            (efp->arr)[C11], 0, n * n), __builtin_expect ((efp->tmp_esp =
                                                           *((char **) esp))
                                                          != 0, 0))
      {
        efp->param = param;
        efp->j = j;
        efp->i = i;
        efp->n = n;
        efp->col = col;
        efp->row = row;
        efp->m = m;
        efp->Z = Z;
        efp->Y = Y;
        efp->X = X;
        efp->_thr = _thr;
        efp->_bk = _bk;
        efp->call_id = 1;
        return;
      L_CALL5:;
        param = efp->param;
        j = efp->j;
        i = efp->i;
        n = efp->n;
        col = efp->col;
        row = efp->row;
        m = efp->m;
        Z = efp->Z;
        Y = efp->Y;
        X = efp->X;
        _thr = efp->_thr;
        _bk = efp->_bk;
        new_esp = esp + 1;
      }
  }
  {
    new_esp = esp;
    while (matAdd_parallel
           (new_esp, _bk, _thr, (efp->arr)[P3], (efp->arr)[P4],
            (efp->arr)[C12], 0, n * n), __builtin_expect ((efp->tmp_esp =
                                                           *((char **) esp))
                                                          != 0, 0))
      {
        efp->param = param;
        efp->j = j;
        efp->i = i;
        efp->n = n;
        efp->col = col;
        efp->row = row;
        efp->m = m;
        efp->Z = Z;
        efp->Y = Y;
        efp->X = X;
        efp->_thr = _thr;
        efp->_bk = _bk;
        efp->call_id = 2;
        return;
      L_CALL6:;
        param = efp->param;
        j = efp->j;
        i = efp->i;
        n = efp->n;
        col = efp->col;
        row = efp->row;
        m = efp->m;
        Z = efp->Z;
        Y = efp->Y;
        X = efp->X;
        _thr = efp->_thr;
        _bk = efp->_bk;
        new_esp = esp + 1;
      }
  }
  {
    new_esp = esp;
    while (matAdd_parallel
           (new_esp, _bk, _thr, (efp->arr)[P5], (efp->arr)[P6],
            (efp->arr)[C21], 0, n * n), __builtin_expect ((efp->tmp_esp =
                                                           *((char **) esp))
                                                          != 0, 0))
      {
        efp->param = param;
        efp->j = j;
        efp->i = i;
        efp->n = n;
        efp->col = col;
        efp->row = row;
        efp->m = m;
        efp->Z = Z;
        efp->Y = Y;
        efp->X = X;
        efp->_thr = _thr;
        efp->_bk = _bk;
        efp->call_id = 3;
        return;
      L_CALL7:;
        param = efp->param;
        j = efp->j;
        i = efp->i;
        n = efp->n;
        col = efp->col;
        row = efp->row;
        m = efp->m;
        Z = efp->Z;
        Y = efp->Y;
        X = efp->X;
        _thr = efp->_thr;
        _bk = efp->_bk;
        new_esp = esp + 1;
      }
  }
  {
    new_esp = esp;
    while (matAdd_parallel
           (new_esp, _bk, _thr, (efp->arr)[P7], (efp->arr)[P8],
            (efp->arr)[C22], 0, n * n), __builtin_expect ((efp->tmp_esp =
                                                           *((char **) esp))
                                                          != 0, 0))
      {
        efp->param = param;
        efp->j = j;
        efp->i = i;
        efp->n = n;
        efp->col = col;
        efp->row = row;
        efp->m = m;
        efp->Z = Z;
        efp->Y = Y;
        efp->X = X;
        efp->_thr = _thr;
        efp->_bk = _bk;
        efp->call_id = 4;
        return;
      L_CALL8:;
        param = efp->param;
        j = efp->j;
        i = efp->i;
        n = efp->n;
        col = efp->col;
        row = efp->row;
        m = efp->m;
        Z = efp->Z;
        Y = efp->Y;
        X = efp->X;
        _thr = efp->_thr;
        _bk = efp->_bk;
        new_esp = esp + 1;
      }
  }
  {
    row = 0, i = 0;
    for (; row < n; (row++, i++))
      {
        {
          col = 0, j = 0;
          for (; col < n; (col++, j++))
            {
              Z[row * m + col] = (efp->arr)[C11][i * n + j];
            }
        }
        {
          col = n, j = 0;
          for (; col < m; (col++, j++))
            {
              Z[row * m + col] = (efp->arr)[C12][i * n + j];
            }
        }
      }
  }
  {
    row = n, i = 0;
    for (; row < m; (row++, i++))
      {
        {
          col = 0, j = 0;
          for (; col < n; (col++, j++))
            {
              Z[row * m + col] = (efp->arr)[C21][i * n + j];
            }
        }
        {
          col = n, j = 0;
          for (; col < m; (col++, j++))
            {
              Z[row * m + col] = (efp->arr)[C22][i * n + j];
            }
        }
      }
  }
  {
    i = 0;
    for (; i < arrs; i++)
      {
        free ((efp->arr)[i]);
      }
  }
}

struct do_many_bk_in_BlockMul_parallel_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  int i12;
  int i22;
  int ifexp_result17;
  int ifexp_result18;
  int ifexp_result15;
  int ifexp_result16;
  int ifexp_result13;
  int ifexp_result14;
  void *tmp;
  struct BlockMul_parallel_frame *xfp;
};

struct BlockMul_parallel_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  int spawned;
  struct BlockTask *pthis;
  int I_end;
  int i3;
  int i;
  int ifexp_result19;
  int ifexp_result20;
  int ifexp_result21;
  int ifexp_result22;
  void *tmp2;
  int i2;
  int i1;
  int n;
  double *(*param)[3];
  struct thread_data *_thr;
  closure_t *_bk;
  closure_t do_many_bk;
};

char *
do_many_bk_in_BlockMul_parallel (char *esp, void *xfp0)
{
  void *tmp;
  int ifexp_result14;
  int ifexp_result13;
  int ifexp_result16;
  int ifexp_result15;
  int ifexp_result18;
  int ifexp_result17;
  int i22;
  int i12;
  char *new_esp;
  struct do_many_bk_in_BlockMul_parallel_frame *efp;
  struct BlockMul_parallel_frame *xfp = xfp0;
  size_t esp_flag = (size_t) esp & 3;
  char *parmp = (char *) ((size_t) esp ^ esp_flag);
  char *argp;
LGOTO:;
  efp = (struct do_many_bk_in_BlockMul_parallel_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct do_many_bk_in_BlockMul_parallel_frame) +
               sizeof (Align_t) + -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  {
    if ((*xfp->_thr).exiting == EXITING_EXCEPTION)
      ifexp_result13 = 1;
    else
      {
        if ((*xfp->_thr).exiting == EXITING_CANCEL)
          ifexp_result14 = 1;
        else
          ifexp_result14 = 0;
        ifexp_result13 = ifexp_result14;
      }
    if (ifexp_result13)
      {
        while ((xfp->spawned)-- > 0)
          {
            {
              {
                new_esp = esp;
                while (__builtin_expect
                       ((tmp =
                         wait_rslt (new_esp, xfp->_thr, 0)) == (void *) 0 - 1,
                        0)
                       && __builtin_expect ((efp->tmp_esp = *((char **) esp))
                                            != 0, 1))
                  {
                    char *goto_fr;
                    *((char **) esp) = 0;
                    efp->i12 = i12;
                    efp->i22 = i22;
                    efp->ifexp_result17 = ifexp_result17;
                    efp->ifexp_result18 = ifexp_result18;
                    efp->ifexp_result15 = ifexp_result15;
                    efp->ifexp_result16 = ifexp_result16;
                    efp->ifexp_result13 = ifexp_result13;
                    efp->ifexp_result14 = ifexp_result14;
                    efp->tmp = tmp;
                    efp->xfp = xfp;
                    goto_fr = lw_call (efp->tmp_esp);
                    if (goto_fr && (char *) goto_fr < (char *) efp)
                      return goto_fr;
                    else;
                    if ((char *) goto_fr == (char *) efp)
                      goto LGOTO;
                    else;
                    i12 = efp->i12;
                    i22 = efp->i22;
                    ifexp_result17 = efp->ifexp_result17;
                    ifexp_result18 = efp->ifexp_result18;
                    ifexp_result15 = efp->ifexp_result15;
                    ifexp_result16 = efp->ifexp_result16;
                    ifexp_result13 = efp->ifexp_result13;
                    ifexp_result14 = efp->ifexp_result14;
                    tmp = efp->tmp;
                    xfp = efp->xfp;
                    new_esp = esp + 1;
                  }
              }
              xfp->pthis = (struct BlockTask *) tmp;
            }
            free (xfp->pthis);
          }
        {
          char *goto_fr;
          argp =
            (char *) ((Align_t *) esp +
                      (sizeof (char *) + sizeof (Align_t) +
                       -1) / sizeof (Align_t));
          *((closure_t **) argp) = xfp->_bk;
          efp->i12 = i12;
          efp->i22 = i22;
          efp->ifexp_result17 = ifexp_result17;
          efp->ifexp_result18 = ifexp_result18;
          efp->ifexp_result15 = ifexp_result15;
          efp->ifexp_result16 = ifexp_result16;
          efp->ifexp_result13 = ifexp_result13;
          efp->ifexp_result14 = ifexp_result14;
          efp->tmp = tmp;
          efp->xfp = xfp;
          goto_fr = lw_call (argp);
          if (goto_fr)
            if ((char *) goto_fr < (char *) efp)
              return goto_fr;
            else
              {
                efp->tmp_esp = 0;
                goto LGOTO;
              }
          else;
          i12 = efp->i12;
          i22 = efp->i22;
          ifexp_result17 = efp->ifexp_result17;
          ifexp_result18 = efp->ifexp_result18;
          ifexp_result15 = efp->ifexp_result15;
          ifexp_result16 = efp->ifexp_result16;
          ifexp_result13 = efp->ifexp_result13;
          ifexp_result14 = efp->ifexp_result14;
          tmp = efp->tmp;
          xfp = efp->xfp;
        }
      }
    else;
  }
  if (!xfp->spawned)
    {
      char *goto_fr;
      argp =
        (char *) ((Align_t *) esp +
                  (sizeof (char *) + sizeof (Align_t) +
                   -1) / sizeof (Align_t));
      *((closure_t **) argp) = xfp->_bk;
      efp->i12 = i12;
      efp->i22 = i22;
      efp->ifexp_result17 = ifexp_result17;
      efp->ifexp_result18 = ifexp_result18;
      efp->ifexp_result15 = ifexp_result15;
      efp->ifexp_result16 = ifexp_result16;
      efp->ifexp_result13 = ifexp_result13;
      efp->ifexp_result14 = ifexp_result14;
      efp->tmp = tmp;
      efp->xfp = xfp;
      goto_fr = lw_call (argp);
      if (goto_fr)
        if ((char *) goto_fr < (char *) efp)
          return goto_fr;
        else
          {
            efp->tmp_esp = 0;
            goto LGOTO;
          }
      else;
      i12 = efp->i12;
      i22 = efp->i22;
      ifexp_result17 = efp->ifexp_result17;
      ifexp_result18 = efp->ifexp_result18;
      ifexp_result15 = efp->ifexp_result15;
      ifexp_result16 = efp->ifexp_result16;
      ifexp_result13 = efp->ifexp_result13;
      ifexp_result14 = efp->ifexp_result14;
      tmp = efp->tmp;
      xfp = efp->xfp;
    }
  else;
  {
    if ((*xfp->_thr).treq_top)
      {
        if (2 <= xfp->I_end - xfp->i3)
          ifexp_result16 = 1;
        else
          ifexp_result16 = 0;
        ifexp_result15 = ifexp_result16;
      }
    else
      ifexp_result15 = 0;
    if (ifexp_result15)
      {
        goto loop_start;
        while (1)
          {
            if ((*xfp->_thr).treq_top)
              {
                if (2 <= xfp->I_end - xfp->i3)
                  ifexp_result18 = 1;
                else
                  ifexp_result18 = 0;
                ifexp_result17 = ifexp_result18;
              }
            else
              ifexp_result17 = 0;
            if (!ifexp_result17)
              goto loop_end;
            else;
          loop_start:;
            i22 = xfp->I_end;
            i12 = (1 + xfp->i3 + xfp->I_end) / 2;
            xfp->I_end = i12;
            xfp->pthis =
              (struct BlockTask *) malloc (sizeof (struct BlockTask));
            {
              (*xfp->pthis).param = xfp->param;
              (*xfp->pthis).n = xfp->n;
              (*xfp->pthis).i1 = i12;
              (*xfp->pthis).i2 = i22;
            }
            (xfp->spawned)++;
            make_and_send_task (xfp->_thr, 1, xfp->pthis, xfp->spawned == 1);
          }
      loop_end:;
      }
    else;
  }
  {
    *((int *) efp) = 0;
    return 0;
  }
  return 0;
}

void
BlockMul_parallel (char *esp, closure_t * _bk, struct thread_data *_thr,
                   double *(*param)[3], int n, int i1, int i2)
{
  void *tmp2;
  int ifexp_result22;
  int ifexp_result21;
  int ifexp_result20;
  int ifexp_result19;
  int i;
  int i3;
  int I_end;
  struct BlockTask *pthis;
  int spawned;
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct BlockMul_parallel_frame *efp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct BlockMul_parallel_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct BlockMul_parallel_frame) +
                   sizeof (Align_t) + -1) / sizeof (Align_t));
      *((char **) esp) = 0;
    LGOTO:switch ((*efp).call_id)
        {
        case 0:
          goto L_CALL9;
        case 1:
          goto L_CALL10;
        case 2:
          goto L_CALL11;
        case 3:
          goto L_CALL12;
        }
      goto L_CALL9;
    }
  else;
  efp = (struct BlockMul_parallel_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct BlockMul_parallel_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  {
    i3 = i1;
    I_end = i2;
    spawned = 0;
    {
      if ((*_thr).req_cncl)
        ifexp_result19 = 1;
      else
        {
          {
            if ((*(*_thr).task_top).cancellation)
              ifexp_result21 = 1;
            else
              {
                if ((*_thr).req)
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
        {
          new_esp = esp;
          while (handle_reqs (new_esp, &efp->do_many_bk, _thr),
                 __builtin_expect ((efp->tmp_esp = *((char **) esp)) != 0, 0))
            {
              efp->spawned = spawned;
              efp->pthis = pthis;
              efp->I_end = I_end;
              efp->i3 = i3;
              efp->i = i;
              efp->ifexp_result19 = ifexp_result19;
              efp->ifexp_result20 = ifexp_result20;
              efp->ifexp_result21 = ifexp_result21;
              efp->ifexp_result22 = ifexp_result22;
              efp->tmp2 = tmp2;
              efp->i2 = i2;
              efp->i1 = i1;
              efp->n = n;
              efp->param = param;
              efp->_thr = _thr;
              efp->_bk = _bk;
              efp->do_many_bk.fun = do_many_bk_in_BlockMul_parallel;
              efp->do_many_bk.fr = (void *) efp;
              efp->call_id = 0;
              return;
            L_CALL9:;
              spawned = efp->spawned;
              pthis = efp->pthis;
              I_end = efp->I_end;
              i3 = efp->i3;
              i = efp->i;
              ifexp_result19 = efp->ifexp_result19;
              ifexp_result20 = efp->ifexp_result20;
              ifexp_result21 = efp->ifexp_result21;
              ifexp_result22 = efp->ifexp_result22;
              tmp2 = efp->tmp2;
              i2 = efp->i2;
              i1 = efp->i1;
              n = efp->n;
              param = efp->param;
              _thr = efp->_thr;
              _bk = efp->_bk;
              new_esp = esp + 1;
            }
        }
      else;
    }
    for (; i3 < I_end; i3++)
      {
        new_esp = esp;
        while (BlockMul
               (new_esp, &efp->do_many_bk, _thr, param[i3][0], param[i3][1],
                param[i3][2], n), __builtin_expect ((efp->tmp_esp =
                                                     *((char **) esp)) != 0,
                                                    0))
          {
            efp->spawned = spawned;
            efp->pthis = pthis;
            efp->I_end = I_end;
            efp->i3 = i3;
            efp->i = i;
            efp->ifexp_result19 = ifexp_result19;
            efp->ifexp_result20 = ifexp_result20;
            efp->ifexp_result21 = ifexp_result21;
            efp->ifexp_result22 = ifexp_result22;
            efp->tmp2 = tmp2;
            efp->i2 = i2;
            efp->i1 = i1;
            efp->n = n;
            efp->param = param;
            efp->_thr = _thr;
            efp->_bk = _bk;
            efp->do_many_bk.fun = do_many_bk_in_BlockMul_parallel;
            efp->do_many_bk.fr = (void *) efp;
            efp->call_id = 1;
            return;
          L_CALL10:;
            spawned = efp->spawned;
            pthis = efp->pthis;
            I_end = efp->I_end;
            i3 = efp->i3;
            i = efp->i;
            ifexp_result19 = efp->ifexp_result19;
            ifexp_result20 = efp->ifexp_result20;
            ifexp_result21 = efp->ifexp_result21;
            ifexp_result22 = efp->ifexp_result22;
            tmp2 = efp->tmp2;
            i2 = efp->i2;
            i1 = efp->i1;
            n = efp->n;
            param = efp->param;
            _thr = efp->_thr;
            _bk = efp->_bk;
            new_esp = esp + 1;
          }
      }
    while (spawned-- > 0)
      {
        {
          {
            new_esp = esp;
            while (__builtin_expect
                   ((tmp2 =
                     wait_rslt (new_esp, _thr, 1)) == (void *) 0 - 1, 0)
                   && __builtin_expect ((efp->tmp_esp = *((char **) esp)) !=
                                        0, 1))
              {
                efp->spawned = spawned;
                efp->pthis = pthis;
                efp->I_end = I_end;
                efp->i3 = i3;
                efp->i = i;
                efp->ifexp_result19 = ifexp_result19;
                efp->ifexp_result20 = ifexp_result20;
                efp->ifexp_result21 = ifexp_result21;
                efp->ifexp_result22 = ifexp_result22;
                efp->tmp2 = tmp2;
                efp->i2 = i2;
                efp->i1 = i1;
                efp->n = n;
                efp->param = param;
                efp->_thr = _thr;
                efp->_bk = _bk;
                efp->do_many_bk.fun = do_many_bk_in_BlockMul_parallel;
                efp->do_many_bk.fr = (void *) efp;
                efp->call_id = 2;
                return;
              L_CALL11:;
                spawned = efp->spawned;
                pthis = efp->pthis;
                I_end = efp->I_end;
                i3 = efp->i3;
                i = efp->i;
                ifexp_result19 = efp->ifexp_result19;
                ifexp_result20 = efp->ifexp_result20;
                ifexp_result21 = efp->ifexp_result21;
                ifexp_result22 = efp->ifexp_result22;
                tmp2 = efp->tmp2;
                i2 = efp->i2;
                i1 = efp->i1;
                n = efp->n;
                param = efp->param;
                _thr = efp->_thr;
                _bk = efp->_bk;
                new_esp = esp + 1;
              }
          }
          pthis = (struct BlockTask *) tmp2;
        }
        if (pthis)
          {
            free (pthis);
          }
        else;
      }
    if ((*_thr).exiting == EXITING_EXCEPTION)
      {
        new_esp = esp;
        while (handle_exception (new_esp, _bk, _thr, (*_thr).exception_tag),
               __builtin_expect ((efp->tmp_esp = *((char **) esp)) != 0, 0))
          {
            efp->spawned = spawned;
            efp->pthis = pthis;
            efp->I_end = I_end;
            efp->i3 = i3;
            efp->i = i;
            efp->ifexp_result19 = ifexp_result19;
            efp->ifexp_result20 = ifexp_result20;
            efp->ifexp_result21 = ifexp_result21;
            efp->ifexp_result22 = ifexp_result22;
            efp->tmp2 = tmp2;
            efp->i2 = i2;
            efp->i1 = i1;
            efp->n = n;
            efp->param = param;
            efp->_thr = _thr;
            efp->_bk = _bk;
            efp->do_many_bk.fun = do_many_bk_in_BlockMul_parallel;
            efp->do_many_bk.fr = (void *) efp;
            efp->call_id = 3;
            return;
          L_CALL12:;
            spawned = efp->spawned;
            pthis = efp->pthis;
            I_end = efp->I_end;
            i3 = efp->i3;
            i = efp->i;
            ifexp_result19 = efp->ifexp_result19;
            ifexp_result20 = efp->ifexp_result20;
            ifexp_result21 = efp->ifexp_result21;
            ifexp_result22 = efp->ifexp_result22;
            tmp2 = efp->tmp2;
            i2 = efp->i2;
            i1 = efp->i1;
            n = efp->n;
            param = efp->param;
            _thr = efp->_thr;
            _bk = efp->_bk;
            new_esp = esp + 1;
          }
      }
    else;
  }
}

void
matMul (double *A, double *B, double *C, int n)
{
  int row;
  int col;
  int k;
  int i;
  int j;
  double sum;
  row = 0;
  col = 0;
  k = 0;
  i = 0;
  j = 0;
  {
    i = 0;
    for (; i < n; i++)
      {
        j = 0;
        for (; j < n; j++)
          {
            sum = 0.0;
            {
              k = 0;
              for (; k < n; k++)
                {
                  sum += A[i * n + k] * B[k * n + j];
                }
            }
            C[i * n + j] = sum;
          }
      }
  }
}

struct do_many_bk2_in_matAdd_parallel_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  int i13;
  int i23;
  int ifexp_result27;
  int ifexp_result28;
  int ifexp_result25;
  int ifexp_result26;
  int ifexp_result23;
  int ifexp_result24;
  void *tmp3;
  struct matAdd_parallel_frame *xfp;
};

struct matAdd_parallel_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  int spawned2;
  struct matAddTask *pthis;
  int I_end2;
  int i4;
  int i;
  int ifexp_result29;
  int ifexp_result30;
  int ifexp_result31;
  int ifexp_result32;
  void *tmp4;
  int i2;
  int i1;
  double *C;
  double *B;
  double *A;
  struct thread_data *_thr;
  closure_t *_bk;
  closure_t do_many_bk2;
};

char *
do_many_bk2_in_matAdd_parallel (char *esp, void *xfp0)
{
  void *tmp3;
  int ifexp_result24;
  int ifexp_result23;
  int ifexp_result26;
  int ifexp_result25;
  int ifexp_result28;
  int ifexp_result27;
  int i23;
  int i13;
  char *new_esp;
  struct do_many_bk2_in_matAdd_parallel_frame *efp;
  struct matAdd_parallel_frame *xfp = xfp0;
  size_t esp_flag = (size_t) esp & 3;
  char *parmp = (char *) ((size_t) esp ^ esp_flag);
  char *argp;
LGOTO:;
  efp = (struct do_many_bk2_in_matAdd_parallel_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct do_many_bk2_in_matAdd_parallel_frame) +
               sizeof (Align_t) + -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  {
    if ((*xfp->_thr).exiting == EXITING_EXCEPTION)
      ifexp_result23 = 1;
    else
      {
        if ((*xfp->_thr).exiting == EXITING_CANCEL)
          ifexp_result24 = 1;
        else
          ifexp_result24 = 0;
        ifexp_result23 = ifexp_result24;
      }
    if (ifexp_result23)
      {
        while ((xfp->spawned2)-- > 0)
          {
            {
              {
                new_esp = esp;
                while (__builtin_expect
                       ((tmp3 =
                         wait_rslt (new_esp, xfp->_thr, 0)) == (void *) 0 - 1,
                        0)
                       && __builtin_expect ((efp->tmp_esp = *((char **) esp))
                                            != 0, 1))
                  {
                    char *goto_fr;
                    *((char **) esp) = 0;
                    efp->i13 = i13;
                    efp->i23 = i23;
                    efp->ifexp_result27 = ifexp_result27;
                    efp->ifexp_result28 = ifexp_result28;
                    efp->ifexp_result25 = ifexp_result25;
                    efp->ifexp_result26 = ifexp_result26;
                    efp->ifexp_result23 = ifexp_result23;
                    efp->ifexp_result24 = ifexp_result24;
                    efp->tmp3 = tmp3;
                    efp->xfp = xfp;
                    goto_fr = lw_call (efp->tmp_esp);
                    if (goto_fr && (char *) goto_fr < (char *) efp)
                      return goto_fr;
                    else;
                    if ((char *) goto_fr == (char *) efp)
                      goto LGOTO;
                    else;
                    i13 = efp->i13;
                    i23 = efp->i23;
                    ifexp_result27 = efp->ifexp_result27;
                    ifexp_result28 = efp->ifexp_result28;
                    ifexp_result25 = efp->ifexp_result25;
                    ifexp_result26 = efp->ifexp_result26;
                    ifexp_result23 = efp->ifexp_result23;
                    ifexp_result24 = efp->ifexp_result24;
                    tmp3 = efp->tmp3;
                    xfp = efp->xfp;
                    new_esp = esp + 1;
                  }
              }
              xfp->pthis = (struct matAddTask *) tmp3;
            }
            free (xfp->pthis);
          }
        {
          char *goto_fr;
          argp =
            (char *) ((Align_t *) esp +
                      (sizeof (char *) + sizeof (Align_t) +
                       -1) / sizeof (Align_t));
          *((closure_t **) argp) = xfp->_bk;
          efp->i13 = i13;
          efp->i23 = i23;
          efp->ifexp_result27 = ifexp_result27;
          efp->ifexp_result28 = ifexp_result28;
          efp->ifexp_result25 = ifexp_result25;
          efp->ifexp_result26 = ifexp_result26;
          efp->ifexp_result23 = ifexp_result23;
          efp->ifexp_result24 = ifexp_result24;
          efp->tmp3 = tmp3;
          efp->xfp = xfp;
          goto_fr = lw_call (argp);
          if (goto_fr)
            if ((char *) goto_fr < (char *) efp)
              return goto_fr;
            else
              {
                efp->tmp_esp = 0;
                goto LGOTO;
              }
          else;
          i13 = efp->i13;
          i23 = efp->i23;
          ifexp_result27 = efp->ifexp_result27;
          ifexp_result28 = efp->ifexp_result28;
          ifexp_result25 = efp->ifexp_result25;
          ifexp_result26 = efp->ifexp_result26;
          ifexp_result23 = efp->ifexp_result23;
          ifexp_result24 = efp->ifexp_result24;
          tmp3 = efp->tmp3;
          xfp = efp->xfp;
        }
      }
    else;
  }
  if (!xfp->spawned2)
    {
      char *goto_fr;
      argp =
        (char *) ((Align_t *) esp +
                  (sizeof (char *) + sizeof (Align_t) +
                   -1) / sizeof (Align_t));
      *((closure_t **) argp) = xfp->_bk;
      efp->i13 = i13;
      efp->i23 = i23;
      efp->ifexp_result27 = ifexp_result27;
      efp->ifexp_result28 = ifexp_result28;
      efp->ifexp_result25 = ifexp_result25;
      efp->ifexp_result26 = ifexp_result26;
      efp->ifexp_result23 = ifexp_result23;
      efp->ifexp_result24 = ifexp_result24;
      efp->tmp3 = tmp3;
      efp->xfp = xfp;
      goto_fr = lw_call (argp);
      if (goto_fr)
        if ((char *) goto_fr < (char *) efp)
          return goto_fr;
        else
          {
            efp->tmp_esp = 0;
            goto LGOTO;
          }
      else;
      i13 = efp->i13;
      i23 = efp->i23;
      ifexp_result27 = efp->ifexp_result27;
      ifexp_result28 = efp->ifexp_result28;
      ifexp_result25 = efp->ifexp_result25;
      ifexp_result26 = efp->ifexp_result26;
      ifexp_result23 = efp->ifexp_result23;
      ifexp_result24 = efp->ifexp_result24;
      tmp3 = efp->tmp3;
      xfp = efp->xfp;
    }
  else;
  {
    if ((*xfp->_thr).treq_top)
      {
        if (2 <= xfp->I_end2 - xfp->i4)
          ifexp_result26 = 1;
        else
          ifexp_result26 = 0;
        ifexp_result25 = ifexp_result26;
      }
    else
      ifexp_result25 = 0;
    if (ifexp_result25)
      {
        goto loop_start2;
        while (1)
          {
            if ((*xfp->_thr).treq_top)
              {
                if (2 <= xfp->I_end2 - xfp->i4)
                  ifexp_result28 = 1;
                else
                  ifexp_result28 = 0;
                ifexp_result27 = ifexp_result28;
              }
            else
              ifexp_result27 = 0;
            if (!ifexp_result27)
              goto loop_end2;
            else;
          loop_start2:;
            i23 = xfp->I_end2;
            i13 = (1 + xfp->i4 + xfp->I_end2) / 2;
            xfp->I_end2 = i13;
            xfp->pthis =
              (struct matAddTask *) malloc (sizeof (struct matAddTask));
            {
              (*xfp->pthis).A = xfp->A;
              (*xfp->pthis).B = xfp->B;
              (*xfp->pthis).C = xfp->C;
              (*xfp->pthis).i1 = i13;
              (*xfp->pthis).i2 = i23;
            }
            (xfp->spawned2)++;
            make_and_send_task (xfp->_thr, 2, xfp->pthis, xfp->spawned2 == 1);
          }
      loop_end2:;
      }
    else;
  }
  {
    *((int *) efp) = 0;
    return 0;
  }
  return 0;
}

void
matAdd_parallel (char *esp, closure_t * _bk, struct thread_data *_thr,
                 double *A, double *B, double *C, int i1, int i2)
{
  void *tmp4;
  int ifexp_result32;
  int ifexp_result31;
  int ifexp_result30;
  int ifexp_result29;
  int i;
  int i4;
  int I_end2;
  struct matAddTask *pthis;
  int spawned2;
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct matAdd_parallel_frame *efp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct matAdd_parallel_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct matAdd_parallel_frame) + sizeof (Align_t) +
                   -1) / sizeof (Align_t));
      *((char **) esp) = 0;
    LGOTO:switch ((*efp).call_id)
        {
        case 0:
          goto L_CALL13;
        case 1:
          goto L_CALL14;
        case 2:
          goto L_CALL15;
        }
      goto L_CALL13;
    }
  else;
  efp = (struct matAdd_parallel_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct matAdd_parallel_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  i = 0;
  {
    i4 = i1;
    I_end2 = i2;
    spawned2 = 0;
    {
      if ((*_thr).req_cncl)
        ifexp_result29 = 1;
      else
        {
          {
            if ((*(*_thr).task_top).cancellation)
              ifexp_result31 = 1;
            else
              {
                if ((*_thr).req)
                  ifexp_result32 = 1;
                else
                  ifexp_result32 = 0;
                ifexp_result31 = ifexp_result32;
              }
            if (ifexp_result31)
              ifexp_result30 = 1;
            else
              ifexp_result30 = 0;
          }
          ifexp_result29 = ifexp_result30;
        }
      if (ifexp_result29)
        {
          new_esp = esp;
          while (handle_reqs (new_esp, &efp->do_many_bk2, _thr),
                 __builtin_expect ((efp->tmp_esp = *((char **) esp)) != 0, 0))
            {
              efp->spawned2 = spawned2;
              efp->pthis = pthis;
              efp->I_end2 = I_end2;
              efp->i4 = i4;
              efp->i = i;
              efp->ifexp_result29 = ifexp_result29;
              efp->ifexp_result30 = ifexp_result30;
              efp->ifexp_result31 = ifexp_result31;
              efp->ifexp_result32 = ifexp_result32;
              efp->tmp4 = tmp4;
              efp->i2 = i2;
              efp->i1 = i1;
              efp->C = C;
              efp->B = B;
              efp->A = A;
              efp->_thr = _thr;
              efp->_bk = _bk;
              efp->do_many_bk2.fun = do_many_bk2_in_matAdd_parallel;
              efp->do_many_bk2.fr = (void *) efp;
              efp->call_id = 0;
              return;
            L_CALL13:;
              spawned2 = efp->spawned2;
              pthis = efp->pthis;
              I_end2 = efp->I_end2;
              i4 = efp->i4;
              i = efp->i;
              ifexp_result29 = efp->ifexp_result29;
              ifexp_result30 = efp->ifexp_result30;
              ifexp_result31 = efp->ifexp_result31;
              ifexp_result32 = efp->ifexp_result32;
              tmp4 = efp->tmp4;
              i2 = efp->i2;
              i1 = efp->i1;
              C = efp->C;
              B = efp->B;
              A = efp->A;
              _thr = efp->_thr;
              _bk = efp->_bk;
              new_esp = esp + 1;
            }
        }
      else;
    }
    for (; i4 < I_end2; i4++)
      {
        C[i4] = A[i4] + B[i4];
      }
    while (spawned2-- > 0)
      {
        {
          {
            new_esp = esp;
            while (__builtin_expect
                   ((tmp4 =
                     wait_rslt (new_esp, _thr, 1)) == (void *) 0 - 1, 0)
                   && __builtin_expect ((efp->tmp_esp = *((char **) esp)) !=
                                        0, 1))
              {
                efp->spawned2 = spawned2;
                efp->pthis = pthis;
                efp->I_end2 = I_end2;
                efp->i4 = i4;
                efp->i = i;
                efp->ifexp_result29 = ifexp_result29;
                efp->ifexp_result30 = ifexp_result30;
                efp->ifexp_result31 = ifexp_result31;
                efp->ifexp_result32 = ifexp_result32;
                efp->tmp4 = tmp4;
                efp->i2 = i2;
                efp->i1 = i1;
                efp->C = C;
                efp->B = B;
                efp->A = A;
                efp->_thr = _thr;
                efp->_bk = _bk;
                efp->do_many_bk2.fun = do_many_bk2_in_matAdd_parallel;
                efp->do_many_bk2.fr = (void *) efp;
                efp->call_id = 1;
                return;
              L_CALL14:;
                spawned2 = efp->spawned2;
                pthis = efp->pthis;
                I_end2 = efp->I_end2;
                i4 = efp->i4;
                i = efp->i;
                ifexp_result29 = efp->ifexp_result29;
                ifexp_result30 = efp->ifexp_result30;
                ifexp_result31 = efp->ifexp_result31;
                ifexp_result32 = efp->ifexp_result32;
                tmp4 = efp->tmp4;
                i2 = efp->i2;
                i1 = efp->i1;
                C = efp->C;
                B = efp->B;
                A = efp->A;
                _thr = efp->_thr;
                _bk = efp->_bk;
                new_esp = esp + 1;
              }
          }
          pthis = (struct matAddTask *) tmp4;
        }
        if (pthis)
          {
            free (pthis);
          }
        else;
      }
    if ((*_thr).exiting == EXITING_EXCEPTION)
      {
        new_esp = esp;
        while (handle_exception (new_esp, _bk, _thr, (*_thr).exception_tag),
               __builtin_expect ((efp->tmp_esp = *((char **) esp)) != 0, 0))
          {
            efp->spawned2 = spawned2;
            efp->pthis = pthis;
            efp->I_end2 = I_end2;
            efp->i4 = i4;
            efp->i = i;
            efp->ifexp_result29 = ifexp_result29;
            efp->ifexp_result30 = ifexp_result30;
            efp->ifexp_result31 = ifexp_result31;
            efp->ifexp_result32 = ifexp_result32;
            efp->tmp4 = tmp4;
            efp->i2 = i2;
            efp->i1 = i1;
            efp->C = C;
            efp->B = B;
            efp->A = A;
            efp->_thr = _thr;
            efp->_bk = _bk;
            efp->do_many_bk2.fun = do_many_bk2_in_matAdd_parallel;
            efp->do_many_bk2.fr = (void *) efp;
            efp->call_id = 2;
            return;
          L_CALL15:;
            spawned2 = efp->spawned2;
            pthis = efp->pthis;
            I_end2 = efp->I_end2;
            i4 = efp->i4;
            i = efp->i;
            ifexp_result29 = efp->ifexp_result29;
            ifexp_result30 = efp->ifexp_result30;
            ifexp_result31 = efp->ifexp_result31;
            ifexp_result32 = efp->ifexp_result32;
            tmp4 = efp->tmp4;
            i2 = efp->i2;
            i1 = efp->i1;
            C = efp->C;
            B = efp->B;
            A = efp->A;
            _thr = efp->_thr;
            _bk = efp->_bk;
            new_esp = esp + 1;
          }
      }
    else;
  }
}

void
matAdd (double *A, double *B, double *C, int m)
{
  int row;
  int col;
  row = 0;
  col = 0;
  {
    row = 0;
    for (; row < m; row++)
      {
        col = 0;
        for (; col < m; col++)
          {
            C[row * m + col] = A[row * m + col] + B[row * m + col];
          }
      }
  }
}

double
get_wall_time (void)
{
  struct timeval time;
  if (gettimeofday (&time, NULL))
    return 0;
  else;
  return (double) time.tv_sec + (double) time.tv_usec * 1.0E-6;
}

void
send_matAddTask_task (struct matAddTask *pthis)
{
}

struct matAddTask *
recv_matAddTask_task ()
{
  struct matAddTask *pthis;
  pthis = malloc (sizeof (struct matAddTask));
  return pthis;
}

void
send_matAddTask_rslt (struct matAddTask *pthis)
{
  free (pthis);
}

void
recv_matAddTask_rslt (struct matAddTask *pthis)
{
}

void
send_BlockTask_task (struct BlockTask *pthis)
{
}

struct BlockTask *
recv_BlockTask_task ()
{
  struct BlockTask *pthis;
  pthis = malloc (sizeof (struct BlockTask));
  return pthis;
}

void
send_BlockTask_rslt (struct BlockTask *pthis)
{
  free (pthis);
}

void
recv_BlockTask_rslt (struct BlockTask *pthis)
{
}

void
send_mm_start_task (struct mm_start *pthis)
{
}

struct mm_start *
recv_mm_start_task ()
{
  struct mm_start *pthis;
  pthis = malloc (sizeof (struct mm_start));
  return pthis;
}

void
send_mm_start_rslt (struct mm_start *pthis)
{
  free (pthis);
}

void
recv_mm_start_rslt (struct mm_start *pthis)
{
}

void (*task_doers[256]) (char *, struct thread_data *, void *) =
{
(void (*)(char *, struct thread_data *, void *)) do_mm_start_task,
    (void (*)(char *, struct thread_data *, void *)) do_BlockTask_task,
    (void (*)(char *, struct thread_data *, void *)) do_matAddTask_task};
void (*task_senders[256]) (void *) =
{
(void (*)(void *)) send_mm_start_task,
    (void (*)(void *)) send_BlockTask_task,
    (void (*)(void *)) send_matAddTask_task};
void *(*task_receivers[256]) () =
{
(void *(*)()) recv_mm_start_task, (void *(*)()) recv_BlockTask_task,
    (void *(*)()) recv_matAddTask_task};
void (*rslt_senders[256]) (void *) =
{
(void (*)(void *)) send_mm_start_rslt,
    (void (*)(void *)) send_BlockTask_rslt,
    (void (*)(void *)) send_matAddTask_rslt};
void (*rslt_receivers[256]) (void *) =
{
(void (*)(void *)) recv_mm_start_rslt,
    (void (*)(void *)) recv_BlockTask_rslt,
    (void (*)(void *)) recv_matAddTask_rslt};

struct worker_data
{
};

void
worker_init (struct thread_data *_thr)
{
}
