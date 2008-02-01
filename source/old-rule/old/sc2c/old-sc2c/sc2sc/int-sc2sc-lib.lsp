;;;;; �롼�뵭�Ҥǰ���Ū�����Ѥ���ؿ�

;;;; ��ά��
(defun id (x) (par-identifier x))
(defun s-or-u (x) (or (eq 'struct x) (eq 'union x)))
(defun scs (x) (par-storage-class-specifier x))
(defun def-or-decl (x) (or (eq 'def x) (eq 'decl x)))
(defun cscs (x) (par-compound-storage-class-specifier x))
(shadow 'exp)
(defun exp (x) (par-expression x))
(defun op (x) (par-operator x))
(defun comp (x) (par-comparator x))
(defun un-op (x) (par-unary-operator x))
(defun as-op (x) (par-assignment-operator x))

;;;; for pattern-variable
(defmacro pattern-variable-p (sym)
  `(member ,sym matched-symbol :test #'eq :key #'car))
(defmacro get-retval (sym)
  `(let ((pv-entry (pattern-variable-p ,sym)))
     (if pv-entry
	 (apply #'values (cdar pv-entry))
       (progn
	 (warn "~S is not a pattern-variable." ,sym)
	 nil))))

;;;; for tracing rule
(defmacro rule-trace (&rest rule-list)
  `(dolist (rl ',rule-list
	       (setq *trace-rule* (delete-duplicates *trace-rule*)))
     (push rl *trace-rule*)))
(defmacro rule-untrace (&rest rule-list)
  `(dolist (rl ',rule-list *trace-rule*)
    (setq *trace-rule* (remove rl *trace-rule* :test #'eq))))
(defmacro rule-cleartrace ()
  '(setq *trace-rule* nil))
(defun rule-alltrace ()
  (Setq *trace-all* (if *trace-all* nil t)))
(defun rule-unmatch-verbose ()
  (setq *trace-unmatch-verbose* (if *trace-unmatch-verbose* nil t)))

;;;; ���̻Ҵ�Ϣ
;;; �ץ����������Ƥμ��̻Ҥ��֤�
(defun get-all-identifier (x)
  (remove-duplicates (get-all-identifier2 x) :test #'string=))
(defun get-all-identifier2 (x &aux idstr)
  (cond
    ((setq idstr (par-identifier x))
     (list idstr))
    ((listp x)
     (apply #'append (mapcar #'get-all-identifier2 x)))
    (t
     nil)))

;;; ʸ����->sc�μ��̻�
(defun id-to-scid (id)
  (let ((symstr "") (mode *print-case*))
    (dotimes (n (length id) (make-symbol symstr))
      (let ((idn (aref id n)))
	(cond
	  ((and (eq ':downcase mode)
		(upper-case-p idn))
	   (setq mode :upcase)
	   (add-string symstr (string+ `("@" ,idn))))
	  ((and (eq ':upcase mode)
		(lower-case-p idn))
	   (setq mode :downcase)
	   (add-string symstr (string+ `("@" ,idn))))
	  (t (add-string symstr (string idn))))))))

; "<basename>"��ngname�˴ޤޤ�Ƥ��ʤ����basename�򤽤Τޤ��֤�
; �ޤޤ�Ƥ����"<basename>2", "<basename>3", ...
; ��Ĵ�٤Ƥ��äơ��ǽ��ngname�˴ޤޤ�Ƥ��ʤ��ä���Τ��֤�
(defun get-new-id (&optional (basename "__scid_") ngname)
  (let ((n 2) ns)
    (unless (member basename ngname :test 'string=)
      (return-from get-new-id basename))
    (loop
      (setq ns  (string+ `(,basename ,(write-to-string n))))
      (unless (member ns ngname :test 'string=)
	(return ns))
      (incf n))))

; (get-new-id basename ngname) ��ngname����Ͽ����
; ����ˤ��η�̤� id-to-scid �ˤ������֤���
; ���̤ˡ����������̻Ҥ���Ͽ����Ȥ��Ϥ��δؿ���Ȥ���
(defmacro generate-id (&optional (basename "__scid_") ngname)
  (let ((new-id (gensym)))
    `(let ((,new-id (get-new-id ,basename ,ngname)))
       (when ,ngname
	 (push ,new-id ,ngname))
       (id-to-scid ,new-id))))

;;;; ����Ϣ

;;; remove type-qualifier from type-expression
(defun remove-type-qualifier (x)
  (when (not (listp x)) (return-from remove-type-qualifier x))
  (setq x (remove-if #'par-type-qualifier x))
  (when (endp (cdr x))
    (return-from remove-type-qualifier 
      (remove-type-qualifier (car x))))
  (mapcar #'remove-type-qualifier x))


;;; deref type (ex. (ptr int) => int )
;;; �ؿ��ݥ��󥿤��Ф���mref��̵���ˤʤ�褦����(2003/1/7)
(defun deref-type (texp)
  (case (car texp)
    ((array)
     (if (>= (length texp) 4)
	 `(array ,(cadr texp) ,@(cdddr texp))
	 (cadr texp)))
    ((ptr)
     (let ((ret (cadr texp)))
       (if (and (listp ret)
		(or (eq 'fn (car ret))
		    (eq 'lightweight (car ret))))  ;lw
	   texp
	 ret)))
    (otherwise
     (error "can't deref ~s" texp))))

;;; whether unsigned-type
(defun unsigned-p (texp)
  (case texp
    ((unsigned-char unsigned-short unsigned-int unsigned-long
		    unsigned-long-long)
     t)
    (otherwise nil)))

(defconstant *type-size-ailst*
  '((char . 8) (signed-char . 8) (unsigned-char . 8)
    (short . 16) (signed-short .16) (unsigned-short . 16)
    (int . 16) (signed-int . 16) (unsigned-int . 16)
    (long . 32) (signed-long . 32) (unsigned-long . 32)
    (long-long . 32) (signed-long-long . 32)
    (unsigned-long-long . 32)
    (float . 32) (double . 64) (long-double . 64)))
(defconstant *type-rank-alist*
  '((char . 0) (signed-char . 0) (unsigned-char . 0)
    (short . 1) (signed-short . 1) (unsigned-short . 1)
    (int . 2) (signed-int . 2) (unsigned-int . 2)
    (long . 3) (signed-long . 3) (unsigned-long . 3)
    (long-long . 4) (signed-long-long . 4)
    (unsigned-long-long . 4)))
(defconstant *signed-to-unsigned-alist*
  '((char . unsigned-char) (signed-char . unsigned-char)
    (short . unsigned-short) (signed-short . unsigned-short)
    (int . unsigned-int) (signed-int . unsigned-int)
    (long . unsigned-long) (signed-long . unsigned-long)
    (long-long . unsigned-long-long)
    (signed-long-long . unsigned-long-long)))

;;; implicit type conversion
;;; lightweight����Ҵؿ����б�(2003/12/28)
(defun type-conversion (texp1 texp2)
  ; Ʊ�����ΤȤ�
  (when (equal texp1 texp2)
    (return-from type-conversion texp1))
  ; �ɤ��餫�����ѷ��Ǥʤ��Ȥ�
  (dolist (texp (list texp1 texp2))
    (when (and (consp texp)
	       (member (car texp) '(fn ptr array struct union)))
      (return-from type-conversion texp)))
  ; �ɤ��餫����ư���������ΤȤ�
  (dolist (float-type '(long-double double float))
    (when (or (eq float-type texp1) (eq float-type texp2))
      (return-from type-conversion float-type)))
  ; enum=>int
  (setq texp1 (if (and (consp texp1) (eq 'enum (car texp1)))
		  'int texp1))
  (setq texp2 (if (and (consp texp2) (eq 'enum (car texp2)))
		  'int texp2))
  ; �ɤ����signed, �ɤ����unsigned
  (when (eq (unsigned-p texp1) (unsigned-p texp2))
    (return-from type-conversion
      (if (> (cdr (assoc texp1 *type-rank-alist*))
	     (cdr (assoc texp2 *type-rank-alist*)))
	  texp1 texp2)))
  ; (texp1��unsigned�ˤʤ�褦�������ؤ�)
  (unless (unsigned-p texp1)
    (let (tmp) (setq tmp texp1) (setq texp1 texp2) (setq texp2 tmp)))
  ; unsigned¦�Υ�󥯤��㤯�ʤ����
  (when (>= (cdr (assoc texp1 *type-rank-alist*))
	    (cdr (assoc texp2 *type-rank-alist*)))
    (return-from type-conversion texp1))
  ; signed¦�η����⤦�����η����ͤ�����ɽ���Ǥ�����
  (when (> (cdr (assoc texp2 *texp-size-alist*))
	   (cdr (assoc texp1 *texp-size-alist*)))
    (return-from type-conversion texp2))
  ; ����ʳ�(signed¦�η���unsigned���Ѵ�)
  (cdr (assoc texp2 *signed-to-unsigned-alist*)))
