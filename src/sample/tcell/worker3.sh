(%defconstant BUFSIZE 1280)             ; ���ޥ�ɹԤ�ʸ�����κ���+1
(%defconstant MAXCMDC 5)                ; ���ޥ�ɤ�argument���κ���ʥ��ޥ��̾���ȴޤ��
(%defconstant CMD-QUEUE-LENGTH 512)     ; ���ޥ���������ƥ��塼��Ĺ��
(%defconstant ARG-SIZE-MAX 256)         ; ���ޥ�ɤγư����ε������Ĺ��
(%defconstant TASK-LIST-LENGTH 256)     ; ����åɤ��Ȥ�TASK, TASK-HOME�ꥹ�Ȥ�Ĺ��
(%defconstant TASK-MAX 256)             ; �ץ���ޤ�����Ǥ��륿�����κ����

(%defconstant clsc)

(%cinclude "sock.h" (:macro))           ; �̿��ط�

(def (enum node) OUTSIDE INSIDE ANY)

;; ����֤Ǥ��Ȥꤹ�륳�ޥ��
;; task, rslt�����Τ�cmd-list�ˤ���
(def (struct cmd)
  (def c int)                           ; ���ޥ�ɤ�argument���ʥ��ޥ��̾���Ȥ�ޤ��
  (def node (enum node))
  (def v (array (ptr char) MAXCMDC))    ; v[i]: i-th argument of the command
                                        ; ʸ������ΰ���̤˳���
  )

(def (struct cmd-list)
  (def cmd (struct cmd))
  (def body (ptr void))              ; task, rslt�����Ρʥ����������¸�ι�¤�Ρ�
  (def task-no int)                  ; task�ֹ��body�������ؿ�����ꤹ�뤿���
  (def next (ptr (struct cmd-list)))
  )

(decl (csym::read-to-eol) (csym::fn void void))

(decl (csym::recv-rslt) (csym::fn void (ptr (struct cmd)) (ptr void)))
(decl (csym::recv-task) (csym::fn void (ptr (struct cmd)) (ptr void)))
(decl (csym::recv-treq) (csym::fn void (ptr (struct cmd))))
(decl (csym::recv-rack) (csym::fn void (ptr (struct cmd))))
(decl (csym::recv-none) (csym::fn void (ptr (struct cmd))))

(decl (struct task))
(decl (struct thread-data))

;; (decl (do-task-body)
;;   (fn void (ptr (struct thread-data)) (ptr void)))
;; (decl (send-task-body)
;;   (fn void (ptr (struct thread-data)) (ptr void)))
;; (decl (csym::recv-task-body)
;;   (fn (ptr void) (ptr (struct thread-data))))
;; (decl (send-rslt-body)
;;   (fn void (ptr (struct thread-data)) (ptr void)))
;; (decl (recv-rslt-body)
;;   (fn void (ptr (struct thread-data)) (ptr void)))
;;; �� do_task_body �ʳ��ϡ�thread_data ���������פǤ�?
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

(def (enum task-stat)
  TASK-ALLOCATED   ; �ΰ�Τߡ�̤�����
  TASK-INITIALIZED ; ���åȤ���Ƥ���
  TASK-STARTED     ; �¹Գ��Ϻ�
  TASK-DONE        ; ��λ�Ѥ�
  TASK-NONE        ; ���äݡ� TASK-ALLOCATED�Ȥϰ㤦�Ρ�
  TASK-SUSPENDED)  ; �Ԥ�����

(def (enum task-home-stat)
  TASK-HOME-ALLOCATED    ; �ΰ�Τߡ�̤�����
  TASK-HOME-INITIALIZED  ; ���åȤ���Ƥ���
  ;;  ����Ԥ��ϡ� task�Τۤ��Ǥ狼�롩
  TASK-HOME-DONE)        ; ��̤���ޤäƤ���

;; �׻�����ץ��å��ˤޤǰ�ư���Ƥ���������
(def (struct task)
  (def stat (enum task-stat))       ; �������ξ���
  (def next (ptr (struct task)))    ;; �������ꥹ��
  (def prev (ptr (struct task)))    ;;
  (def task-no int)                 ; �¹Ԥ��륿�����ֹ��tcell�ɲá�
  (def body (ptr void))             ; task-data��¤�ΤؤΥݥ���
  (def ndiv int)                    ; ����ʬ�����ƤǤ��������� (task cell)��
  (def rslt-to (enum node))         ; ���������μ��̡��̥Ρ��� or �Ρ����� or any��
  (def rslt-head (array char 256))) ; ��������襢�ɥ쥹

;; ʬ�䤷�ƤǤ�������������ְ�ư��
(def (struct task-home)
  (def stat (enum task-home-stat))     ; ����
  (def id int)                         ; ��������˳����Ƥ���ID�ʳƷ׻��Ρ��ɤǰ�ա�
  (def task-no int)                    ; �¹Ԥ��륿�����ֹ��tcell�ɲá�
  (def req-from (enum node))           ; �Ż�������μ��̡��̥Ρ��� or �Ρ����� or any��
  (def next (ptr (struct task-home)))  ; ��󥯡ʼ��ζ������� or �����å���1������
  (def body (ptr void))                ; task-data��¤�ΤؤΥݥ���
  (def task-head (array char 256)))    ; ��������������ʼ���֤��衤rack���������

(def (struct thread-data)
  (def req (ptr (struct task-home)))    ; �Ż����׵᤬��Ƥ��뤫��
  (def id int)        ; ��������˳����Ƥ���ID
  (def w-rack int)    ; ��rslt���������ơ�rack�Ԥ��ο�
  (def w-none int)    ; none�Ԥ��ο�
  (def ndiv int)      ; �Ż���ʬ�䤵�줿���
  (def task-free (ptr (struct task)))      ; �����������ѥե꡼�ꥹ��
  (def task-top (ptr (struct task)))    ; ����åɤ�Ϳ����줿�Ż��Υꥹ��
  ;; treq-free����Ϥޤ�ե꡼�ꥹ�Ȥ���
  ;; treq-top�����å���sub�����å�����ݤ��Ƥ���
  (def treq-free (ptr (struct task-home))) ; �����������Ԥ������å������ѥե꡼�ꥹ��
  (def treq-top (ptr (struct task-home)))  ; �嵭�����å��Υȥå�
  (def sub (ptr (struct task-home)))       ; ���֥�����(initialized)�����å���
  (def mut pthread-mutex-t)           ; mutex
  (def rack-mut pthread-mutex-t)      ; rack mutex
  (def cond pthread-cond-t)           ; task, none�Ԥ���̲�餻��Ȥ��ξ���ѿ�
  (def cond-r pthread-cond-t)         ; rslt�Ԥ���̲�餻��Ȥ��ξ���ѿ�
  (def ndiv-buf (array char 32))
  (def tno-buf (array char 8))        ; task-no������ʸ����Хåե���tcell�ɲá�
  (def id-str (array char 32))        ; id ��ʸ����ɽ��
  (def buf (array char ARG-SIZE-MAX)))  ; thread local �Хåե�

(extern divisibility-flag int) ;�� �ä��Ƥ��ޤ�����
(decl (csym::send-divisible) (csym::fn void void))
(decl (csym::make-and-send-task thr task-no body)
      (csym::fn void (ptr (struct thread-data)) int (ptr void)))
(decl (wait-rslt thr) (fn (ptr void) (ptr (struct thread-data))))

;; C�Υץߥ�ߥƥ��ַ���Sender, Reciever
(decl (csym::send-int n) (csym::fn void int))
(decl (csym::recv-int) (csym::fn int void))
