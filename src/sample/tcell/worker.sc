;;; Copyright (c) 2008 Tasuku Hiraishi <hiraisi@kuis.kyoto-u.ac.jp>
;;; All rights reserved.

;;; Redistribution and use in source and binary forms, with or without
;;; modification, are permitted provided that the following conditions
;;; are met:
;;; 1. Redistributions of source code must retain the above copyright
;;;    notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright
;;;    notice, this list of conditions and the following disclaimer in the
;;;    documentation and/or other materials provided with the distribution.

;;; THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
;;; ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;;; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
;;; ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
;;; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
;;; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
;;; OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
;;; HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
;;; LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
;;; OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
;;; SUCH DAMAGE.


;;;; Tascell worker
(%include "rule/nestfunc-setrule.sh")

(c-exp "#include<stdio.h>")
(c-exp "#include<stdlib.h>")
(c-exp "#include<string.h>")
(c-exp "#include<math.h>")
(c-exp "#include<pthread.h>")
(c-exp "#include<sys/time.h>")
(c-exp "#include<getopt.h>")
#+tcell-gtk (c-exp "#include<gtk/gtk.h>")
(%include "worker.sh")


;;; Command-line options
(%defconstant HOSTNAME-MAXSIZE 256)
(def (struct runtime-option)
  (def num-thrs int)                    ; worker��
  (def sv-hostname (array char HOSTNAME-MAXSIZE))
                                        ; Tascell�����ФΥۥ���̾��""�ʤ�stdout
  (def port unsigned-short)             ; Tascell�����Фؤ���³�ݡ����ֹ�
  (def speculative int)                 ; �굡Ū�˳�����treq
  )
(static option (struct runtime-option))



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

(def threads (array (struct thread-data) 64))
(def num-thrs unsigned-int)

;;; recv-exec-send ����� wait-rslt ����
;;; treq�ʰ�ötreq������դ�����̤������� �����ޤäƤ�����
;;; �������˴����������� none�����롥
;;; thr->mut �� send_mut �� lock ��
(def (csym::flush-treq-with-none thr) (csym::fn void (ptr (struct thread-data)))
  (def rcmd (struct cmd))
  (def hx (ptr (struct task-home)))
  (= rcmd.c 1)
  (= rcmd.w NONE)
  (while (= hx thr->treq-top)
    (= rcmd.node hx->req-from)          ; ����or����
    (csym::copy-address (aref rcmd.v 0) hx->task-head)
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
  (= rcmd.c 2)
  ;; req-to �ϼ���֤��Ǥ���м���֤��衤�����Ǥʤ����ANY
  (= rcmd.node (if-exp (> num-thrs 1) req-to OUTSIDE))
  (= rcmd.w TREQ)
  (= (aref rcmd.v 0 0) thr->id)
  (= (aref rcmd.v 0 1) TERM)
  (csym::copy-address (aref rcmd.v 1) treq-head)
  (do-while (!= (fref tx -> stat) TASK-INITIALIZED)
    ;; �ǽ��treq�����ޤäƤ����顤none������
    ;; �������� treq �����Ӹ򤦤��⤷��ʤ�����
    ;; none ������ʤ��Ȥ�������ȡ�
    ;; �ߤ��� none or task �������Ƥ���Τ��Ԥ�
    ;; �Ȥ��������פΥǥåɥ�å��Ȥʤ롥
    (csym::flush-treq-with-none thr)
    (= tx->stat TASK-ALLOCATED)         ; *-NONE�ˤ���Ƥ뤫�⤷��ʤ��Τ򸵤��᤹
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
          (begin
           (if (!= tx->stat TASK-NONE)
               (inc thr->w-none))
           (goto Lnone)))
      (if (!= tx->stat TASK-ALLOCATED)
          (break))
      (csym::pthread-cond-wait (ptr thr->cond) (ptr thr->mut))
      )
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
               (if (> delay (* 40 1000 1000))
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

  ;; �����ǡ�stat��TASK-INITIALIZED
  ;; �������饿��������
  (= tx->stat TASK-STARTED)
  (= old-ndiv thr->ndiv)
  (= thr->ndiv tx->ndiv)
  (csym::pthread-mutex-unlock (ptr thr->mut))

  ((aref task-doers tx->task-no) thr tx->body) ; �������¹�

  ;; task�ν�����λ��ϡ�����task-home��send-rslt����
  (= rcmd.w RSLT)
  (= rcmd.c 1)
  (= rcmd.node tx->rslt-to)             ; ����or����
  (csym::copy-address (aref rcmd.v 0) tx->rslt-head)
  (csym::send-command (ptr rcmd) tx->body tx->task-no)

  ;; �����Ǥ�treq�����ޤäƤ����� none������
  (csym::flush-treq-with-none thr)
  (csym::pthread-mutex-lock (ptr thr->rack-mut))
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
  (if (< pcmd->c 4)
      (csym::proto-error "wrong-task" pcmd))
  ;; ��������Υ�å������ξ�硤���ޥ�ɤ�³��task���Τ�������
  ;; ����������ξ��ϰ�����Ϳ�����Ƥ����
  (= task-no (aref pcmd->v 3 0))
  (if (== pcmd->node OUTSIDE)           ; ���������task�ξ��Ϥ�����body��������
      (begin
       (= body ((aref task-receivers task-no)))
       (csym::read-to-eol)))
  ;; <task-head>�򸫤ơ���������¹Ԥ��륹��åɤ���롥
  (= id (aref pcmd->v 2 0))
  (if (not (< id num-thrs))
      (csym::proto-error "wrong task-head" pcmd))
  (= thr (+ threads id))                ; thr: task��¹Ԥ��륹��å�

  ;; ����åɤ˼¹Ԥ��٤����������ɲä���
  (csym::pthread-mutex-lock (ptr thr->mut))
  (= tx thr->task-top)                  ; tx: thr�����٤��Ż��ꥹ��
  (= tx->rslt-to pcmd->node)            ; ��̤���������� [INSIDE|OUTSIDE]
  (csym::copy-address tx->rslt-head (aref pcmd->v 1))
                                        ; [1]: ���긵���̤�������
  (= tx->ndiv (aref pcmd->v 0 0))       ; [0]: ʬ����
  (= tx->task-no task-no)               ; �������ֹ�
  ;; �������Υѥ�᡼����task specific�ʹ�¤�Ρˤμ������
  (= tx->body body)
  (= tx->stat TASK-INITIALIZED)
  (csym::pthread-mutex-unlock (ptr thr->mut))

  ;; �Ż��Ԥ���̲�äƤ������򵯤���
  (csym::pthread-cond-signal (ptr thr->cond))
  )

(def (csym::recv-none pcmd) (csym::fn void (ptr (struct cmd)))
  (def thr (ptr (struct thread-data)))
  (def id unsigned-int)
  (def len size-t)
  (if (< pcmd->c 1)
      (csym::proto-error "Wrong none" pcmd))
  (= id (aref pcmd->v 0 0))
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

(def (csym::recv-rslt pcmd body) (csym::fn void (ptr (struct cmd)) (ptr void))
  (def rcmd (struct cmd))               ; rack���ޥ��
  (def thr (ptr (struct thread-data)))
  (def hx (ptr (struct task-home)))
  (def tid unsigned-int)
  (def sid unsigned-int)
  ;; �����ο������å�
  (if (< pcmd->c 1)
      (csym::proto-error "Wrong rslt" pcmd))
  ;; ��̼���ͷ��� "<thread-id>:<task-home-id>"
  (= tid (aref pcmd->v 0 0))
  (if (not (< tid num-thrs))
      (csym::proto-error "wrong rslt-head" pcmd))
  (= sid (aref pcmd->v 0 1))
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
    ((aref rslt-receivers hx->task-no) hx->body)
    (csym::read-to-eol))
   ((== pcmd->node INSIDE)
    (= hx->body body))
   (else
    (csym::proto-error "Wrong cmd.node" pcmd)))
  ;; rack���֤�����äȸ�Τۤ����褤��
  (= rcmd.c 1)
  (= rcmd.node pcmd->node)
  (= rcmd.w RACK)
  (csym::copy-address (aref rcmd.v 0) hx->task-head)
                                        ; �����补rslt�ǤϤʤ�����Ȥ�task���ޥ�ɤΤ�Ф��Ƥ���
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
       (if (!= pcmd->node OUTSIDE)
           (= hx->req-from INSIDE)
         (= hx->req-from OUTSIDE))
       (= thr->treq-top hx)
       (= thr->req hx)
       ))
  (csym::pthread-mutex-unlock (ptr thr->mut))
  (return avail))

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
  (def rcmd (struct cmd))
  (def id unsigned-int)
  (if (< pcmd->c 2)                     ; �����ο������å� 0:from, 1:to
      (csym::proto-error "Wrong treq" pcmd))
  ;; �Ż����׵᤹�륹��åɤ���ơ��׵��Ф�
  (if (== (aref pcmd->v 1 0) ANY)
      (let ((myid int) (start-id int) (d int))
        (= myid (aref pcmd->v 0 0))     ; �׵ḵ
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
     (= id (aref pcmd->v 1 0))
     (if (not (< id num-thrs))
         (csym::proto-error "Wrong task-head" pcmd))
     (if (csym::try-treq pcmd id (aref pcmd->v 0)) ; treq�Ǥ���
         (return))))
  
  ;; ���������treq any�˻Ż����֤��ʤ��ä����
  (if (== pcmd->node ANY)
      (if (== (aref pcmd->v 0 0) 0)     ; v[0]:from
          ;; 0��worker�����treq�ξ��ϳ������䤤��碌��
          (begin
           (= pcmd->node OUTSIDE)
           (csym::send-command pcmd 0 0)
           (return))
        ;; ����ʳ���worker�ˤ�ñ��none���֤�
        (= pcmd->node INSIDE)))

  ;; none���֤�
  (= rcmd.c 1)
  (= rcmd.node pcmd->node)              ; [INSIDE|OUTSIDE]
  (= rcmd.w NONE)
  (csym::copy-address (aref rcmd.v 0) (aref pcmd->v 0))
  (csym::send-command (ptr rcmd) 0 0))

;; rack <rack������header(�����Ǥ�thread-id)>
(def (csym::recv-rack pcmd) (csym::fn void (ptr (struct cmd)))
  (def tx (ptr (struct task)))
  (def thr (ptr (struct thread-data)))
  (def id unsigned-int)
  (def len size-t)
  (if (< pcmd->c 1)
      (csym::proto-error "Wrong rack" pcmd))
  ;; id��<task-head>�˴ޤ��
  (= id (aref pcmd->v 0 0))
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
  (= tcmd.c 4)
  (= tcmd.node hx->req-from)
  (= tcmd.w TASK)

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
    (= thr->task-top->stat TASK-STARTED)
    (if (== sub->stat TASK-HOME-DONE)
        (break))
    (recv-exec-send thr sub->task-head sub->req-from))
  (= body sub->body)
  (= thr->sub sub->next)                ; ���֥�����stack��pop
  (= sub->next thr->treq-free)          ; pop������ʬ��...
  (= thr->treq-free sub)                ; ...�ե꡼�ꥹ�Ȥ��֤�
  (csym::pthread-mutex-unlock (ptr thr->mut))
  (return body))


;;; Handling command-line options
(def (csym::usage argc argv) (csym::fn void int (ptr (ptr char)))
  (csym::fprintf stderr
                 "Usage: %s [-s hostname] [-p port-num] [-n n-threads] [-S]~%"
                 (aref argv 0))
  (csym::exit 1))

(def (set-option argc argv) (csym::fn void int (ptr (ptr char)))
  (def i int) (def ch int)
  ;; Default values
  (= option.num-thrs 1)
  (= (aref option.sv-hostname 0) #\NULL)
  (= option.port 8888)
  (= option.speculative 0)
  ;; Parse and set options
  (while (!= -1 (= ch (csym::getopt argc argv "n:s:p:S")))
    (for ((= i 0) (< i argc) (inc i))
      (switch ch
        (case #\n)                      ; number of threads
        (= option.num-thrs (csym::atoi optarg))
        (break)
        
        (case #\s)                      ; server name
        (if (csym::strcmp "stdout" optarg)
            (begin
             (csym::strncpy option.sv-hostname optarg
                            HOSTNAME-MAXSIZE)
             (= (aref option.sv-hostname (- HOSTNAME-MAXSIZE 1)) 0))
          (= (aref option.sv-hostname 0) #\NULL))
        (break)
        
        (case #\p)                      ; connection port number
        (= option.port (csym::atoi optarg))
        (break)

        (case #\S)                      ; turn-on speculative task receipt from external nodes
        (= option.speculative 1)
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


;; �ʲ� tcell-gtk�Ȥ���Τϥǥ��Ѥ��ɲå����ɡ��Դ�����
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
  (for ((= i 0) (< i num-thrs) (inc i))
    (let ((thr (ptr (struct thread-data)) (+ threads i)))
      (systhr-create worker thr)))

  ;; �ܥ���åɤ�OUTSIDE����Υ�å���������
  (while 1
    (= pcmd (csym::read-command))
    (csym::proc-cmd pcmd 0))
  (csym::exit 0))
