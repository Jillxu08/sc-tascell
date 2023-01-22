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
extern void (*task_doers[256]) (struct thread_data *, void *);
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

void *wait_rslt (struct thread_data *thr, int stback);

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

void handle_reqs (int (*)(void), struct thread_data *);

void handle_exception (int (*)(void), struct thread_data *, long);
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

void BlockMul_parallel (int (*_bk) (void), struct thread_data *_thr,
                        double *(*param)[3], int n, int i1, int i2);

void BlockMul (int (*_bk) (void), struct thread_data *_thr, double *X,
               double *Y, double *Z, int m);

void matAdd_parallel (int (*_bk) (void), struct thread_data *_thr, double *A,
                      double *B, double *C, int i1, int i2);

void matMul (double *A, double *B, double *C, int n);

void matAdd (double *A, double *B, double *C, int m);

double get_wall_time (void);
enum no_match
{ x11, x12, x21, x22, y11, y12, y21, y22, P1, P2, P3, P4, P5, P6, P7, P8, C11,
    C12, C21, C22, arrs };

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

void
do_mm_start_task (struct thread_data *_thr, struct mm_start *pthis)
{

  int _bk (void)
  {
    if (_thr->exiting == EXITING_EXCEPTION || _thr->exiting == EXITING_CANCEL)
      {
        fprintf (stderr,
                 "Error: Exception is thrown but this program is complied invalidating exception support.\n");
        exit (99);
      }
    else;
    return 0;
  }
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
  fprintf (stderr, "mm4_parallel1\'s result is as follows:   \n");
  fprintf (stderr,
           "--------------------------worker=1-----------------------------------------------\n");
  N = 2048;
  fprintf (stderr, "Matrix size = %2d    ", N);
  X = malloc (sizeof (double) * (N * N));
  Y = malloc (sizeof (double) * (N * N));
  Z = malloc (sizeof (double) * (N * N));
  W = malloc (sizeof (double) * (N * N));
  if (X == NULL || (Y == NULL || (Z == NULL || W == NULL)))
    {
      fprintf (stderr, "Out of memory (1)\n");
      exit (1);
    }
  else;
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
  BlockMul (_bk, _thr, X, Y, Z, N);
  end2 = get_wall_time ();
  elapsed2 = end2 - start2;
  fprintf (stderr, "BlockMul = %5.4f  \n", elapsed2);
  int flag = 0;
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

void
do_BlockTask_task (struct thread_data *_thr, struct BlockTask *pthis)
{

  int _bk (void)
  {
    if (_thr->exiting == EXITING_EXCEPTION || _thr->exiting == EXITING_CANCEL)
      {
        fprintf (stderr,
                 "Error: Exception is thrown but this program is complied invalidating exception support.\n");
        exit (99);
      }
    else;
    return 0;
  }
  BlockMul_parallel (_bk, _thr, (*pthis).param, (*pthis).n, (*pthis).i1,
                     (*pthis).i2);
BlockTask_exit:return;
}

void
do_matAddTask_task (struct thread_data *_thr, struct matAddTask *pthis)
{

  int _bk (void)
  {
    if (_thr->exiting == EXITING_EXCEPTION || _thr->exiting == EXITING_CANCEL)
      {
        fprintf (stderr,
                 "Error: Exception is thrown but this program is complied invalidating exception support.\n");
        exit (99);
      }
    else;
    return 0;
  }
  matAdd_parallel (_bk, _thr, (*pthis).A, (*pthis).B, (*pthis).C, (*pthis).i1,
                   (*pthis).i2);
matAddTask_exit:return;
}

void
BlockMul (int (*_bk) (void), struct thread_data *_thr, double *X, double *Y,
          double *Z, int m)
{
  int row = 0;
  int col = 0;
  int n = m / 2;
  int i = 0;
  int j = 0;
  double *arr[arrs];
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
        arr[i] = (double *) malloc (sizeof (double) * (n * n));
        if (arr[i] == NULL)
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
              arr[x11][i * n + j] = X[row * m + col];
              arr[y11][i * n + j] = Y[row * m + col];
            }
        }
        {
          col = n, j = 0;
          for (; col < m; (col++, j++))
            {
              arr[x12][i * n + j] = X[row * m + col];
              arr[y12][i * n + j] = Y[row * m + col];
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
              arr[x21][i * n + j] = X[row * m + col];
              arr[y21][i * n + j] = Y[row * m + col];
            }
        }
        {
          col = n, j = 0;
          for (; col < m; (col++, j++))
            {
              arr[x22][i * n + j] = X[row * m + col];
              arr[y22][i * n + j] = Y[row * m + col];
            }
        }
      }
  }
  double *(*param)[3];
  param = (double *(*)[3]) malloc (sizeof (double *[3]) * 8);
  param[0][0] = arr[x11];
  param[0][1] = arr[y11];
  param[0][2] = arr[P1];
  param[1][0] = arr[x12];
  param[1][1] = arr[y21];
  param[1][2] = arr[P2];
  param[2][0] = arr[x11];
  param[2][1] = arr[y12];
  param[2][2] = arr[P3];
  param[3][0] = arr[x12];
  param[3][1] = arr[y22];
  param[3][2] = arr[P4];
  param[4][0] = arr[x21];
  param[4][1] = arr[y11];
  param[4][2] = arr[P5];
  param[5][0] = arr[x22];
  param[5][1] = arr[y21];
  param[5][2] = arr[P6];
  param[6][0] = arr[x21];
  param[6][1] = arr[y12];
  param[6][2] = arr[P7];
  param[7][0] = arr[x22];
  param[7][1] = arr[y22];
  param[7][2] = arr[P8];
  BlockMul_parallel (_bk, _thr, param, n, 0, 8);
  matAdd_parallel (_bk, _thr, arr[P1], arr[P2], arr[C11], 0, n * n);
  matAdd_parallel (_bk, _thr, arr[P3], arr[P4], arr[C12], 0, n * n);
  matAdd_parallel (_bk, _thr, arr[P5], arr[P6], arr[C21], 0, n * n);
  matAdd_parallel (_bk, _thr, arr[P7], arr[P8], arr[C22], 0, n * n);
  {
    row = 0, i = 0;
    for (; row < n; (row++, i++))
      {
        {
          col = 0, j = 0;
          for (; col < n; (col++, j++))
            {
              Z[row * m + col] = arr[C11][i * n + j];
            }
        }
        {
          col = n, j = 0;
          for (; col < m; (col++, j++))
            {
              Z[row * m + col] = arr[C12][i * n + j];
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
              Z[row * m + col] = arr[C21][i * n + j];
            }
        }
        {
          col = n, j = 0;
          for (; col < m; (col++, j++))
            {
              Z[row * m + col] = arr[C22][i * n + j];
            }
        }
      }
  }
  {
    i = 0;
    for (; i < arrs; i++)
      {
        free (arr[i]);
      }
  }
}

void
BlockMul_parallel (int (*_bk) (void), struct thread_data *_thr,
                   double *(*param)[3], int n, int i1, int i2)
{
  int i;
  {
    int i = i1;
    int I_end = i2;
    struct BlockTask *pthis;
    int spawned = 0;

    int do_many_bk (void)
    {
      if (_thr->exiting == EXITING_EXCEPTION
          || _thr->exiting == EXITING_CANCEL)
        {
          while (spawned-- > 0)
            {
              pthis = (struct BlockTask *) wait_rslt (_thr, 0);
              free (pthis);
            }
          _bk ();
        }
      else;
      if (!spawned)
        _bk ();
      else;
      while (_thr->treq_top && 2 <= I_end - i)
        {
          int i2 = I_end;
          int i1;
          i1 = (1 + i + I_end) / 2;
          I_end = i1;
          pthis = (struct BlockTask *) malloc (sizeof (struct BlockTask));
          {
            (*pthis).param = param;
            (*pthis).n = n;
            (*pthis).i1 = i1;
            (*pthis).i2 = i2;
          }
          spawned++;
          make_and_send_task (_thr, 1, pthis, spawned == 1);
        }
      return 0;
    }
    if (_thr->req_cncl || _thr->task_top->cancellation || _thr->req)
      handle_reqs (do_many_bk, _thr);
    else;
    for (; i < I_end; i++)
      {
        BlockMul (do_many_bk, _thr, param[i][0], param[i][1], param[i][2], n);
      }
    while (spawned-- > 0)
      {
        pthis = (struct BlockTask *) wait_rslt (_thr, 1);
        if (pthis)
          {
            free (pthis);
          }
        else;
      }
    if (_thr->exiting == EXITING_EXCEPTION)
      handle_exception (_bk, _thr, _thr->exception_tag);
    else;
  }
}

void
matMul (double *A, double *B, double *C, int n)
{
  int row = 0;
  int col = 0;
  int k = 0;
  int i = 0;
  int j = 0;
  double sum;
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

void
matAdd_parallel (int (*_bk) (void), struct thread_data *_thr, double *A,
                 double *B, double *C, int i1, int i2)
{
  int i = 0;
  {
    int i = i1;
    int I_end2 = i2;
    struct matAddTask *pthis;
    int spawned2 = 0;

    int do_many_bk2 (void)
    {
      if (_thr->exiting == EXITING_EXCEPTION
          || _thr->exiting == EXITING_CANCEL)
        {
          while (spawned2-- > 0)
            {
              pthis = (struct matAddTask *) wait_rslt (_thr, 0);
              free (pthis);
            }
          _bk ();
        }
      else;
      if (!spawned2)
        _bk ();
      else;
      while (_thr->treq_top && 2 <= I_end2 - i)
        {
          int i2 = I_end2;
          int i1;
          i1 = (1 + i + I_end2) / 2;
          I_end2 = i1;
          pthis = (struct matAddTask *) malloc (sizeof (struct matAddTask));
          {
            (*pthis).A = A;
            (*pthis).B = B;
            (*pthis).C = C;
            (*pthis).i1 = i1;
            (*pthis).i2 = i2;
          }
          spawned2++;
          make_and_send_task (_thr, 2, pthis, spawned2 == 1);
        }
      return 0;
    }
    if (_thr->req_cncl || _thr->task_top->cancellation || _thr->req)
      handle_reqs (do_many_bk2, _thr);
    else;
    for (; i < I_end2; i++)
      {
        C[i] = A[i] + B[i];
      }
    while (spawned2-- > 0)
      {
        pthis = (struct matAddTask *) wait_rslt (_thr, 1);
        if (pthis)
          {
            free (pthis);
          }
        else;
      }
    if (_thr->exiting == EXITING_EXCEPTION)
      handle_exception (_bk, _thr, _thr->exception_tag);
    else;
  }
}

void
matAdd (double *A, double *B, double *C, int m)
{
  int row = 0;
  int col = 0;
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
  struct matAddTask *pthis = malloc (sizeof (struct matAddTask));
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
  struct BlockTask *pthis = malloc (sizeof (struct BlockTask));
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
  struct mm_start *pthis = malloc (sizeof (struct mm_start));
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

void (*task_doers[256]) (struct thread_data *, void *) =
{
(void (*)(struct thread_data *, void *)) do_mm_start_task,
    (void (*)(struct thread_data *, void *)) do_BlockTask_task,
    (void (*)(struct thread_data *, void *)) do_matAddTask_task};
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
