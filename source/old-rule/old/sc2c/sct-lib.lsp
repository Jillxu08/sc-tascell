;;;;; �롼�뵭�Ҥǰ���Ū�����Ѥ���ؿ�
(provide "SCT-LIB")
(in-package "SC-TRANSFORMER")

(export '(id s-or-u scs def-decl cscs expr op comp un-op as-op
	  pattern-variable-p get-retval matched-symbol
	  get-all-identifier id-to-scid generate-id
	  remove-type-qualifier deref-type unsigned-p
	  *type-size-alist* *type-rank-alist* *signed-to-unsigned-alist*
	  type-conversion))

(require "SC-UTIL")
(require "SC-MISC")
(use-package "SC-MISC")

;;;;;
(defconstant *keyword-package* (find-package "KEYWORD"))

;;;; ��ά��
(defun id (x) (sc-util:par-identifier x))

(defun s-or-u (x) (or (symbol= 'struct x) (symbol= 'union x)))
(defun scs (x) (sc-util:par-storage-class-specifier x))
(defun def-decl (x) (or (symbol= 'def x) (symbol= 'decl x)))
(defun cscs (x) (sc-util:par-compound-storage-class-specifier x))
(defun expr (x) (sc-util:par-expression x))

(defun op (x) (sc-util:par-operator x))
(defun comp (x) (sc-util:par-comparator x))
(defun un-op (x) (sc-util:par-unary-operator x))
(defun as-op (x) (sc-util:par-assignment-operator x))

;;;; for pattern-variable
(defmacro pattern-variable-p (sym)
  `(member ,sym matched-symbol :test #'symbol= :key #'car))
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
    ((setq idstr (sc-util:par-identifier x))
     (list idstr))
    ((listp x)
     (reduce #'append (mapcar #'get-all-identifier2 x) :from-end t))
    (t
     nil)))

;;; C�μ��̻Ҥ�ɽ��ʸ����->sc�μ��̻�
(defun id-to-scid (id)
  (let ((symstr "") (mode *print-case*))
    (dotimes (n (length id) (make-symbol symstr))
      (let ((idn (aref id n)))
	(cond
	  ((and (eq ':downcase mode)
		(upper-case-p idn))
	   (setq mode :upcase)
	   (add-string symstr (string+ "@" idn)))
	  ((and (eq ':upcase mode)
		(lower-case-p idn))
	   (setq mode :downcase)
	   (add-string symstr (string+ "@" idn)))
	  (t (add-string symstr (string idn))))))))

; "<basename>"��ngname�˴ޤޤ�Ƥ��ʤ����basename�򤽤Τޤ��֤�
; �ޤޤ�Ƥ����"<basename>2", "<basename>3", ...
; ��Ĵ�٤Ƥ��äơ��ǽ��ngname�˴ޤޤ�Ƥ��ʤ��ä���Τ��֤�
(defun get-new-id (&optional (basename "__scid_") ngname)
  (let ((n 2) ns)
    (unless (member basename ngname :test 'string=)
      (return-from get-new-id basename))
    (loop
     (setq ns  (string+ basename (write-to-string n)))
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
  (setq x (remove-if #'sc-util:par-type-qualifier x))
  (when (endp (cdr x))
    (return-from remove-type-qualifier 
      (remove-type-qualifier (car x))))
  (mapcar #'remove-type-qualifier x))


;;; deref type (ex. (ptr int) => int )
;;; �ؿ��ݥ��󥿤��Ф���mref��̵���ˤʤ�褦����(2003/1/7)
(defun deref-type (texp)
  (assert (consp texp))
  (case (immigrate-package (car texp) *keyword-package*)
    ((:array)
     (if (>= (length texp) 4)
	 `(,(car texp) ,(second texp) ,@(cdddr texp))
	 (second texp)))
    ((:ptr)
     (let ((ret (cadr texp)))
       (if (and (listp ret)
		(or (symbol= 'fn (car ret))
		    (symbol= 'lightweight (car ret))))  ;lw
	   texp
	   ret)))
    (otherwise
     (error "can't deref ~s" texp))))

;;; whether unsigned-type
(defun unsigned-p (texp)
  (case (immigrate-package texp *keyword-package*)
    ((:unsigned-char :unsigned-short :unsigned-int :unsigned-long
		     :unsigned-long-long)
     t)
    (otherwise nil)))

(defconstant *type-size-alist*
  '((:char . 8) (:signed-char . 8) (:unsigned-char . 8)
    (:short . 16) (:signed-short .16) (:unsigned-short . 16)
    (:int . 16) (:signed-int . 16) (:unsigned-int . 16)
    (:long . 32) (:signed-long . 32) (:unsigned-long . 32)
    (:long-long . 32) (:signed-long-long . 32)
    (:unsigned-long-long . 32)
    (:float . 32) (:double . 64) (:long-double . 64)))
(defconstant *type-rank-alist*
  '((:char . 0) (:signed-char . 0) (:unsigned-char . 0)
    (:short . 1) (:signed-short . 1) (:unsigned-short . 1)
    (:int . 2) (:signed-int . 2) (:unsigned-int . 2)
    (:long . 3) (:signed-long . 3) (:unsigned-long . 3)
    (:long-long . 4) (:signed-long-long . 4)
    (:unsigned-long-long . 4)))
(defconstant *signed-to-unsigned-alist*
  '((:char . :unsigned-char) (:signed-char . :unsigned-char)
    (:short . :unsigned-short) (:signed-short . :unsigned-short)
    (:int . :unsigned-int) (:signed-int . :unsigned-int)
    (:long . :unsigned-long) (:signed-long . :unsigned-long)
    (:long-long . :unsigned-long-long)
    (:signed-long-long . :unsigned-long-long)))
(defconstant *float-type-list*
   '(:long-double :double :float))

;; return signed, unsigned ,enum ,float or other
(defun classify-type (k-texp)
  (cond 
    ((member k-texp (mapcar #'car *signed-to-unsigned-alist*) :test #'eq)
     'signed)
    ((member k-texp (mapcar #'cdr *signed-to-unsigned-alist*) :test #'eq)
     'unsigned)
    ((and (consp k-texp) (eq :enum (car k-texp)))
     'enum)
    ((member k-texp *float-type-list*)
     'float)
    (t 'other)))

;;; implicit type conversion
;;; lightweight����Ҵؿ����б�(2003/12/28)
(defun type-conversion (texp1 texp2 &key (package *package*) 
			&aux ret)
  (let* ((k-texp1 (immigrate-package texp1 *keyword-package*))
	 (k-texp2 (immigrate-package texp2 *keyword-package*))
	 (class1 (classify-type k-texp1))
	 (class2 (classify-type k-texp2)))
    (cond
      ;; Ʊ�����ΤȤ�
      ((equal texp1 texp2)
       texp1)
      ;; �ɤ��餫�����ѷ��Ǥʤ��Ȥ�
      ((eq 'other class1) texp1)
      ((eq 'other class2) texp2)
      ;; �ɤ��餫����ư���������ΤȤ�
      ((eq 'float class1) texp1)
      ((eq 'float class2) texp2)
      ;; ξ��������
      (t
       ;; enum=>int
       (let ((p-int  (immigrate-package 'int  package))) 
	 (when (eq 'enum class1)
	   (setq texp1 p-int k-texp1 :int class1 'signed))
	 (when (eq 'enum class2)
	   (setq texp2 p-int k-texp2 :int class2 'signed)))
       ;; (���ʤ��Ȥ������unsigned�ξ�硢texp1��unsigned�ˤʤ�褦�������ؤ�)
       (unless (eq 'unsigned class1)
	 (swap texp1 texp2)
	 (swap k-texp1 k-texp2)
	 (swap class1 class2))
       (cond
	 ;; �ɤ����signed, �ɤ����unsigned
	 ((eq (eq 'unsigned class1) (eq 'unsigned class2))
	  (if (> (cdr (assoc k-texp1 *type-rank-alist*))
		 (cdr (assoc k-texp2 *type-rank-alist*)))
	      texp1 texp2))
	 ;; unsigned¦�Υ�󥯤��㤯�ʤ����
	 ((>= (cdr (assoc k-texp1 *type-rank-alist*))
	      (cdr (assoc k-texp2 *type-rank-alist*)))
	  texp1)
	 ;; signed¦�η����⤦�����η����ͤ�����ɽ���Ǥ�����
	 ((> (cdr (assoc k-texp2 *type-size-alist*))
	     (cdr (assoc k-texp1 *type-size-alist*)))
	  texp2)
	 ;; ����ʳ�(signed¦�η���unsigned���Ѵ�)
	 (t
	  (immigrate-package
	   (cdr (assoc k-texp2 *signed-to-unsigned-alist*))
	   package)))))))
