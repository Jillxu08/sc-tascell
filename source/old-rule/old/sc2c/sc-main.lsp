;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;; SC������ main ;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;
(provide "SC-MAIN")

(unless (find-package "SC-MAIN")
  (make-package "SC-MAIN" :use '("LISP")))
(unless (find-package "SC")
  (make-package "SC" :use '())
  ;; defmacro�ǻȤ��ʤ��Τ����ؤʤΤ�
  (import '(lisp:&allow-other-keys lisp:&aux lisp:&key
            lisp:&optional lisp:&rest lisp:&body) 
          "SC")) 

(in-package "SC-MAIN")

(export '(sc2c *code-package*))

(require "SC-MISC")
(require "SCT-RULE2LSP")
(require "SCPP")

(use-package "SC-MISC")

(defconstant *scmain-package* *package*)

;;;;; *code-package* ������
(defconstant *code-package* (find-package "SC"))

;;;;;
(defconstant *sc2c-rule-name* "sc2c")

(defun c-indent (c-file &optional (option "-i2"))
  (system (format nil "indent ~a ~a" option c-file))
  (values))

(defun sc2c (x
	     &key
	     ((:rule rule-list) nil)
	     (rule-path sc-transformer:*rule-path*)
	     (intermediate nil)
	     (output-file :default)
	     (package *package*)
	     (indent t)
	     &aux
	     (input-file nil)
	     (*print-case* :downcase))
  "Args: (x &key (rule nil)
              (rule-path sc-transformer:*rule-path*)
              (intermediate nil)
              (output-file :default)
              (indent t))
Compiles SC-program specified by X into C-program.
When OUTPUT-FILE is non nil, write the result to an output file and returns the file name. Otherwise returns the result as a String.
X is an S-expression or a filespec."


  ;;;;; ����S�� => x
  ;; ���ϥե�������Υ���ܥ���ϡ��ޤ�*code-package*����Ͽ����롣
  ;; scpp �Ǥν������ *code-package* �Τޤ�
  ;; ��§���ѷ�����Ȥ��ϡ��Ƶ�§��package����Ͽ���ʤ�����
  ;; �ѷ��塢�����᤹
  (when (or (pathnamep x) (stringp x))
    (setq input-file (or (probe-file x)
			 (probe-file (change-extension x "sc"))))
    (setq x (read-file input-file :package *code-package*)))
  
  ;; ���ϥե�����̾
  (setq output-file
	(if (eq :default output-file)
	    (if input-file
		(change-extension input-file "c")
		"a.c")
	    output-file))
  
  ;; scpp�����
  (scpp:scpp-initialize)

  ;; sc-transform
  (when (stringp rule-list) (setq rule-list (list rule-list)))
  (let ((i 1))
    (dolist (rule rule-list)
      (format *error-output* "~%>>> Applying ~a rule...~%" rule)
      ;; apply scpp
      (setq x (scpp:scpp x))
      ;; apply rule
      (setq x (funcall (sct:get-initiator rule) x :package *code-package*))
      ;; write to intermediate file if requried		       
      (when intermediate
	(write-file (change-extension output-file
				      (format nil "~d.~a.sc" i rule))
		    x :overwrite t :package *code-package*)
	(incf i))))
  
  ;; sc2c
  (format *error-output* "~%>>> Applying ~a...~%" *sc2c-rule-name*)
  (setq x (scpp:scpp x))
  (setq x (funcall (sct:get-initiator *sc2c-rule-name*) 
		   x :package *code-package*))

  ;; output-file
  (if output-file
      (progn
	(write-file output-file x :overwrite t :write-string t 
		    :package *code-package*)
	(when indent (c-indent output-file))
	output-file)
      x))
  

  