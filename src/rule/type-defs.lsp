(provide "TYPE-DEFS")

(defpackage "TYPE"
  (:nicknames "TYPE-INFO")
  (:export :with-typerule-environment :with-function
           :with-new-environment :with-field-information
           :add-variable :assume-variable :add-typename :assume-typename
           :unnamed-id :get-struct-member-info :add-struct
           :leave-typename
           :assoc-vartype :assoc-struct :expand-typedef-name
           :$type
           :handle-exp-args :handle-exp-1arg
           :add-flow-printed :already-printed)
  (:use "CL" "SC-MISC"))

(in-package "TYPE")

;;; *str-alist* : list of ( <strname> . ( [(<field-id> . <type>)]* ) )
;;; *var-alist* : list of ( <name> . <type> )
;;; deftype -> ( name . ($type <type>) ) in *var-alist*
;;; enum and struct without name -> $<typedef-name> 
;;; (struct $str) (enum $en) etc.
(defvar *str-alist* (list))
(defvar *var-alist* (list))
(defvar *interim-str-alist* (list (cons '$dummy nil))) ; ̤������β������̤���ѡ�
(defvar *interim-var-alist* (list (cons '$dummy nil))) ; ̤������β����

(defvar *flow-printed* '())

;;;;;;;;;;;;;;;;;;;;
;;; �Ķ�����

(defmacro with-typerule-environment (&body body)
  `(let ((*interim-str-alist* (list (cons '$dummy nil)))
         (*interim-var-alist* (list (cons '$dummy nil))))
     (let ((*str-alist* *interim-str-alist*)
           (*var-alist* *interim-var-alist*))
       ,@body)))

(defmacro with-function (id &body body)
  `(error-indent (format nil "In function ~S:" ,id)
                 (let ((*flow-printed* '())) ; print-flow��
                   ,@body)))

(defmacro with-new-environment (&body body)
  `(let ((*str-alist* *str-alist*)
         (*var-alist* *var-alist*))
     ,@body))

(defmacro with-field-information (struct-id &body body)
  (with-fresh-variables (asc-str)
    `(let ((,asc-str (assoc-struct ,struct-id)))
       (error-indent (format nil "Referencing field of ~S." ,struct-id)
                     (let ((*var-alist* (append ,asc-str (leave-typename *var-alist*)))
                           (*interim-var-alist* ,asc-str))
                       ,@body))) ))

;;;;;;;;;;;;;;;;;;;;
;;; �ѿ���Ͽ

;;; Add variable declaration
(defun add-variable (id type &optional (remove-type-qualifier t))
  (push (cons id (funcall (if remove-type-qualifier
                              #'rule:remove-type-qualifier #'identity)
                          type))
        *var-alist*))

;;; Add variable declaration ad interim
(defun assume-variable (id type &optional (remove-type-qualifier t))
  (push (cons id (funcall (if remove-type-qualifier
                              #'rule:remove-type-qualifier #'identity)
                          type))
        (cdr *interim-var-alist*)))

;;; Add typename definition
(defun add-typename (id type &optional (remove-type-qualifier t))
  (add-variable id (list '$type type) remove-type-qualifier))

;;; Add variable declaration ad interim
(defun assume-typename (id type &optional (remove-type-qualifier t))
  (assume-variable id (list '$type type) remove-type-qualifier))

;;; Get struct member definition
;;; ���������alist��struct-declaration �ˤ���ѷ���̤��֤�
(defun get-struct-member-info (sdeclist)
  (let* ((orig-var-alist *var-alist*)
         (*var-alist* *var-alist*)
         (sdecl-with-type (mapcar #'rule:struct-declaration! sdeclist))
                                        ; ������*var-alist*����Ͽ�����
         (member-alist (ldiff *var-alist* orig-var-alist)))
    (values member-alist sdecl-with-type)))

;;; for unnamed struct/union/enum id
(defun unnamed-id (base-id)
  (symbol+ '$ base-id))

;;; Add struct
(defun add-struct (id member-alist)
  (push (cons id member-alist) *str-alist*))

;;; Add struct ad interim
(defun assume-struct (id member-alist)
  (push (cons id member-alist) (cdr *interim-str-alist*)))

;;;;;;;;;;;;;;;;;;;;
;;; �ѿ��ꥹ�ȤΥե��륿���

;;; leave only type-entry from *var-alist*
(defun leave-typename (&optional (alist *var-alist*))
  (remove-if-not
   #'(lambda (x) (and (consp (cdr x))
                      (eq '$type (cadr x))))
   alist))

;;;;;;;;;;;;;;;;;;;;
;;; �ѿ�����

;;; get type of variable
;;; expect: id���ѿ�����̾�Τɤ���Ǥ���Ϥ����ʴ��ԤȰ��פ��ʤ����error��
(defun assoc-vartype (id &optional (expect nil))
  ;; (format t "(assoc-vartype ~s)~%" id)
  (acond
   ((assoc id *var-alist* :test #'eq)
    (let ((datum (cdr it)))
      (if (and (consp datum) (eq '$type (car datum)))
          (progn                        ; typename
            (when (and expect (not (eq :typename expect)))
              (ierror "~S is declared as typename." id))
            (values (cadr datum) :typename))
        (progn                          ; variable
          (when (and expect (not (eq :variable expect)))
            (ierror "~S is declared as variable." id))
          (values datum :variable)))))
   ;; ���Ĥ���ʤ��ä����
   ((eq :typename expect)
    (iwarn "The type ~S is undefined." id)
    (assume-typename id id)
    (values id :typename))
   (t
    (iwarn "The variable ~S is undeclared.~%~
            ~AAssume its type as '~S'."
           id *error-indent-spaces* 'undefined)
    (assume-variable id 'undefined)
    (values 'undefined :variable))
   ))

;;; get struct member list
(defun assoc-struct (id)
  (aif (assoc id *str-alist* :test #'eq)
       (cdr it)
       (let ((interim-member (list (cons '$dummy nil))))
         (iwarn "The struct-or-union ~S is undefined." id)
         (assume-struct id interim-member)
         interim-member)))

;;; Expand typename
;; (defun expand-typedef-name (texp)
;;   ;; (format t "(expand-typedef-name ~s)~%" texp)
;;   (if (listp texp)
;;       (if (or (rule:struct-or-union-specifier? texp)
;;               (rule:enum-specifier? texp))
;;           texp
;;         (mapcar #'expand-typedef-name texp))
;;     (if (or (not (rule:identifier? texp))
;;             (eq 'undefined texp))
;;         texp
;;       (let ((asc (assoc-vartype texp :typename)))
;;         (if (eq texp asc)
;;             asc
;;           (expand-typedef-name asc))))))

;;; * operator
;;; * �����η��ꥹ��->�����Τη� �δؿ�
;;; * operator�ΰ����ꥹ�� (expression�Τ�)
;;; ������Ȥäơ������Ѵ���̤��֤�
(defun handle-exp-args (op fun exp-list)
  (let* ((exp-rets (mapcar #'rule:expression! exp-list))
         (exp-types (mapcar #'second exp-rets))
         (ret-type (funcall fun exp-types)))
    ~(the ,ret-type (,op ,@exp-rets))))

;; arg��1�ĤΤȤ���
(defun handle-exp-1arg (op fun exp)
  (let* ((exp-ret (rule:expression! exp))
         (exp-type (second exp-ret))
         (ret-type (funcall fun exp-type)))
    ~(the ,ret-type (,op ,exp-ret))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; print-flow ��
(defun add-flow-printed (fexp)
  (push fexp *flow-printed*))

(defun already-printed (fexp)
  (member fexp *flow-printed* :test #'equal))