(%defconstant BUFSIZE 1024)
(%defconstant MAXCMDC 4)

(%defconstant FN fn)  ;; fn or lightweight

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

(decl (csym::recv-rslt) (fn void (struct cmd) (ptr void)))
(decl (csym::recv-task) (fn void (struct cmd) (ptr void)))
(decl (csym::recv-treq) (fn void (struct cmd)))
(decl (csym::recv-rack) (fn void (struct cmd)))
(decl (recv-none) (fn void (struct cmd)))

(decl (struct task))
(decl (struct thread-data))

(decl (do-task-body)
  (fn void (ptr (struct thread-data)) (ptr void)))
(decl (send-task-body)
  (fn void (ptr (struct thread-data)) (ptr void)))
(decl (csym::recv-task-body)
  (fn (ptr void) (ptr (struct thread-data))))
(decl (send-rslt-body)
  (fn void (ptr (struct thread-data)) (ptr void)))
(decl (recv-rslt-body)
  (fn void (ptr (struct thread-data)) (ptr void)))

(def (enum task-stat)
  TASK-ALLOCATED TASK-INITIALIZED TASK-STARTED TASK-DONE
  TASK-NONE TASK-SUSPENDED)

(def (enum task-home-stat)
  TASK-HOME-ALLOCATED TASK-HOME-INITIALIZED
  ;;  ����Ԥ��ϡ� task�Τۤ��Ǥ狼�롩
  TASK-HOME-DONE)

(def (struct task)
  (def stat (enum task-stat))       ; �������ξ���
  (def next (ptr (struct task)))    ;; �������ꥹ��
  (def prev (ptr (struct task)))    ;;
  (def body (ptr void))             ; task-data��¤�ΤؤΥݥ���
  (def ndiv int)                    ; ʬ����
  (def rslt-to (enum node))         ; ���������μ���
  (def rslt-head (array char 256))) ; ��������襢�ɥ쥹

;; ����������������Ԥ������å�������
(def (struct task-home)
  (def stat (enum task-home-stat))
  (def id int)                         ; ��������˳����Ƥ���ID 
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
  (def id-str (array char 32))
  (def buf (array char BUFSIZE)))

(extern divisibility-flag int)

;; C�Υץߥ�ߥƥ��ַ���Sender, Reciever
(decl (csym::send-int n) (fn void int))
(decl (csym::recv-int n) (fn void (ptr int)))

