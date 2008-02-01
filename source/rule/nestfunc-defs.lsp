(defpackage "NESTED-FUNCTION"
  (:nicknames "LW" "NESTFUNC")
  (:export :*frame-alist* :*current-func* :*estack-size* :*all-in-estack*
           :make-finfo :finfo-name :finfo-parent-func :finfo-ret-type
           :finfo-argp :finfo-label-list :finfo-var-list :finfo-tmp-list
           :finfo-nf-list :finfo-local-label :finfo-estack-var
           :with-nestfunc-environment
           :add-global-func
           :thread-origin-p :get-frame :get-frame-name
           :estack-variable-p :static-variable-p 
           :howmany-outer :make-frame-decl
           :add-local :add-nestfunc :typeconv :make-resume
           :make-init-efp-esp :save-args-into-estack :make-extid
           :make-normalize-nf :make-frame-save :make-frame-resume
           :make-suspend-return :find-local-label :combine-ret-list
           :simple-exp-p)
  (:use "RULE" "CL" "SC-MISC")
  (:shadow cl:declaration))
(in-package "NESTED-FUNCTION")

;;; global�ؿ�̾
(defvar *global-funcs* '())

;;; �ؿ��ե졼��ι�¤�ξ���
;;; ( <�ؿ�̾(symbol)> . <��¤�ξ���> ) �Υꥹ��
;;; <��¤�ξ���> := ( <��¤��̾> ) 
(defvar *frame-alist* '() )
;;; ���ߤ���ؿ��ξ���
(defvar *current-func* nil)

(defvar *estack-size* 65536)
(defvar *all-in-estack* nil)

;;; nestfunc.rule �ǻȤ��ѿ����ؿ������
(provide "NESTFUNC-DEFS")

(defstruct finfo 
  name           ;�ؿ�̾
  parent-func    ;�ƴؿ���finfo��¤�Ρ���nil iff ����Ҵؿ���
  ret-type       ;�֤��ͤη�
  argp           ;argp����/���� (=����Ҵؿ��ƤӽФ���̵ͭ)
  ;; (<�������֤򼨤���٥�̾> . <�ե졼�������ѥ�����>) �Υꥹ��(�ս�)
  label-list
  ;; ( <symbol> . <texp> )
  ;; frame�ˤ� var-list #|�� tmp-list��ξ����|# �����
  ;; ��nf-list �� orig-name ��������
  ;; frame-save, frame-resume ��var-list�Τ�
  ;; tmp-list �����פ���
  var-list     ;�̾�Υ������ѿ��Υꥹ�ȡܴؿ��Υݥ�����¸��
  tmp-list     ;����ѿ��Υꥹ�ȡ�call���ΰ���������Ҵؿ��ݥ�����¸�ѡ�
  nf-list      ;������줿����Ҵؿ��Υꥹ�� ( <orig-name> . <ext-name> )
  ;; ����Ū�����å��򻲾Ȥ���������ѿ���list of <id(symbol)>
  ;; var-list�Ƚ�ʣ������Ҵؿ��� nf-list�Ǵ�������ΤǤ����ˤ�����ʤ�
  ;; search-ptr �Ǥμ������ˤˤ�ꡤvar-list�Ǥʤ���Τ��������롥
  estack-var
  ;; static��������줿�ѿ� (<orig-name> . <ext-name>)��var-list�Ƚ�ʣ��
  static-var
  ;; �ɽ��٥�Υꥹ��(def ,id __label__)�����������Ҵؿ�æ����
  ;; (<��٥�̾> . <����������>)������ȵս�
  local-label
  )

;;;; �Ķ�����
(defmacro with-nestfunc-environment (&body body)
  `(let ((*global-funcs* '())
	 (*estack-size* (ruleset-param 'rule::estack-size))
	 (*all-in-estack* (ruleset-param 'rule::all-in-estack))
	 (*frame-alist* '())
	 (*current-func* nil))
     ,@body))
  

;;;; �ؿ�

(defun add-global-func (fid)
  (push fid *global-funcs*))

(defun global-func-p (fid)
  (member fid *global-funcs* :test #'eq))

;;; �ޥ������åɴĶ��ǡ������å��κǽ�Υե졼���Ƚ�Ǥ�����
(defun thread-origin-p (finfo-or-fid)
  (let ((fid (if (symbolp finfo-or-fid)
		 finfo-or-fid
		 (finfo-name finfo-or-fid))))
    (or (eq ~main fid)
	(eq ~thread-origin fid))))  ; ���פ���


;;; �ؿ�̾��*frame-alist*����õ���ƴؿ��ե졼��ι�¤�ξ�������롣
;;; ̤��Ͽ�ξ�����Ͽ���ơ���Ͽ������¤�ξ�����֤���
(defun get-frame (x)
  (let* ((asc (assoc x *frame-alist* :test #'eq)))
    (if asc
	(cdr asc)
	(let* ((strx (identifier x))
	       (frame-name (generate-id (string+ strx "_frame")))
	       (frame-info ~(,frame-name )))
	  (push (cons x frame-info) *frame-alist*)
	  frame-info))))

;;; �ؿ�̾=>�ؿ��Υե졼�๽¤��̾
(defun get-frame-name (fname)
  (car (get-frame fname)))

;;; ���ؿ��ʿƤϴޤޤʤ��ˤ�local-variable����
;;; tmp-list��Τ�Ρ�����Ҵؿ��ϸ����оݤǤϤʤ���
(defun local-variable-p (id &optional (finfo *current-func*))
  (and *current-func*
       (assoc id (finfo-var-list finfo) :test #'eq)))

;;; ���ؿ��ʿƤϴޤޤʤ��ˤ�����Ū�����å����ͤ�����local-variable����
(defun estack-variable-p (id &optional (finfo *current-func*)
			  (skip-lv-check nil))  ; local-variable-p �Υ����å����ά
  (and (or skip-lv-check (local-variable-p id finfo))
       (member id (finfo-estack-var finfo) :test #'eq)))

;;; ���ؿ��ʿƤϴޤޤʤ��ˤ�static��������줿local-variable����
;;; �����ʤ顤ext-id���֤���
(defun static-variable-p (id &optional (finfo *current-func*)
			  (skip-lv-check nil))  ; local-variable-p �Υ����å����ά
  (and (or skip-lv-check (local-variable-p id finfo))
       (cdr (assoc id (finfo-static-var finfo) :test #'eq))))

;;; ���ؿ�(=0)���餤���ĳ��δؿ���������줿 local-variable/nestfunc ��?
;;; ���Ĥ���ʤ���� -1
;;; �����֤��ͤ� local-varriable-> :var, nestfunc-> :nestfunc
;;; �軰�֤��ͤ� ���Ĥ��ä��ؿ���finfo
;;; ����֤��ͤ� local-variable-p/nestfunc-extid ���֤���
(defun howmany-outer (id &optional (finfo *current-func*))
  (labels ((rec (curfunc acc)
             (acond
               ((null curfunc)
                -1)
               ((local-variable-p id curfunc)
                (values acc :var finfo it))
               ((nestfunc-extid id curfunc)
                (values acc :nestfunc finfo it))
               (t (rec (finfo-parent-func curfunc) (1+ acc))) )))
    (rec finfo 0)))

;;; Ϳ����줿�ؿ����󤫤�ե졼�๽¤�Τ��������
(defun make-frame-decl (fi)
  (let ((frame-name (get-frame-name (finfo-name fi)))
	(member-list (append (finfo-var-list fi)
                             ;; (finfo-tmp-list fi)
			     (mapcar #'(lambda (x)
                                         (cons (car x) ~closure-t))
                                     (finfo-nf-list fi)))))
    ~(def (struct ,frame-name)
	  (def tmp-esp (ptr char))    ; ����ϡ����Ф���Ƭ
	  (def argp (ptr char))
	  (def call-id int)
	  ,@(mapcar #'(lambda (x)
                        ~(def ,(car x) ,(cdr x)))
                    member-list))))

;;; *current-func*���ѿ�������ɲä��ơ�declarationʸ���֤�
(defun add-local (id texp mode &key (init nil) (finfo *current-func*))
  (let ((typeconv-texp (typeconv texp)))
    ;; mode�� :var or :temp
    (when finfo
      (case mode
	((:var)     ; ��estack-var�Ǥʤ���С�save/resume ���о�
	 (when (let ((ttexp (remove-type-qualifier texp)))
		 (or (and (listp ttexp)
                          (eq ~array (car ttexp)))
                     *all-in-estack*))
	   (pushnew id (finfo-estack-var finfo) :test #'eq))
	 (push (cons id typeconv-texp) (finfo-var-list finfo)))
	((:tmp)     ; save/resume ���оݤˤʤ�ʤ�
         (push (cons id typeconv-texp) (finfo-tmp-list finfo)))
	((:static)  ; ���˽Ф���frame�ˤ�����ʤ���
	 (let ((ext-id (generate-id
			(string+ (identifier0 id) "_in_" 
				 (identifier0 (finfo-name finfo))))))
	   (push (cons id typeconv-texp) (finfo-var-list finfo))
	   (push (cons id ext-id) (finfo-static-var finfo))
	   (setq id ext-id))) ; ̾������ͤ��ʤ��褦���ѹ�
        ((:system)  ; ����ѿ����ä������Ѥʤ���
         )
	(otherwise
	 (error "unexpected value of 'mode'(~s)" mode))))
    (if init
	~(def ,id ,typeconv-texp ,(rule::expression-rec init))
      ~(def ,id ,typeconv-texp))))

;;; *current-func* ������Ҵؿ�������ɲ�
(defun add-nestfunc (id extid &optional (finfo *current-func*))
  (push (cons id extid)
        (finfo-nf-list finfo)) )

;;; * lightweight => closure-t
;;; * fn�����������ɲ�
(defun typeconv (texp)
  (if (and texp (listp texp))
      (case (car texp)
        ((sc::lightweight) ~closure-t)
        ((sc::fn)          ~(fn ,(typeconv (cadr texp))
                                (ptr char)
                                ,@(mapcar #'typeconv (cddr texp))))
        (otherwise (mapcar #'typeconv texp)))
    texp))

;;; Ϳ����줿�ؿ����󤫤�����������Ԥ�statement����
(defun make-resume (fi)
  (unless (or (finfo-label-list fi)
              (finfo-local-label fi))
    (return-from make-resume
      ~((label LGOTO nil))))
  (let ((reconst-impossible (or (eq ~main (finfo-name fi))
                                (finfo-parent-func fi)
                                *all-in-estack*))
                                        ; �����å����Ѥ�ľ������������ʤ�
        (case-goto
         (append
          ;; ����Ҵؿ��ƤӽФ���λ�������
          (do ((ret nil)
               (k 0 (1+ k))
               (lb (reverse (finfo-label-list fi)) (cdr lb)))
              ((endp lb) (apply #'append (nreverse ret)))
            (push ~((case ,k)
                    ,@(cdar lb) 
                    (goto ,(caar lb)))
                  ret))
          ;; goto�ˤ������Ҵؿ�����ƴؿ��ؤ�æ����
          (do ((ret nil)
               (k -1 (1- k))
               (lb (reverse (finfo-local-label fi)) (cdr lb)))
              ((endp lb) (apply #'append (nreverse ret)))
            (push ~((case ,k)
                    ,@(cdar lb) 
                    (goto ,(caar lb)))
                  ret))))
        (frame-type ~(struct ,(get-frame-name (finfo-name fi)))))
    ~((if ,(if reconst-impossible
               ~0
             ~esp-flag)
          (begin
           ,@(unless reconst-impossible
               ~( (= esp (cast (ptr char)
                               (bit-xor (cast size-t esp) esp-flag)))
                  (= efp (cast (ptr ,frame-type) esp))
                  (= esp (aligned-add esp (sizeof ,frame-type)))
                  (= (mref-t (ptr char) esp) 0) ))
           (label LGOTO
                  (switch (fref (mref efp) call-id) ,@case-goto))
           ,@(when (finfo-label-list fi)
               ~( (goto ,(caar (last (finfo-label-list fi)))) )))))))

;;; efp(xfp)�����ꤪ��� esp��ե졼�ॵ����ʬ��ư������
(defun make-init-efp-esp (fi)
  (let ((frame-type  ~(struct ,(get-frame-name (finfo-name fi)))))
    ~((= efp (cast (ptr ,frame-type) esp))
      (= esp (aligned-add esp (sizeof ,frame-type)))
      (= (mref-t (ptr char) esp) 0)
      ,@(when (and *all-in-estack*
                   (finfo-parent-func fi))
          ~( (= (fref efp -> xfp) xfp) ))
      )))

;;; ��*all-in-estack*���˰������ͤ�estack����¸
(defun save-args-into-estack (argid-list argtexp-list
                              &optional (finfo *current-func*))
  (when *all-in-estack*
    ;; ����äȼ�ȴ���Ƿ�����ʤ�
    (mapcar #'(lambda (id texp)
                (if (finfo-parent-func finfo)
                    ~(= (fref efp -> ,id) (pop-arg ,texp parmp)) 
                    ~(= (fref efp -> ,id) ,id) ))
            argid-list
            argtexp-list) ))

;;; ����Ҵؿ���id -> �ȥåץ�٥�˰ܤ����ؿ���id
(defun make-extid (id &optional (pfinfo *current-func*))
  (generate-id (string+ (identifier0 id) "_in_"
                        (identifier0 (finfo-name pfinfo)))) )

;;; id�����ߤδؿ��ʿƤϽ��������������줿����Ҵؿ�����
;;; �⤷�����ʤ顤ext-name ���֤�
(defun nestfunc-extid (id &optional (finfo *current-func*))
  (and *current-func*
       (cdr (assoc id (finfo-nf-list finfo) :test #'eq))))

;;; ����Ҵؿ��λ��� -> etack�ؤλ���
;;; ��pfinfo: �ƴؿ������
(defun nestfunc-in-estack (fid &optional (pfinfo *current-func*))
  (declare (ignore pfinfo))
  ~(ptr (fref efp -> ,fid)))

;;; Ϳ����줿�ؿ����󤫤�����Ҵؿ������������륳���ɤ���
(defun make-normalize-nf (&optional (fi *current-func*))
  (let ((nf-list (finfo-nf-list fi)))
    (apply #'nconc
           (mapcar
            #'(lambda (x) 
                ~( (= (fref efp -> ,(car x) fun)
                      ,(cdr x))
                   (= (fref efp -> ,(car x) fr)
                      (cast (ptr void) efp)) ))
            nf-list))))

;;; Ϳ����줿�ؿ����󤫤�ե졼��������¸���륳���ɤ���
(defun make-frame-save (&optional (fi *current-func*))
  (mapcar
   #'(lambda (x)
       ~(= (fref efp -> ,(car x)) ,(car x)))
   (remove-if #'(lambda (x)
                  (or (estack-variable-p (car x) fi t)
                      (eq ~closure-t (cdr x))))
              (finfo-var-list fi))))

;;; Ϳ����줿�ؿ����󤫤�ե졼���������褹�륳���ɤ���
(defun make-frame-resume (&optional (fi *current-func*))
  (mapcar
   #'(lambda (x) 
       ~(= ,(car x) (fref efp -> ,(car x))))
   (remove-if #'(lambda (x)
                  (or (estack-variable-p (car x) fi t)
                      (eq ~closure-t (cdr x))))
              (finfo-var-list fi))))

;;; Ϳ����줿�ؿ����󤫤�ؿ������Ѥ�return���������륳���ɤ���
(defun make-suspend-return (&optional (fi *current-func*))
  (cond ((finfo-parent-func fi)
         ;;~(return (fref efp -> tmp-esp)))
         (error "make-suspend-return called in lightweight-func"))
        ((eq ~void (finfo-ret-type *current-func*))
         ~(return))
        (t
         ~(return (SPECIAL ,(finfo-ret-type *current-func*))))))

;;; Ϳ����줿��٥�̾�����ƴؿ��ζɽ��٥�Ȥ����������Ƥ��뤫Ĵ�٤롣
;;; �������Ƥ��ʤ����,�֤��ͤ�nil���������Ƥ���С�
;;; (values <��ʬ����ߤƲ����ܤοƴؿ����������Ƥ�����>
;;;         <������� ( <label> . <��������> )> 
;;;         <���Υ�٥뤬�ؿ���ǲ����ܤ�push���줿��Τ�>)
(defun find-local-label (lid fi &aux (lids (identifier lid)))
  (labels ((find-local-label-tail (cfi acc &aux memb)
             (cond ((null cfi)
                    nil)
                   ((let* ((memb0 (member 
                                   lids
                                   (finfo-local-label cfi)
                                   :test #'string=
                                   :key #'(lambda (x)
                                            (identifier (car x))))))
                      (setq memb memb0))
                    (values acc (car memb) (length memb)))
                   (t
                    (find-local-label-tail
                     (finfo-parent-func cfi) (1+ acc))))))
    (find-local-label-tail fi 0)))

;; begin�Ȥ���(Nfb0 body)���֤��ͤ��顢
;; begin���Τ�Τ��֤��ͤ��롣
;; ((r1-1 r1-2 r1-3 r1-4) ... (rn-1 rn-2 rn-3 rn-4))
;; => ( (,@r1-1 ... ,@rn-1)
;;      (,@r1-2 ... ,@rn-2) 
;;      nil
;;      (,@prev-4th ,@r1-3 ,r1-4 ... ,@rn-3 ,rn-4 ) )
(defun combine-ret-list (ret-list &optional prev-4th)
  (let ((fst (mapcar #'first ret-list))
        (scd (mapcar #'second ret-list))
        (thd-4th (mapcar
                  #'(lambda (x) ~(,@(third x) ,(fourth x)))
                  ret-list)))
    (list (apply #'append fst)
          (apply #'append scd)
          nil
          (remove nil (apply #'append prev-4th thd-4th)))))

;; ����ѿ���Ȥ�ɬ�פ��ʤ����Ĥޤ�
;; * ����Ҵؿ��ƽФ�����ѹ����ä����ʤ�(permit-change=nil����
;; * �����Ѥ򵯤����ʤ�
;; ���Ȥ��ݾڤǤ��뼰
(defun simple-exp-p (the-exp &optional (permit-change nil))
  (let ((type (second the-exp))
        (exp (third the-exp)))
    (or (and (symbolp exp) permit-change)
        (and (global-func-p exp) (not (local-variable-p exp)))
        (eq 'type::undefined type)
        (sc-number exp)
        (sc-character exp)
        (sc-string exp))))
