(c-exp "#include<stdio.h>")
(c-exp "#include<pthread.h>")
(%include "worker.sh")

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

(def (csym::systhr-create start-func arg)
     (fn int (ptr (fn (ptr void) (ptr void))) (ptr void))
  (def status int 0)
  (def tid pthread-t)
  (def attr pthread-attr-t)

  (csym::pthread-attr-init (ptr attr))
  (= status (csym::pthread-attr-setscope (ptr attr) PTHREAD-SCOPE-SYSTEM))
  (if (== status 0)
      (= status (csym::pthread-create (ptr tid) (ptr attr) start-func arg))
      (= status (csym::pthread-create (ptr tid) 0          start-func arg)))
  (return status))

(def (csym::mem-error str) (fn void (ptr (const char)))
  (csym::fputs str stderr)
  (csym::fputc #\Newline stderr))

(def cmd-queue (array (struct cmd-list) 512))
(def cmd-v-buf (array char 512 4 256))
(def cmd-in (ptr (struct cmd-list)))
(def cmd-out (ptr (struct cmd-list)))
(def buf (array char BUFSIZE))
(def divisibility-flag int 0)

;;; fgets��OUTSIDE����Υ�å������������� -> struct cmd
(def (recv-command) (fn (struct cmd))
  (def r (struct cmd))
  (defs char p c)
  (def b (ptr char) buf)
  
  (csym::fgets b BUFSIZE stdin)
  (= (fref r c) 0)
  (= (fref r node) OUTSIDE)
  (for ((exps (= p #\NULL)
	      (= c (mref b)))
	c
	(exps (= p c)
	      (= c (mref (++ b)))))
    (if (or (== c #\Space)
	    (== c #\Newline))
	(begin (= c #\NULL)
	  (= (mref b) #\NULL))
      (if (and (== p #\NULL)
	       (< (fref r c) MAXCMDC))
	  (= (aref (fref r v) (inc (fref r c))) b))))
  
  (return r) )

(def send-mut pthread-mutex-t PTHREAD-MUTEX-INITIALIZER)
(def queue-mut pthread-mutex-t PTHREAD-MUTEX-INITIALIZER)
(def cond-q pthread-cond-t PTHREAD-COND-INITIALIZER)

;;; struct cmd -> output��stdout�ء�
(def (csym::send-command cmd) (fn void (struct cmd))
  (def i int)
  (for ((= i 0) (< i (- (fref cmd c) 1)) (inc i))
    (csym::fputs (aref (fref cmd v) i) stdout)
    (csym::fputc #\Space stdout))
  (if (> (fref cmd c) 0)
      (begin
	(csym::fputs (aref (fref cmd v)
		       (- (fref cmd c) 1))
		     stdout)
	(csym::fputc #\Newline stdout))))

;;; ���顼��å�����str��stderr��
(def (csym::proto-error str cmd) (fn void (ptr (const char)) (struct cmd))
  (def i int)
  (csym::fputs str stderr)
  (csym::fputc #\Newline stderr)
  (for ((= i 0) (< i (- (fref cmd c) 1)) (inc i))
    (csym::fputs (aref (fref cmd v) i) stderr)
    (csym::fputc #\Newline stderr))
  (if (> (fref cmd c) 0)
      (begin
	(csym::fputs (aref (fref cmd v)
		       (- (fref cmd c) 1))
		     stderr)
	(csym::fputc #\Newline stderr))))

;; INSIDE transmission��queue�ؤΥ�å������ɲ�
(def (csym::enqueue-command cmd body)
  (fn void (struct cmd) (ptr void))
  (def i int)
  (def len size-t)
  (def q (ptr (struct cmd-list)))
  
  ;; cmd-queue ��lock�Ѥ�
  (= q cmd-in)
  (if (== (fref cmd-in -> next) cmd-out)
      (begin
	(csym::perror "cmd-queue overflow~%")
	(csym::exit 0)))
  (= cmd-in (fref cmd-in -> next))
  (= (fref q -> cmd c) (fref cmd c))
  (= (fref q -> cmd node) (fref cmd node))
  (for ((= i 0) (< i (fref cmd c)) (inc i))
    (= len (csym::strlen (aref (fref cmd v) i)))
    (if (> len 254)
	(csym::proto-error "too long cmd" cmd))
    (csym::strncpy (aref (fref q -> cmd v) i)
		   (aref (fref cmd v) i)
		   (+ len 1)))
  (= (fref q -> body) body))

;; INSIDE transmission��queue �ˤ��ޤä���å���������
(def (exec-queue-cmd arg) (fn (ptr void) (ptr void))
  (def cmd (struct cmd))
  (def body (ptr void))
  (loop
    (csym::pthread-mutex-lock (ptr queue-mut))
    (while (== cmd-in cmd-out)
      (csym::pthread-cond-wait
       (ptr cond-q) (ptr queue-mut)))
    (= cmd (fref cmd-out -> cmd))
    (= body (fref cmd-out -> body))
    (csym::pthread-mutex-unlock (ptr queue-mut))
    (if (== 0 (csym::strcmp (aref (fref cmd v) 0) "task"))
	(csym::recv-task cmd body)
      (if (== 0 (csym::strcmp (aref (fref cmd v) 0) "rslt"))
	  (csym::recv-rslt cmd body)
	(if (== 0 (csym::strcmp (aref (fref cmd v) 0) "treq"))
	    (csym::recv-treq cmd)
	  (if (== 0 (csym::strcmp (aref (fref cmd v) 0) "none"))
	      (recv-none cmd)
	    (if (== 0 (csym::strcmp (aref (fref cmd v) 0) "rack"))
		(csym::recv-rack cmd)
	      (begin
		(csym::proto-error "wrong cmd" cmd)
		(csym::exit 0)))))))
    (= cmd-out (fref cmd-out -> next))
    ))

;; EOL�ޤǤ�̵��
(def (csym::read-to-eol) (fn void void)
  (def c int)
  (while (!= EOF (= c (csym::getc stdin)))
    (if (== c #\Newline) (break))))

(def (csym::write-eol) (fn void void)
  (csym::putc #\Newline stdout))

(def (csym::flush-send) (fn void void)
  (csym::fflush stdout))


(def snr-mut pthread-mutex-t PTHREAD-MUTEX-INITIALIZER)
(def cond-snr pthread-cond-t PTHREAD-COND-INITIALIZER)
(def snr-queue (array (struct cmd-list) 32))
(def snr-v-buf (array char 32 2 256))
(def snr-in (ptr (struct cmd-list)))
(def snr-out (ptr (struct cmd-list)))

(def (csym::enqueue-snr cmd) (fn void (struct cmd))
  (def i int)
  (def len size-t)
  (def q (ptr (struct cmd-list)))

  (= q snr-in)
  (if (== (fref snr-in -> next) snr-out)
      (begin
	(csym::perror "snr-queue overflow~%")
	(csym::exit 0)))
  (= snr-in (fref snr-in -> next))
  (= (fref q -> cmd c) (fref cmd c))
  (= (fref q -> cmd node) (fref cmd node))
  (for ((= i 0) (< i (fref cmd c)) (inc i))
    (= len (csym::strlen (aref (fref cmd v) i)))
    (if (> len 254)
	(csym::proto-error "too long cmd" cmd))
    (csym::strncpy (aref (fref q -> cmd v) i)
		   (aref (fref cmd v) i)
		   (+ len 1))))

(def (send-none-rack arg) (fn (ptr void) (ptr void))
  (def cmd (struct cmd))
  (loop
    (csym::pthread-mutex-lock (ptr snr-mut))
    (while (== snr-in snr-out)  ; ���塼�����ʤ���Ͽ�������å��������Ԥ�
      (csym::pthread-cond-wait (ptr cond-snr) (ptr snr-mut)))
    (= cmd (fref snr-out -> cmd))
    (csym::pthread-mutex-unlock (ptr snr-mut))
    (csym::pthread-mutex-lock (ptr send-mut))
    (csym::send-command cmd)
    (csym::flush-send)
    (if (== 0 (csym::strcmp (aref (fref cmd v) 0) "none"))
	(= divisibility-flag 1))  ; none�����ä���dvbl�ե饰��on�ʡ���
    (csym::pthread-mutex-unlock (ptr send-mut))
    (= snr-out (fref snr-out -> next)))) ; ���ä���Τ򥭥塼���鳰��

(def threads (array (struct thread-data) 64))
(def num-thrs unsigned-int)

#|
  ��⡼�Ȥ� treq ���Ƥ��ä� task (copy)
    - treq �ޤ��� allocate
    x �Ȥ��ɤ����ꥹ�Ȥ��� DONE �Ȥʤä���Τ����
      (����ʤ顤rack ������ޤ� DONE �ˤ��ʤ��褦�ˤ���
       active �ʥ���åɿ����̴�������Ф褤?)
    x  �Ȥ��ɤ��Ǥʤ��ơ�rack �������Ȥ��Ǥ褤?
    - rslt �����ä��鼫ʬ�Ǿä��롥

  ʬ�䤷�ƺ�ä� task (home/orig) => task-home
    - thread-data �� sub ����Υꥹ�ȤȤʤꡤid �Ͻ�ʣ���ʤ��褦���դ��롥
    x �ǽ餫�顤STARTED�������� treq �������롥
    o treq �λ����� ALLOCATED �ˤ��Ƥ�?
    - rslt �������顤DONE �ˤ��ơ�rack ���֤�
    - DONE �ˤʤäƤ����顤ʬ�丵task ���ޡ������ƾõ�

  ?? task-home �򤽤Τޤ�Ʊ���Ρ��ɤǽ������륱�����⤢��Τ�??

    treq <task-head> <treq-head>
      <task-head>  ������������
      <treq-head>  �׵�������

    task <ndiv> <rslt-head> <task-head>
      <ndiv>       ʬ���� ��٤Υ��������ܰ� (sp2���Ҥ�Ƚ�Ǥ˻Ȥ�)
      <rslt-head>  ���������
      <task-head>  ������������

    rslt <rslt-head>
      <rslt-head>  ���������

    rack <task-head>
      <task-head>  rack������
      (w-rack �����󥿤�Ȥ��٤�)

    none <task-head>
      <task-head>  (no)������������

   [ prev  ] -> [ prev  ] -> [ prev  ]  -> 
<- [ next  ] <- [ next  ] <- [ next  ] <- 

|#  

(def (csym::flush-treq-with-none-1 thr) (fn void (ptr (struct thread-data)))
  (def rcmd (struct cmd))
  (def hx (ptr (struct task-home)))
  (while (= hx (fref thr -> treq-top))
    (= (fref rcmd c) 2)
    (= (fref rcmd node) (fref hx -> req-from))
    (= (aref (fref rcmd v) 0) "none")
    (= (aref (fref rcmd v) 1) (fref hx -> task-head))
    (if (== (fref rcmd node) INSIDE)
	(begin
	  (csym::pthread-mutex-lock (ptr queue-mut))
	  (csym::enqueue-command rcmd NULL)
	  (csym::pthread-cond-signal (ptr cond-q))
	  (csym::pthread-mutex-unlock (ptr queue-mut)))
      (if (== (fref rcmd node) OUTSIDE)
	  (begin
	    (csym::send-command rcmd)
	    (= divisibility-flag 1))
	(begin
	  (csym::perror "Invalid rcmd.node in flush-treq-with-none-1~%")
	  (csym::fprintf stderr "%d~%" (fref rcmd node))
	  (csym::exit 0))))
    (= (fref thr -> treq-top) (fref hx -> next))
    (= (fref hx -> next) (fref thr -> treq-free))
    (= (fref thr -> treq-free) hx)))

(def (csym::flush-treq-with-none thr) (fn void (ptr (struct thread-data)))
  ;; treq �����ޤäƤ����� none������
  ;; thr->mut �� lock��
  (if (fref thr -> treq-top)
      (begin
	(csym::pthread-mutex-lock (ptr send-mut))
	(csym::flush-treq-with-none-1 thr)
	(csym::flush-send)
	(csym::pthread-mutex-unlock (ptr send-mut)))))

;;; �������׵� -> ������� -> �������
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

  ;; ���ȤǤȤɤ� none ���Ϥ��ޤ��Ԥ�
  (while (> (fref thr -> w-none) 0)
    (csym::pthread-cond-wait (ptr (fref thr -> cond))
			     (ptr (fref thr -> mut)))
    ;; rslt�����夷�Ƥ�����
    (if (and (fref thr -> sub)
	     (== (fref thr -> sub -> stat) TASK-HOME-DONE))
	(return)))

  ;; allocate
  (= tx (fref thr -> task-free))
  (if (not tx)
      (csym::mem-error "Not enough task memory"))
  (= (fref thr -> task-top) tx)
  (= (fref thr -> task-free) (fref tx -> prev))

  (= delay (* 2 1000 1000))

  (do-while (!= (fref tx -> stat) TASK-INITIALIZED)
    ;; �ǽ��treq�����ޤäƤ����顤none������
    (csym::flush-treq-with-none thr)

    (= (fref tx -> stat) TASK-ALLOCATED)
    (csym::pthread-mutex-unlock (ptr (fref thr -> mut)))
    (= (fref rcmd c) 3)
    (if (> num-thrs 1)
	(= (fref rcmd node) req-to)
      (= (fref rcmd node) OUTSIDE))
    (= (aref (fref rcmd v) 0) "treq")
    (= (aref (fref rcmd v) 1) (fref thr -> id-str))
    (= (aref (fref rcmd v) 2) treq-head)

    (if (!= (fref rcmd node) OUTSIDE)
	(begin
	  (csym::pthread-mutex-lock (ptr queue-mut)) 
	  (csym::enqueue-command rcmd NULL)
	  (csym::pthread-cond-signal (ptr cond-q))
	  (csym::pthread-mutex-unlock (ptr queue-mut)))
      (begin
	(csym::pthread-mutex-lock (ptr send-mut))
	(csym::send-command rcmd)
	(csym::flush-send)
	(= divisibility-flag 1)
	(csym::pthread-mutex-unlock (ptr send-mut))))

    (csym::pthread-mutex-lock (ptr (fref thr -> mut)))
    ;; recv-task �ǽ���������Τ��Ԥ�
    (loop
      ;; rslt �����夷�Ƥ����顤���̤���ˤ�����
      (if (and (!= (fref tx -> stat) TASK-INITIALIZED)
	       (fref thr -> sub)
	       (== (fref thr -> sub -> stat) TASK-HOME-DONE))
	  (begin
	    (if (== (fref tx -> stat) TASK-NONE)
		(goto Lnone))
	    (inc (fref thr -> w-none))
	    (goto Lnone)))
      (if (!= (fref tx -> stat) TASK-ALLOCATED)
	  (break))
      (csym::pthread-cond-wait (ptr (fref thr -> cond))
			       (ptr (fref thr -> mut)))
      )

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

  ;; TASK-INITIALIZED
  (= (fref tx -> stat) TASK-STARTED)
  (= old-ndiv (fref thr -> ndiv))
  (= (fref thr -> ndiv) (fref tx -> ndiv))
  ;; �����˥��ޥե���
  (csym::pthread-mutex-unlock (ptr (fref thr -> mut)))
  (do-task-body thr (fref tx -> body))    ; �������¹�
  (csym::pthread-mutex-lock (ptr (fref thr -> mut)))

  ;; task�ν�����λ��ϡ�����task-home��send-rslt����
  (= (fref rcmd c) 2)
  (= (fref rcmd node) (fref tx -> rslt-to))
  (= (aref (fref rcmd v) 0) "rslt")
  (= (aref (fref rcmd v) 1) (fref tx -> rslt-head))

  (if (== (fref rcmd node) INSIDE)
      (begin
	(csym::pthread-mutex-lock (ptr queue-mut))
	(csym::enqueue-command rcmd (fref tx -> body))
	(csym::pthread-cond-signal (ptr cond-q))
	(csym::pthread-mutex-unlock (ptr queue-mut))
	(csym::pthread-mutex-lock (ptr send-mut)) ; flush-treq�Τ���
	)
    (if (== (fref rcmd node) OUTSIDE)
	(begin
	  (csym::pthread-mutex-lock (ptr send-mut))
	  (csym::pthread-mutex-unlock (ptr (fref thr -> mut)))
	  (csym::send-command rcmd)
	  ;; body �ǤϤʤ���do-task-body �� return value �ˤ���ΤϤɤ���?
	  (send-rslt-body thr (fref tx -> body))
	  ;; �ޤ��ϡ�x->body.h.class->send-rslt-body(x->body);
	  (csym::write-eol)
	  (csym::pthread-mutex-lock (ptr (fref thr -> mut))))
      (begin
	(csym::perror "Invalid rcmd.node in recv-exec-send~%")
	(csym::fprintf stderr "%d~%" (fref rcmd node))
	(csym::exit 0))))

  ;; �Ǹ�ˤ�treq�����ޤäƤ����� none������
  (csym::flush-treq-with-none-1 thr)
  (csym::flush-send)
  (csym::pthread-mutex-unlock (ptr send-mut))
  (csym::pthread-mutex-lock (ptr (fref thr -> rack-mut)))
  (inc (fref (mref thr) w-rack))
  (csym::pthread-mutex-unlock (ptr (fref thr -> rack-mut)))
  (= (fref thr -> ndiv) old-ndiv)

  (label Lnone
    (begin
      (= (fref thr -> task-free) tx)
      (= (fref thr -> task-top) (fref tx -> next))))
  )

(def (worker arg) (fn (ptr void) (ptr void))
  (def thr (ptr (struct thread-data)) arg)
  (csym::pthread-mutex-lock (ptr (fref thr -> mut)))
  (loop
    (recv-exec-send thr "any" ANY))
  (csym::pthread-mutex-unlock (ptr (fref thr -> mut))))

;;; recv-task <��������ʬ�䤵�줿���> <������> <������>
(def (csym::recv-task cmd body) (fn void (struct cmd) (ptr void))
  (def tx (ptr (struct task)))
  (def thr (ptr (struct thread-data)))
  (def id unsigned-int)
  (def len size-t)
  (if (< (fref cmd c) 4)   ; �ѥ�᡼���������å�
      (csym::proto-error "wrong-task" cmd))
  ;; id��<task-head>�˴ޤ��
  (= id (csym::atoi (aref (fref cmd v) 3)))
  (if (not (< id num-thrs))
      (csym::proto-error "wrong task-head" cmd))
  (= thr (+ threads id))    ; thr: task��¹Ԥ��륹��å�
  (csym::pthread-mutex-lock (ptr (fref thr -> mut)))
  (= tx (fref thr -> task-top)) ; ���ξ��ˤ�äƤۤ���task�����񤭹���
  (= (fref tx -> rslt-to) (fref cmd node))  ; ��̤������褬�ɤ���
  (= len (csym::strlen (aref (fref cmd v) 2)))
  (if (> len 254)
      (csym::proto-error "Too long rslt-head for task" cmd))
  (csym::strncpy (fref tx -> rslt-head) ; "rslt" ��������˻Ȥ����ɥ쥹
		 (aref (fref cmd v) 2)
		 (+ len 1))
  (= (fref tx -> ndiv) (csym::atoi (aref (fref cmd v) 1))) ; ʬ����
  ;; �������Υѥ�᡼����task specific�ʹ�¤�Ρˤμ������
  (if (== (fref cmd node) INSIDE)
      (= (fref tx -> body) body)      ; INSIDE���ä���ݥ��󥿤��Ϥ�����
    (if (== (fref cmd node) OUTSIDE)
	(begin                        ; OUTSIDE���ä���ʸ������ɤ߹���
	  (= (fref tx -> body)
	     (csym::recv-task-body thr)) ; �ɤ߽Ф��ϥ�å����������Τۤ�����������
	  (csym::read-to-eol))
      (begin
	(csym::perror "Invalid cmd.node in recv-task~%")
	(csym::fprintf stderr "%d~%" (fref cmd node))
	(csym::exit 0))))

  ;; task �������ä��塤�Ρ�������ԤäƤ��륹��å�(���)�򵯤���
  (= (fref tx -> stat) TASK-INITIALIZED) ; ���⤽���å���ɬ�פʤΤϤ�����������
  ;; treq���Ƥ���task��������ޤǤδ֤�stat����������������ʤ���������
  
  ;; ����åɿ��ξ�¤�Ķ����ʤ顤���(�ɤ줫�Υ���åɤ������Ȥ�)
  ;; signal ����٤���?
  ;; ������ϡ��̤˥��ޥե��Ǿ�¤��������ۤ�����ñ
  (csym::pthread-cond-broadcast (ptr (fref thr -> cond)))
  (csym::pthread-mutex-unlock (ptr (fref thr -> mut))))

(def (recv-none cmd) (fn void (struct cmd))
  (def thr (ptr (struct thread-data)))
  (def id unsigned-int)
  (def len size-t)
  (if (< (fref cmd c) 2)
      (csym::proto-error "Wrong none" cmd))
  (= id (csym::atoi (aref (fref cmd v) 1)))
  (if (not (< id num-thrs))
      (csym::proto-error "Wrong task-head" cmd))
  (= thr (+ threads id))
  (csym::pthread-mutex-lock (ptr (fref thr -> mut)))
  (if (> (fref thr -> w-none) 0)
      (dec (fref thr -> w-none))
    (= (fref thr -> task-top -> stat) TASK-NONE))
  (csym::pthread-cond-broadcast (ptr (fref thr -> cond)))
  (csym::pthread-mutex-unlock (ptr (fref thr -> mut))))

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

(def (csym::recv-rslt cmd rbody) (fn void (struct cmd) (ptr void))
  (def rcmd (struct cmd))
  (def thr (ptr (struct thread-data)))
  (def hx (ptr (struct task-home)))
  (def tid unsigned-int)
  (def sid unsigned-int)
  (def b (ptr char))
  (def h-buf (array char 256))
  (if (< (fref cmd c) 2)  ; �����ο������å�
      (csym::proto-error "Wrong rslt" cmd))
  (= b (aref (fref cmd v) 1)) ; ��������� (<tid>:<sid>)
  (= tid (csym::atoi b))
  (= b (csym::strchr b #\:))
  (if (not b)
      (csym::proto-error "Wrong rslt-head" cmd))
  (= sid (csym::atoi (+ b 1)))
  (= thr (+ threads tid))

  (csym::pthread-mutex-lock (ptr (fref thr -> mut)))
  (= hx (fref thr -> sub))
  ;; hx = �֤äƤ���rslt���ԤäƤ���task-home��.id==sid�ˤ�õ��
  (while (and hx (!= (fref hx -> id) sid))
    (= hx (fref hx -> next)))
  (if (not hx)
      (csym::proto-error "Wrong rslt-head" cmd))
  (if (== (fref cmd node) INSIDE)  ; ��̤μ������
      (= (fref hx -> body) rbody)  ; INSIDE�ʤ�body�Υݥ�������
    (if (== (fref cmd node) OUTSIDE) ; OUTSIDE�ʤ饹�ȥ꡼�फ��
	(begin
	  (csym::recv-rslt-body thr (fref hx -> body))
	  (csym::read-to-eol))
      (begin
	(csym::perror "Invalid cmd.node in recv-rslt~%")
	(csym::fprintf stderr "%d~%" (fref cmd node))
	(csym::exit 0))))

  ;; rack���֤�����äȸ�Τۤ����褤��
  (= (fref rcmd c) 2)
  (= (fref rcmd node) (fref cmd node))
  (= (aref (fref rcmd v) 0) "rack")
  (csym::strncpy
   h-buf (fref hx -> task-head)
   (+ 1 (csym::strlen (fref hx -> task-head))))
  (= (aref (fref rcmd v) 1) h-buf)
  ;; hx ��˵�Ͽ���줿 task-head �� rack ��������ʤ顤
  ;; �����ǤϤʤ������ޤ� free ���줿���ʤ��Τǡ��Ĥʤ��ʤ�������
  (= (fref hx -> stat) TASK-HOME-DONE)
  (if (== hx (fref thr -> sub))
      (begin
	(csym::pthread-cond-broadcast (ptr (fref thr -> cond-r)))
	(csym::pthread-cond-broadcast (ptr (fref thr -> cond))))
    )
  (csym::pthread-mutex-unlock (ptr (fref thr -> mut)))
  (if (fref cmd node)
      (begin
	(csym::pthread-mutex-lock (ptr queue-mut))
	(csym::enqueue-command rcmd NULL)
	(csym::pthread-cond-signal (ptr cond-q))
	(csym::pthread-mutex-unlock (ptr queue-mut)))
    (begin
      ;; �ǥåɥ�å��ɻߤΤ����ɤ߽Ф��򤹤륹��åɤǽ񤭹��ߤ򤷤ʤ��褦�ˤ��Ƥ���
      (csym::pthread-mutex-lock (ptr snr-mut))
      (csym::enqueue-snr rcmd)
      (csym::pthread-cond-signal (ptr cond-snr))
      (csym::pthread-mutex-unlock (ptr snr-mut)))))

;; worker �����Ф餯������äƤ���ʤ��Ƥ⡤
;; treq �ϼ�����äƤ����ۤ����褵������
;; ʣ���� treq ��ꥹ�Ȥˤ��Ƽ�����餻�Ƥ�褤�ΤǤ�?
;; treq �� task-home �η��ˤ��Ƥ����Ƥ�褤���⡥
;; task-home �� �����å��������ȤΤĤ����ä���...

;;; threads[id] ��treq���ߤ�
(def (csym::try-treq cmd id) (fn int (struct cmd) unsigned-int)
  (def hx (ptr (struct task-home)))
  (def thr (ptr (struct thread-data)))
  (def len size-t)
  (def avail int 0)

  (= thr (+ threads id))
  (csym::pthread-mutex-lock (ptr (fref thr -> mut)))
  (csym::pthread-mutex-lock (ptr (fref thr -> rack-mut)))
  (if (and (fref thr -> task-top)
	   (or (== (fref thr -> task-top -> stat) TASK-STARTED)
	       (== (fref thr -> task-top -> stat) TASK-INITIALIZED))
	   (== (fref thr -> w-rack) 0))
      (= avail 1))
  (csym::pthread-mutex-unlock (ptr (fref thr -> rack-mut)))

  ;; �����ʤ饿���������Ԥ������å���push
  (if avail
      (begin
	(= hx (fref thr -> treq-free))
	(if (not hx)
	    (csym::mem-error "Not enough task-home memory"))
	(= (fref thr -> treq-free) (fref hx -> next))
	(= (fref hx -> next) (fref thr -> treq-top))
	(= (fref hx -> stat) TASK-HOME-ALLOCATED)
	(= len (csym::strlen (aref (fref cmd v) 1)))
	(if (> len 254)
	    (csym::proto-error "Too long task-head for treq" cmd))
	(csym::strncpy (fref hx -> task-head)
		       (aref (fref cmd v) 1)
		       (+ len 1))
	(if (!= (fref cmd node) OUTSIDE)
	    (= (fref hx -> req-from) INSIDE)
	  (= (fref hx -> req-from) OUTSIDE))
	(= (fref thr -> treq-top) hx)
	(= (fref thr -> req) hx)
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

(def (csym::recv-treq cmd) (fn void (struct cmd))
  ;;   task id �����ꤵ��Ƥ�����ȡ�any �ξ�礬���롥
  ;; any �ξ��ϡ�any �ǤȤäƤ�����ΤΤۤ����礭���ΤǤ�?
  ;; => ���⤽�⡤���٤Ƥ� task ��ư���Ƥ���櫓�ǤϤʤ���
  ;;    �����ʤ��Ȥ⡤rslt �ޤ��ǡ��� task ��Ω���夲�� task ��
  ;;    req ���Ƥ���ᡥ
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
  (def rcmd (struct cmd))
  (def id unsigned-int)
  (if (< (fref cmd c) 3)  ; �����ο������å�
      (csym::proto-error "Wrong treq" cmd))
  ;; id ��<task-head>�˴ޤ��
  (if (== (csym::strcmp (aref (fref cmd v) 2) "any") 0)
      (let ((myid int))  ; ������id
	(= myid (csym::atoi (aref (fref cmd v) 1)))
	(for ((= id 0) (< id num-thrs) (inc id))
	  (if (and (!= (fref cmd node) OUTSIDE)
		   (== id myid))
	      (continue))  ; ��ʬ���Ȥ��׵��Ф��褦�ʥݥ��Ϥ��ʤ�
	  (if (csym::try-treq cmd id) (break)))
	(if (!= id num-thrs) ; treq�Ǥ���
	    (return)))
    (begin  ; "any"����ʤ����
      (= id (csym::atoi (aref (fref cmd v) 2)))
      (if (not (< id num-thrs))
	  (csym::proto-error "Wrong task-head" cmd))
      (if (csym::try-treq cmd id)
	  (return))))

  ;; treq�Ǥ��ʤ��ä����
  (if (== (fref cmd node) ANY)
      (if (== (csym::atoi (aref (fref cmd v) 1)) 0)
	  (begin
	    (csym::pthread-mutex-lock (ptr send-mut))
	    (csym::send-command cmd)
	    (csym::flush-send)
	    (= divisibility-flag 1)
	    (csym::pthread-mutex-unlock (ptr send-mut))
	    (return))
	(= (fref cmd node) INSIDE)))

  ;; none���֤�
  (= (fref rcmd c) 2)
  (= (fref rcmd node) (fref cmd node))
  (= (aref (fref rcmd v) 0) "none")
  (= (aref (fref rcmd v) 1) (aref (fref cmd v) 1))
  (if (== (fref rcmd node) INSIDE)
      (begin
	(csym::pthread-mutex-lock (ptr queue-mut))
	(csym::enqueue-command rcmd NULL)
	(csym::pthread-cond-signal (ptr cond-q))
	(csym::pthread-mutex-unlock (ptr queue-mut)))
    (if (== (fref rcmd node) OUTSIDE)
	(begin
	  ;; �ǥåɥ�å��ɻߤΤ����ɤ߽Ф��򤹤륹��åɤǽ񤭹��ޤʤ��褦�ˤ��Ƥ���
	  (csym::pthread-mutex-lock (ptr snr-mut))
	  (csym::enqueue-snr rcmd)
	  (csym::pthread-cond-signal (ptr cond-snr))
	  (csym::pthread-mutex-unlock (ptr snr-mut)))
      (begin
	(csym::perror "Invalid rcmd.node in recv-treq~%")
	(csym::fprintf stderr "%d~%" (fref rcmd node))
	(csym::exit 0)))))

(def (csym::recv-rack cmd) (fn void (struct cmd))
  (def tx (ptr (struct task)))
  (def thr (ptr (struct thread-data)))
  (def id unsigned-int)
  (def len size-t)
  (if (< (fref cmd c) 2)
      (csym::proto-error "Wrong rack" cmd))
  ;; id��<task-head>�˴ޤ��
  (= id (csym::atoi (aref (fref cmd v) 1)))
  (if (not (< id num-thrs))
      (csym::proto-error "Wrong task-head" cmd))
  (= thr (+ threads id))
  (csym::pthread-mutex-lock (ptr (fref thr -> rack-mut)))
  (dec (fref thr -> w-rack))
  (csym::pthread-mutex-unlock (ptr (fref thr -> rack-mut))))

(def (csym::send-divisible) (fn void void)
  (def cmd (struct cmd))
  (if (csym::pthread-mutex-trylock (ptr send-mut))
      (return))
  (= divisibility-flag 0)
  (= (fref cmd c) 1)
  (= (aref (fref cmd v) 0) "dvbl")
  (csym::send-command cmd)
  (csym::flush-send)
  (csym::pthread-mutex-unlock (ptr send-mut)))


(def (handle-req -bk -thr)
  (fn void (ptr (FN int void)) (ptr (struct thread-data)))

  (csym::pthread-mutex-lock (ptr (fref -thr -> mut)))
  (if (fref -thr -> req)
      (begin
	(-bk)
	(= (fref -thr -> req) (fref -thr -> treq-top))))
  (csym::pthread-mutex-unlock (ptr (fref -thr -> mut))))

(def (csym::make-and-send-task thr body)
  (fn void (ptr (struct thread-data)) (ptr void))

  (def tcmd (struct cmd))
  (def hx (ptr (struct task-home))
    (fref thr -> treq-top))
  (= (fref thr -> treq-top) (fref hx -> next))
  (= (fref hx -> next) (fref thr -> sub))
  (= (fref thr -> sub) hx)
  (= (fref hx -> body) body)
  (= (fref hx -> id) (if-exp (fref hx -> next)
			 (+ (fref hx -> next -> id) 1)
		       0))
  (= (fref hx -> stat) TASK-HOME-INITIALIZED)
  (= (fref tcmd c) 4)
  (= (fref tcmd node) (fref hx -> req-from))
  (= (aref (fref tcmd v) 0) "task")
  ;; ����ʤΤ����
  (csym::sprintf (fref thr -> ndiv-buf) "%d"
		 (++ (fref thr -> ndiv)))
  (csym::sprintf (fref thr -> buf) "%s:%d"
		 (fref thr -> id-str) (fref hx -> id))
  (= (aref (fref tcmd v) 1) (fref thr -> ndiv-buf))
  (= (aref (fref tcmd v) 2) (fref thr -> buf))
  (= (aref (fref tcmd v) 3) (fref hx -> task-head))

  (if (== (fref tcmd node) INSIDE)
      (begin
	(csym::pthread-mutex-lock (ptr queue-mut))
	(csym::enqueue-command tcmd body)
	(csym::pthread-cond-signal (ptr cond-q))
	(csym::pthread-mutex-unlock (ptr queue-mut)))
    (if (== (fref tcmd node) OUTSIDE)
	(begin
	  (csym::pthread-mutex-lock (ptr send-mut))
	  (csym::pthread-mutex-unlock (ptr (fref thr -> mut)))
	  (csym::send-command tcmd)
	  (csym::send-task-body thr body)
	  (csym::write-eol)
	  (csym::flush-send)
	  (csym::pthread-mutex-unlock (ptr send-mut))
	  (csym::pthread-mutex-lock (ptr (fref thr -> mut))))
      (begin
	(csym::perror "Invalid tcmd.node in make-and-send-task~%")
	(csym::fprintf stderr "%d~%" (fref tcmd node))
	(csym::exit 0)))))

(def (wait-rslt thr) (fn (ptr void) (ptr (struct thread-data)))
  (def body (ptr void))
  (def t1 (struct timespec))
  (def now (struct timeval))
  (def nsec long)
  (def sub (ptr (struct task-home)))
  (csym::pthread-mutex-lock (ptr (fref thr -> mut)))
  (= sub (fref thr -> sub))
  (while (!= (fref sub -> stat) TASK-HOME-DONE)
    (csym::gettimeofday (ptr now) 0)
    (= nsec (* 1000 (fref now tv-usec)))
    (+= nsec (* 5 1000 1000))
    ;; (+= nsec (* 10 1000 1000))
    (= (fref t1 tv-nsec) (if-exp (> nsec 999999999)
			     (- nsec 999999999)
			   nsec))
    (= (fref t1 tv-sec) (+ (fref now tv-sec)
			   (if-exp (> nsec 999999999) 1 0)))
    (csym::pthread-cond-timedwait
     (ptr (fref thr -> cond-r)) (ptr (fref thr -> mut))
     (ptr t1))
    (if (== (fref sub -> stat) TASK-HOME-DONE)
	(break))
    ;; fprintf(stderr, "sub %d\n", sub);

    (recv-exec-send
     thr (fref sub -> task-head) (fref sub -> req-from)))
  (= body (fref sub -> body))
  (= (fref thr -> sub) (fref sub -> next))
  (= (fref sub -> next) (fref thr -> treq-free))
  (= (fref thr -> treq-free) sub)
  ;; fprintf(stderr, "nsub %d\n", thr->sub);
  (csym::pthread-mutex-unlock (ptr (fref thr -> mut)))
  (return body))



;; main
(def (main argc argv) (fn int int (ptr (ptr char)))
  (defs int i j)
  (def dummy (ptr void))
  (def cmd (struct cmd))

  ;; cmd-queue�ʼ�����å��������塼�� �ν����
  (for ((= i 0) (< i 512) (inc i))
    (= (fref (aref cmd-queue i) next)
       (ptr (aref cmd-queue (% (+ i 1) 512))))
    (for ((= j 0) (< j 4) (inc j))
      (= (aref (fref (aref cmd-queue i) cmd v) j)
	 (aref cmd-v-buf i j))))
  (= cmd-in (ptr (aref cmd-queue 0)))
  (= cmd-out (ptr (aref cmd-queue 0)))
  (csym::systhr-create exec-queue-cmd NULL) ; ��å�������������å�

  ;; snr-queue��none, rack��å������������塼�ˤν����
  (for ((= i 0) (< i 32) (inc i))
    (= (fref (aref  snr-queue i) next)
       (ptr (aref snr-queue (% (+ i 1) 32))))
    (for ((= j 0) (< j 2) (inc j))
      (= (aref (fref (aref snr-queue i) cmd v) j)
	 (aref snr-v-buf i j))))
  (= snr-in (ptr (aref snr-queue 0)))
  (= snr-out (ptr (aref snr-queue 0)))
  (csym::systhr-create send-none-rack NULL)  ; none, rack��������å�
  
  (= num-thrs (if-exp (> argc 1)
		  (csym::atoi (aref argv 1))
		1))
  ;; ������ thread-data �ν����, task �� ������list ��
  ;; num-thrs = 1;
  (for ((= i 0) (< i num-thrs) (inc i))
    (let ((thr (ptr (struct thread-data)) (+ threads i))
	  (tx (ptr (struct task)))
	  (hx (ptr (struct task-home))))
      (= (fref thr -> id) i)
      (= (fref thr -> w-rack) 0)
      (= (fref thr -> w-none) 0)
      (= (fref thr -> ndiv) 0)
      (csym::pthread-mutex-init (ptr (fref thr -> mut)) 0)
      (csym::pthread-mutex-init (ptr (fref thr -> rack-mut)) 0)
      (csym::pthread-cond-init (ptr (fref thr -> cond)) 0)
      (csym::pthread-cond-init (ptr (fref thr -> cond-r)) 0)
      (csym::sprintf (fref thr -> id-str) "%d" i)

      ;; task���������ꥹ��
      (= tx (cast (ptr (struct task))
	      (csym::malloc (* (sizeof (struct task)) 512))))
      (= (fref thr -> task-top) 0)
      (= (fref thr -> task-free) tx)
      (for ((= j 0) (< j 511) (inc j))
	(= (fref (aref tx j) prev) (ptr (aref tx (+ j 1))))
	(= (fref (aref tx (+ j 1)) next) (ptr (aref tx j))))
      (= (fref (aref tx 0) next) 0)
      (= (fref (aref tx 511) prev) 0)

      ;; task-home�Υꥹ��
      (= hx (cast (ptr (struct task-home))
	      (csym::malloc (* (sizeof (struct task-home)) 512))))
      (= (fref thr -> treq-top) 0)
      (= (fref thr -> treq-free) hx)
      (= (fref thr -> sub) 0)
      (for ((= j 0) (< j 511) (inc j))
	(= (fref (aref hx j) next) (ptr (aref hx (+ j 1)))))
      (= (fref (aref hx 511) next) 0)
      ;; �������å�����
      ;; Ʊ��Ρ�����ʶ�ͭ����ˤǤ��꤯��Ǥ��뤫��
      (csym::systhr-create worker thr)))
  
  ;; master����åɤ�OUTSIDE����Υ�å���������
  (while (exps (= cmd (recv-command))
	       (and (> (fref cmd c) 0)
		    (!= 0 (csym::strcmp
			   (aref (fref cmd v) 0) "exit"))))
    
    (if (== 0 (csym::strcmp (aref (fref cmd v) 0) "task"))
	(csym::recv-task cmd dummy)
      (if (== 0 (csym::strcmp (aref (fref cmd v) 0) "rslt"))
	  (csym::recv-rslt cmd dummy)
	(if (== 0 (csym::strcmp (aref (fref cmd v) 0) "treq"))
	    (csym::recv-treq cmd)
	  (if (== 0 (csym::strcmp (aref (fref cmd v) 0) "none"))
	      (recv-none cmd)
	    (if (== 0 (csym::strcmp (aref (fref cmd v) 0) "rack"))
		(csym::recv-rack cmd)
	      (csym::proto-error "wrong cmd" cmd)))))))
  (csym::exit 0))

