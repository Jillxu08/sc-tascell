;; worker3���顤ics08���˲���
;; * ���ޥ�ɤ�����ʥ�����ɥ쥹�ˤ�ɬ�׻��ʳ�ʸ����ǰ���ʤ�
;;   - serialize, deserialize�ؿ�(in cmd-serial.sc)���ä�����Ѵ���⥸�塼�벽
;;   - cmd.v[0]�˥��ޥ��̾������ʤ����ʹ����ˤ��餹
;; * �����ؤ�send�����Ƥμ���ˤĤ���sender����åɤ�ô��
;; --> ��������worker���Ȥ�����
;; * �����ؤ�send�ˤĤ��Ƥ� inms-queue���ɲ� => ������å�������������åɤ�ô��
;; --> ����������������worker���Ȥ�ɬ�פʽ�����Ԥ�
;; * �����Υ�å����������ˤĤ��Ƥϡ�ʸ������Ȥ�Ω�Ƥ�Τ����
;; * timewait-rslt
;;   treq->none�������äƤ����Ȥ����ˤ��Ф餯�ԤĤ�����

;; *** �ʲ�̤�����β��ɤ��٤���
;; [07/08/13]
;; * �Ȥ��ɤ��Τ����treq��any�ʳ���treq�ˤ��Ф��Ƥϼ�ʬ�����ˤʤ�ޤ�
;; ��treq any����������ˤʤ�ޤǡ�none���֤��ʤ�
;; ��ͳ��a->b ��treq���ƻŻ����ʤ��Τϡ�b->c��treq���Ƥ��뤫��Ǥ����礬¿���Τ�
;;         any��treq���ä����ɤ����ϼ�����ä����˳Ф��Ƥ���ɬ��
;; ���Ӥ��󤬤���������version: ~yasugi/lang/c/loadb/

;; [08/01/16]
;; ��³���ȥ��ꥢ�饤�������̥Ρ��ɤ����������Ƥ������

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

(%include "rule/nestfunc-setrule.sh")

(c-exp "#include<stdio.h>")
(c-exp "#include<stdlib.h>")
(c-exp "#include<pthread.h>")
(c-exp "#include<sys/time.h>")
(c-exp "#include<getopt.h>")
#+tcell-gtk (c-exp "#include<gtk/gtk.h>")
(%include "worker4.sh")

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
  (def buf (array char BUFSIZE)) 
  (csym::serialize-cmd buf pcmd)
  (csym::fputs str stderr)
  (csym::fputc #\> stderr)
  (csym::fputc #\Space stderr)
  (csym::fputs buf stderr)
  (csym::fputc #\Newline stderr)
  )

;; �����ؤ�������å�
(def send-mut pthread-mutex-t)

;; �����Фؤ������������å�(<0:stdin/out)��main������
(def sv-socket int)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; ���������

;; EOL�ޤǤ�̵��
(def (csym::read-to-eol) (csym::fn void void)
  (def c int)
  (while (!= EOF (= c (csym::receive-char sv-socket)))
                                        ;(!= EOF (= c (csym::getc stdin)))
    (if (== c #\Newline) (break))))

(def (csym::write-eol) (csym::fn void void)
  (csym::send-char #\Newline sv-socket))

(def (csym::flush-send) (csym::fn void void)
  (if (< sv-socket 0) (csym::fflush stdout)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; fgets��OUTSIDE����Υ�å������������� -> cmd-buf �˽񤭹���
;;; 1�Ԥ��ɤ߹�����塤���ڡ�����NULLʸ�����֤������뤳�Ȥ�ʸ�����ʬ�䤹�롥
;;; �ޤ���cmd.v[]��ʬ�䤵�줿��ʸ�����ؤ��褦�ˤ���
;;;   fgets �� 0-terminated buffer ���֤���
;;;   fgets returns 0 �ΤȤ����н�ɬ��?
;;; buf, cmd-buf �ϰ�ĤʤΤ���աʺ���main���ܥ���åɤ�������ʤ���
;; ��������Υ�å����������Хåե�
(def buf (array char BUFSIZE))
(def cmd-buf (struct cmd))

(def (csym::read-command) (csym::fn (ptr (struct cmd)))
  (defs char p c)
  (def b (ptr char) buf)
  (def cmdc int)

  (csym::receive-line b BUFSIZE sv-socket)
  (= cmd-buf.node OUTSIDE)
  ;; p:�������ʸ����c:���ߤ�ʸ��
  (csym::deserialize-cmd (ptr cmd-buf) b)
  (return (ptr cmd-buf)))

;;; struct cmd -> output��stdout�ء�
;;; task, rslt�Ǥ� body�����Ƥ�task-no�����ꤹ��ؿ�������
(def send-buf (array char BUFSIZE))
(def (csym::send-out-command pcmd body task-no)
    (csym::fn void (ptr (struct cmd)) (ptr void) int)
  (def ret int)
  (def w (enum command))
  (= w pcmd->w)
  ;; <--- sender-lock <---
  (csym::pthread-mutex-lock (ptr send-mut))
  ;; ���ޥ��̾
  (csym::serialize-cmd send-buf pcmd)
  (csym::send-string send-buf sv-socket)
  (csym::write-eol)
  ;; task, rslt��body
  (if body
      (cond
       ((== w TASK)
        ((aref task-senders task-no) body)
        (csym::write-eol))
       ((== w RSLT)
        ((aref rslt-senders task-no) body)
        (csym::write-eol))))
  (csym::flush-send)
  (csym::pthread-mutex-unlock (ptr send-mut))
  ;; ---> sender-lock --->
  )

;;; �ʼ�����cmd����äƥ�å������μ����Ŭ�����ؿ���ƽФ�
;;; body�ϥ��������֥������ȡ������̿��ΤȤ���
;;; �ޤ���0�ʳ����̿��ΤȤ������input stream�������������äƺ���
(def (csym::proc-cmd pcmd body) (csym::fn void (ptr (struct cmd)) (ptr void))
  (def w (enum command))
  (= w pcmd->w)
  (cond
   ((== w TASK) (csym::recv-task pcmd body))
   ((== w RSLT) (csym::recv-rslt pcmd body))
   ((== w TREQ) (csym::recv-treq pcmd))
   ((== w NONE) (csym::recv-none pcmd))
   ((== w RACK) (csym::recv-rack pcmd))
   ((== w EXIT) (csym::exit 0))
   (else
    (csym::proto-error "wrong cmd" pcmd))))

;;; �Ρ�����/������˥��ޥ�ɤ�����
;;; body��task, rslt�����Ρ�����ʳ��Υ��ޥ�ɤǤ�NULL��
(def (csym::send-command pcmd body task-no) (csym::fn void (ptr (struct cmd)) (ptr void) int)
  (if (== pcmd->node OUTSIDE)
      (csym::send-out-command pcmd body task-no) ; ����������
    (csym::proc-cmd pcmd body)          ; �����������ʤȤ�����worker���Ȥ����ޥ�ɽ�����
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
  (= rcmd.c 1)                          ; 4�Ǥؤ餷��
  (= rcmd.w NONE)
  (while (= hx thr->treq-top)
    (= rcmd.node hx->req-from)          ; ����or����
    (csym::copy-address (aref rcmd.v 0) hx->task-head) ; 4�Ǥؤ餷��
    (csym::send-command (ptr rcmd) 0 0)
    (= thr->treq-top hx->next)          ; treq�����å���pop
    (= hx->next thr->treq-free)         ; �ե꡼�ꥹ�Ȥ�...
    (= thr->treq-free hx))              ; ...�ΰ���ֵ�
  )

;;; worker or wait-rslt ����
;;; �ʸ�Ԥϳ����ꤲ���������η���Ԥ�����̤λŻ������Ȥ������
;;; �������׵� -> ������� -> �׻� -> �������
;;; thr->mut �ϥ�å���
;;; (treq-head,req-to): �ɤ��˥������׵��Ф�����any or ����֤���
(def (recv-exec-send thr treq-head req-to)
    (fn void (ptr (struct thread-data)) (ptr (enum node)) (enum node))
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
  (while (> thr->w-none 0)
    (csym::pthread-cond-wait (ptr thr->cond) (ptr thr->mut))
    ;; rslt�����Ƥ����鼫ʬ�ν�����Ƴ��Ǥ���Τ�return
    (if (and thr->sub
             (== thr->sub->stat TASK-HOME-DONE))
        (return)))

  ;; allocate
  (= tx thr->task-free)
  (= tx->stat TASK-ALLOCATED)
  (if (not tx)
      (csym::mem-error "Not enough task memory"))
  (= thr->task-top tx)
  (= thr->task-free tx->prev)

  ;;(= delay (* 2 1000 1000))
  (= delay 1000)
  
  ;; treq���ޥ��
  (= rcmd.c 2)                          ; 4�Ǥؤ餷��
  (if (> num-thrs 1)
      (= rcmd.node req-to)
    (= rcmd.node OUTSIDE))
  (= rcmd.w TREQ)
  (= (aref rcmd.v 0 0) thr->id)         ; 4�Ǥؤ餷��
  (= (aref rcmd.v 0 1) TERM)
  (csym::copy-address (aref rcmd.v 1) treq-head) ; 4�Ǥؤ餷��
  (do-while (!= (fref tx -> stat) TASK-INITIALIZED)
    ;; �ǽ��treq�����ޤäƤ����顤none������
    ;; �������� treq �����Ӹ򤦤��⤷��ʤ�����
    ;; none ������ʤ��Ȥ�������ȡ�
    ;; �ߤ��� none or task �������Ƥ���Τ��Ԥ�
    ;; �Ȥ��������פΥǥåɥ�å��Ȥʤ롥
    (csym::flush-treq-with-none thr)
    (= tx->stat TASK-ALLOCATED)         ; *-NONE�ˤ���Ƥ뤫�⤷��ʤ��Τ򸵤��᤹
    ;; �� �����ǡ�send_mut ��Ȥ����ˡ�thr->mut �� unlock ���٤��Ǥ�?
    (begin
     (csym::pthread-mutex-unlock (ptr thr->mut))
     (csym::send-command (ptr rcmd) 0 0) ; treq����
     (csym::pthread-mutex-lock (ptr thr->mut)))
    ;; recv-task �� *tx ����������Τ��Ԥ�
    (loop
      ;; rslt �����夷�Ƥ����顤���̤���ˤ�����
      (if (and (!= tx->stat TASK-INITIALIZED)
               thr->sub                 ; rslt��....
               (== thr->sub->stat TASK-HOME-DONE)) ;...���夷������
          ;; rslt �����夹��褦�ʾ�硤�����Ǥ�treq�ϼ��ᤷ�Ǥ��ꡤ
          ;; ���μ��ᤷ�ϼ��Ԥ��ơ�none���֤����Ϥ��Ǥ��롥
          ;; ���Ǥ� none ���֤���Ƥ��뤫�ǡ��Ԥ����Υ����󥿤�
          ;; ���䤹���ɤ������ۤʤ�
          ;; �� ���Τۤ��ˤ��롤rack �Υ����ߥ󥰤ˤ�äƤ�
          ;; ��������������ǽ�����ꡥ�������줿���ϡ�
          ;; w_none �ϻȤ��ʤ��Τǡ�none �� task ���֤����Τ򤳤ξ��
          ;; �Ԥ���task ���ä����ϡ��ʲ��� do_task_body �Τۤ��˿ʤ�ΤǤ�?
          (begin
           (if (!= tx->stat TASK-NONE)
               (inc thr->w-none))
           (goto Lnone)))
      (if (!= tx->stat TASK-ALLOCATED)
          (break))
      (csym::pthread-cond-wait (ptr thr->cond) (ptr thr->mut))
      )
    ;; none �������ä��Ȥ��ˡ����Ф餯�ԤĤ褦�ˤ��ƤߤƤ��롥
    ;; ����¾�μ�ϻȤ��ʤ���? (Ʊ��ʣ���Ľ��Ԥ������β�����)
    ;; �� �����ʤ��Ȥ�ꥯ�������褬 any �Ǥʤ��ʤ顤
    ;;    rslt �ȥ��åȤ�none ���֤����ʳ��ϡ�
    ;;    none���֤�ɬ�פϤʤ��Ԥ����Ƥ����Ф褤��
    ;; �� any �ξ�硤treq combining �Ȥ���ʣ���Ľ��Ԥ��Ȥ�
    ;;    �����Ǥϡ�delay ���ܡ��Ȥ��Ƥ��äƤ��롥2ms ����Ϥ����
    (if (== tx->stat TASK-NONE)
        (begin
         ;; ���ؤμ���֤��˼��Ԥ����Τʤ餷�Ф餯�Ԥ�
         (if 1;; (and thr->sub
              ;;    (== PARENT (aref thr->sub->task-head 0)))
             (let ((t1 (struct timespec))
                   (now (struct timeval))
                   (nsec long))
               (csym::gettimeofday (ptr now) 0)
               (= nsec (* now.tv-usec 1000))
               (+= nsec delay)
               (+= delay delay)         ; ������Ԥ����֤����䤹
               (if (> delay (* 40 1000 1000)) ; �Ǥ���٤�����
                   (= delay (* 40 1000 1000)))
               (= t1.tv-nsec (if-exp (> nsec 999999999)
                                     (- nsec 999999999)
                                     nsec))
               (= t1.tv-sec (+ now.tv-sec
                               (if-exp (> nsec 999999999) 1 0)))
               (csym::pthread-cond-timedwait (ptr thr->cond-r)
                                             (ptr thr->mut)
                                             (ptr t1))
               ))
         ;; rslt�����夷�Ƥ�����
         (if (and thr->sub
                  (== thr->sub->stat TASK-HOME-DONE))
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
  (= tx->stat TASK-STARTED)
  (= old-ndiv thr->ndiv)
  (= thr->ndiv tx->ndiv)
  (csym::pthread-mutex-unlock (ptr thr->mut))

  ((aref task-doers tx->task-no) thr tx->body) ; �������¹�
  
  ;; task�ν�����λ��ϡ�����task-home��send-rslt����
  (= rcmd.w RSLT)
  (= rcmd.c 1)                          ; 4�Ǥؤ餷��
  (= rcmd.node tx->rslt-to)             ; ����or����
  (csym::copy-address (aref rcmd.v 0) tx->rslt-head) ; �����襢�ɥ쥹 ; 4�Ǥؤ餷��
  (csym::send-command (ptr rcmd) tx->body tx->task-no)

  ;; �����Ǥ�treq�����ޤäƤ����� none������
  (csym::flush-treq-with-none thr)
  (csym::pthread-mutex-lock (ptr thr->rack-mut))
  ;; w_rack == 0 �ˤʤ�ޤǻߤޤ�ɬ�פϤʤ������������Ϥ��ʤ������Ǥ褤
  (inc thr->w-rack)
  (csym::pthread-mutex-unlock (ptr thr->rack-mut))

  (csym::pthread-mutex-lock (ptr thr->mut))
  (= thr->ndiv old-ndiv)
  ;; ������stack��pop���ƥե꡼�ꥹ�Ȥ��֤�
  (label Lnone
         (begin
          (= thr->task-free tx)
          (= thr->task-top tx->next)))
  )

(def (worker arg) (fn (ptr void) (ptr void))
  (def thr (ptr (struct thread-data)) arg)
  ;; �� ��unlock��ɸ��פ��ѹ����٤�
  (csym::pthread-mutex-lock (ptr thr->mut))
  (loop
    (recv-exec-send thr (init (array (enum node) 2) (array ANY TERM)) ANY))
  (csym::pthread-mutex-unlock (ptr thr->mut)))

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
  (if (< pcmd->c 4)                     ; 4�Ǥؤ餷��
      (csym::proto-error "wrong-task" pcmd))
  ;; ��������Υ�å������ξ�硤���ޥ�ɤ�³��task���Τ�������
  ;; ����������ξ��ϰ�����Ϳ�����Ƥ����
  (= task-no (aref pcmd->v 3 0))        ; [3]: �������ֹ� ; 4�Ǥؤ餷��
  (if (== pcmd->node OUTSIDE)           ; ���������task�ξ��Ϥ�����body��������
      (begin
       (= body ((aref task-receivers task-no)))
       (csym::read-to-eol)))
  ;; <task-head>�򸫤ơ���������¹Ԥ��륹��åɤ���롥
  (= id (aref pcmd->v 2 0))             ; 4�Ǥؤ餷��
  (if (not (< id num-thrs))
      (csym::proto-error "wrong task-head" pcmd))
  (= thr (+ threads id))                ; thr: task��¹Ԥ��륹��å�

  ;; ����åɤ˼¹Ԥ��٤����������ɲä���
  (csym::pthread-mutex-lock (ptr thr->mut))
  (= tx thr->task-top)                  ; tx: thr�����٤��Ż��ꥹ��
  (= tx->rslt-to pcmd->node)            ; ��̤���������� [INSIDE|OUTSIDE]
  (csym::copy-address tx->rslt-head (aref pcmd->v 1))
                                        ; [1]: ���긵���̤������� ; 4�Ǥؤ餷��
  (= tx->ndiv (aref pcmd->v 0 0))       ; [0]: ʬ���� ; 4�Ǥؤ餷��
  (= tx->task-no task-no)               ; �������ֹ�
  ;; �������Υѥ�᡼����task specific�ʹ�¤�Ρˤμ������
  ;; �� thr->mut �� unlock �ξ��֤� read ���٤��ǤϤʤ���?
  ;; �� do_task_body �ʳ��� thr������ʤ���?
  (= tx->body body)
  ;; treq���Ƥ���task��������ޤǤδ֤�stat����������������ʤ���������
  ;; ����åɿ��ξ�¤�Ķ����ʤ顤���(�ɤ줫�Υ���åɤ������Ȥ�)
  ;; signal ����٤���?
  ;; ������ϡ��̤˥��ޥե��Ǿ�¤��������ۤ�����ñ
  (= tx->stat TASK-INITIALIZED)         ; ���⤽���å���ɬ�פʤΤϤ�����������
  (csym::pthread-mutex-unlock (ptr thr->mut))

  ;; �Ż��Ԥ���̲�äƤ������򵯤���
  (csym::pthread-cond-signal (ptr thr->cond))
  )

(def (csym::recv-none pcmd) (csym::fn void (ptr (struct cmd)))
  (def thr (ptr (struct thread-data)))
  (def id unsigned-int)
  (def len size-t)
  (if (< pcmd->c 1)                     ; 4�Ǥؤ餷��
      (csym::proto-error "Wrong none" pcmd))
  (= id (aref pcmd->v 0 0))             ; 4�Ǥؤ餷��
  (if (not (< id num-thrs))
      (csym::proto-error "Wrong task-head" pcmd))
  (= thr (+ threads id))
  (csym::pthread-mutex-lock (ptr thr->mut))
  (if (> thr->w-none 0)
      (dec thr->w-none)
    (= thr->task-top->stat TASK-NONE))
  (csym::pthread-mutex-unlock (ptr thr->mut))
  (csym::pthread-cond-signal (ptr thr->cond))
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
  ;; �����ο������å�
  (if (< pcmd->c 1)                     ; 4�Ǥؤ餷��
      (csym::proto-error "Wrong rslt" pcmd))
  ;; ��̼���ͷ��� "<thread-id>:<task-home-id>"
  (= tid (aref pcmd->v 0 0))            ; 4�Ǥؤ餷��
  (if (not (< tid num-thrs))
      (csym::proto-error "wrong rslt-head" pcmd))
  (= sid (aref pcmd->v 0 1))            ; 4�Ǥؤ餷��
  (if (== TERM sid)
      (csym::proto-error "Wrong rslt-head (no task-id)" pcmd))
  (= thr (+ threads tid))

  (csym::pthread-mutex-lock (ptr (fref thr -> mut)))
  ;; hx = �֤äƤ���rslt���ԤäƤ���task-home��.id==sid�ˤ�õ��
  (= hx thr->sub)
  (while (and hx (!= hx->id sid))
    (= hx hx->next))
  (if (not hx)
      (csym::proto-error "Wrong rslt-head (specified task not exists)" pcmd))
  ;; ��������Υ�å������ξ�硤���ޥ�ɤ�³��rslt���Τ�������
  ;; ����������ξ��ϰ�����Ϳ�����Ƥ����
  (cond
   ((== pcmd->node OUTSIDE)
    ;; �� thr->mut ����ä��ޤ�read���٤��ǤϤʤ�
    ((aref rslt-receivers hx->task-no) hx->body)
    (csym::read-to-eol))
   ((== pcmd->node INSIDE)
    ;; �� ������ local-rslt-copy
    (= hx->body body))
   (else
    (csym::proto-error "Wrong cmd.node" pcmd)))
  ;; rack���֤�����äȸ�Τۤ����褤��
  (= rcmd.c 1)                          ; 4�Ǥؤ餷��
  (= rcmd.node pcmd->node)
  (= rcmd.w RACK)
  (csym::copy-address (aref rcmd.v 0) hx->task-head)
                                        ; �����补rslt�ǤϤʤ�����Ȥ�task���ޥ�ɤΤ�Ф��Ƥ���
                                        ; 4�Ǥؤ餷��
  ;; hx ��˵�Ͽ���줿 task-head �� rack ��������ʤ顤
  ;; �����ǤϤʤ������ޤ� free ���줿���ʤ��Τǡ��Ĥʤ��ʤ�������
  (= hx->stat TASK-HOME-DONE)
  (if (== hx thr->sub)
      (begin
       (csym::pthread-cond-signal (ptr thr->cond-r))
       (csym::pthread-cond-signal (ptr thr->cond)))
    )
  (csym::pthread-mutex-unlock (ptr thr->mut))
  (csym::send-command (ptr rcmd) 0 0))  ;rack����

;;; threads[id] ��treq���ߤ�
(def (csym::try-treq pcmd id from-addr)
    (csym::fn int (ptr (struct cmd)) unsigned-int (ptr (enum node)))
  (def hx (ptr (struct task-home)))
  (def thr (ptr (struct thread-data)))
  (def avail int 0)
  (def from-head (enum node) (aref from-addr 0))
  
  (= thr (+ threads id))
  (csym::pthread-mutex-lock (ptr thr->mut))
  (csym::pthread-mutex-lock (ptr thr->rack-mut))
  ;; rack�ξ��Ϥʤ�ɬ�ס�
  ;; ->rslt�����ä�ľ��˼���֤���treq���褿�Ȥ��ˡ��Ż���
  ;;   ʬ�䤷�Ƥ��ޤ�ʤ�����
  (if (and (== thr->w-rack 0)           ; rack�Ԥ��ʤ����
           (or ;;(and (!= PARENT from-head) ; �狼��ʤ�Ȥꤢ�������������Ԥ����Ƥ���
               ;;   (< id from-head))
               (and thr->task-top       ; �Ż���ʤ����̤˼���
                    (or (== thr->task-top->stat TASK-STARTED)
                        (== thr->task-top->stat TASK-INITIALIZED)))))
      (= avail 1))
  (csym::pthread-mutex-unlock (ptr thr->rack-mut))

  ;; �����ʤ�task-home�ʻŻ��η���Ԥ��� �����å���push
  (if avail
      (begin
       (= hx thr->treq-free)
       (if (not hx)
           (csym::mem-error "Not enough task-home memory"))
       (= thr->treq-free hx->next)      ; �ե꡼�ꥹ�Ȥ����ΰ�����
       (= hx->next thr->treq-top)       ; ������next�ϥ����å��Υ��
       (= hx->stat TASK-HOME-ALLOCATED)
       (csym::copy-address hx->task-head (aref pcmd->v 0)) ; v[0]: ������
                                        ; 4�Ǥؤ餷��
       (if (!= pcmd->node OUTSIDE)
           (= hx->req-from INSIDE)
         (= hx->req-from OUTSIDE))
       (= thr->treq-top hx)
       (= thr->req hx)                  ; ��req �� volatile, broadcast ����
       ))
  (csym::pthread-mutex-unlock (ptr thr->mut))
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
;; �� any ���Ф��� none���ޤ��� ����ᤷ(take bask)���Ф���none ��?
;;   ����ᤷ(take bask)���Ф���none�Ϥޤ�����Ф褤��
;;   treq �ʳ��� tbak ���ߤ���Ȥ褤���⤷��ʤ�����å������Ϥ��Τޤ�
;;   �Ȥ��Ƥ⡤trep_top �Ȥ��̴����Τ�ΤȤ���

;; �Ƥ���Υ�������
(def random-seed1 double 0.2403703)
(def random-seed2 double 3.638732)

;; 0--max-1�ޤǤ�����������֤�
(def (csym::my-random max pseed1 pseed2) (fn int int (ptr double) (ptr double))
  (= (mref pseed1) (+ (* (mref pseed1) 3.0) (mref pseed2)))
  (-= (mref pseed1) (cast int (mref pseed1)))
  (return (* max (mref pseed1))))

(def (csym::choose-treq from-node) (fn int (enum node))
  (cond
    ((<= 0 from-node)
     (let ((thr (ptr (struct thread-data)) (+ threads from-node)))
       ;; ��ά��������Ѥ���
       (= thr->last-choose (% (+ 1 thr->last-choose) NKIND-CHOOSE))
       (cond
         ((== CHS-RANDOM thr->last-choose) ; ������
          (return (csym::my-random num-thrs
                                   (ptr thr->random-seed1)
                                   (ptr thr->random-seed2))))
         ((== CHS-ORDER thr->last-choose) ; ���֤�
          (= thr->last-treq (% (+ 1 thr->last-treq) num-thrs))
          (return thr->last-treq))
         (else
          (return 0)))))
    ((== PARENT from-node)
     (return (csym::my-random num-thrs (ptr random-seed1) (ptr random-seed2))))
    (else
     (return 0))))

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
  (if (< pcmd->c 2)                     ; �����ο������å� 0:from, 1:to ; 4�Ǥؤ餷��
      (csym::proto-error "Wrong treq" pcmd))
  ;; �Ż����׵᤹�륹��åɤ���ơ��׵��Ф�
  (if (== (aref pcmd->v 1 0) ANY)       ; 4�Ǥؤ餷��
      (let ((myid int) (start-id int) (d int))
        (= myid (aref pcmd->v 0 0))     ; �׵ḵ ; 4�Ǥؤ餷��
        (= start-id (csym::choose-treq myid))
        (for ((= d 0) (< d num-thrs) (inc d))
           (= id (% (+ d start-id) num-thrs)) ; �׵���id
          (if (and (!= pcmd->node OUTSIDE)
                   (== id myid))
              (continue))               ; ��ʬ���Ȥˤ��׵��Ф��ʤ�
          (if (csym::try-treq pcmd id (aref pcmd->v 0)) (break)))
        (if (!= d num-thrs)             ; treq�Ǥ���
            (return)))
    (begin                              ; "any"�Ǥʤ����ʼ���֤���
     (= id (aref pcmd->v 1 0))          ; 4�Ǥؤ餷��
     (if (not (< id num-thrs))
         (csym::proto-error "Wrong task-head" pcmd))
     ;; ��rslt ���֤�����ʳ��ϡ�����ᤷ���Ф��� none �����餺
     ;; �ޤ����Ƥ����Ф褤�Τǡ��׽���
     ;; ��try-treq����rack�Ԥ��Ǥʤ����1���֤��褦�ˤʤäƤ�Τ������
     (if (csym::try-treq pcmd id (aref pcmd->v 0)) ; treq�Ǥ���
         (return))))
  
  ;; ���������treq any�˻Ż����֤��ʤ��ä����
  (if (== pcmd->node ANY)
      (if (== (aref pcmd->v 0 0) 0)     ; v[0]:from ; 4�Ǥؤ餷��
          ;; 0��worker�����treq�ξ��ϳ������䤤��碌��
          (begin
           (= pcmd->node OUTSIDE)
           (csym::send-command pcmd 0 0)
           (return))
        ;; ����ʳ���worker�ˤ�ñ��none���֤�
        (= pcmd->node INSIDE)))

  ;; none���֤�
  (= rcmd.c 1)                          ; 4�Ǥؤ餷��
  (= rcmd.node pcmd->node)              ; [INSIDE|OUTSIDE]
  (= rcmd.w NONE)
  (csym::copy-address (aref rcmd.v 0) (aref pcmd->v 0)) ; 4�Ǥؤ餷��
  (csym::send-command (ptr rcmd) 0 0))

;; rack <rack������header(�����Ǥ�thread-id)>
(def (csym::recv-rack pcmd) (csym::fn void (ptr (struct cmd)))
  (def tx (ptr (struct task)))
  (def thr (ptr (struct thread-data)))
  (def id unsigned-int)
  (def len size-t)
  (if (< pcmd->c 1)                     ; 4�Ǥؤ餷��
      (csym::proto-error "Wrong rack" pcmd))
  ;; id��<task-head>�˴ޤ��
  (= id (aref pcmd->v 0 0))             ; 4�Ǥؤ餷��
  (if (not (< id num-thrs))
      (csym::proto-error "Wrong task-head" pcmd))
  (= thr (+ threads id))
  (csym::pthread-mutex-lock (ptr thr->rack-mut))
  (dec thr->w-rack)
  (csym::pthread-mutex-unlock (ptr thr->rack-mut)))

;; �������åɤ��Ż�ʬ�䳫�ϻ��˸Ƥ�
(def (handle-req -bk -thr)
    (fn void (ptr (NESTFN int void)) (ptr (struct thread-data)))
  (csym::pthread-mutex-lock (ptr -thr->mut))
  (if -thr->req
      (begin
       (-bk)
       (= -thr->req -thr->treq-top)))
  (csym::pthread-mutex-unlock (ptr -thr->mut)))

;; �������åɤ�put���˸Ƥ�
;; -thr->mut ��å��Ѥ�
(def (csym::make-and-send-task thr task-no body) ; task-no��tcell�ɲ�
    (csym::fn void (ptr (struct thread-data)) int (ptr void))
  (def tcmd (struct cmd))
  (def hx (ptr (struct task-home)) thr->treq-top)
  (= thr->treq-top hx->next)            ; task�׵᥹���å���pop
  (= hx->next thr->sub)                 ; �����ꥵ�֥�����stack�Υ��
  (= thr->sub hx)                       ; ���֥�����stack��push
  (= hx->task-no task-no)
  (= hx->body body)
  (= hx->id (if-exp hx->next            ; ���֥�����ID = �줫�鲿���ܤ�
                    (+  hx->next->id 1)
                    0))
  (= hx->stat TASK-HOME-INITIALIZED)
  (= tcmd.c 4)                          ; 4�Ǥؤ餷��
  (= tcmd.node hx->req-from)
  (= tcmd.w TASK)
  ;; �ʲ���4�Ǥؤ餷��
  (= (aref tcmd.v 0 0) (++ thr->ndiv))  ; ʬ����
  (= (aref tcmd.v 0 1) TERM)
  (= (aref tcmd.v 1 0) thr->id)         ; ��������� "<worker-id>:<subtask-id>"
  (= (aref tcmd.v 1 1) hx->id)
  (= (aref tcmd.v 1 2) TERM)
  (csym::copy-address (aref tcmd.v 2) hx->task-head) ; �������������try-treq�ǵ����ѡ�
  (= (aref tcmd.v 3 0) task-no)         ; �������ֹ�
  (= (aref tcmd.v 3 1) TERM)
  (csym::send-command (ptr tcmd) body task-no))

;; �������åɤ����֥������η���Ԥ���碌���˸Ƥ�
(def (wait-rslt thr) (fn (ptr void) (ptr (struct thread-data)))
  (def body (ptr void))
  (def t1 (struct timespec))
  (def now (struct timeval))
  (def nsec long)
  (def sub (ptr (struct task-home)))
  (csym::pthread-mutex-lock (ptr thr->mut))
  (= sub thr->sub)                      ; ����åɤΥ��֥������֤���
  (while (!= sub->stat TASK-HOME-DONE)
    ;; �� �ʲ���ԥ����ȤˤʤäƤ���
    ;; -> ���Ф餯rslt���ʤ���� recv-exec-send��flush����Τ����ס�
    ;; (csym::flush-treq-with-none thr)
    ;; (= (fref thr -> task-top -> stat) TASK-SUSPENDED)
    ;; �����Ρ��ɤ����ä��Ż��ʤ����ä��Ԥ�
    (if (== PARENT (aref sub->task-head 0))
        (begin
         (csym::gettimeofday (ptr now) 0)
         (= nsec (* 1000 (fref now tv-usec)))
         (+= nsec 1000);(* 5 1000 1000))      ; ����ä��ԤäƤ���ʤ����ο������Ԥ����֡�
         ;; (+= nsec (* 10 1000 1000))
         (= (fref t1 tv-nsec) (if-exp (> nsec 999999999)
                                      (- nsec 999999999)
                                      nsec))
         (= (fref t1 tv-sec) (+ (fref now tv-sec)
                                (if-exp (> nsec 999999999) 1 0)))
         (csym::pthread-cond-timedwait (ptr thr->cond-r) (ptr thr->mut)
                                       (ptr t1))
         ))
    ;; �� �ʲ���ԥ����ȤˤʤäƤ���
    (= thr->task-top->stat TASK-STARTED)
    (if (== sub->stat TASK-HOME-DONE)
        (break))
    ;; fprintf(stderr, "sub %d\n", sub);
    ;; ���Ф餯�ޤäƤ⤢���󤫤ä���Ż���Ȥ��֤��˹Ԥ�
    (recv-exec-send thr sub->task-head sub->req-from))
  (= body sub->body)
  (= thr->sub sub->next)                ; ���֥�����stack��pop
  (= sub->next thr->treq-free)          ; pop������ʬ��...
  (= thr->treq-free sub)                ; ...�ե꡼�ꥹ�Ȥ��֤�
  ;; fprintf(stderr, "nsub %d\n", thr->sub);
  (csym::pthread-mutex-unlock (ptr thr->mut))
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

  ;; send-mut�ʳ���������å��ˤν����
  (csym::pthread-mutex-init (ptr send-mut) 0)

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
      (= (fref thr -> last-treq) i)
      (= (fref thr -> last-choose) CHS-RANDOM)
      (let ((r double) (q double))
        (= r (csym::sqrt (+ 0.5 i)))
        (= q (csym::sqrt (+ r i)))
        (-= r (cast int r))
        (= (fref thr -> random-seed1) r)
        (= (fref thr -> random-seed2) q))
      (csym::pthread-mutex-init (ptr (fref thr -> mut)) 0)
      (csym::pthread-mutex-init (ptr (fref thr -> rack-mut)) 0)
      (csym::pthread-cond-init (ptr (fref thr -> cond)) 0)
      (csym::pthread-cond-init (ptr (fref thr -> cond-r)) 0)

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
  (while 1
    (= pcmd (csym::read-command))
    (csym::proc-cmd pcmd 0))
  (csym::exit 0))
