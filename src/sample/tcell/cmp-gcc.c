#include <pthread.h>
#include <stdio.h>
#include<sys/time.h>
#include "sock.h"
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

enum Affinity
{ COMPACT, SCATTER, SHAREDMEMORY };

struct send_block
{
  char *buf;
  int len;
  int size;
  int rank;
};

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
extern void *(*task_allocators[256]) ();
extern void (*task_receivers[256]) (void *);
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
{ EV_SEND_TASK_INSIDE, EV_SEND_TASK_OUTSIDE, EV_STRT_TASK_INSIDE,
    EV_STRT_TASK_OUTSIDE, EV_RSLT_TASK_INSIDE, EV_RSLT_TASK_OUTSIDE,
    EV_EXCP_TASK_INSIDE, EV_EXCP_TASK_OUTSIDE, EV_ABRT_TASK_INSIDE,
    EV_ABRT_TASK_OUTSIDE, EV_SEND_CNCL_INSIDE, EV_SEND_CNCL_OUTSIDE };
static char *ev_strings[] =
  { "EV-SEND-TASK-INSIDE", "EV-SEND-TASK-OUTSIDE", "EV-STRT-TASK-INSIDE",
"EV-STRT-TASK-OUTSIDE", "EV-RSLT-TASK-INSIDE", "EV-RSLT-TASK-OUTSIDE", "EV-EXCP-TASK-INSIDE",
"EV-EXCP-TASK-OUTSIDE", "EV-ABRT-TASK-INSIDE", "EV-ABRT-TASK-OUTSIDE", "EV-SEND-CNCL-INSIDE",
"EV-SEND-CNCL-OUTSIDE" };
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
int volatile progress;

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
  int ev_cnt[12];
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

void finalize_tcounter ();

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
  int port;
  char *node_name;
  char *initial_task;
  int auto_exit;
  int affinity;
  int always_flush_accepted_treq;
  enum Affinity thread_affinity;
  int cpu_num;
  int thread_per_cpu;
  int verbose;
  char *timechart_file;
};
extern struct runtime_option option;
#include "sendrecv.h"
#include <stdlib.h>

void handle_reqs (int (*)(void), struct thread_data *);

void handle_exception (int (*)(void), struct thread_data *, long);
#include<sys/time.h>
#include<stdint.h>

int printf (char const *, ...);

int fprintf (FILE *, char const *, ...);

int fputc (int, FILE *);

void *malloc (size_t);

void free (void *);

double sqrt (double);

double fabs (double);

void send_int (int n);

int recv_int (void);

void send_long (long n);

long recv_long (void);

void send_longlong (long long n);

long long recv_longlong (void);

int send_binary_header (int elmsize, int nelm);

int recv_binary_header (int *pelmsize, int *pnelm);

void send_binary_terminator (void);

void recv_binary_terminator (void);

void swap_int32s (int *a, int n);

int send_int32s (int *a, int nelm);

int recv_int32s (int *a, int nelm);

void swap_doubles (double *a, int n);

int send_double_seq (double *a, int nelm);

int recv_double_seq (double *a, int nelm);

int send_doubles (double *a, int nelm);

int recv_doubles (double *a, int nelm);
#if !defined(MIN)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
int N0 = 0;
extern unsigned int num_thrs;

double
cmp_probability (int n)
{
  return MIN (1.0, (double) num_thrs * ((double) n / (double) N0));
}

double
elapsed_time (struct timeval tp[2])
{
  return (tp[1]).tv_sec - (tp[0]).tv_sec + 1.0E-6 * ((tp[1]).tv_usec -
                                                     (tp[0]).tv_usec);
}

struct cmp
{
  int r;
  int n1;
  int n2;
  int *d1;
  int *d2;
  char _dummy_[1000];
};

int cmp_1 (int (*_bk) (void), struct thread_data *_thr, int n1, int n2,
           int *d1, int *d2);

void
do_cmp_task (struct thread_data *_thr, struct cmp *pthis)
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
  struct timeval tp[2];
  int i;
  int j;
  if (option.verbose >= 1)
    fprintf (stderr, "start %d %d\n", (*pthis).n1, (*pthis).n2);
  else;
  if (0 > (*pthis).n2)
    {
      (*pthis).d1 = (int *) malloc (sizeof (int) * (*pthis).n1);
      (*pthis).n2 = (*pthis).n1;
      (*pthis).d2 = (int *) malloc (sizeof (int) * (*pthis).n2);
      {
        i = 0;
        for (; i < (*pthis).n1; i++)
          {
            ((*pthis).d1)[i] = i;
          }
      }
      {
        i = 0;
        for (; i < (*pthis).n2; i++)
          {
            ((*pthis).d2)[i] = -i;
          }
      }
      N0 = (*pthis).n1;
      _thr->probability = 1.0;
      gettimeofday (tp, 0);
      wait_progress (_thr, 1);
      (*pthis).r =
        cmp_1 (_bk, _thr, (*pthis).n1, (*pthis).n2, (*pthis).d1, (*pthis).d2);
      gettimeofday (tp + 1, 0);
      fprintf (stderr, "time: %lf\n", elapsed_time (tp));
    }
  else
    {
      _thr->probability = cmp_probability (MIN ((*pthis).n1, (*pthis).n2));
      wait_progress (_thr, 1);
      (*pthis).r =
        cmp_1 (_bk, _thr, (*pthis).n1, (*pthis).n2, (*pthis).d1, (*pthis).d2);
    }
cmp_exit:return;
}

int
cmp_2 (int n1, int n2, int *d1, int *d2)
{
  int i;
  int j;
  int s = 0;
  {
    i = 0;
    for (; i < n1; i++)
      {
        j = 0;
        for (; j < n2; j++)
          {
            if ((d1[i] ^ d2[j]) == -1)
              s++;
            else;
          }
      }
  }
  return s;
}

int
cmp_1 (int (*_bk) (void), struct thread_data *_thr, int n1, int n2, int *d1,
       int *d2)
{
  int s1;
  int s2;
  if (n1 < 5)
    return cmp_2 (n1, n2, d1, d2);
  else;
  if (n1 > n2)
    {
      int n1_1 = n1 / 2;
      int n1_2 = n1 - n1_1;
      {
        struct cmp *pthis;
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
                pthis = (struct cmp *) malloc (sizeof (struct cmp));
                {
                  _thr->probability = cmp_probability (n1_1);
                  (*pthis).n1 = n1_2;
                  (*pthis).n2 = n2;
                  (*pthis).d1 = d1 + n1_1;
                  (*pthis).d2 = d2;
                }
                spawned = 1;
                make_and_send_task (_thr, 0, pthis, 1);
                return 1;
              }
            return 0;
          }
          if (_thr->req_cncl || _thr->task_top->cancellation || _thr->req)
            handle_reqs (do_two_bk, _thr);
          else;
          {
            s1 = cmp_1 (do_two_bk, _thr, n1_1, n2, d1, d2);
          }
        }
        if (spawned)
          if (pthis = wait_rslt (_thr, 1))
            {
              {
                s2 = (*pthis).r;
              }
              free (pthis);
            }
          else if (_thr->exiting == EXITING_EXCEPTION)
            handle_exception (_bk, _thr, _thr->exception_tag);
          else;
        else
          {
            wait_progress (_thr, 2);
            s2 = cmp_1 (_bk, _thr, n1_2, n2, d1 + n1_1, d2);
          }
      }
    }
  else
    {
      int n2_1 = n2 / 2;
      int n2_2 = n2 - n2_1;
      {
        struct cmp *pthis;
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
                pthis = (struct cmp *) malloc (sizeof (struct cmp));
                {
                  _thr->probability = cmp_probability (n2_1);
                  (*pthis).n1 = n1;
                  (*pthis).n2 = n2_2;
                  (*pthis).d1 = d1;
                  (*pthis).d2 = d2 + n2_1;
                }
                spawned2 = 1;
                make_and_send_task (_thr, 0, pthis, 1);
                return 1;
              }
            return 0;
          }
          if (_thr->req_cncl || _thr->task_top->cancellation || _thr->req)
            handle_reqs (do_two_bk2, _thr);
          else;
          {
            s1 = cmp_1 (do_two_bk2, _thr, n1, n2_1, d1, d2);
          }
        }
        if (spawned2)
          if (pthis = wait_rslt (_thr, 1))
            {
              {
                s2 = (*pthis).r;
              }
              free (pthis);
            }
          else if (_thr->exiting == EXITING_EXCEPTION)
            handle_exception (_bk, _thr, _thr->exception_tag);
          else;
        else
          {
            wait_progress (_thr, 2);
            s2 = cmp_1 (_bk, _thr, n1, n2_2, d1, d2 + n2_1);
          }
      }
    }
  return s1 + s2;
}

void
send_cmp_task (struct cmp *pthis)
{
  send_int ((*pthis).n1);
  send_int ((*pthis).n2);
  if (0 > (*pthis).n2)
    return;
  else;
  send_int32s ((*pthis).d1, (*pthis).n1);
  send_int32s ((*pthis).d2, (*pthis).n2);
}

struct cmp *
alloc_cmp_task (void)
{
  struct cmp *pthis = malloc (sizeof (struct cmp));
  return pthis;
}

void
recv_cmp_task (struct cmp *pthis)
{
  (*pthis).n1 = recv_int ();
  (*pthis).n2 = recv_int ();
  int i;
  if (!(0 > (*pthis).n2))
    {
      (*pthis).d1 = (int *) malloc (sizeof (int) * (*pthis).n1);
      (*pthis).d2 = (int *) malloc (sizeof (int) * (*pthis).n2);
      if ((*pthis).n2 > (*pthis).n1)
        {
          recv_int32s ((*pthis).d1, (*pthis).n1);
          recv_int32s ((*pthis).d2, (*pthis).n2 / 2);
          set_progress (1);
          recv_int32s ((*pthis).d2 + (*pthis).n2 / 2, (*pthis).n2 / 2);
          set_progress (2);
        }
      else
        {
          recv_int32s ((*pthis).d2, (*pthis).n2);
          recv_int32s ((*pthis).d1, (*pthis).n1 / 2);
          set_progress (1);
          recv_int32s ((*pthis).d1 + (*pthis).n1 / 2, (*pthis).n1 / 2);
          set_progress (2);
        }
    }
  else;
}

void
send_cmp_rslt (struct cmp *pthis)
{
  send_int ((*pthis).r);
  free ((*pthis).d1);
  free ((*pthis).d2);
  free (pthis);
}

void
recv_cmp_rslt (struct cmp *pthis)
{
  (*pthis).r = recv_int ();
}

void (*task_doers[256]) (struct thread_data *, void *) =
{
(void (*)(struct thread_data *, void *)) do_cmp_task};

void (*task_senders[256]) (void *) =
{
(void (*)(void *)) send_cmp_task};

void *(*task_allocators[256]) (void) =
{
(void *(*)(void)) alloc_cmp_task};

void (*task_receivers[256]) (void *) =
{
(void (*)(void *)) recv_cmp_task};

void (*rslt_senders[256]) (void *) =
{
(void (*)(void *)) send_cmp_rslt};

void (*rslt_receivers[256]) (void *) =
{
(void (*)(void *)) recv_cmp_rslt};

struct worker_data
{
};

void
worker_init (struct thread_data *_thr)
{
}
