(defpackage "RENAME"
  (:nicknames "REN")
  (:export :with-rename-environment :repl-id :bind-id
	   :inc-block-level :begin-function)
  (:shadow cl:declaration)
  (:use "RULE" "CL" "SC-MISC"))
(provide "RENAME-DEFS")

(in-package "RENAME")
  
(defvar *id-alist* nil)      ; list of (<id> . <block-level>)
(defvar *replace-alist* nil) ; list of (<oldid> . <newid>)
(defvar *block-id* 0)        ; ���ߤΥ֥�å�ID
(defvar *next-block-id* 0)   ; ���˻��Ѥ���֥�å�ID-1

(defmacro with-rename-environment (&body body)
  `(let ((*id-alist* *id-alist*)
         (*replace-alist* *replace-alist*)
         (*block-id* 0)
         (*next-block-id* 0))	      
     ,@body))
       
; get *block-id* of id
(defun assoc-id-level (id)
  (cdr (assoc id *id-alist* :test #'eq)))

; get replacement-id of old-id
(defun assoc-replacement-id (id)
  (or (cdr (assoc id *replace-alist* :test #'eq))
      id))

; entry identifier
; �֤��ͤϹ������ (values *id-alist* *replace-alist*) 
(defun entry-identifier (id)
  (let ((lev (assoc-id-level id)))
    (if lev 
	(if (= *block-id* lev)     
	    ;Ʊ�쥹�����פ������ ->���Τޤ�
	    (values *id-alist* *replace-alist*)
	    ;�̤Υ������פ������
	    (let* ((new-id (generate-id (identifier0 id :sc2c))))
	      (values
	       (cons (cons id *block-id*) *id-alist*)
	       (cons (cons id new-id) *replace-alist*))))
      ; ̤���
      (values 
       (cons (cons id *block-id*) *id-alist*)
       (cons (cons id id) *replace-alist*)))))

; replace all identifier in s-expression
(defun repl-id (x)
  (map-all-atoms #'assoc-replacement-id x))

; entry id (list) and bind *id-alist* *replace-alist* 
; Returns replacement id (list)
(defun bind-id (id)
  (if (listp id)
      (mapcar #'bind-id id)
    (progn
      (multiple-value-setq
	  (*id-alist* *replace-alist*) (entry-identifier id))
      (assoc-replacement-id id))))

; increment block-level
(defmacro inc-block-level (&body form)
  `(let ((*replace-alist* *replace-alist*)
	 (*block-id* (incf *next-block-id*)))    
    ,@form))

; begin function
(defmacro begin-function (&body form)
  `(let ((*id-alist* *id-alist*)
	 (*replace-alist* *replace-alist*)
	 (*block-id* (incf *next-block-id*)))
    ,@form))
