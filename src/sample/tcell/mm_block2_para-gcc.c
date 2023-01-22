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
#include <sys/time.h>
#include "th.h"
double d[N][N];
double e[N][N];

double get_wall_time (void);

void createarr (double temp[N][N]);

void matMul (int n);

void block_recursive (int (*_bk) (void), struct thread_data *_thr, int a_r,
                      int a_c, int b_r, int b_c, int c_r, int c_c, int len);

void print (double c[N][N]);

void printarray (double a[N][N]);

void merge (double A[N][N]);

void
worker_init (struct thread_data *_thr)
{
}

struct mm_start
{
  char _dummy_[1000];
};

struct mm_block_para
{
  int sa_r;
  int sa_c;
  int sb_r;
  int sb_c;
  int sc_r;
  int sc_c;
  int n;
  char _dummy_[1000];
};

void
do_mm_block_para_task (struct thread_data *_thr, struct mm_block_para *pthis)
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
  block_recursive (_bk, _thr, (*pthis).sa_r, (*pthis).sa_c, (*pthis).sb_r,
                   (*pthis).sb_c, (*pthis).sc_r, (*pthis).sc_c, (*pthis).n);
mm_block_para_exit:return;
}

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
  int i;
  int j;
  int a_r;
  int a_c;
  int b_r;
  int b_c;
  int c_r;
  int c_c;
  int len;
  double start1;
  double end1;
  double start2;
  double end2;
  double elapsed1;
  double elapsed2;
  len = N;
  a_r = 0;
  a_c = 0;
  b_r = 0;
  b_c = 0;
  c_r = 0;
  c_c = 0;
  fprintf (stderr, "mm_block2_para\'s result is as follows:   \n");
  fprintf (stderr,
           "-------------------------------------------------------------------------\n");
  fprintf (stderr, "Matrix size = %2d\n", N);
  fprintf (stderr, "threshold = %2d\n", 64);
  fprintf (stderr, "#workers = 36\n");
  srand (0);
  createarr (a);
  createarr (b);
  start1 = get_wall_time ();
  matMul (len);
  end1 = get_wall_time ();
  elapsed1 = end1 - start1;
  fprintf (stderr, "naive_time = %5.4f    ", elapsed1);
  start2 = get_wall_time ();
  block_recursive (_bk, _thr, a_r, a_c, b_r, b_c, c_r, c_c, len);
  merge (e);
  end2 = get_wall_time ();
  elapsed2 = end2 - start2;
  fprintf (stderr, "Block_time = %5.4f  \n", elapsed2);
  fprintf (stderr,
           "-------------------------------------------------------------------------\n");
  int flag = 0;
  {
    i = 0;
    for (; i < len; i++)
      {
        {
          j = 0;
          for (; j < len; j++)
            {
              if (fabs (e[i][j] - d[i][j]) > 1.0E-6)
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
  fprintf (stderr,
           "-------------------------------------------------------------------------\n");
  fprintf (stderr,
           "------------------- Output Matrices e and d ---------------------------\n\n");
mm_start_exit:return;
}

void
block_recursive (int (*_bk) (void), struct thread_data *_thr, int a_r,
                 int a_c, int b_r, int b_c, int c_r, int c_c, int len)
{
  int n = len;
  if (n <= 64)
    {
      mm_sub (a_r, a_c, b_r, b_c, c_r, c_c, len, _thr->id);
    }
  else
    {
      int sa1_r;
      int sa1_c;
      int sb1_r;
      int sb1_c;
      int sc1_r;
      int sc1_c;
      int sa2_r;
      int sa2_c;
      int sb2_r;
      int sb2_c;
      int sc2_r;
      int sc2_c;
      int sa3_r;
      int sa3_c;
      int sb3_r;
      int sb3_c;
      int sc3_r;
      int sc3_c;
      int sa4_r;
      int sa4_c;
      int sb4_r;
      int sb4_c;
      int sc4_r;
      int sc4_c;
      sa1_r = a_r;
      sa1_c = a_c;
      sa2_r = a_r;
      sa2_c = a_c + n / 2;
      sa3_c = a_c;
      sa3_r = a_r + n / 2;
      sa4_r = a_r + n / 2;
      sa4_c = a_c + n / 2;
      sb1_r = b_r;
      sb1_c = b_c;
      sb2_r = b_r;
      sb2_c = b_c + n / 2;
      sb3_c = b_c;
      sb3_r = b_r + n / 2;
      sb4_r = b_r + n / 2;
      sb4_c = b_c + n / 2;
      sc1_r = c_r;
      sc1_c = c_c;
      sc2_r = c_r;
      sc2_c = c_c + n / 2;
      sc3_c = c_c;
      sc3_r = c_r + n / 2;
      sc4_r = c_r + n / 2;
      sc4_c = c_c + n / 2;
      {
        struct mm_block_para *pthis;
        int spawned = 0;
        {

          int do_two_bk (void)
          {
            if (_thr->exiting == EXITING_EXCEPTION
                || _thr->exiting == EXITING_CANCEL)
              {
                while (spawned-- > 0)
                  {
                    wait_rslt (_thr, 0);
                  }
                _bk ();
              }
            else;
            if (spawned)
              return 0;
            else;
            _bk ();
            while (_thr->treq_top)
              {
                pthis =
                  (struct mm_block_para *)
                  malloc (sizeof (struct mm_block_para));
                {
                  (*pthis).sa_r = sa2_r;
                  (*pthis).sa_c = sa2_c;
                  (*pthis).sb_r = sb3_r;
                  (*pthis).sb_c = sb3_c;
                  (*pthis).sc_r = sc1_r;
                  (*pthis).sc_c = sc1_c;
                  (*pthis).n = n / 2;
                }
                spawned = 1;
                make_and_send_task (_thr, 1, pthis, 1);
                return 1;
              }
            return 0;
          }
          if (_thr->req_cncl || _thr->task_top->cancellation || _thr->req)
            handle_reqs (do_two_bk, _thr);
          else;
          {
            block_recursive (do_two_bk, _thr, sa1_r, sa1_c, sb1_r, sb1_c,
                             sc1_r, sc1_c, n / 2);
          }
        }
        if (spawned)
          if (pthis = wait_rslt (_thr, 1))
            {
              {
              }
              free (pthis);
            }
          else if (_thr->exiting == EXITING_EXCEPTION)
            handle_exception (_bk, _thr, _thr->exception_tag);
          else;
        else
          {
            block_recursive (_bk, _thr, sa2_r, sa2_c, sb3_r, sb3_c, sc1_r,
                             sc1_c, n / 2);
          }
      }
      {
        struct mm_block_para *pthis;
        int spawned2 = 0;
        {

          int do_two_bk2 (void)
          {
            if (_thr->exiting == EXITING_EXCEPTION
                || _thr->exiting == EXITING_CANCEL)
              {
                while (spawned2-- > 0)
                  {
                    wait_rslt (_thr, 0);
                  }
                _bk ();
              }
            else;
            if (spawned2)
              return 0;
            else;
            _bk ();
            while (_thr->treq_top)
              {
                pthis =
                  (struct mm_block_para *)
                  malloc (sizeof (struct mm_block_para));
                {
                  (*pthis).sa_r = sa2_r;
                  (*pthis).sa_c = sa2_c;
                  (*pthis).sb_r = sb4_r;
                  (*pthis).sb_c = sb4_c;
                  (*pthis).sc_r = sc2_r;
                  (*pthis).sc_c = sc2_c;
                  (*pthis).n = n / 2;
                }
                spawned2 = 1;
                make_and_send_task (_thr, 1, pthis, 1);
                return 1;
              }
            return 0;
          }
          if (_thr->req_cncl || _thr->task_top->cancellation || _thr->req)
            handle_reqs (do_two_bk2, _thr);
          else;
          {
            block_recursive (do_two_bk2, _thr, sa1_r, sa1_c, sb2_r, sb2_c,
                             sc2_r, sc2_c, n / 2);
          }
        }
        if (spawned2)
          if (pthis = wait_rslt (_thr, 1))
            {
              {
              }
              free (pthis);
            }
          else if (_thr->exiting == EXITING_EXCEPTION)
            handle_exception (_bk, _thr, _thr->exception_tag);
          else;
        else
          {
            block_recursive (_bk, _thr, sa2_r, sa2_c, sb4_r, sb4_c, sc2_r,
                             sc2_c, n / 2);
          }
      }
      {
        struct mm_block_para *pthis;
        int spawned3 = 0;
        {

          int do_two_bk3 (void)
          {
            if (_thr->exiting == EXITING_EXCEPTION
                || _thr->exiting == EXITING_CANCEL)
              {
                while (spawned3-- > 0)
                  {
                    wait_rslt (_thr, 0);
                  }
                _bk ();
              }
            else;
            if (spawned3)
              return 0;
            else;
            _bk ();
            while (_thr->treq_top)
              {
                pthis =
                  (struct mm_block_para *)
                  malloc (sizeof (struct mm_block_para));
                {
                  (*pthis).sa_r = sa4_r;
                  (*pthis).sa_c = sa4_c;
                  (*pthis).sb_r = sb3_r;
                  (*pthis).sb_c = sb3_c;
                  (*pthis).sc_r = sc3_r;
                  (*pthis).sc_c = sc3_c;
                  (*pthis).n = n / 2;
                }
                spawned3 = 1;
                make_and_send_task (_thr, 1, pthis, 1);
                return 1;
              }
            return 0;
          }
          if (_thr->req_cncl || _thr->task_top->cancellation || _thr->req)
            handle_reqs (do_two_bk3, _thr);
          else;
          {
            block_recursive (do_two_bk3, _thr, sa3_r, sa3_c, sb1_r, sb1_c,
                             sc3_r, sc3_c, n / 2);
          }
        }
        if (spawned3)
          if (pthis = wait_rslt (_thr, 1))
            {
              {
              }
              free (pthis);
            }
          else if (_thr->exiting == EXITING_EXCEPTION)
            handle_exception (_bk, _thr, _thr->exception_tag);
          else;
        else
          {
            block_recursive (_bk, _thr, sa4_r, sa4_c, sb3_r, sb3_c, sc3_r,
                             sc3_c, n / 2);
          }
      }
      {
        struct mm_block_para *pthis;
        int spawned4 = 0;
        {

          int do_two_bk4 (void)
          {
            if (_thr->exiting == EXITING_EXCEPTION
                || _thr->exiting == EXITING_CANCEL)
              {
                while (spawned4-- > 0)
                  {
                    wait_rslt (_thr, 0);
                  }
                _bk ();
              }
            else;
            if (spawned4)
              return 0;
            else;
            _bk ();
            while (_thr->treq_top)
              {
                pthis =
                  (struct mm_block_para *)
                  malloc (sizeof (struct mm_block_para));
                {
                  (*pthis).sa_r = sa4_r;
                  (*pthis).sa_c = sa4_c;
                  (*pthis).sb_r = sb4_r;
                  (*pthis).sb_c = sb4_c;
                  (*pthis).sc_r = sc4_r;
                  (*pthis).sc_c = sc4_c;
                  (*pthis).n = n / 2;
                }
                spawned4 = 1;
                make_and_send_task (_thr, 1, pthis, 1);
                return 1;
              }
            return 0;
          }
          if (_thr->req_cncl || _thr->task_top->cancellation || _thr->req)
            handle_reqs (do_two_bk4, _thr);
          else;
          {
            block_recursive (do_two_bk4, _thr, sa3_r, sa3_c, sb2_r, sb2_c,
                             sc4_r, sc4_c, n / 2);
          }
        }
        if (spawned4)
          if (pthis = wait_rslt (_thr, 1))
            {
              {
              }
              free (pthis);
            }
          else if (_thr->exiting == EXITING_EXCEPTION)
            handle_exception (_bk, _thr, _thr->exception_tag);
          else;
        else
          {
            block_recursive (_bk, _thr, sa4_r, sa4_c, sb4_r, sb4_c, sc4_r,
                             sc4_c, n / 2);
          }
      }
    }
}

void
createarr (double temp[N][N])
{
  int i;
  int j;
  i = 0;
  j = 0;
  {
    i = 0;
    for (; i < N; i++)
      {
        j = 0;
        for (; j < N; j++)
          {
            temp[i][j] = rand () / (RAND_MAX + 1.0);
          }
      }
  }
}

void
print (double c[N][N])
{
  int i;
  int j;
  fprintf (stderr, "\n====================================\n");
  {
    i = 0;
    for (; i < N; i++)
      {
        {
          j = 0;
          for (; j < N; j++)
            {
              fprintf (stderr, "%5.4f ", c[i][j]);
            }
        }
        fprintf (stderr, "\n");
      }
  }
  fprintf (stderr, "====================================\n");
}

void
printarray (double a[N][N])
{
  int i;
  int j;
  fprintf (stderr, "====================================\n");
  {
    i = 0;
    for (; i < N; i++)
      {
        {
          j = 0;
          for (; j < N; j++)
            {
              fprintf (stderr, "%5.4f  ", a[i][j]);
            }
        }
        fprintf (stderr, "\n");
      }
  }
  fprintf (stderr, "====================================\n");
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
matMul (int n)
{
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
                  sum += a[i][k] * b[k][j];
                }
            }
            d[i][j] = sum;
          }
      }
  }
}

void
merge (double A[N][N])
{
  int i = 0;
  int j = 0;
  int k = 0;
  {
    i = 0;
    for (; i < 36; i++)
      {
        j = 0;
        for (; j < N; j++)
          {
            k = 0;
            for (; k < N; k++)
              {
                A[j][k] += c[i][j][k];
              }
          }
      }
  }
}

void
send_mm_block_para_task (struct mm_block_para *pthis)
{
  send_int ((*pthis).sa_r);
  send_int ((*pthis).sa_c);
  send_int ((*pthis).sb_r);
  send_int ((*pthis).sb_c);
  send_int ((*pthis).sc_r);
  send_int ((*pthis).sc_c);
  send_int ((*pthis).n);
}

struct mm_block_para *
recv_mm_block_para_task ()
{
  struct mm_block_para *pthis = malloc (sizeof (struct mm_block_para));
  (*pthis).sa_r = recv_int ();
  (*pthis).sa_c = recv_int ();
  (*pthis).sb_r = recv_int ();
  (*pthis).sb_c = recv_int ();
  (*pthis).sc_r = recv_int ();
  (*pthis).sc_c = recv_int ();
  (*pthis).n = recv_int ();
  return pthis;
}

void
send_mm_block_para_rslt (struct mm_block_para *pthis)
{
  free (pthis);
}

void
recv_mm_block_para_rslt (struct mm_block_para *pthis)
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
    (void (*)(struct thread_data *, void *)) do_mm_block_para_task};
void (*task_senders[256]) (void *) =
{
(void (*)(void *)) send_mm_start_task,
    (void (*)(void *)) send_mm_block_para_task};
void *(*task_receivers[256]) () =
{
(void *(*)()) recv_mm_start_task, (void *(*)()) recv_mm_block_para_task};

void (*rslt_senders[256]) (void *) =
{
(void (*)(void *)) send_mm_start_rslt,
    (void (*)(void *)) send_mm_block_para_rslt};
void (*rslt_receivers[256]) (void *) =
{
(void (*)(void *)) recv_mm_start_rslt,
    (void (*)(void *)) recv_mm_block_para_rslt};

struct worker_data
{
};
