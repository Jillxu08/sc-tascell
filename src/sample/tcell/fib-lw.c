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
struct do_fib_task_frame;
struct _bk_in_do_fib_task_frame;
struct do_fib_start_task_frame;
struct _bk_in_do_fib_start_task_frame;
struct fib_frame;
struct do_two_bk_in_fib_frame;
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
#include<sys/time.h>

int printf (char const *, ...);

int fprintf (FILE *, char const *, ...);

int fputc (int, FILE *);

void *malloc (size_t);

void free (void *);

double sqrt (double);

double fabs (double);

double
elapsed_time (struct timeval tp[2])
{
  return (tp[1]).tv_sec - (tp[0]).tv_sec + 1.0E-6 * ((tp[1]).tv_usec -
                                                     (tp[0]).tv_usec);
}

long N0 = 0;

double
my_probability (int n)
{
  if (n < 20)
    return (double) n / 20.0;
  else
    return 1.0;
}

long fib (char *esp, closure_t * _bk, struct thread_data *_thr, long n);

struct fib
{
  long n;
  long r;
  char _dummy_[1000];
};

struct _bk_in_do_fib_task_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  int ifexp_result;
  int ifexp_result2;
  struct do_fib_task_frame *xfp;
};

struct do_fib_task_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  long tmp;
  struct fib *pthis;
  struct thread_data *_thr;
  closure_t _bk;
};

char *
_bk_in_do_fib_task (char *esp, void *xfp0)
{
  int ifexp_result2;
  int ifexp_result;
  char *new_esp;
  struct _bk_in_do_fib_task_frame *efp;
  struct do_fib_task_frame *xfp = xfp0;
  size_t esp_flag = (size_t) esp & 3;
  char *parmp = (char *) ((size_t) esp ^ esp_flag);
LGOTO:;
  efp = (struct _bk_in_do_fib_task_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct _bk_in_do_fib_task_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
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
do_fib_task (char *esp, struct thread_data *_thr, struct fib *pthis)
{
  long tmp;
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct do_fib_task_frame *efp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct do_fib_task_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct do_fib_task_frame) + sizeof (Align_t) +
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
  efp = (struct do_fib_task_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct do_fib_task_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  {
    {
      new_esp = esp;
      while (__builtin_expect
             ((tmp =
               fib (new_esp, &efp->_bk, _thr, (*pthis).n)) == (long) 0 - 1, 0)
             && __builtin_expect ((efp->tmp_esp = *((char **) esp)) != 0, 1))
        {
          efp->tmp = tmp;
          efp->pthis = pthis;
          efp->_thr = _thr;
          efp->_bk.fun = _bk_in_do_fib_task;
          efp->_bk.fr = (void *) efp;
          efp->call_id = 0;
          return;
        L_CALL:;
          tmp = efp->tmp;
          pthis = efp->pthis;
          _thr = efp->_thr;
          new_esp = esp + 1;
        }
    }
    (*pthis).r = tmp;
  }
fib_exit:return;
}

struct fib_start
{
  long n;
  long r;
  char _dummy_[1000];
};

struct _bk_in_do_fib_start_task_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  int ifexp_result3;
  int ifexp_result4;
  struct do_fib_start_task_frame *xfp;
};

struct do_fib_start_task_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  struct timeval tp[2];
  long tmp2;
  struct fib_start *pthis;
  struct thread_data *_thr;
  closure_t _bk;
};

char *
_bk_in_do_fib_start_task (char *esp, void *xfp0)
{
  int ifexp_result4;
  int ifexp_result3;
  char *new_esp;
  struct _bk_in_do_fib_start_task_frame *efp;
  struct do_fib_start_task_frame *xfp = xfp0;
  size_t esp_flag = (size_t) esp & 3;
  char *parmp = (char *) ((size_t) esp ^ esp_flag);
LGOTO:;
  efp = (struct _bk_in_do_fib_start_task_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct _bk_in_do_fib_start_task_frame) +
               sizeof (Align_t) + -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  {
    if ((*xfp->_thr).exiting == EXITING_EXCEPTION)
      ifexp_result3 = 1;
    else
      {
        if ((*xfp->_thr).exiting == EXITING_CANCEL)
          ifexp_result4 = 1;
        else
          ifexp_result4 = 0;
        ifexp_result3 = ifexp_result4;
      }
    if (ifexp_result3)
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
do_fib_start_task (char *esp, struct thread_data *_thr,
                   struct fib_start *pthis)
{
  long tmp2;
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct do_fib_start_task_frame *efp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct do_fib_start_task_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct do_fib_start_task_frame) +
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
  efp = (struct do_fib_start_task_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct do_fib_start_task_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  fprintf (stderr, "start fib(%ld)\n", (*pthis).n);
  N0 = (*pthis).n;
  gettimeofday (efp->tp, 0);
  {
    {
      new_esp = esp;
      while (__builtin_expect
             ((tmp2 =
               fib (new_esp, &efp->_bk, _thr, (*pthis).n)) == (long) 0 - 1, 0)
             && __builtin_expect ((efp->tmp_esp = *((char **) esp)) != 0, 1))
        {
          efp->tmp2 = tmp2;
          efp->pthis = pthis;
          efp->_thr = _thr;
          efp->_bk.fun = _bk_in_do_fib_start_task;
          efp->_bk.fr = (void *) efp;
          efp->call_id = 0;
          return;
        L_CALL2:;
          tmp2 = efp->tmp2;
          pthis = efp->pthis;
          _thr = efp->_thr;
          new_esp = esp + 1;
        }
    }
    (*pthis).r = tmp2;
  }
  gettimeofday (efp->tp + 1, 0);
  fprintf (stderr, "time: %lf\n", elapsed_time (efp->tp));
fib_start_exit:return;
}

struct do_two_bk_in_fib_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  int ifexp_result5;
  int ifexp_result6;
  struct fib_frame *xfp;
};

struct fib_frame
{
  char *tmp_esp;
  char *argp;
  int call_id;
  int spawned;
  struct fib *pthis;
  long s2;
  long s1;
  void *tmp3;
  int ifexp_result7;
  int ifexp_result8;
  int ifexp_result9;
  int ifexp_result10;
  long n;
  struct thread_data *_thr;
  closure_t *_bk;
  closure_t do_two_bk;
};

char *
do_two_bk_in_fib (char *esp, void *xfp0)
{
  int ifexp_result6;
  int ifexp_result5;
  char *new_esp;
  struct do_two_bk_in_fib_frame *efp;
  struct fib_frame *xfp = xfp0;
  size_t esp_flag = (size_t) esp & 3;
  char *parmp = (char *) ((size_t) esp ^ esp_flag);
  char *argp;
LGOTO:;
  efp = (struct do_two_bk_in_fib_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct do_two_bk_in_fib_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  {
    if ((*xfp->_thr).exiting == EXITING_EXCEPTION)
      ifexp_result5 = 1;
    else
      {
        if ((*xfp->_thr).exiting == EXITING_CANCEL)
          ifexp_result6 = 1;
        else
          ifexp_result6 = 0;
        ifexp_result5 = ifexp_result6;
      }
    if (ifexp_result5)
      {
        while ((xfp->spawned)-- > 0)
          {
            new_esp = esp;
            while (wait_rslt (new_esp, xfp->_thr, 0),
                   __builtin_expect ((efp->tmp_esp =
                                      *((char **) esp)) != 0, 0))
              {
                char *goto_fr;
                *((char **) esp) = 0;
                efp->ifexp_result5 = ifexp_result5;
                efp->ifexp_result6 = ifexp_result6;
                efp->xfp = xfp;
                goto_fr = lw_call (efp->tmp_esp);
                if (goto_fr && (char *) goto_fr < (char *) efp)
                  return goto_fr;
                else;
                if ((char *) goto_fr == (char *) efp)
                  goto LGOTO;
                else;
                ifexp_result5 = efp->ifexp_result5;
                ifexp_result6 = efp->ifexp_result6;
                xfp = efp->xfp;
                new_esp = esp + 1;
              }
          }
        {
          char *goto_fr;
          argp =
            (char *) ((Align_t *) esp +
                      (sizeof (char *) + sizeof (Align_t) +
                       -1) / sizeof (Align_t));
          *((closure_t **) argp) = xfp->_bk;
          efp->ifexp_result5 = ifexp_result5;
          efp->ifexp_result6 = ifexp_result6;
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
          ifexp_result5 = efp->ifexp_result5;
          ifexp_result6 = efp->ifexp_result6;
          xfp = efp->xfp;
        }
      }
    else;
  }
  if (xfp->spawned)
    {
      *((int *) efp) = 0;
      return 0;
    }
  else;
  {
    char *goto_fr;
    argp =
      (char *) ((Align_t *) esp +
                (sizeof (char *) + sizeof (Align_t) + -1) / sizeof (Align_t));
    *((closure_t **) argp) = xfp->_bk;
    efp->ifexp_result5 = ifexp_result5;
    efp->ifexp_result6 = ifexp_result6;
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
    ifexp_result5 = efp->ifexp_result5;
    ifexp_result6 = efp->ifexp_result6;
    xfp = efp->xfp;
  }
  while ((*xfp->_thr).treq_top)
    {
      xfp->pthis = (struct fib *) malloc (sizeof (struct fib));
      {
        (*xfp->pthis).n = xfp->n - 2;
      }
      xfp->spawned = 1;
      make_and_send_task (xfp->_thr, 0, xfp->pthis, 1);
      {
        *((int *) efp) = 1;
        return 0;
      }
    }
  {
    *((int *) efp) = 0;
    return 0;
  }
  return 0;
}

long
fib (char *esp, closure_t * _bk, struct thread_data *_thr, long n)
{
  int ifexp_result10;
  int ifexp_result9;
  int ifexp_result8;
  int ifexp_result7;
  void *tmp3;
  long s1;
  long s2;
  struct fib *pthis;
  int spawned;
  size_t esp_flag = (size_t) esp & 3;
  char *new_esp;
  struct fib_frame *efp;
  if (esp_flag)
    {
      esp = (char *) ((size_t) esp ^ esp_flag);
      efp = (struct fib_frame *) esp;
      esp =
        (char *) ((Align_t *) esp +
                  (sizeof (struct fib_frame) + sizeof (Align_t) +
                   -1) / sizeof (Align_t));
      *((char **) esp) = 0;
    LGOTO:switch ((*efp).call_id)
        {
        case 0:
          goto L_CALL3;
        case 1:
          goto L_CALL4;
        case 2:
          goto L_CALL5;
        case 3:
          goto L_CALL6;
        case 4:
          goto L_CALL7;
        }
      goto L_CALL3;
    }
  else;
  efp = (struct fib_frame *) esp;
  esp =
    (char *) ((Align_t *) esp +
              (sizeof (struct fib_frame) + sizeof (Align_t) +
               -1) / sizeof (Align_t));
  *((char **) esp) = 0;
  if (n <= 2)
    return 1;
  else
    {
      {
        spawned = 0;
        {
          {
            if ((*_thr).req_cncl)
              ifexp_result7 = 1;
            else
              {
                {
                  if ((*(*_thr).task_top).cancellation)
                    ifexp_result9 = 1;
                  else
                    {
                      if ((*_thr).req)
                        ifexp_result10 = 1;
                      else
                        ifexp_result10 = 0;
                      ifexp_result9 = ifexp_result10;
                    }
                  if (ifexp_result9)
                    ifexp_result8 = 1;
                  else
                    ifexp_result8 = 0;
                }
                ifexp_result7 = ifexp_result8;
              }
            if (ifexp_result7)
              {
                new_esp = esp;
                while (handle_reqs (new_esp, &efp->do_two_bk, _thr),
                       __builtin_expect ((efp->tmp_esp =
                                          *((char **) esp)) != 0, 0))
                  {
                    efp->spawned = spawned;
                    efp->pthis = pthis;
                    efp->s2 = s2;
                    efp->s1 = s1;
                    efp->tmp3 = tmp3;
                    efp->ifexp_result7 = ifexp_result7;
                    efp->ifexp_result8 = ifexp_result8;
                    efp->ifexp_result9 = ifexp_result9;
                    efp->ifexp_result10 = ifexp_result10;
                    efp->n = n;
                    efp->_thr = _thr;
                    efp->_bk = _bk;
                    efp->do_two_bk.fun = do_two_bk_in_fib;
                    efp->do_two_bk.fr = (void *) efp;
                    efp->call_id = 0;
                    return (long) 0 - 1;
                  L_CALL3:;
                    spawned = efp->spawned;
                    pthis = efp->pthis;
                    s2 = efp->s2;
                    s1 = efp->s1;
                    tmp3 = efp->tmp3;
                    ifexp_result7 = efp->ifexp_result7;
                    ifexp_result8 = efp->ifexp_result8;
                    ifexp_result9 = efp->ifexp_result9;
                    ifexp_result10 = efp->ifexp_result10;
                    n = efp->n;
                    _thr = efp->_thr;
                    _bk = efp->_bk;
                    new_esp = esp + 1;
                  }
              }
            else;
          }
          {
            new_esp = esp;
            while (__builtin_expect
                   ((s1 =
                     fib (new_esp, &efp->do_two_bk, _thr,
                          n - 1)) == (long) 0 - 1, 0)
                   && __builtin_expect ((efp->tmp_esp = *((char **) esp)) !=
                                        0, 1))
              {
                efp->spawned = spawned;
                efp->pthis = pthis;
                efp->s2 = s2;
                efp->s1 = s1;
                efp->tmp3 = tmp3;
                efp->ifexp_result7 = ifexp_result7;
                efp->ifexp_result8 = ifexp_result8;
                efp->ifexp_result9 = ifexp_result9;
                efp->ifexp_result10 = ifexp_result10;
                efp->n = n;
                efp->_thr = _thr;
                efp->_bk = _bk;
                efp->do_two_bk.fun = do_two_bk_in_fib;
                efp->do_two_bk.fr = (void *) efp;
                efp->call_id = 1;
                return (long) 0 - 1;
              L_CALL4:;
                spawned = efp->spawned;
                pthis = efp->pthis;
                s2 = efp->s2;
                s1 = efp->s1;
                tmp3 = efp->tmp3;
                ifexp_result7 = efp->ifexp_result7;
                ifexp_result8 = efp->ifexp_result8;
                ifexp_result9 = efp->ifexp_result9;
                ifexp_result10 = efp->ifexp_result10;
                n = efp->n;
                _thr = efp->_thr;
                _bk = efp->_bk;
                new_esp = esp + 1;
              }
          }
        }
        if (spawned)
          {
            {
              new_esp = esp;
              while (__builtin_expect
                     ((tmp3 =
                       wait_rslt (new_esp, _thr, 1)) == (void *) 0 - 1, 0)
                     && __builtin_expect ((efp->tmp_esp = *((char **) esp)) !=
                                          0, 1))
                {
                  efp->spawned = spawned;
                  efp->pthis = pthis;
                  efp->s2 = s2;
                  efp->s1 = s1;
                  efp->tmp3 = tmp3;
                  efp->ifexp_result7 = ifexp_result7;
                  efp->ifexp_result8 = ifexp_result8;
                  efp->ifexp_result9 = ifexp_result9;
                  efp->ifexp_result10 = ifexp_result10;
                  efp->n = n;
                  efp->_thr = _thr;
                  efp->_bk = _bk;
                  efp->do_two_bk.fun = do_two_bk_in_fib;
                  efp->do_two_bk.fr = (void *) efp;
                  efp->call_id = 2;
                  return (long) 0 - 1;
                L_CALL5:;
                  spawned = efp->spawned;
                  pthis = efp->pthis;
                  s2 = efp->s2;
                  s1 = efp->s1;
                  tmp3 = efp->tmp3;
                  ifexp_result7 = efp->ifexp_result7;
                  ifexp_result8 = efp->ifexp_result8;
                  ifexp_result9 = efp->ifexp_result9;
                  ifexp_result10 = efp->ifexp_result10;
                  n = efp->n;
                  _thr = efp->_thr;
                  _bk = efp->_bk;
                  new_esp = esp + 1;
                }
            }
            if (pthis = tmp3)
              {
                {
                  s2 = (*pthis).r;
                }
                free (pthis);
              }
            else if ((*_thr).exiting == EXITING_EXCEPTION)
              {
                new_esp = esp;
                while (handle_exception
                       (new_esp, _bk, _thr, (*_thr).exception_tag),
                       __builtin_expect ((efp->tmp_esp =
                                          *((char **) esp)) != 0, 0))
                  {
                    efp->spawned = spawned;
                    efp->pthis = pthis;
                    efp->s2 = s2;
                    efp->s1 = s1;
                    efp->tmp3 = tmp3;
                    efp->ifexp_result7 = ifexp_result7;
                    efp->ifexp_result8 = ifexp_result8;
                    efp->ifexp_result9 = ifexp_result9;
                    efp->ifexp_result10 = ifexp_result10;
                    efp->n = n;
                    efp->_thr = _thr;
                    efp->_bk = _bk;
                    efp->do_two_bk.fun = do_two_bk_in_fib;
                    efp->do_two_bk.fr = (void *) efp;
                    efp->call_id = 3;
                    return (long) 0 - 1;
                  L_CALL6:;
                    spawned = efp->spawned;
                    pthis = efp->pthis;
                    s2 = efp->s2;
                    s1 = efp->s1;
                    tmp3 = efp->tmp3;
                    ifexp_result7 = efp->ifexp_result7;
                    ifexp_result8 = efp->ifexp_result8;
                    ifexp_result9 = efp->ifexp_result9;
                    ifexp_result10 = efp->ifexp_result10;
                    n = efp->n;
                    _thr = efp->_thr;
                    _bk = efp->_bk;
                    new_esp = esp + 1;
                  }
              }
            else;
          }
        else
          {
            new_esp = esp;
            while (__builtin_expect
                   ((s2 = fib (new_esp, _bk, _thr, n - 2)) == (long) 0 - 1, 0)
                   && __builtin_expect ((efp->tmp_esp = *((char **) esp)) !=
                                        0, 1))
              {
                efp->spawned = spawned;
                efp->pthis = pthis;
                efp->s2 = s2;
                efp->s1 = s1;
                efp->tmp3 = tmp3;
                efp->ifexp_result7 = ifexp_result7;
                efp->ifexp_result8 = ifexp_result8;
                efp->ifexp_result9 = ifexp_result9;
                efp->ifexp_result10 = ifexp_result10;
                efp->n = n;
                efp->_thr = _thr;
                efp->_bk = _bk;
                efp->do_two_bk.fun = do_two_bk_in_fib;
                efp->do_two_bk.fr = (void *) efp;
                efp->call_id = 4;
                return (long) 0 - 1;
              L_CALL7:;
                spawned = efp->spawned;
                pthis = efp->pthis;
                s2 = efp->s2;
                s1 = efp->s1;
                tmp3 = efp->tmp3;
                ifexp_result7 = efp->ifexp_result7;
                ifexp_result8 = efp->ifexp_result8;
                ifexp_result9 = efp->ifexp_result9;
                ifexp_result10 = efp->ifexp_result10;
                n = efp->n;
                _thr = efp->_thr;
                _bk = efp->_bk;
                new_esp = esp + 1;
              }
          }
      }
      return s1 + s2;
    }
}

void
send_fib_start_task (struct fib_start *pthis)
{
  send_long ((*pthis).n);
}

struct fib_start *
recv_fib_start_task ()
{
  struct fib_start *pthis;
  pthis = malloc (sizeof (struct fib_start));
  (*pthis).n = recv_long ();
  return pthis;
}

void
send_fib_start_rslt (struct fib_start *pthis)
{
  send_long ((*pthis).r);
  free (pthis);
}

void
recv_fib_start_rslt (struct fib_start *pthis)
{
  (*pthis).r = recv_long ();
}

void
send_fib_task (struct fib *pthis)
{
  send_long ((*pthis).n);
}

struct fib *
recv_fib_task ()
{
  struct fib *pthis;
  pthis = malloc (sizeof (struct fib));
  (*pthis).n = recv_long ();
  return pthis;
}

void
send_fib_rslt (struct fib *pthis)
{
  send_long ((*pthis).r);
  free (pthis);
}

void
recv_fib_rslt (struct fib *pthis)
{
  (*pthis).r = recv_long ();
}

void (*task_doers[256]) (char *, struct thread_data *, void *) =
{
(void (*)(char *, struct thread_data *, void *)) do_fib_task,
    (void (*)(char *, struct thread_data *, void *)) do_fib_start_task};
void (*task_senders[256]) (void *) =
{
(void (*)(void *)) send_fib_task, (void (*)(void *)) send_fib_start_task};

void *(*task_receivers[256]) () =
{
(void *(*)()) recv_fib_task, (void *(*)()) recv_fib_start_task};

void (*rslt_senders[256]) (void *) =
{
(void (*)(void *)) send_fib_rslt, (void (*)(void *)) send_fib_start_rslt};

void (*rslt_receivers[256]) (void *) =
{
(void (*)(void *)) recv_fib_rslt, (void (*)(void *)) recv_fib_start_rslt};

struct worker_data
{
};

void
worker_init (struct thread_data *_thr)
{
}
