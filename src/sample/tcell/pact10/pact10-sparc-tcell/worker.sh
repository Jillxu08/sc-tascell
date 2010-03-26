(%include "dprint.sh")

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(%defconstant BUFSIZE 1280)             ; ���ޥ�ɹԤ�ʸ�����κ���+1
(%defconstant MAXCMDC 4)                ; ���ޥ�ɤ�argument���κ���ʥ��ޥ��̾���ȴޤ��
(%defconstant ARG-SIZE-MAX 16)          ; ���ޥ�ɤγư����ε������Ĺ��
(%defconstant TASK-LIST-LENGTH 1024)    ; ����åɤ��Ȥ�TASK, TASK-HOME�ꥹ�Ȥ�Ĺ��
(%defconstant TASK-MAX 256)             ; �ץ�����ޤ�����Ǥ��륿�����κ����
(%defconstant DUMMY-SIZE 1000)          ; false-sharing�ɻߤΤ����padding������

(%defconstant DELAY-MAX (* 1 1000 1000 1000))          ; none��->treq�������ޤǤλ��֤ξ�� [nsec]
; (%defconstant BUSYWAIT)                 ; �����treq���ֻ���busywait���ԤĤʤ�uncomment

(%defconstant clsc)

(%cinclude "sock.h" (:macro))           ; �̿��ط�

;; 0�ʾ�ο���thread-id����������Τǡ�enum id�ˤ���ο��������Ƥ�
(def (enum node) (OUTSIDE -1) (INSIDE -2) (ANY -3) (PARENT -4) (FORWARD -5) (TERM -99))
(def (enum command)
    TASK RSLT TREQ NONE BACK RACK DREQ DATA
    STAT VERB EXIT WRNG)
(extern-decl cmd-strings (array (ptr char))) ; �����б�����ʸ����cmd-serial.sc�������

;; treq any �����������ˡ
(def (enum choose) CHS-RANDOM CHS-ORDER)
(%defconstant NKIND-CHOOSE 2)           ; choose�μ����

;; ����֤Ǥ��Ȥꤹ�륳�ޥ��
;; task, rslt�����Τ�cmd-list�ˤ���
(def (struct cmd)
  (def w (enum command))                ; ���ޥ�ɤμ���
  (def c int)                           ; ���ޥ�ɤ�argument���ʥ��ޥ��̾���Ȥ�ޤ��
  (def node (enum node))                ; �ɤ��������å������� INSIDE|OUTSIDE|ANY
  (def v (array (enum node) MAXCMDC ARG-SIZE-MAX)) ; v[i]: i-th argument of the command
                                        ; TERM�Ǥ����[enum���|0�ʾ������]������
  )

(def (struct cmd-list)
  (def cmd (struct cmd))
  (def body (ptr void))              ; task, rslt�����Ρʥ����������¸�ι�¤�Ρ�
  (def task-no int)                  ; task�ֹ��body�������ؿ�����ꤹ�뤿���
  (def next (ptr (struct cmd-list)))
  )

(decl (struct task))
(decl (struct thread-data))

;;; �� do_task_body �ʳ��ϡ�thread_data ���������פǤ�?
;;; Tascell�ץ������¦�ǡʼ�ư���������task object��sender/receiver
(decl task-doers
      (array (ptr (fn void (ptr (struct thread-data)) (ptr void))) TASK-MAX))
(decl task-senders
      (array (ptr (csym::fn void (ptr void))) TASK-MAX))
(decl task-receivers
      (array (ptr (csym::fn (ptr void))) TASK-MAX))
(decl rslt-senders
      (array (ptr (csym::fn void (ptr void))) TASK-MAX))
(decl rslt-receivers
      (array (ptr (csym::fn void (ptr void))) TASK-MAX))

;;; Tascell�ץ������¦����������׵�������ǡ�����allocator/sender/receiver��
;; �����ϥǡ����Υ�������data-flag�ο�
;; -init-data �� data-flags�ν������˸ƽФ�
(decl (csym::data-allocate) (csym::fn void int))
;; ����������������������ǡ������ϰϡ�data-flags��ź�����б�����������
(decl (csym::data-send) (csym::fn void int int))
(decl (csym::data-receive) (csym::fn void int int))

;;; worker local storage �ι�¤�Τ���ӽ�����ؿ�������ϥ桼���ץ������ǡ�
(decl (struct worker-data))
(decl (csym::worker-init) (csym::fn void (ptr (struct thread-data))))

;;; Tascell�ץ�����ޤ��󶡤��뵡ǽ (worker.sc�����)
;;; ��Tascell�ǤϺǽ�� 'csym::-' �������̾����request-data����Ƭ����thr��tcell.rule���ɲá�
;; �ǡ����ΰ�γ��ݤ���ӥե饰�������ʰ����ϥǡ����Υ�������data-flag�ο���
;; ʣ����ƽФ��Ƥ���٤����¹Ԥ���ʤ�
(decl (csym::-setup-data) (csym::fn void int))
;; �ƥ������˻��ꤵ�줿�ϰϤΥǡ�����dreq��ȯ�Ԥ���
(decl (csym::-request-data) (csym::fn void (ptr (struct thread-data)) int int))
;; ���ꤵ�줿�ϰϤΥǡ�����·���ޤ��Ԥ�
(decl (csym::-wait-data) (csym::fn void int int))
;; ���ꤵ�줿�ϰϤ�data-flags��DATA-EXIST�ˤ���ʻŻ����ϥΡ����ѡ�
(decl (csym::-set-exist-flag) (csym::fn void int int))

(def (enum task-stat)
  TASK-ALLOCATED   ; �ΰ�Τߡ�̤�����
  TASK-INITIALIZED ; ���åȤ���Ƥ���
  TASK-STARTED     ; �¹Գ��Ϻ�
  TASK-DONE        ; ��λ�Ѥ�
  TASK-NONE        ; ALLOC��treq������none���֤äƤ���
  TASK-SUSPENDED)  ; �Ԥ�����
;; -(treq����)-> ALLOCATED --(task�������)--> INITIALIZED --> STARTED --> DONE -->
;;                 |  |none�������                               |
;;           ��ĩ��|  |                                           |
;;                 NONE                                        SUSPENDED

(def (enum task-home-stat)
  TASK-HOME-ALLOCATED    ; �ΰ�Τߡ�̤�����
  TASK-HOME-INITIALIZED  ; ���åȤ���Ƥ���
  ;;  ����Ԥ��ϡ� task�Τۤ��Ǥ狼�롩
  TASK-HOME-DONE)        ; ��̤���ޤäƤ���

;; �׻�����worker�ˤޤǰ�ư���Ƥ���������
(def (struct task)
  (def stat (enum task-stat))       ; �������ξ���
  (def next (ptr (struct task)))    ; �������ꥹ��......
  (def prev (ptr (struct task)))    ; ..........�Υ��
  (def task-no int)                 ; �¹Ԥ��륿�����ֹ��tcell�ɲá�
  (def body (ptr void))             ; task object��¤�ΤؤΥݥ���
  (def ndiv int)                    ; ����ʬ�����ƤǤ��������� (task cell)��
  (def rslt-to (enum node))         ; ���������μ��̡��̥Ρ��� or �Ρ����� or any��
  (def rslt-head (array (enum node) ARG-SIZE-MAX))) ; ��������襢�ɥ쥹

;; Worker����ä����֥�������������
(def (struct task-home)
  (def stat (enum task-home-stat))      ; ����
  (def id int)                          ; ��������˳����Ƥ���ID�ʳƥ���ǰ�ա�
  (def owner (ptr (struct task)))       ; ���Υ��֥�������spawn����������
  (def task-no int)                     ; �¹Ԥ��륿�����ֹ��tcell�ɲá�
  (def req-from (enum node))            ; �Ż�������μ��̡��̥Ρ��� or �Ρ����� or any��
  (def next (ptr (struct task-home)))   ; ��󥯡ʼ��ζ������� or �����å���1������
  (def body (ptr void))                 ; task-data��¤�ΤؤΥݥ���
  (def task-head (array (enum node) ARG-SIZE-MAX))
                                        ; ��������������ʼ���֤��衤rack���������
  )

(def (struct thread-data)
  (def req (ptr (struct task-home)))    ; �Ż����׵᤬��Ƥ��뤫��
  (def id int)                          ; ��������˳����Ƥ���ID
  (def w-rack int)                      ; ��rslt���������ơ�rack�Ԥ��ο�
  (def w-none int)                      ; none�Ԥ��ο�
  (def ndiv int)                        ; ����äƤ�Ż���ʬ�䤵�줿���
  (def last-treq int)                   ; �����ؤ�treq any�ǡ��Ǹ��treq�������
  (def last-choose (enum choose))       ; �����ؤ�treq any�ǡ��Ǹ�˺��Ѥ���������ˡ
  (def random-seed1 double)             ; ����μ� treq any�ǻ��ѡ�����
  (def random-seed2 double)             ; ����μ� treq any�ǻ��ѡ�����
  (def task-free (ptr (struct task)))   ; �����������ѥե꡼�ꥹ��
  (def task-top (ptr (struct task)))    ; ����åɤ�Ϳ����줿�Ż��Υꥹ��
  ;; treq-free����Ϥޤ�ե꡼�ꥹ�Ȥ���
  ;; 2�ĤΥ����å���treq-top�����å���sub�����å��ˤ���ݤ��Ƥ���
  (def treq-free (ptr (struct task-home))) ; �����������Ԥ������å������ѥե꡼�ꥹ��
  (def treq-top (ptr (struct task-home))) ; �嵭�����å��Υȥå�
  (def sub (ptr (struct task-home)))    ; ���֥�����(initialized)�����å���
  (def mut pthread-mutex-t)             ; mutex
  (def rack-mut pthread-mutex-t)        ; rack mutex
  (def cond pthread-cond-t)             ; task, none�Ԥ���̲�餻��Ȥ��ξ���ѿ�
  (def cond-r pthread-cond-t)           ; rslt�Ԥ���̲�餻��Ȥ��ξ���ѿ�
  (def wdptr (ptr void))                ; worker local storage��¤�Ρʥ桼��������ˤؤΥݥ���
  (def dummy (array char DUMMY-SIZE)) ; false sharing�ɻߤ�padding
  )

;;;; ɬ�׻��ǡ����׵��Ϣ

;; ¸�ߥե饰�μ���
(def (enum DATA-FLAG) DATA-NONE DATA-REQUESTING DATA-EXIST)

;; dreq�����ؿ����Ϥ�����
(def (struct dhandler-arg)
  (def data-to (enum node))                   ; �ǡ�����requester (INSIDE|OUTSIDE)
  (def head (array (enum node) ARG-SIZE-MAX)) ; �ǡ�����requester
  (def dreq-cmd (struct cmd))           ; ����˿Ƥ�dreq���ꤲ��ݤο��� (for DATA-NONE)
  (def dreq-cmd-fwd (struct cmd))       ; ����˿Ƥ�dreq���ꤲ��ݤο��� (for DATA-REQUESTING)
  (def start int)                       ; �ǡ������׵��ϰ�
  (def end int)                         ; �ǡ������׵��ϰ�
  )


;;;; worker.sc �δؿ��ץ��ȥ��������
(decl (csym::make-and-send-task thr task-no body)
      (csym::fn void (ptr (struct thread-data)) int (ptr void)))
(decl (wait-rslt thr) (fn (ptr void) (ptr (struct thread-data))))

(decl (csym::proto-error str pcmd) (csym::fn void (ptr (const char)) (ptr (struct cmd))))
(decl (csym::read-to-eol) (csym::fn void void))

(decl (csym::init-data-flag) (csym::fn void int))

(decl (csym::recv-rslt) (csym::fn void (ptr (struct cmd)) (ptr void)))
(decl (csym::recv-task) (csym::fn void (ptr (struct cmd)) (ptr void)))
(decl (csym::recv-treq) (csym::fn void (ptr (struct cmd))))
(decl (csym::recv-rack) (csym::fn void (ptr (struct cmd))))
(decl (csym::recv-dreq) (csym::fn void (ptr (struct cmd))))
(decl (csym::recv-data) (csym::fn void (ptr (struct cmd))))
(decl (csym::recv-none) (csym::fn void (ptr (struct cmd))))
(decl (csym::recv-back) (csym::fn void (ptr (struct cmd))))
(decl (csym::print-task-list task-top name) (csym::fn void (ptr (struct task)) (ptr char)))
(decl (csym::print-task-home-list treq-top name) (csym::fn void (ptr (struct task-home)) (ptr char)))
(decl (csym::print-thread-status thr) (csym::fn void (ptr (struct thread-data))))
(decl (csym::print-status) (csym::fn void (ptr (struct cmd))))
(decl (csym::set-verbose-level) (csym::fn void (ptr (struct cmd))))
(decl (csym::recv-exit) (csym::fn void (ptr (struct cmd))))

;;;; cmd-serial.sc �δؿ��ץ��ȥ��������
(decl (csym::serialize-cmdname buf w) (fn int (ptr char) (enum command)))
(decl (csym::deserialize-cmdname buf str) (fn int (ptr (enum command)) (ptr char)))
(decl (csym::serialize-arg buf arg) (fn int (ptr char) (ptr (enum node))))
(decl (csym::deserialize-node str) (fn (enum node) (ptr char)))
(decl (csym::deserialize-arg buf str) (fn int (ptr (enum node)) (ptr char)))
(decl (csym::serialize-cmd buf pcmd) (fn int (ptr char) (ptr (struct cmd))))
(decl (csym::deserialize-cmd pcmd str) (fn int (ptr (struct cmd)) (ptr char)))
(decl (csym::copy-address dst src) (fn int (ptr (enum node)) (ptr (enum node))))
(decl (csym::address-equal adr1 adr2) (fn int (ptr (enum node)) (ptr (enum node))))

;;;; Command line options
(%defconstant HOSTNAME-MAXSIZE 256)
(def (struct runtime-option)
  (def num-thrs int)                    ; worker��
  (def sv-hostname (array char HOSTNAME-MAXSIZE))
                                        ; Tascell�����ФΥۥ���̾��""�ʤ�stdout
  (def port unsigned-short)             ; Tascell�����Фؤ���³�ݡ����ֹ�
  (def initial-task (ptr char))         ; ��ưŪ�˺ǽ���������륿�����ѥ�᡼��
  (def auto-exit int)                   ; �����˺ǽ��rslt�����ä��鼫ư��λ
  (def affinity int)                    ; use sched_setaffinity
  (def prefetch int)                    ; �굡Ū�˳�����treq
  (def verbose int)                     ; verbose level
  )
(extern-decl option (struct runtime-option))