;; worker2���顤ppopp08��˲��ɡ�dvbl��Ϥ���ľ���ΥС������
;; * �����ؤ�send�����Ƥμ���ˤĤ���sender����åɤ�ô��
;;   - �ƥ����send-queue���ɲ�
;;   - �����ؤ�send�ˤĤ��Ƥ� inms-queue���ɲ� => ������å�������������åɤ�ô��
;; * ���ޤޤǤФ�Ф�˽񤤤Ƥ�����å�->���塼�ɲäν��������� 
;; * �����μ����Ϥ��κݤι�¤�Υ��ԡ���Ǥ�������ݥ����Ϥ����ѹ�

;; *** �ʲ�̤�����β��ɤ��٤���
;; task�������餹��none���֤� or ���Ф餯�Ԥ� or "dvbl"(<- divisible flag�Υ����å��Υ����С��إå�)
;; areq: availability request �ʻŻ���ʬ��Ǥ�����֤ˤʤä����ֻ��򤹤�ˡ�
;; * �����Ф�none���֤����Ρ��ɤ��Ф�������
;;   o none�����ä��餫�ʤ餺areq������Τ���������ʤ��Ƥ褤��
;; * �ֻ�"dvbl"������ޤ�treq�򤽤��ˤ�����ʤ�
;; * treq_top����������areq_top��thread_data->req�ˤϥե饰�Τߡ�<- "req"��̾���Ϥ�����٤�����
;; treq��areq�ǥꥯ�����ȥ��塼��ͭ
;; [07/08/13]
;; * rslt_<>_body: �Ȥ��������Ǽ�äƤ���thread��¤�ΤϤ���ʤ�
;; * dvbl�ե饰�Υ����å��ϤȤ�٤��ʢ���areq�ε����ǡ�
;; * �Ȥ��ɤ��Τ����treq��any�ʳ���treq�ˤ��Ф��Ƥϼ�ʬ�����ˤʤ�ޤ�
;; ��treq any����������ˤʤ�ޤǡ�none���֤��ʤ�
;; ��ͳ��a->b ��treq���ƻŻ����ʤ��Τϡ�b->c��treq���Ƥ��뤫��Ǥ����礬¿���Τ�
;;         any��treq���ä����ɤ����ϼ�����ä����˳Ф��Ƥ���ɬ��
;; ���Ӥ��󤬤���������version: ~yasugi/lang/c/loadb/
(%ifndef* NF-TYPE
  (%defconstant NF-TYPE GCC)) ; one of (GCC LW-SC CL-SC XCC XCCCL)
(%include "rule/nestfunc-setrule.sh")

(c-exp "#include<stdio.h>")
(c-exp "#include<stdlib.h>")
(c-exp "#include<pthread.h>")
(c-exp "#include<sys/time.h>")
(c-exp "#include<getopt.h>")
#+tcell-gtk (c-exp "#include<gtk/gtk.h>")
(%include "worker3.sh")

;; (%cinclude "<stdio.h>" "<stdlib.h>" "<pthread.h>" "<sys/time.h>" "<getopt.h>"
;;            (:macro NULL EOF stdin stdout stderr PTHREAD-SCOPE-SYSTEM)
;; ;;            (:var FILE
;; ;;                  printf fprintf fputs fputc getc fflush atoi exit mallloc
;; ;;                  strcmp strstr strcpy strncpy
;; ;;                  pthread-cond-t pthread-condattr-t
;; ;;                  pthread-mutex-t pthread-mutexattr-t
;; ;;                  pthread-t pthread-attr-t
;; ;;                  pthread-create pthread-suspend pthread-cond-wait
;; ;;                  pthread-scope-system
;; ;;                  pthread-mutex-init pthread-cond-init
;; ;;                  suseconds-t time-t gettimeofday
;; ;;                  getopt optarg)
;; ;;            (:aggr timespec timeval timezone)
;;            )
;; (%defconstant NULL csym::NULL)
;; (%defconstant EOF csym::EOF)
;; (%defconstant stdin csym::stdin)
;; (%defconstant stdout csym::stdout)
;; (%defconstant stderr csym::stderr)
;; (%defconstant PTHREAD-SCOPE-SYSTEM csym::PTHREAD-SCOPE-SYSTEM)


#|
����������
�����֥������Ȼظ��ˤ���ʤ顥����
(def (struct hoge)
    index�Ȥ����ä���¤�ΤؤΥݥ���
  (def i int 0)
  (def ))

External data representation(XDR)

���Ρ��ɤ�Ʊ���Ż����ΤäƤ��뤬
�̤���ʬ��ô�����������⡤
�� ����������Ȥ��� "�ޡ���������ʬ" ����Ĺ������Ƥ���
|#

(def (systhr-create start-func arg)
    (fn int (ptr (fn (ptr void) (ptr void))) (ptr void))
  (def status int 0)
  (def tid pthread-t)
  (def attr pthread-attr-t)

  (csym::pthread-attr-init (ptr attr))
  (= status (csym::pthread-attr-setscope (ptr attr) PTHREAD-SCOPE-SYSTEM))
  (if (== status 0)
      (= status (pthread-create (ptr tid) (ptr attr) start-func arg))
    (= status (pthread-create (ptr tid) 0          start-func arg)))
  (return status))

(def (csym::mem-error str) (csym::fn void (ptr (const char)))
  (csym::fputs str stderr)
  (csym::fputc #\Newline stderr))

;;; ���顼��å�����str�ȥ��ޥ�ɤ�stderr�˽���
(def (csym::proto-error str pcmd) (csym::fn void (ptr (const char)) (ptr (struct cmd)))
  (def i int)
  (csym::fputs str stderr)
  (csym::fputc #\> stderr)
  (csym::fputc #\Space stderr)
  (for ((= i 0) (< i (- (fref pcmd -> c) 1)) (inc i))
    (csym::fputs (aref (fref pcmd -> v) i) stderr)
    (csym::fputc #\Space stderr))
  (if (> (fref pcmd -> c) 0)
      (begin
       (csym::fputs (aref (fref pcmd -> v)
                          (- (fref pcmd -> c) 1))
                    stderr)
       (csym::fputc #\Newline stderr))))

;; ���ޥ�ɤΥ��塼����å�������ѿ��դ�
(def (struct cmd-queue)
  (def len int)
  (def queue (ptr (struct cmd-list)))   ; queue������
  (def arg-buf (ptr char))              ; (* len MAXCMDC ARG-SIZE-MAX) �ư�����ʸ������¸�ΰ�
  (def arg-size-max int)
  (def in (ptr (struct cmd-list)))
  (def out (ptr (struct cmd-list)))
  (def mut pthread-mutex-t)
  (def cond pthread-cond-t))

;; ���塼�����������
(def (csym::make-cmd-queue length maxcmdc arg-size-max) (csym::fn (ptr (struct cmd-queue)) int int int)
  (def p-newq (ptr (struct cmd-queue)))
  (defs int i j)
  (= p-newq (cast (ptr (struct cmd-queue))
              (csym::malloc (sizeof (struct cmd-queue)))))
  (= (fref p-newq -> len) length)
  (= (fref p-newq -> queue) (cast (ptr (struct cmd-list))
                              (csym::malloc (* length (sizeof (struct cmd-list))))))
  (= (fref p-newq -> arg-buf)
     (cast (ptr char) (csym::malloc (* length maxcmdc arg-size-max (sizeof char)))))
  (= (fref p-newq -> arg-size-max) arg-size-max)
  (= (fref p-newq -> in) (fref p-newq -> queue))
  (= (fref p-newq -> out) (fref p-newq -> queue))
  (csym::pthread-mutex-init (ptr (fref p-newq -> mut)) 0)
  (csym::pthread-cond-init (ptr (fref p-newq -> cond)) 0)
  ;; ���塼����Ȥν����
  (for ((= i 0) (< i length) (inc i))
    ;; ���
    (= (fref (aref (fref p-newq -> queue) i) next)
       (ptr (aref (fref p-newq -> queue) (% (+ i 1) length))))
    ;; queue[i].cmd.v[j] �˥Хåե���Ŭ�ڤʰ��֤ؤΥݥ���
    ;; ��i���ܤΥ��ޥ��ʸ�����j���ܤΰ�����ʸ�����
    (for ((= j 0) (< j maxcmdc) (inc j))
      (= (aref (fref (aref (fref p-newq -> queue) i) cmd v) j)
         (+ (fref p-newq -> arg-buf)
            (* maxcmdc arg-size-max i)
            (* arg-size-max j)))))
  (return p-newq))

;; queue�ؤΥ�å������ɲáʹ�¤�Τ����ʸ����Υ��ԡ���ȼ����
(def (csym::enqueue-command pcmd body task-no pq)
    (csym::fn void (ptr (struct cmd)) (ptr void) int (ptr (struct cmd-queue)))
  (def i int)
  (def len size-t)
  (def q (ptr (struct cmd-list)))
  (def len-max int (fref pq -> arg-size-max))
  (csym::pthread-mutex-lock (ptr (fref pq -> mut)))
  (= q (fref pq -> in))
  (if (== (fref q -> next) (fref pq -> out))
      (begin
       (csym::perror "queue overflow~%")
       (csym::exit 1)))
  (= (fref pq -> in) (fref q -> next))
  ;; cmd�Υ��ԡ�
  (= (fref q -> cmd c) (fref pcmd -> c))
  (= (fref q -> cmd node) (fref pcmd -> node))
  (for ((= i 0) (< i (fref pcmd -> c)) (inc i))
    (= len (csym::strlen (aref (fref pcmd -> v) i)))
    (if (>= len len-max)
        (csym::proto-error "too long cmd" pcmd))
    (csym::strncpy (aref (fref q -> cmd v) i)
                   (aref (fref pcmd -> v) i)
                   (+ len 1)))
  (= (fref q -> body) body)             ; task, rslt��body
  (= (fref q -> task-no) task-no)       ; task�ֹ�
  (csym::pthread-mutex-unlock (ptr (fref pq -> mut)))
  ;; �ɲä������Ȥ�����
  (csym::pthread-cond-signal (ptr (fref pq -> cond)))
  )

;; �����������塼
(def inms-queue (ptr (struct cmd-queue)))
;; �����ؤ��������ꥭ�塼
(def send-queue (ptr (struct cmd-queue)))

;; ����do-two�����褿��"dvbl"�������ե饰
(def divisibility-flag int 1)           ; ���ä��Ƥ��ޤ�����

;; ��������Υ�å����������Хåե�
(def buf (array char BUFSIZE))
(def cmd-buf (struct cmd))

;; �����Фؤ������������å�(<0:stdin/out)��main������
(def sv-socket int)

;;; fgets��OUTSIDE����Υ�å������������� -> struct cmd
;;; 1�Ԥ��ɤ߹�����塤���ڡ�����NULLʸ�����֤������뤳�Ȥ�ʸ�����ʬ�䤹�롥
;;; �ޤ���cmd.v[]��ʬ�䤵�줿��ʸ�����ؤ��褦�ˤ���
;;;   fgets �� 0-terminated buffer ���֤���
;;;   fgets returns 0 �ΤȤ����н�ɬ��?
;;; buf, cmd-buf �ϰ�ĤʤΤ���աʺ���main���ܥ���åɤ�������ʤ���
(def (csym::recv-command) (csym::fn (ptr (struct cmd)))
  (defs char p c)
  (def b (ptr char) buf)
  (def cmdc int)

  (csym::receive-line b BUFSIZE sv-socket) ;; (csym::fgets b BUFSIZE stdin)
  (= (fref cmd-buf node) OUTSIDE)
  ;; p:�������ʸ����c:���ߤ�ʸ��
  (= cmdc 0)
  (for ((exps (= p #\NULL) (= c (mref b)))
        c
        (exps (= p c)      (= c (mref (++ b)))))
    (if (or (== c #\Space) (== c #\Newline))
        (begin (= c #\NULL)
               (= (mref b) #\NULL))
      (if (and (== p #\NULL) (< cmdc MAXCMDC))
          (= (aref (fref cmd-buf v) (inc cmdc)) b))))
  (= (fref cmd-buf c) cmdc)
  (return (ptr cmd-buf)))

;;; struct cmd -> output��stdout�ء�
;;; sender����åɤ�sender-loop�Τߤ���ƤФ��
(def (csym::send-out-command pcmd) (csym::fn void (ptr (struct cmd)))
  (def i int)
  (def narg int (fref pcmd -> c))
  (for ((= i 0) (< i (- narg 1)) (inc i))
    (csym::send-string (aref (fref pcmd -> v) i) sv-socket)
    (csym::send-char #\Space sv-socket))
  (if (> narg 0)
      (begin
       (csym::send-string (aref (fref pcmd -> v) (- narg 1)) sv-socket)
       (csym::send-char #\Newline sv-socket))))

;;; �Ρ�����/������˥��ޥ�ɤ�����
;;; body��task, rslt�����Ρ�����ʳ��Υ��ޥ�ɤǤ�NULL��
(def (csym::send-command pcmd body task-no) (csym::fn void (ptr (struct cmd)) (ptr void) int)
  (if (== (fref pcmd -> node) OUTSIDE)
      (csym::enqueue-command pcmd body task-no send-queue) ; ����������
    (csym::enqueue-command pcmd body task-no inms-queue) ; ����������
    ))

;; �Ρ������å�������queue �ˤ��ޤä���å���������
;; ������å�������������åɤϤ����򤰤뤰��
(def (exec-queue-cmd arg) (fn (ptr void) (ptr void))
  (def pcmd (ptr (struct cmd)))
  (def body (ptr void))
  (def p-mut (ptr pthread-mutex-t) (ptr (fref inms-queue -> mut)))
  (def p-cond (ptr pthread-cond-t) (ptr (fref inms-queue -> cond)))
  (loop
    ;; �����Ƥ����٤���ʤ��褦�˥��塼����κ���Ϥ��Ȥ�
    ;; ���ɲ����cmd�����Ƥ򻲾Ȥ��ʤ��褦�˥�å���ɬ��
    ;; �����Ф�����ͤ����ʤΤ�recv-*��Υ�å������פ���
    (csym::pthread-mutex-lock p-mut)
    (while (== (fref inms-queue -> in)
               (fref inms-queue -> out))
      (csym::pthread-cond-wait p-cond p-mut))
    (= pcmd (ptr (fref inms-queue -> out -> cmd)))
    (= body (fref inms-queue -> out -> body))
    (csym::pthread-mutex-unlock p-mut)
    ;; (csym::proto-error "INSIDE" pcmd)   ; debug print
    (cond
     ((== 0 (csym::strcmp (aref (fref pcmd -> v) 0) "task"))
      (csym::proto-error "INSIDE" pcmd) ; debug print
      (csym::recv-task pcmd body))
     ((== 0 (csym::strcmp (aref (fref pcmd -> v) 0) "rslt"))
      (csym::proto-error "INSIDE" pcmd) ; debug print
      (csym::recv-rslt pcmd body))
     ((== 0 (csym::strcmp (aref (fref pcmd -> v) 0) "treq"))
      (csym::recv-treq pcmd))
     ((== 0 (csym::strcmp (aref (fref pcmd -> v) 0) "none"))
      (csym::recv-none pcmd))
     ((== 0 (csym::strcmp (aref (fref pcmd -> v) 0) "rack"))
      (csym::recv-rack pcmd))
     (else
      (csym::proto-error "wrong cmd" pcmd)
      (csym::exit 0)))
    (csym::pthread-mutex-lock p-mut)
    (= (fref inms-queue -> out)   ; dequeue
       (fref inms-queue -> out -> next))
    (csym::pthread-mutex-unlock p-mut)
    ))

;; EOL�ޤǤ�̵��
(def (csym::read-to-eol) (csym::fn void void)
  (def c int)
  (while (!= EOF (= c (csym::receive_char sv-socket)))
                                        ;(!= EOF (= c (csym::getc stdin)))
    (if (== c #\Newline) (break))))

(def (csym::write-eol) (csym::fn void void)
  (csym::send-char #\Newline sv-socket))

(def (csym::flush-send) (csym::fn void void)
  (csym::fflush stdout))

;; sender����åɤϤ����򤰤뤰��
;; ���ϥ��ȥ꡼����å����Ƥ��ʤ��Τ����
;; ��sender����åɤ����Ȥ�ʤ��Ȳ����
(def (sender-loop arg) (fn (ptr void) (ptr void))
  (def pcmd (ptr (struct cmd)))
  (def p-mut (ptr pthread-mutex-t) (ptr (fref send-queue -> mut)))
  (def p-cond (ptr pthread-cond-t) (ptr (fref send-queue -> cond)))
  (def body (ptr void))
  (loop
    (csym::pthread-mutex-lock p-mut)
    (while (== (fref send-queue -> in)
               (fref send-queue -> out))
      ;; ���塼�����ʤ��Ԥ�
      (csym::pthread-cond-wait p-cond p-mut))
    (= pcmd (ptr (fref send-queue -> out -> cmd)))
    (csym::pthread-mutex-unlock p-mut)
    (csym::send-out-command pcmd)
    (= body (fref send-queue -> out -> body))
    (if body
        (begin
         (cond
          ((== 0 (csym::strcmp (aref (fref pcmd -> v) 0) "task"))
           ((aref task-senders (fref send-queue -> out -> task-no)) body)
           (csym::write-eol))
          ((== 0 (csym::strcmp (aref (fref pcmd -> v) 0) "rslt"))
           ((aref rslt-senders (fref send-queue -> out -> task-no)) body)
           (csym::write-eol)))
         (= (fref send-queue -> out -> body) 0)))
    (csym::flush-send)
    (if (== 0 (csym::strcmp (aref (fref pcmd -> v) 0) "none"))
        (= divisibility-flag 1))        ; none�����ä���dvbl�ե饰��on
    (csym::pthread-mutex-lock p-mut)
    (= (fref send-queue -> out)
       (fref send-queue -> out -> next)) ; ���塼�����ɤ��Ф�
    (csym::pthread-mutex-unlock p-mut)
    (csym::flush-send)
    ))

(def threads (array (struct thread-data) 64)) ; �� 64�Ǥ褤��
(def num-thrs unsigned-int)

;;; ��⡼�Ȥ� treq ���Ƥ��ä� task (copy)
;;; - treq �ޤ��� allocate
;;; x �Ȥ��ɤ����ꥹ�Ȥ��� DONE �Ȥʤä���Τ����
;;; (����ʤ顤rack ������ޤ� DONE �ˤ��ʤ��褦�ˤ���
;;;                 active �ʥ���åɿ����̴�������Ф褤?)
;;; x  �Ȥ��ɤ��Ǥʤ��ơ�rack �������Ȥ��Ǥ褤?
;;; - rslt �����ä��鼫ʬ�Ǿä��롥

;;; ʬ�䤷�ƺ�ä� task (home/orig) => task-home
;;; - thread-data �� sub ����Υꥹ�ȤȤʤꡤid �Ͻ�ʣ���ʤ��褦���դ��롥
;;; x �ǽ餫�顤STARTED�������� treq �������롥
;;; o treq �λ����� ALLOCATED �ˤ��Ƥ�?
;;; - rslt �������顤DONE �ˤ��ơ�rack ���֤�
;;; - DONE �ˤʤäƤ����顤ʬ�丵task ���ޡ������ƾõ�

;;; ?? task-home �򤽤Τޤ�Ʊ���Ρ��ɤǽ������륱�����⤢��Τ�??

;;; treq <task-head> <treq-head>
;;; <task-head>  ������������
;;; <treq-head>  �׵�������

;;; task <ndiv> <rslt-head> <task-head>
;;; <ndiv>       ʬ���� ��٤Υ��������ܰ� (sp2���Ҥ�Ƚ�Ǥ˻Ȥ�)
;;; <rslt-head>  ���������
;;; <task-head>  ������������

;;; rslt <rslt-head>
;;; <rslt-head>  ���������

;;; rack <task-head>
;;; <task-head>  rack������
;;; (w-rack �����󥿤�Ȥ��٤�)

;;; none <task-head>
;;; <task-head>  (no)������������

;;; task_top, task_free �Ϥ��η�?
;;; ����ͤ� task_top = 0, task_free �Ϻ�ü(prev�Ǽ���free�֥�å�)
;;; task_top �򺸤��鱦�˰�ư������С�������Υ�������next�������롥
;;; allocate�� task_top = task_free, task_free = task_free->prev
;;; free ��    task_free = task_top, task_top = task_top->next

;;; [ prev  ] -> [ prev  ] -> [ prev  ]  -> 
;;; <- [ next  ] <- [ next  ] <- [ next  ] <- 

;;; recv-exec-send ����� wait-rslt ����
;;; treq�ʰ�ötreq������դ�����̤������� �����ޤäƤ�����
;;; �������˴����������� none�����롥
;;; thr->mut �� send_mut �� lock ��
;;; �� ξ��lock �Ϥ���ʤΤǤ�?
(def (csym::flush-treq-with-none thr) (csym::fn void (ptr (struct thread-data)))
  (def rcmd (struct cmd))
  (def hx (ptr (struct task-home)))
  (= (fref rcmd c) 2)
  (= (aref (fref rcmd v) 0) "none")
  (while (= hx (fref thr -> treq-top))
    (= (fref rcmd node) (fref hx -> req-from))
    (= (aref (fref rcmd v) 1) (fref hx -> task-head))
    (csym::send-command (ptr rcmd) 0 0)
    (= (fref thr -> treq-top) (fref hx -> next))  ; treq�����å���pop
    (= (fref hx -> next) (fref thr -> treq-free)) ; �ե꡼�ꥹ�Ȥ�...
    (= (fref thr -> treq-free) hx))               ; ...�ΰ���ֵ�
  )

;;; worker or wait-rslt ����
;;; �ʸ�Ԥϳ����ꤲ���������η���Ԥ�����̤λŻ������Ȥ������
;;; �������׵� -> ������� -> �׻� -> �������
;;; thr->mut �ϥ�å���
;;; (treq-head,req-to): �ɤ��˥������׵��Ф�����any or ����֤���
(def (recv-exec-send thr treq-head req-to)
    (fn void (ptr (struct thread-data)) (ptr char) int)
  (def rcmd (struct cmd))
  (def tx (ptr (struct task)))
  (def delay long)
  (def old-ndiv int)
  ;; �ǽ��tx��Ȥ�
  ;; ��������Ȥ��Ȥ��Ƥ���֤ˡ�rslt ���Ϥ����鹩�פ���Τ�?
  ;; ñ�ˡ�rslt �Ԥ��ˤĤ��Ƥϡ�wait ���ʤ��ǡ����Ū�ˤߤ롥
  ;; �ߤ��Ȥ��ˡ��ʤ���С�task �Ԥ��ˤ��Ƥ��ޤ��Ȥ����Τ���ñ?
  ;; ���ä� task �Ԥ��ˤ����� treq ������Τ�����
  ;; �����Ǥϡ�task �Ԥ��Τ�����
  ;; task �ˤĤ��Ƥϡ�treq �����Ǿ����äƤ��� => �Ρ���������ˤ�
  ;; Ʊ������åɿ���Ķ���ʤ��褦�ˤϡ����ӥ��ޥե���
  ;; �� ���ٱ䱣�äΤ���;�פ˥���åɤ�����Υ���åɿ�

  ;; �������ä�treq�ʼ���ᤷ�ˤؤ� none ���Ϥ��ޤ��Ԥ�
  ;; �ʤ��줫������treq���Ф����ΤȺ�Ʊ���ʤ������
  (while (> (fref thr -> w-none) 0)
    (csym::pthread-cond-wait (ptr (fref thr -> cond))
                             (ptr (fref thr -> mut)))
    ;; rslt����������å�
    (if (and (fref thr -> sub)
             (== (fref thr -> sub -> stat) TASK-HOME-DONE))
        (return)))

  ;; allocate
  (= tx (fref thr -> task-free))
  (= (fref tx -> stat) TASK-ALLOCATED)
  (if (not tx)
      (csym::mem-error "Not enough task memory"))
  (= (fref thr -> task-top) tx)
  (= (fref thr -> task-free) (fref tx -> prev))

  (= delay (* 2 1000 1000))

  ;; treq���ޥ��
  (= (fref rcmd c) 3)
  (if (> num-thrs 1)
      (= (fref rcmd node) req-to)
    (= (fref rcmd node) OUTSIDE))
  (= (aref (fref rcmd v) 0) "treq")
  (= (aref (fref rcmd v) 1) (fref thr -> id-str))
  (= (aref (fref rcmd v) 2) treq-head)
  (do-while (!= (fref tx -> stat) TASK-INITIALIZED)
    ;; �ǽ��treq�����ޤäƤ����顤none������
    ;; �������� treq �����Ӹ򤦤��⤷��ʤ�����
    ;; none ������ʤ��Ȥ�������ȡ�
    ;; �ߤ��� none or task �������Ƥ���Τ��Ԥ�
    ;; �Ȥ��������פΥǥåɥ�å��Ȥʤ롥
    (csym::flush-treq-with-none thr)
    (= (fref tx -> stat) TASK-ALLOCATED) ; *-NONE�ˤ���Ƥ뤫�⤷��ʤ��Τ򸵤��᤹
    ;; �� �����ǡ�send_mut ��Ȥ����ˡ�thr->mut �� unlock ���٤��Ǥ�?
    (begin
     (csym::pthread-mutex-unlock (ptr (fref thr -> mut)))
     (csym::send-command (ptr rcmd) 0 0) ; treq����
     (csym::pthread-mutex-lock (ptr (fref thr -> mut))))
    ;; recv-task �� *tx ����������Τ��Ԥ�
    (loop
      ;; rslt �����夷�Ƥ����顤���̤���ˤ�����
      (if (and (!= (fref tx -> stat) TASK-INITIALIZED)
               (fref thr -> sub)
               (== (fref thr -> sub -> stat) TASK-HOME-DONE))
          ;; rslt �����夹��褦�ʾ�硤�����Ǥ�treq�ϼ��ᤷ�Ǥ��ꡤ
          ;; ���μ��ᤷ�ϼ��Ԥ��ơ�none���֤����Ϥ��Ǥ��롥
          ;; ���Ǥ� none ���֤���Ƥ��뤫�ǡ��Ԥ����Υ����󥿤�
          ;; ���䤹���ɤ������ۤʤ�
          ;; �� ���Τۤ��ˤ��롤rack �Υ����ߥ󥰤ˤ�äƤ�
          ;; ��������������ǽ�����ꡥ�������줿���ϡ�
          ;; w_none �ϻȤ��ʤ��Τǡ�none �� task ���֤����Τ򤳤ξ��
          ;; �Ԥ���task ���ä����ϡ��ʲ��� do_task_body �Τۤ��˿ʤ�ΤǤ�?
          (begin
           (if (!= (fref tx -> stat) TASK-NONE)
               (inc (fref thr -> w-none)))
           (goto Lnone)))
      (if (!= (fref tx -> stat) TASK-ALLOCATED)
          (break))
      (csym::pthread-cond-wait (ptr (fref thr -> cond))
                               (ptr (fref thr -> mut)))
      )
    ;; none �������ä��Ȥ��ˡ����Ф餯�ԤĤ褦�ˤ��ƤߤƤ��롥
    ;; ����¾�μ�ϻȤ��ʤ���? (Ʊ��ʣ���Ľ��Ԥ������β�����)
    ;; �� �����ʤ��Ȥ�ꥯ�������褬 any �Ǥʤ��ʤ顤
    ;;    rslt �ȥ��åȤ�none ���֤����ʳ��ϡ�
    ;;    none���֤�ɬ�פϤʤ��Ԥ����Ƥ����Ф褤��
    ;; �� any �ξ�硤treq combining �Ȥ���ʣ���Ľ��Ԥ��Ȥ�
    ;;    �����Ǥϡ�delay ���ܡ��Ȥ��Ƥ��äƤ��롥2ms ����Ϥ����
    (if (== (fref tx -> stat) TASK-NONE)
        (let ((t1 (struct timespec))
              (now (struct timeval))
              (nsec long))
          (csym::gettimeofday (ptr now) 0)
          (= nsec (* (fref now tv-usec) 1000))
          ;; nsec += 10 * 1000 * 1000
          (+= nsec delay)
          (+= delay delay)
          (if (> delay (* 40 1000 1000))
              (= delay (* 40 1000 1000)))
          (= (fref t1 tv-nsec)
             (if-exp (> nsec 999999999)
                     (- nsec 999999999)
                     nsec))
          (= (fref t1 tv-sec)
             (+ (fref now tv-sec)
                (if-exp (> nsec 999999999) 1 0)))
          (csym::pthread-cond-timedwait
           (ptr (fref thr -> cond-r))
           (ptr (fref thr -> mut))
           (ptr t1))
          ;; rslt�����夷�Ƥ�����
          (if (and (fref thr -> sub)
                   (== (fref thr -> sub -> stat)
                       TASK-HOME-DONE))
              (goto Lnone))))
    )
  
  ;; none, task, rslt ��Ʊ�����ԤƤʤ���?
  ;; rack ��Ȥ��¤�Ǥϡ�treq ��� rslt ���Ϥ������Ϥ��θ�
  ;; ���ʤ餺 none ����äƤ��롥
  ;; none ���Ԥ����ˡ�w-none �� inc ���ơ�none ���Ϥ����Ȥ��ˤ�
  ;; "struct task �ν񤭹���/signal"������ dec ��������Ф褵������
  ;; rack ��Ȥ�ʤ��ʤ顤rslt �θ塤task ���Ϥ����⡥
  ;; ���߽������ task �� task-top �Ǥʤ��Ƥ�褤�ʤ�����ʤ���
  ;; ���Τ���ˤϡ�task-top �Ǥʤ��Ȥ���ʬ����褦�ˤ���ȤȤ��
  ;; treq ���� thread�ֹ�ˡ�task�ֹ��ä���ɬ�פ�����Ȼפ���
  ;; �� ���ޤϡ�rack, w-none ��ȤäƤ���Ϥ���

  ;; �����ǡ�stat��TASK-INITIALIZED
  ;; �������饿��������
  (= (fref tx -> stat) TASK-STARTED)
  (= old-ndiv (fref thr -> ndiv))
  (= (fref thr -> ndiv) (fref tx -> ndiv))
  (csym::pthread-mutex-unlock (ptr (fref thr -> mut)))
  ((aref task-doers (fref tx -> task-no)) thr (fref tx -> body))
  (csym::pthread-mutex-lock (ptr (fref thr -> mut)))

  ;; task�ν�����λ��ϡ�����task-home��send-rslt����
  (= (fref rcmd c) 2)
  (= (fref rcmd node) (fref tx -> rslt-to))
  (= (aref (fref rcmd v) 0) "rslt")
  (= (aref (fref rcmd v) 1) (fref tx -> rslt-head))
  ;; �� ξ��lock�Ϥ���ʤΤǤϡ�
  (csym::send-command (ptr rcmd) (fref tx -> body) (fref tx -> task-no))

  ;; �����Ǥ�treq�����ޤäƤ����� none������
  ;; ��ξ��lock �Ϥ���ʤΤǤ�?
  (csym::flush-treq-with-none thr)
  (csym::pthread-mutex-lock (ptr (fref thr -> rack-mut)))
  ;; w_rack == 0 �ˤʤ�ޤǻߤޤ�ɬ�פϤʤ������������Ϥ��ʤ������Ǥ褤
  (inc (fref (mref thr) w-rack))
  (csym::pthread-mutex-unlock (ptr (fref thr -> rack-mut)))
  (= (fref thr -> ndiv) old-ndiv)
  ;; ������stack��pop���ƥե꡼�ꥹ�Ȥ��֤�
  (label Lnone
         (begin
          (= (fref thr -> task-free) tx)
          (= (fref thr -> task-top) (fref tx -> next))))
  )

(def (worker arg) (fn (ptr void) (ptr void))
  (def thr (ptr (struct thread-data)) arg)
  ;; �� ��unlock��ɸ��פ��ѹ����٤�
  (csym::pthread-mutex-lock (ptr (fref thr -> mut)))
  (loop
    (recv-exec-send thr "any" ANY))
  (csym::pthread-mutex-unlock (ptr (fref thr -> mut))))

;;;; recv-* �ϡ���������åɡ���������ӳ�����2�ġˤΤ߼¹�

;;; recv-task
;;; task <��������ʬ�䤵�줿���> <������> <������ʼ�ʬ��> <�������ֹ�>
(def (csym::recv-task pcmd body) (csym::fn void (ptr (struct cmd)) (ptr void))
  (def tx (ptr (struct task)))
  (def thr (ptr (struct thread-data)))
  (def id unsigned-int)
  (def task-no int)
  (def len size-t)
  ;; �ѥ�᡼���������å�
  (if (< (fref pcmd -> c) 5)
      (csym::proto-error "wrong-task" pcmd))
  ;; ��������Υ�å������ξ�硤���ޥ�ɤ�³��task���Τ�������
  ;; ����������ξ��ϰ�����Ϳ�����Ƥ����
  (= task-no (csym::atoi (aref (fref pcmd -> v) 4))) ; [4]: �������ֹ�
  (if (== (fref pcmd -> node) OUTSIDE)
      (begin
       (= body ((aref task-receivers task-no)))
       (csym::read-to-eol)))
  ;; <task-head>�򸫤ơ���������¹Ԥ��륹��åɤ���롥
  (= id (csym::atoi (aref (fref pcmd -> v) 3)))
  (if (not (< id num-thrs))
      (csym::proto-error "wrong task-head" pcmd))
  (= thr (+ threads id))                ; thr: task��¹Ԥ��륹��å�

  ;; ����åɤ˼¹Ԥ��٤����������ɲä���
  (csym::pthread-mutex-lock (ptr (fref thr -> mut)))
  (= tx (fref thr -> task-top))         ; tx: thr�����٤��Ż��ꥹ��
  (= (fref tx -> rslt-to) (fref pcmd -> node)) ; ��̤����������
  (= len (csym::strlen (aref (fref pcmd -> v) 2))) ; [2]: ���긵��=��̤��������
  (if (>= len ARG-SIZE-MAX)
      (csym::proto-error "Too long rslt-head for task" pcmd))
  (csym::strncpy (fref tx -> rslt-head)
                 (aref (fref pcmd -> v) 2)
                 (+ len 1))
  (= (fref tx -> ndiv) (csym::atoi (aref (fref pcmd -> v) 1))) ; [1]: ʬ����
  (= (fref tx -> task-no) task-no) ; �������ֹ�
  ;; �������Υѥ�᡼����task specific�ʹ�¤�Ρˤμ������
  ;; �� thr->mut �� unlock �ξ��֤� read ���٤��ǤϤʤ���?
  ;; �� do_task_body �ʳ��� thr������ʤ���?
  (= (fref tx -> body) body)
  ;; treq���Ƥ���task��������ޤǤδ֤�stat����������������ʤ���������
  ;; ����åɿ��ξ�¤�Ķ����ʤ顤���(�ɤ줫�Υ���åɤ������Ȥ�)
  ;; signal ����٤���?
  ;; ������ϡ��̤˥��ޥե��Ǿ�¤��������ۤ�����ñ
  (= (fref tx -> stat) TASK-INITIALIZED) ; ���⤽���å���ɬ�פʤΤϤ�����������
  (csym::pthread-mutex-unlock (ptr (fref thr -> mut))) 

  ;; �Ż��Ԥ���̲�äƤ������򵯤���
  (csym::pthread-cond-signal (ptr (fref thr -> cond)))
  )

(def (csym::recv-none pcmd) (csym::fn void (ptr (struct cmd)))
  (def thr (ptr (struct thread-data)))
  (def id unsigned-int)
  (def len size-t)
  (if (< (fref pcmd -> c) 2)
      (csym::proto-error "Wrong none" pcmd))
  (= id (csym::atoi (aref (fref pcmd -> v) 1)))
  (if (not (< id num-thrs))
      (csym::proto-error "Wrong task-head" pcmd))
  (= thr (+ threads id))
  (csym::pthread-mutex-lock (ptr (fref thr -> mut)))
  (if (> (fref thr -> w-none) 0)
      (dec (fref thr -> w-none))
    (= (fref thr -> task-top -> stat) TASK-NONE))
  (csym::pthread-mutex-unlock (ptr (fref thr -> mut)))
  (csym::pthread-cond-signal (ptr (fref thr -> cond)))
  )

;; rack �ˤĤ���
;; rslt �ˤ� <rack-head> �Ϥʤ� => <task-head> ��Ȥ�
;; treq ��ʤ顤rack ���֤��ʤ��Ȥ��Ƥ��뤬? (sp00b)
;; rack ���֤����Ȥ��ˤϡ�����˷�̤�����ޤ��褦�Ȼ�ߤ�
;; 
;; treq ��� rslt ����ä��顤������ rack ���֤�����
;; ���� treq ��ɬ�� none �����Ϥ������顤none ���б�����rack ���֤���
;; �ʤ�?  sp.memo �˽񤤤� SP: �ʹ� �Τ褦�ˡ�FIFO�����ʤ��ȡ�treq ��
;;        rack ���ɤ��ۤ��Ƥ��ޤ����Ȥ����뤫�顥
;; => �Ȥꤢ�����ϡ�FIFO�����ꤷ�Ƥ��Τޤ�
;; �� ��treq �� rack ��ȴ�����ȡ�treq������֤��ǤϤʤ���������
;;    ��ˤ��뤫�⤷��ʤ���treq combining �ʤɤ���̯�˱ƶ���?
;;    ���⤽�⡤none �ʳ���������Ȥ����ꤵ��Ƥ��ʤ��ΤǤ�?
;; �� rack �� treq ��ȴ��������?
;;    rack ���夯�ޤǤ� treq ���Ԥ����뤫 none ���֤��Τ�
;;    -> ����֤��Ǥʤ����������֤���뤳�ȤϤʤ���
(def (csym::recv-rslt pcmd body) (csym::fn void (ptr (struct cmd)) (ptr void))
  (def rcmd (struct cmd))               ; rack���ޥ��
  (def thr (ptr (struct thread-data)))
  (def hx (ptr (struct task-home)))
  (def tid unsigned-int)
  (def sid unsigned-int)
  (def b (ptr char))
  (def h-buf (array char ARG-SIZE-MAX))
  ;; �����ο������å�
  (if (< (fref pcmd -> c) 2)
      (csym::proto-error "Wrong rslt" pcmd))
  ;; ��̼���ͷ��� "<thread-id>:<task-home-id>"
  (= b (aref (fref pcmd -> v) 1))
  (= tid (csym::atoi b))
  (if (not (< tid num-thrs))
      (csym::proto-error "wrong rslt-head" pcmd))
  (= b (csym::strchr b #\:))
  (if (not b)
      (csym::proto-error "Wrong rslt-head" pcmd))
  (= sid (csym::atoi (+ b 1)))
  (= thr (+ threads tid))

  (csym::pthread-mutex-lock (ptr (fref thr -> mut)))
  ;; hx = �֤äƤ���rslt���ԤäƤ���task-home��.id==sid�ˤ�õ��
  (= hx (fref thr -> sub))
  (while (and hx (!= (fref hx -> id) sid))
    (= hx (fref hx -> next)))
  (if (not hx)
      (csym::proto-error "Wrong rslt-head" pcmd))
  ;; ��������Υ�å������ξ�硤���ޥ�ɤ�³��rslt���Τ�������
  ;; ����������ξ��ϰ�����Ϳ�����Ƥ����
  (cond
   ((== (fref pcmd -> node) OUTSIDE)
    ;; �� thr->mut ����ä��ޤ�read���٤��ǤϤʤ�
    ((aref rslt-receivers (fref hx -> task-no)) (fref hx -> body))
    (csym::read-to-eol))
   ((== (fref pcmd -> node) INSIDE)
    (= (fref hx -> body) body))
   (else
    (csym::proto-error "Wrong cmd.node" pcmd)))
  ;; rack���֤�����äȸ�Τۤ����褤��
  (= (fref rcmd c) 2)
  (= (fref rcmd node) (fref pcmd -> node))
  (= (aref (fref rcmd v) 0) "rack")
  (csym::strncpy                        ; �����补rslt�ǤϤʤ�����Ȥ�task���ޥ�ɤΤ�Ф��Ƥ���
   h-buf (fref hx -> task-head) ARG-SIZE-MAX)
  (= (aref (fref rcmd v) 1) h-buf)
  ;; hx ��˵�Ͽ���줿 task-head �� rack ��������ʤ顤
  ;; �����ǤϤʤ������ޤ� free ���줿���ʤ��Τǡ��Ĥʤ��ʤ�������
  (= (fref hx -> stat) TASK-HOME-DONE)
  (if (== hx (fref thr -> sub))
      (begin
       (csym::pthread-cond-signal (ptr (fref thr -> cond-r)))
       (csym::pthread-cond-signal (ptr (fref thr -> cond))))
    )
  (csym::pthread-mutex-unlock (ptr (fref thr -> mut)))

  (csym::send-command (ptr rcmd) 0 0))

;; worker �����Ф餯������äƤ���ʤ��Ƥ⡤
;; treq �ϼ�����äƤ����ۤ����褵������
;; ʣ���� treq ��ꥹ�Ȥˤ��Ƽ�����餻�Ƥ�褤�ΤǤ�?
;; treq �� task-home �η��ˤ��Ƥ����Ƥ�褤���⡥
;; task-home �� �����å��������ȤΤĤ����ä���...
;; �� �����η��ˤ��ǤˤʤäƤ���
;; �� treq �ʳ��ˤ�����none ���֤���(���none���֤���)��available �ˤʤä���
;;   ������֤��褦�� task query �Ȥ����� availability requenst (areq) �Ϥɤ���?
;;   �ֻ��� able �Ȥ���aval �Ȥ���
;; �� ������Ŭ������������Ǥ� dvbl �ˤʤ�Ϥ�������dvbl ����Ŭ�ʤΤ�������

;;; threads[id] ��treq���ߤ�
(def (csym::try-treq pcmd id) (csym::fn int (ptr (struct cmd)) unsigned-int)
  (def hx (ptr (struct task-home)))
  (def thr (ptr (struct thread-data)))
  (def len size-t)
  (def avail int 0)

  (= thr (+ threads id))
  (csym::pthread-mutex-lock (ptr (fref thr -> mut)))
  (csym::pthread-mutex-lock (ptr (fref thr -> rack-mut)))
  ;; worker���Ż��򤷤Ƥ��ơ�rack�Ԥ��Ǥ�ʤ����treq��ǽ�Ȥߤʤ�
  ;; rack�ξ��Ϥʤ�ɬ�ס�
  ;; ->rslt�����ä�ľ��˼���֤���treq���褿�Ȥ��ˡ��Ż���
  ;;   ʬ�䤷�Ƥ��ޤ�ʤ�����
  (if (and (fref thr -> task-top)
           (or (== (fref thr -> task-top -> stat) TASK-STARTED)
               (== (fref thr -> task-top -> stat) TASK-INITIALIZED))
           (== (fref thr -> w-rack) 0))
      (= avail 1))
  (csym::pthread-mutex-unlock (ptr (fref thr -> rack-mut)))

  ;; �����ʤ�task-home �����å���push
  (if avail
      (begin
       (= hx (fref thr -> treq-free))
       (if (not hx)
           (csym::mem-error "Not enough task-home memory"))
       (= (fref thr -> treq-free) (fref hx -> next)) ; �ե꡼�ꥹ�Ȥ����ΰ�����
       (= (fref hx -> next) (fref thr -> treq-top))  ; ������next�ϥ����å��Υ��
       (= (fref hx -> stat) TASK-HOME-ALLOCATED)
       (= len (csym::strlen (aref (fref pcmd -> v) 1))) ; v[1]: ������
       (if (>= len ARG-SIZE-MAX)
           (csym::proto-error "Too long task-head for treq" pcmd))
       (csym::strncpy (fref hx -> task-head)
                      (aref (fref pcmd -> v) 1)
                      (+ len 1))
       (if (!= (fref pcmd -> node) OUTSIDE)
           (= (fref hx -> req-from) INSIDE)
         (= (fref hx -> req-from) OUTSIDE))
       (= (fref thr -> treq-top) hx)
       (= (fref thr -> req) hx) ; ��req �� volatile, broadcast ����
       ))
  (csym::pthread-mutex-unlock (ptr (fref thr -> mut)))
  (return avail))

;; treq �򤿤�Ƥ���������
;;   ��ʬ�� treq ��ΤȤ��ϡ�
;;     none ���֤����ʤ��ȥǥåɥ�å��δ���
;;     ������Ĥ����ƥ롼�פ��ʤ���С��ޤ����Ƥ�褤��
;;   ��ʬ��ư���Ƥ��뤬�Ϥ���Τ��ʤ��Ȥ�
;;     none ���֤��Ȳ��٤⤤�äƤ��뤫�⡥
;;     none ���֤��ʤ��ȡ�¾�ˤ����Ф褤�Τ��ԤĤ��Ȥˤʤ뤫�⡥
;;     ���� none �ǵ��ݤ��줿��Ф��Ƥ�����treq �����դ��ޤ���
;;     ��������������ޤǤϤ��ΥΡ��ɤˤ� treq ���ʤ��Ȥ���...
;; �� ���� (able��) �Ͼ���ɲä��� areq �Ȥ���...
;; �� any ���Ф��� none���ޤ��� ����ᤷ(take bask)���Ф���none ��?
;;   ����ᤷ(take bask)���Ф���none�Ϥޤ�����Ф褤��
;;   treq �ʳ��� tbak ���ߤ���Ȥ褤���⤷��ʤ�����å������Ϥ��Τޤ�
;;   �Ȥ��Ƥ⡤trep_top �Ȥ��̴����Τ�ΤȤ���

(def (csym::recv-treq pcmd) (csym::fn void (ptr (struct cmd)))
  ;; task id �����ꤵ��Ƥ�����ʤȤ��֤��ˤȡ�any �ξ�礬���롥
  ;; any �ξ��ϡ�any �ǤȤäƤ�����ΤΤۤ����礭���ΤǤ�?
  ;; => ���⤽�⡤���٤Ƥ� task ��ư���Ƥ���櫓�ǤϤʤ���
  ;;    �����ʤ��Ȥ⡤rslt �ޤ��ǡ��� task ��Ω���夲�� task ��
  ;;    req ���Ƥ���ᡥ
  ;; �� �ʲ�����Ԥ狼��ˤ���
  ;; * thread ñ�̤� req ����Ĥʤ餽��Ϥ���� OK
  ;; * task ñ�̤� req ����Ĥʤ顤
  ;;   + task �� req ��򸫤Ĥ����뤫�Ȥ�������
  ;;   + ��ư���Ƥ����Τ� req ���٤�
  ;;     regulation (���ޥե�)�ǡ������� task �� req ���Ƥ��館�ʤ�����
  ;;      -> any ���ä����оݤ���Ϥ�����
  ;;         �ޤ��ϡ� �����task �ʤ顤���Ū�� regu ��Ϥ���?
  ;;         regu ���ʤ��ʤ餷�ʤ��ǡ�����ϡ��ʤޤʤ���Τ��Ф롥
  ;;         ���١�regu��Ϥ����Ƥ⡤task-send��� regu ���ǧ
  ;;      -> ����ä�;ʬ�� thread ��ȤäƤ⡤������ʬ�䤬����ʬ
  ;;         �ʤߤ������ʤ�ʤ顤��ɡ��ٱ䱣�äˤʤ�ʤ���ǽ�����⤤��
  ;;         �ٱ䱣�äΤ���ˤϡ�thread ��̲�äƤ��Ƥ� task ��ʬ���ǽ
  ;;         ���Ȥ褤����������ʬ�䤷�Ƥ����Ȥ�...
  ;;         -> ����åɿ�1�����ʬ�䤹��ΤϤ��ޤ��̣�Ϥʤ����⡥
  ;;              => �⤰��ʤ����̤Ϥ���?
  ;;            ��������ʬ�䤹�뤰�餤�ʤ顤����åɿ����ܤǤ褤?
  ;;  �����Τ�����ϡ�����ٱ䱣�äΤ���˥���åɤ����䤹���θ�Ƥ
  ;;     * ����åɿ����礦�ɤξ������ʬ��������
  ;;     * ���ȥ꡼����뤳�Ȥ��Ǥ���褦�˥ǡ�������¸(���ԡ�)���Ƥ���
  ;;       ���ƥ����뤵��ʤ���м�ʬ�ǽ���
  ;;        -> log N �󤯤餤
  ;;     * ����ʬ��ǡ�����ư�����Ϥ�Ƥ��ޤ��ȡ�(��ư�Ǥ��ʤ��Τ�
  ;;       ��ư�Ǥ��륿�������뤿���) ��ʬ��ˡּ¹ԡפ�ɬ�פȤʤ롥
  ;;       -> ����åɤǤ��ٱ䱣�äȤ����Τϼ���(�褯�ʤ������ǥ���)
  (def rcmd (struct cmd))
  (def id unsigned-int)
  (if (< (fref pcmd -> c) 3)            ; �����ο������å� 0:"treq", 1:from, 2:to
      (csym::proto-error "Wrong treq" pcmd))
  ;; �Ż����׵᤹�륹��åɤ���ơ��׵��Ф�
  (if (== (csym::strcmp (aref (fref pcmd -> v) 2) "any") 0)
      (let ((myid int))
        (= myid (csym::atoi (aref (fref pcmd -> v) 1)))
        (for ((= id 0) (< id num-thrs) (inc id))
          (if (and (!= (fref pcmd -> node) OUTSIDE)
                   (== id myid))
              (continue))               ; ��ʬ���Ȥˤ��׵��Ф��ʤ�
          (if (csym::try-treq pcmd id) (break)))
        (if (!= id num-thrs)            ; treq�Ǥ���
            (return)))
    (begin                              ; "any"�Ǥʤ����ʼ���֤���
     (= id (csym::atoi (aref (fref pcmd -> v) 2)))
     (if (not (< id num-thrs))
         (csym::proto-error "Wrong task-head" pcmd))
     ;; ��rslt ���֤�����ʳ��ϡ�����ᤷ���Ф��� none �����餺
     ;; �ޤ����Ƥ����Ф褤�Τǡ��׽���
     ;; ��try-treq����rack�Ԥ��Ǥʤ����1���֤��褦�ˤʤäƤ�Τ������
     (if (csym::try-treq pcmd id)        ; treq�Ǥ���
         (return))))
  
  ;; ���������treq any�˻Ż����֤��ʤ��ä����
  (if (== (fref pcmd -> node) ANY)
      (if (== (csym::atoi (aref (fref pcmd -> v) 1)) 0) ; v[1]:from
          ;; 0��worker�����treq�ξ��ϳ������䤤��碌��
          (begin
           (= (fref pcmd -> node) OUTSIDE)
           (csym::send-command pcmd 0 0)
           (= divisibility-flag 1)      ; �� �ä��Ƥ��ޤ�����
           (return))
        ;; ����ʳ���worker�ˤ�ñ��none���֤�
        (= (fref pcmd -> node) INSIDE)))

  ;; none���֤�
  ;; �� receiver����åɤ���ʬ�� none �����äƤ���ΤϤ褯�ʤ���
  ;;    Ŭ���ʥ���åɤ�Ǥ����٤��Ǥ�?
  (= (fref rcmd c) 2)
  (= (fref rcmd node) (fref pcmd -> node))
  (= (aref (fref rcmd v) 0) "none")
  (= (aref (fref rcmd v) 1) (aref (fref pcmd -> v) 1))
  (csym::send-command (ptr rcmd) 0 0))

;; rack <rack������header(�����Ǥ�thread-id)>
(def (csym::recv-rack pcmd) (csym::fn void (ptr (struct cmd)))
  (def tx (ptr (struct task)))
  (def thr (ptr (struct thread-data)))
  (def id unsigned-int)
  (def len size-t)
  (if (< (fref pcmd -> c) 2)
      (csym::proto-error "Wrong rack" pcmd))
  ;; id��<task-head>�˴ޤ��
  (= id (csym::atoi (aref (fref pcmd -> v) 1)))
  (if (not (< id num-thrs))
      (csym::proto-error "Wrong task-head" pcmd))
  (= thr (+ threads id))
  (csym::pthread-mutex-lock (ptr (fref thr -> rack-mut)))
  (dec (fref thr -> w-rack))
  (csym::pthread-mutex-unlock (ptr (fref thr -> rack-mut))))

(def (csym::send-divisible) (csym::fn void void)
  (def cmd (struct cmd))
  ;;   (if (csym::pthread-mutex-trylock (ptr send-mut))
  ;;       (return))
  (= divisibility-flag 0) ; �� �ä��Ƥ��ޤ�����
  (= (fref cmd c) 1)
  (= (fref cmd node) OUTSIDE)
  (= (aref (fref cmd v) 0) "dvbl")
  (csym::send-command (ptr cmd) 0 0))

;; �������åɤ��Ż�ʬ�䳫�ϻ��˸Ƥ�
(def (handle-req -bk -thr)
    (fn void (ptr (NESTFN int void)) (ptr (struct thread-data)))
  (csym::pthread-mutex-lock (ptr (fref -thr -> mut)))
  (if (fref -thr -> req)
      (begin
       (-bk)
       (= (fref -thr -> req) (fref -thr -> treq-top))))
  (csym::pthread-mutex-unlock (ptr (fref -thr -> mut))))

;; �������åɤ�put���˸Ƥ�
;; -thr->mut ��å��Ѥ�
(def (csym::make-and-send-task thr task-no body) ; task-no��tcell�ɲ�
    (csym::fn void (ptr (struct thread-data)) int (ptr void))
  (def tcmd (struct cmd))
  (def hx (ptr (struct task-home)) (fref thr -> treq-top))
  (= (fref thr -> treq-top) (fref hx -> next)) ; task�׵᥹���å���pop
  (= (fref hx -> next) (fref thr -> sub)) ; �����ꥵ�֥�����stack�Υ��
  (= (fref thr -> sub) hx)              ; ���֥�����stack��push
  (= (fref hx -> task-no) task-no)
  (= (fref hx -> body) body)
  (= (fref hx -> id) (if-exp (fref hx -> next) ; ���֥�����ID = �줫�鲿���ܤ�
                             (+ (fref hx -> next -> id) 1)
                             0))
  (= (fref hx -> stat) TASK-HOME-INITIALIZED)
  (= (fref tcmd c) 5)
  (= (fref tcmd node) (fref hx -> req-from))
  (= (aref (fref tcmd v) 0) "task")
  ;; ����ʤΤ����
  ;; �� �����ڡ����Ƕ��ڤäƤ���Τǡ�������Ĥ����ʤ�����Ƥ�������
  ;;    sprintf �� �Хåե����դ�̤��ǧ������? 
  (csym::sprintf (fref thr -> ndiv-buf) "%d"
                 (++ (fref thr -> ndiv)))
  (csym::sprintf (fref thr -> buf) "%s:%d"
                 (fref thr -> id-str) (fref hx -> id))
  (csym::sprintf (fref thr -> tno-buf) "%d" task-no)
  (= (aref (fref tcmd v) 1) (fref thr -> ndiv-buf)) ; ʬ����
  (= (aref (fref tcmd v) 2) (fref thr -> buf)) ; ��������� "<worker-id>:<subtask-id>"
  (= (aref (fref tcmd v) 3) (fref hx -> task-head)) ; �������������try-treq�ǽ����ѡ�
  (= (aref (fref tcmd v) 4) (fref thr -> tno-buf)) ; �������ֹ�
  (csym::send-command (ptr tcmd) body task-no))

;; �������åɤ����֥������η���Ԥ����˸Ƥ�
(def (wait-rslt thr) (fn (ptr void) (ptr (struct thread-data)))
  (def body (ptr void))
  (def t1 (struct timespec))
  (def now (struct timeval))
  (def nsec long)
  (def sub (ptr (struct task-home)))
  (csym::pthread-mutex-lock (ptr (fref thr -> mut)))
  (= sub (fref thr -> sub))
  (while (!= (fref sub -> stat) TASK-HOME-DONE)
    ;; �� �ʲ���ԥ����ȤˤʤäƤ���
    ;; -> ���Ф餯rslt���ʤ���� recv-exec-send��flush����Τ����ס�
    ;; (csym::flush-treq-with-none thr)
    ;; (= (fref thr -> task-top -> stat) TASK-SUSPENDED)
    (csym::gettimeofday (ptr now) 0)
    (= nsec (* 1000 (fref now tv-usec)))
    (+= nsec (* 5 1000 1000))          ; ����ä��ԤäƤ���ʤ����ο������Ԥ����֡�
    ;; (+= nsec (* 10 1000 1000))
    (= (fref t1 tv-nsec) (if-exp (> nsec 999999999)
                                 (- nsec 999999999)
                                 nsec))
    (= (fref t1 tv-sec) (+ (fref now tv-sec)
                           (if-exp (> nsec 999999999) 1 0)))
    (csym::pthread-cond-timedwait
     (ptr (fref thr -> cond-r)) (ptr (fref thr -> mut))
     (ptr t1))
    ;; �� �ʲ���ԥ����ȤˤʤäƤ���
    (= (fref thr -> task-top -> stat) TASK-STARTED)
    (if (== (fref sub -> stat) TASK-HOME-DONE)
        (break))
    ;; fprintf(stderr, "sub %d\n", sub);
    ;; ���Ф餯�ޤäƤ⤢���󤫤ä���Ż���Ȥ��֤��˹Ԥ�
    (recv-exec-send
     thr (fref sub -> task-head) (fref sub -> req-from)))
  (= body (fref sub -> body))
  (= (fref thr -> sub) (fref sub -> next))       ; ���֥�����stask��pop
  (= (fref sub -> next) (fref thr -> treq-free)) ; pop������ʬ��...
  (= (fref thr -> treq-free) sub)                ; ...�ե꡼�ꥹ�Ȥ��֤�
  ;; fprintf(stderr, "nsub %d\n", thr->sub);
  (csym::pthread-mutex-unlock (ptr (fref thr -> mut)))
  (return body))


(%defconstant HOSTNAME-MAXSIZE 256)
(def (struct runtime-option)
  (def num-thrs int)                    ; ����åɿ�
  (def sv-hostname (array char HOSTNAME-MAXSIZE))
                                        ; Tascell�����ФΥۥ���̾��""�ʤ�stdout
  (def port unsigned-short)             ; Tascell�����Фؤ���³�ݡ����ֹ�
  )
(static option (struct runtime-option))

(def (csym::usage argc argv) (csym::fn void int (ptr (ptr char)))
  (csym::fprintf stderr
                 "Usage: %s [-s hostname] [-p port-num] [-n n-threads]~%"
                 (aref argv 0))
  (csym::exit 1))

(def (set-option argc argv) (csym::fn void int (ptr (ptr char)))
  (def i int) (def ch int)
  ;; Default values
  (= (fref option num-thrs) 1)
  (= (aref (fref option sv-hostname) 0) #\NULL)
  (= (fref option port) 8888)
  (while (!= -1 (= ch (csym::getopt argc argv "n:s:p:")))
    (for ((= i 0) (< i argc) (inc i))
      (switch ch
        (case #\n)                      ; number of threads
        (= (fref option num-thrs) (csym::atoi optarg))
        (break)
        
        (case #\s)                      ; server name
        (if (csym::strcmp "stdout" optarg)
            (begin
             (csym::strncpy (fref option sv-hostname) optarg
                            HOSTNAME-MAXSIZE)
             (= (aref (fref option sv-hostname) (- HOSTNAME-MAXSIZE 1)) 0))
          (= (aref (fref option sv-hostname) 0) #\NULL))
        (break)
        
        (case #\p)                      ; connection port number
        (= (fref option port) (csym::atoi optarg))
        (break)

        (case #\h)                      ; usage
        (csym::usage argc argv)
        (break)
        
        (default)
        (csym::fprintf stderr "Unknown option: %c~%" ch)
        (csym::usage argc argv)
        (break))))
  (return)
  )


#+tcell-gtk (def window (ptr GtkWidget))
#+tcell-gtk (def darea (ptr GtkWidget))
#+tcell-gtk (def gc (ptr GdkGC) 0)
#+tcell-gtk (def pixmap (ptr GdkPixmap) 0)

#+tcell-gtk
(def (csym::set-color r g b) (fn (ptr GdkGC) gushort gushort gushort)
  (decl color GdkColor) 
  (= (fref color red) r) 
  (= (fref color green) g)
  (= (fref color blue) b)
  (csym::gdk-color-alloc (csym::gdk-colormap-get-system) (ptr color))
  (csym::gdk-gc-set-foreground gc (ptr color))
  (return gc))

#+tcell-gtk                             ; �������������ѹ���
(def (csym::configure-event widget event data)
  (fn void (ptr GtkWidget) (ptr GdkEventConfigure) gpointer)
  ;; �Ť�pixmap������г���
  (if pixmap (csym::gdk-pixmap-unref pixmap))
  ;; ��������������pixmap�����
  (= pixmap
     (csym::gdk-pixmap-new (fref (mref widget) window)
                           (fref (fref (mref widget) allocation) width)
                           (fref (fref (mref widget) allocation) height) (- 1))))

#+tcell-gtk                             ; ������
(def (csym::expose-event widget event data)
  (fn void (ptr GtkWidget) (ptr GdkEventExpose) gpointer)
  (csym::gdk-draw-pixmap (fref widget -> window)
                         (aref (fref widget -> style -> fg-gc)
                               (csym::GTK-WIDGET-STATE widget))
                         pixmap
                         (fref (fref (mref event) area) x)
                         (fref (fref (mref event) area) y)
                         (fref (fref (mref event) area) x)
                         (fref (fref (mref event) area) y)
                         (fref (fref (mref event) area) width)
                         (fref (fref (mref event) area) height)))

#+tcell-gtk                             ; ������֤��Ȥ����补���ץꥱ�������¦�����
(extern-decl (csym::repaint) (fn gint gpointer))
;; gint repaint(gpointer data){
;;     static x;
;;     GtkWidget *drawing_area = GTK_WIDGET (data);
;;     gdk_draw_rectangle(pixmap,
;;                        set_color(0xffff, 0x0, 0x0),
;;                        TRUE,
;;                        0, 0,
;;                        drawing_area->allocation.width,
;;                        drawing_area->allocation.height);
;;     x++;
;;     gdk_draw_rectangle(pixmap,
;;                        set_color(0xffff, 0xffff, 0x0),
;;                        TRUE,
;;                        x, x,
;;                        30, 30);
;;     /* ���褹�� (expose_event��ƤӽФ�) */
;;     gtk_widget_draw(drawing_area, NULL);
;;     return TRUE;
;; }

;; main
;; �ǡ���������åɤ���������ư���Ƥ���
;; ������å������μ����롼�פ�����
(def (main argc argv) (fn int int (ptr (ptr char)))
  (defs int i j)
  (def dummy (ptr void))
  (def pcmd (ptr (struct cmd)))         ; ������������������ޥ��
  
  ;; ���ޥ�ɥ饤�󥪥ץ����
  #+tcell-gtk (csym::gtk-init (ptr argc) (ptr argv))
  (csym::set-option argc argv)

  #+tcell-gtk (begin (= window (csym::gtk-window-new GTK-WINDOW-TOPLEVEL))
                     (gtk-widget-show window)
                     (= darea (csym::gtk-drawing-area-new))
                     (csym::gtk-drawing-area-size (csym::GTK-DRAWING-AREA darea) 300 200)
                     (csym::gtk-container-add (csym::GTK-CONTAINER window) darea)
                     (= gc (csym::gdk-gc-new (fref window -> window)))
                     (csym::gtk-signal-connect (csym::GTK-OBJECT darea) "configure_event"
                                               (csym::GTK-SIGNAL-FUNC csym::configure-event) 0)
                     (csym::gtk-signal-connect (csym::GTK-OBJECT darea) "expose_event"
                                               (csym::GTK-SIGNAL-FUNC csym::expose-event) 0)
                     (csym::gtk-timeout-add 33 repaint (cast gpointer darea))
                     (csym::gtk-widget-show-all window)
                     (csym::systhr-create gtk-main 0)
                     )
  
  ;; �����Ф���³
  (= sv-socket (if-exp (== #\NULL (aref (fref option sv-hostname) 0))
                       -1
                       (csym::connect-to (fref option sv-hostname)
                                         (fref option port))))

  ;; inms-queue�������̿���å��������塼�� �ν����
  (= inms-queue (csym::make-cmd-queue CMD-QUEUE-LENGTH MAXCMDC ARG-SIZE-MAX))
  (systhr-create exec-queue-cmd NULL)   ; ������å�������������å�
  ;; send-queue�ʳ����������塼�ˤν����
  (= send-queue (csym::make-cmd-queue CMD-QUEUE-LENGTH MAXCMDC ARG-SIZE-MAX))
  (systhr-create sender-loop NULL)   ; ��������å�
  ;; thread-data �ν����, task �� ������list ��
  (= num-thrs (fref (fref option num-thrs)))
  (for ((= i 0) (< i num-thrs) (inc i))
    (let ((thr (ptr (struct thread-data)) (+ threads i))
          (tx (ptr (struct task)))
          (hx (ptr (struct task-home))))
      (= (fref thr -> req) 0)
      (= (fref thr -> id) i)
      (= (fref thr -> w-rack) 0)
      (= (fref thr -> w-none) 0)
      (= (fref thr -> ndiv) 0)
      (csym::pthread-mutex-init (ptr (fref thr -> mut)) 0)
      (csym::pthread-mutex-init (ptr (fref thr -> rack-mut)) 0)
      (csym::pthread-cond-init (ptr (fref thr -> cond)) 0)
      (csym::pthread-cond-init (ptr (fref thr -> cond-r)) 0)
      (csym::sprintf (fref thr -> id-str) "%d" i)

      ;; task���������ꥹ�ȡʥ���åɤ��¹Ԥ���٤���������
      (= tx (cast (ptr (struct task))
              (csym::malloc (* (sizeof (struct task)) TASK-LIST-LENGTH))))
      (= (fref thr -> task-top) 0)
      (= (fref thr -> task-free) tx)
      (for ((= j 0) (< j (- TASK-LIST-LENGTH 1)) (inc j))
        (= (fref (aref tx j) prev) (ptr (aref tx (+ j 1))))
        (= (fref (aref tx (+ j 1)) next) (ptr (aref tx j))))
      (= (fref (aref tx 0) next) 0)
      (= (fref (aref tx (- TASK-LIST-LENGTH 1)) prev) 0)

      ;; task-home�Υꥹ�ȡ�ʬ�䤷�ƤǤ�����������
      (= hx (cast (ptr (struct task-home))
              (csym::malloc (* (sizeof (struct task-home)) TASK-LIST-LENGTH))))
      (= (fref thr -> treq-top) 0)
      (= (fref thr -> treq-free) hx)
      (= (fref thr -> sub) 0)
      ;; �ե꡼�ꥹ�Ȥ���
      (for ((= j 0) (< j (- TASK-LIST-LENGTH 1)) (inc j))
        (= (fref (aref hx j) next) (ptr (aref hx (+ j 1)))))
      (= (fref (aref hx (- TASK-LIST-LENGTH 1)) next) 0)))
  ;; �������å�����
  ;; Ʊ��Ρ�����ʶ�ͭ����ˤǤ��꤯��Ǥ��뤫��
  ;; �� ����������٤ƽ���äƤ��� fork ���٤��Ǥ�?
  (for ((= i 0) (< i num-thrs) (inc i))
    (let ((thr (ptr (struct thread-data)) (+ threads i)))
      (systhr-create worker thr)))
    
  ;; �ܥ���åɤ�OUTSIDE����Υ�å���������
  (= pcmd (csym::recv-command))
  (while (and (> (fref pcmd -> c) 0)
              (!= 0 (csym::strcmp
                     (aref (fref pcmd -> v) 0) "exit")))
    (cond 
     ((== 0 (csym::strcmp (aref (fref pcmd -> v) 0) "task"))
      (csym::recv-task pcmd dummy))
     ((== 0 (csym::strcmp (aref (fref pcmd -> v) 0) "rslt"))
      (csym::recv-rslt pcmd dummy))
     ((== 0 (csym::strcmp (aref (fref pcmd -> v) 0) "treq"))
      (csym::recv-treq pcmd))
     ((== 0 (csym::strcmp (aref (fref pcmd -> v) 0) "none"))
      (csym::recv-none pcmd))
     ((== 0 (csym::strcmp (aref (fref pcmd -> v) 0) "rack"))
      (csym::recv-rack pcmd))
     (else
      (csym::proto-error "wrong cmd" pcmd)))
    (= pcmd (csym::recv-command))
    )
  (csym::exit 0))
