(%defconstant BUFSIZE 1024)
(%defconstant MAXCMDC 5)
(%defconstant TASK-MAX 256)

(%defconstant FN lightweight)  ; fn or lightweight
(%defconstant clsc)

(def (enum node) OUTSIDE INSIDE ANY)

(def (struct cmd)
  (def c int)                           ; ���ޥ�ɤ�argument���ʥ��ޥ��̾���Ȥ�ޤ��
  (def node (enum node))
  (def v (array (ptr char) MAXCMDC))
  )

(def (struct cmd-list)
  (def cmd (struct cmd))
  (def body (ptr void))
  (def next (ptr (struct cmd-list)))
  )

(decl (csym::read-to-eol) (csym::fn void void))

(decl (csym::recv-rslt) (csym::fn void (struct cmd) (ptr void)))
(decl (csym::recv-task) (csym::fn void (struct cmd) (ptr void)))
(decl (csym::recv-treq) (csym::fn void (struct cmd)))
(decl (csym::recv-rack) (csym::fn void (struct cmd)))
(decl (csym::recv-none) (csym::fn void (struct cmd)))

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
(decl task-doers
      (array (ptr (fn void (ptr (struct thread-data)) (ptr void))) TASK-MAX))
(decl task-senders
      (array (ptr (csym::fn void (ptr (struct thread-data)) (ptr void))) TASK-MAX))
(decl task-receivers
      (array (ptr (csym::fn (ptr void) (ptr (struct thread-data)))) TASK-MAX))
(decl rslt-senders
      (array (ptr (csym::fn void (ptr (struct thread-data)) (ptr void))) TASK-MAX))
(decl rslt-receivers
      (array (ptr (csym::fn void (ptr (struct thread-data)) (ptr void))) TASK-MAX))

(def (enum task-stat)
  TASK-ALLOCATED TASK-INITIALIZED TASK-STARTED TASK-DONE
  TASK-NONE TASK-SUSPENDED)

(def (enum task-home-stat)
  TASK-HOME-ALLOCATED TASK-HOME-INITIALIZED
  ;;  ����Ԥ��ϡ� task�Τۤ��Ǥ狼�롩
  TASK-HOME-DONE)

;; �������ʼ¹�¦��
(def (struct task)
  (def stat (enum task-stat))       ; �������ξ���
  (def next (ptr (struct task)))    ;; �������ꥹ��
  (def prev (ptr (struct task)))    ;;
  (def task-no int)                 ; �¹Ԥ��륿�����ֹ��tcell�ɲá�
  (def body (ptr void))             ; task-data��¤�ΤؤΥݥ���
  (def ndiv int)                    ; ʬ����
  (def rslt-to (enum node))         ; ���������μ���
  (def rslt-head (array char 256))) ; ��������襢�ɥ쥹

;; �������ʺ���������Ԥ�¦��
(def (struct task-home)
  (def stat (enum task-home-stat))
  (def id int)                         ; ��������˳����Ƥ���ID�ʳƷ׻��Ρ��ɤǰ�ա�
  (def task-no int)                    ; �¹Ԥ��륿�����ֹ��tcell�ɲá�
  (def req-from (enum node))
  (def next (ptr (struct task-home)))
  (def body (ptr void))
  (def task-head (array char 256)))    ; ���������׵ᤷ�Ƥ�ͤΥ��ɥ쥹

(def (struct thread-data)
  (def req (volatile (ptr (struct task-home)))) ; �Ż����׵᤬��Ƥ��뤫��
  (def id int)        ; ��������˳����Ƥ���ID
  (def w-rack int)    ; ��rslt���������ơ�rack�Ԥ��ο�
  (def w-none int)    ; 
  (def ndiv int)      ; �Ż���ʬ�䤵�줿���
  (def task-free (ptr (struct task)))      ; �����������ѥե꡼�ꥹ��
  (def task-top (ptr (struct task)))       ; ����åɤ�Ϳ����줿�Ż��Υꥹ��
  (def treq-free (ptr (struct task-home))) ; �����������Ԥ������å������ѥե꡼�ꥹ��
  (def treq-top (ptr (struct task-home)))  ; �嵭�����å��Υȥå�
  (def sub (ptr (struct task-home)))       ; ����Ԥ��Υ��������󥹥��å��Υȥå�
  (def mut pthread-mutex-t)
  (def rack-mut pthread-mutex-t)
  (def cond pthread-cond-t)           ; task, none�Ԥ���̲�餻��Ȥ��ξ���ѿ�
  (def cond-r pthread-cond-t)         ; rslt�Ԥ���̲�餻��Ȥ��ξ���ѿ�
  (def ndiv-buf (array char 32))
  (def tno-buf (array char 8))        ; task-no������ʸ����Хåե���tcell�ɲá�
  (def id-str (array char 32))
  (def buf (array char BUFSIZE)))

(extern divisibility-flag int)
(decl (csym::send-divisible) (csym::fn void void))
(decl (csym::make-and-send-task thr task-no body)
      (csym::fn void (ptr (struct thread-data)) int (ptr void)))
(decl (wait-rslt thr) (fn (ptr void) (ptr (struct thread-data))))

;; C�Υץߥ�ߥƥ��ַ���Sender, Reciever
(decl (csym::send-int n) (csym::fn void int))
(decl (csym::recv-int) (csym::fn int void))
