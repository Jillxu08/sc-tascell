;;;; ��§�ե����� -> lsp�ե����� ����ѥ���
(provide "SCT-RULE2LSP")

(unless (find-package "SC-TRANSFORMER")
  (make-package "SC-TRANSFORMER" 
                :use '("LISP")
                :nicknames '(sct)))
(in-package "SC-TRANSFORMER")

(export '(rule2lsp make-match-func require-rule get-initiator))

(require "SCT")
(require "SC-MAIN")
(require "SC-MISC")

(use-package "SC-MISC")

;;;;; �ѷ���§���Ȥ�����
(defvar *rule-package* nil) ; rule�ե�����Υե�����̾������
(defvar *requirement* '("SC-MISC" "SC-UTIL" "SCT-LIB"))    ; %require
(defvar *using-package* '("SC-TRANSFORMER" "SC-MISC"       ; %use-package
			  "SC-UTIL"))                      
(defvar *otherwise-action* '(error "No pattern matched.")) ; %otherwise
(defvar *initial-rule* nil)                                ; %initial-rule
;;;;;
(defconstant *init-function-name* "INITIATE")
(defconstant *arg-name* "X")
(defconstant *block-label* 'pattern-matched)
;;;;;

;; Ϳ����줿��§̾���б�����rule�ե������lisp�ץ����˥���ѥ���
;; �֤��ͤϽ��ϥե�����̾
(defun rule2lsp (rule-name
		 &optional (search-path *rule-path*)
		 &aux input-file output-file rule-list ret
		 (*rule-package* nil)
		 (*requirement* *requirement*)
		 (*using-package* *using-package*)
		 (*otherwise-action* *otherwise-action*)
		 (*initial-rule* nil))
  ;; ����/���ϥե�����̾
  (unless (setq input-file (get-rule-rulefile rule-name))
    (error "Can't find rule-file for ~a" rule-name))
  (setq output-file (change-extension input-file "rule.lsp"))
  
  ;; �ѥå�����̾ == ���ϥե�����̾
  (setq *rule-package* (get-rule-package rule-name))
  (renew-rule-package-environment)
  
  ;; ��§�ե������ɤ߹���
  (format t "Reading ~A...~%" input-file)
  (setq rule-list (read-rule input-file))
  
  ;; �������� => ret
  (format t "Compiling ~a...~%" input-file)
  (let*
      ((sym-rpackage (immigrate-package '*rule-package* *rule-package*))
       (rule-id-list (mapcar #'car rule-list))
       (ext-rule-id-list (mapcar #'make-ext-rule-id rule-id-list))
       (ret 
	`(;; provide, in/use-package, module, constant
	  (provide ,rule-name)
	  (in-package ,rule-name)
	  (export ',ext-rule-id-list)
	  ,@(mapcar #'(lambda (x) `(require ,@(mklist x))) *requirement*)
	  (defconstant ,sym-rpackage *package*)
	  ,@(mapcar #'(lambda (x) `(use-package ,x)) *using-package*)
	  ;; initial function
	  (defun ,(intern *init-function-name* *rule-package*) 
	      (x &key (package *package*))
	    (,(make-ext-rule-id *initial-rule*) x :package package))	    
	  ;; rule called from external
	  ,@(mapcar
	     #'(lambda (rule-id ext-rule-id)
		 `(defun ,ext-rule-id (x &key (package *package*)
				       &aux (*package* ,sym-rpackage))
		   (immigrate-package 
		    (,rule-id (immigrate-package x ,sym-rpackage package))
		    package ,sym-rpackage)))
	     rule-id-list ext-rule-id-list)		
	  ;; compiled functions
	  ,@(mapcar 
	     #'(lambda (rule &aux (rname (car rule)))
		 `(defun ,rname (,(intern *arg-name* *rule-package*))
		   (block ,*block-label*
		     ,@(mapcar #'compile-match-pattern (cdr rule))
		     (return-from ,*block-label* ,*otherwise-action*) 
		     )))
	     rule-list)))
       
       (*print-case* :downcase))
    
    (write-file output-file ret :overwrite t :package *rule-package*))
  
  output-file
  )

;;; *rule-package* �δĶ���ºݤ�load����Ʊ���ˤ���
(defun renew-rule-package-environment ()
  (let ((*package* *rule-package*))
    (mapcar #'(lambda (module)
		(apply #'require (mklist module)))
	    *requirement*)
    (mapcar #'use-package *using-package*))
  (values))

;;; ��������θƤӽФ��Ѥε�§symbol
;;; rule -> @rule
(defun make-ext-rule-id (rule-id)
  (intern (string+ "@" (symbol-name rule-id)) *rule-package*))

;;;; ��§�ե������ɤ߹���(rule2lsp�Ѥ˻���Ū�����ѹ�)
;;; �֤��ͤ� list of ( <rulename> . <list of ( <pattern> . <action> )> )
(defun read-rule (rule-file
		  &aux (rule-list nil) (input-data nil)
		  (arrow (intern "->" *rule-package*)))
  (with-open-file (istream rule-file :direction :input)
    (let ((*package* *rule-package*))
      ;; read file to make input-data and to set environment
      (do ((y (read-pattern istream nil 'eof) 
	      (read-pattern istream nil 'eof))
	   (patlist nil))
	  ((eq y 'eof) (setq input-data (nreverse input-data)))
	(cond
	  ;; set environment
	  ((and (listp y)
		(symbolp (car y))
		(char= #\% (aref (symbol-name (car y)) 0)))
	   (case (immigrate-package (car y) *sctrans-package*)
	     ((%require)
	      (let ((pathname 
		     (aif (third y)
			  it
			  (let ((modstr (string-downcase
					 (string (second y)))))
			    (let ((file-list
				   (mapcar 
				    #'(lambda (ext)
					(format nil "~a~a" modstr ext))
				    '("" ".o" ".lsp")))
				  (directory-list
				   `(,(pathname-directory rule-file)
				     (:current)
				     ,@*rule-path*)))
			      (car 
			       (remove-if-not
				#'probe-file
				(mapcar
				 #'(lambda (dir-file)
				     (make-pathname :directory (first dir-file)
						    :name (second dir-file)))
				 (make-all-comb directory-list
						file-list)))))))))
		(setq *requirement*
		      (append1 *requirement* `(,(second y) ,pathname))))
	      (renew-rule-package-environment))
	     ((%use-package) 
	      (setq *using-package*
		    (append1 *using-package* (second y)))
	      (renew-rule-package-environment))
	     ((%otherwise)
	      (setq *otherwise-action* 
		    `(quote ,(second y))))
	     ((%initial-rule) 
	      (setq *initial-rule* (second y)))
	     (otherwise
	      (error "undefined %-command ~A" (car y)))))
	      
	  ;; (rule <pattern>)
	  ((listp y)
	   (setq patlist
		 (append patlist (reduce #'append (extract-optional y)
					 :from-end t))))
	  ;; ->
	  ((eq arrow y)
	   (progn
	     ;; <action>��read
	     (setq y (read istream))
	     ;; "<pat1> <pat2> -> <action>" 
	     ;; ==> "<pat1> -> <action> <pat2> -> <action>"
	     (dolist (pat patlist)
	       (pushs pat arrow y input-data))
	     (setq patlist nil)))
	  (t
	   (error "syntax-error: ~A" y))))))
    
  ;; rule-list ������
  (do* ((xx input-data (cdr xx))
	(x (car xx) (car xx))
	(md 0 (rem (1+ md) 3))
	(p nil) (pat nil))
       ((endp xx)
	(progn
	  (setq rule-list (nreverse rule-list))
	  (dolist (rule rule-list rule-list)
	    (rplacd rule (nreverse (cdr rule))))))
    (case md
      ((0) ; read rule-name, pattern and arguments
       (unless (consp x)
	 (error "syntax error at ~s" x))
       (unless (symbolp (car x))
	 (error "~s is not rule name" (car x)))
       (unless (setq p (member (car x) rule-list :key #'car :test #'eq))
	 (setq p (push `(,(car x)) rule-list)))
       (setq pat (cdr x)))
      ((1) ; read symbol '->'
       (unless (eq arrow x)
	 (setq md 2)))
      ((2) ; read function value
       (rplacd (car p) (cons `(,pat . ,x) (cdar p))))
      (otherwise 
       (error "internal error:unexpected value of md"))))

  (unless *initial-rule*
    (setq *initial-rule* (caar rule-list)))
  
  rule-list
  )

;; pat = ((pattern ,@rest-arg) . action)
(defun compile-match-pattern (pat
			      &aux
			      (pattern (caar pat))
			      (rest-arg (cdar pat))
			      (action (cdr pat)))
  (make-matching-exp pattern (intern *arg-name* *rule-package*)
		     `(return-from ,*block-label* ,action))
  )

;; �ѥ�����Υѥ����ѿ�̾�Υꥹ�Ȥ�����
(defun get-pattern-variable (&optional (pat (read-pattern)))
  (labels ((gpv-acc (pat acc)
	     (cond
	       ((not (listp pat)) acc)
	       ((null pat) acc)
	       ((or (eq comma (car pat))
		    (eq comma-at (car pat)))
		(list (second pat)))
	       (t
		(gpv-acc (cdr pat)
			 (append (gpv-acc (car pat) nil) acc))))))
    (gpv-acc pat nil)))

;; pattern0 �� arg-symbol ���ͤ� match ���뤫�ɤ�����Ƚ�ꤷ��
;; �ѥ����ѿ��˥ޥå������ͤ򥻥åȡ�
;; matched-symbol �� <list of (<symbol> . <retval-list>) >)
;; �ˤʤ�褦�ˤ����Ķ���
;; �ޥå����Ƥ���� then-form
;; �ޥå����ʤ���� else-form ���֤�������
(defun make-matching-exp (&optional
			  (pattern0 (read-pattern)) 
			  (arg-symbol 'x)
			  (then-form t) (else-form nil))
  (labels 
      ;; ����Ҵؿ����֤��ͤ� (<check-s> <check-c>)
      ;; check-s : �ޥå����뤿��˿��ˤʤ�٤����Υꥹ��
      ;;           (�ꥹ�Ȥι�¤�˴ؤ�����)
      ;; check-c : �ޥå����뤿��˿��ˤʤ�٤����Υꥹ��
      ;;           ([...]�ǻ��ꤵ�줿�ؿ�)
      ;; �Ǹ�� check-s, check-c �� and �ǤĤʤ����֤�
      ;; (��ñ�ˤǤ�������å�����˹Ԥ���[..]��δؿ���ɾ���Ϥʤ�٤����)
      ;; focus : arg-symbol �θ��ߤ�������ʬ�����뤿��μ�
      ;; rest  : arg-symbol �λĤ����ʬ�����뤿��μ�
      ((matched-symbol-renewing-exp (symbol newcdr)
	 `(rplacd (find ',symbol matched-symbol :key #'car) ,newcdr))
       (combine-check-list (list-of-s-c)
	 `(and ,@(reverse (first list-of-s-c)) ,@(reverse (second list-of-s-c))))
       ;;;
       (make-matching-1exp (pattern check-s check-c focus)
	 (if (listp pattern)
	     (cond
	       ;; ,<symbol>
	       ((eq comma (car pattern))
		(let ((sname (second pattern)))
		  (list
		   check-s
		   (cons `(progn (setq ,sname ,focus) t)
			 (aif (third pattern)
			      (cons `(let ((retval (multiple-value-list 
						    (,it ,focus))))
				      (and (car retval)
				       ,(matched-symbol-renewing-exp
					 sname 'retval)))
				    check-c)
			      check-c))))
		)
	       ;; ,@<symbol> -> error
	       ((eq comma-at (car pattern))
		(error "~a: pattern consists of only ,@<symbol>" 
		       (pattern-print pattern0))
		)
	       ;; <list>
	       (t
		(make-matching-list
		 pattern
		 (cons `(listp ,focus) check-s)
		 check-c
		 focus
		 )))
	     ;; atom
	     (list (cons (if (symbolp pattern)
			     ;; <symbol>
			     `(eq ',pattern ,focus)
			     ;; otherwise(number,string, etc.)
			     `(equal ,pattern ,focus))
			 check-s)
		   check-c)
	     ))
       ;;;;;;
       (make-matching-list (pattern check-s check-c rest)
	 (cond 
	   ((endp pattern)
	    (list (cons `(endp ,rest) check-s)
		  check-c))
	   ;; (,@<symbol> ...)
	   ((and (listp (car pattern))
		 (eq comma-at (caar pattern)))
	    (let ((func-checkrest
		   `#'(lambda (y) 
			,(combine-check-list
			  (make-matching-list (cdr pattern) nil nil 'y)))))
	      (list check-s
		    (cons
		     (let ((sname `,(second (car pattern)))
			   (testfunc (aif (third (car pattern))
					   `#',it)))
		       `(let ((crest ,rest))
			 (loop
			  (when (funcall ,func-checkrest crest)
			    (let ((prev (ldiff ,rest crest)))
			      ,(if testfunc
				   `(awhen 
				     (or
				      (and (endp prev) (list nil))
				      (multiple-value-list
				       (check-mapcar ,testfunc prev)))
				     (setq ,sname prev)
				     ,(matched-symbol-renewing-exp
				       sname 'it)
				     (return t))
				   `(progn 
				     (setq ,sname prev)
				     (return t)))))
			  (progn
			    (if (endp crest)
				(return nil)
				(setq crest (cdr crest)))))))  
		     check-c))))
	   ;; ����ʹ�,@���ʤ�����Ƚ�����ǲ�
	   ((not (member comma-at pattern
			 :key #'(lambda (pat1)
				  (and (listp pat1) (car pat1)))))
	    (let* ((pat-l (length pattern))
		   (map-make (mapcar 
			      #'(lambda (i)
				  (make-matching-1exp
				   (nth i pattern) nil nil `(nth ,i ,rest)))
			      (integer-list (1- pat-l) 0 -1)))
		   (check-s-add (reduce #'append (mapcar #'first map-make)
					:from-end t))
		   (check-c-add (reduce #'append (mapcar #'second map-make)
					:from-end t)))
	      (list `(,@check-s-add 
		      (= ,pat-l (length ,rest))
		      ,@check-s)
		    (append check-c-add check-c))))
	   ;; genelal list
	   (t
	    (let* ((make1 (make-matching-1exp (car pattern)
					      `(,rest) nil `(car ,rest)))
		   (check-s1 (first make1))
		   (check-c1 (second make1)))
	      (make-matching-list 
	       (cdr pattern)
	       (append check-s1 check-s)
	       (append check-c1 check-c) 	
	       `(cdr ,rest))))
	    )))
    ;;;;;
    (let ((pv-list (get-pattern-variable pattern0)))
      `(let (,@pv-list
	     (matched-symbol 
	      (list ,@(mapcar #'(lambda (pv) `(list ',pv)) pv-list))))
	(if ,(combine-check-list
		(make-matching-1exp pattern0 nil nil arg-symbol))
	    ,then-form
	    ,else-form)))
    ))



;;; rule ���б����� .rule.lsp rule.o �Τ����ǿ��Τ�Τ�load
;;; .rule ���ǿ��ξ��ϥ���ѥ��뤷�Ƥ���load
(defun require-rule (rule-name &key (force-load nil))
  (flet ((fname-date (get-func)
	   (aif (funcall get-func rule-name)
		(cons it (file-write-date it))
		nil)))
    (let ((rule-file (fname-date #'get-rule-rulefile))
	  (lisp-file (fname-date #'get-rule-lspfile))
	  (fasl-file (fname-date #'get-rule-ofile))
	  (load-func #'(lambda (pathname)
			 (if force-load
			     (load pathname)
			     (require rule-name pathname)))))
      (unless (or rule-file lisp-file fasl-file)
	(error "Can't find ~s rule" rule-name))
      (let (;; �Ǹ�˥ե����뤬�������줿����¤��ؤ�
	    (file-prio-list
	     (sort (list rule-file lisp-file fasl-file)
		   #'(lambda (x y)
		       (cond 
			 ((and x y) (> (cdr x) (cdr y)))
			 (x t)
			 (t nil))))))
	(if (eq rule-file (first file-prio-list))
	    (funcall load-func (rule2lsp rule-name))
	    (funcall load-func (car (first file-prio-list))))))))

;;;; ��§̾���б�����ƥ⥸�塼��
;; ��§�ե�����
(defun get-rule-rulefile (rulename)
  (path-search (change-extension rulename "rule") *rule-path*))
;; lisp����ѥ���ѵ�§�ե�����
(defun get-rule-lspfile (rulename)
  (path-search (change-extension rulename "rule.lsp") *rule-path*))
;; fasl����ѥ���ѵ�§�ե�����
(defun get-rule-ofile (rulename)
  (path-search (change-extension rulename "rule.o") *rule-path*))
;; �ѥå�����
(defun get-rule-package (rulename)
  (find-package2 rulename))
;; ��§�ƽФ��Ѵؿ�
(defun get-initiator (rule-name)
  (require-rule rule-name)
  (symbol-function (find-symbol *init-function-name* 
				(get-rule-package rule-name))))



































;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; ���ޤ�
;;;;;;;;;;;;;
;; pattern0 �� *arg-name* �� match ���뤫�ɤ�����Ƚ�ꤹ��ؿ�����
;; pattern -> ( x -> {nil | <bind-list>} )
(defun make-match-func (&optional (pattern0 (read-pattern)))
  (labels 
      ;; ����Ҵؿ����֤��ͤ� (<check-s> <check-c>)
      ;; check-s : �ޥå����뤿���ɬ�פʥ������Υꥹ��
      ;;           (�ꥹ�Ȥι�¤�˴ؤ�����)
      ;; check-c : �ޥå����뤿���ɬ�פʥ������Υꥹ��
      ;;           ([...]�ǻ��ꤵ�줿�ؿ�)
      ;; get-focus : x �θ��ߤ�������ʬ�����뤿��δؿ�
      ;; get-rest  : x �λĤ����ʬ�����뤿��δؿ�
      ((make-match-func-1exp (pattern check-s check-c get-focus)
	 (if (listp pattern)
	     (cond
	       ((eq comma (car pattern))
		(list
		 (cons (compose
			(aif (third pattern)
			     #'(lambda (x)
				 (let ((retval-list
					(multiple-value-list
					 (funcall it (second x)))))
				   (when (car retval-list)
				     (list (append x retval-list)))))
			     #'list)
			#'(lambda (x)
			    (list (second pattern) 
				  (funcall get-focus x))))
		       check-s)
		 check-c)
		)
	       ;;
	       ((eq comma-at (car pattern))
		(error "~a: pattern consists of only ,@<symbol>" 
		       (pattern-print pattern0))
		)
	       ;;
	       (t
		(make-match-func-list
		 pattern
		 (cons (compose #'listp get-focus) check-s)
		 check-c
		 get-focus
		 )))
	     ;;
	     (list (cons (compose #'(lambda (x) (symbol= pattern x))
				  get-focus)
			 check-s)
		   check-c)
	     ))
       ;;;;;;
       (make-match-func-list (pattern check-s check-c get-rest)
	 (cond 
	   ((endp pattern)
	    (list (cons (compose #'endp get-rest) check-s)
		  check-c))
	   ;;
	   ((and (listp (car pattern))
		 (eq comma-at (caar pattern)))
	    (let ((matchrest (make-match-func (cdr pattern))))
	      (list check-s
		    (cons #'(lambda (x 
				     &aux 
				     (rest0 (funcall get-rest x))
				     (rest rest0)
				     (sname (second (car pattern)))
				     (testfunc (third (car pattern))))
			      (loop
			       (awhen (funcall matchrest rest)
				 (let ((prev (ldiff rest0 rest))
				       (retval-list nil))
				   (when (or (not testfunc)
					     (car 
					      (setq retval-list
						    (multiple-value-list
						     (check-mapcar testfunc
								   prev)))))
				     (return 
				       (cons `(,sname ,prev ,@retval-list)
					     (remove 'x it :key #'car))))))
			       (progn
				 (if (endp rest)
				     (return nil)
				     (setq rest (cdr rest))))))  
			  check-c))))
	   ;;
	   (t
	    (let* ((make1 (make-match-func-1exp
			   (car pattern)
			   nil nil
			   (compose #'car get-rest)))
		   (check-s1 (first make1))
		   (check-c1 (second make1)))
	      (make-match-func-list
	       (cdr pattern)
	       (cons (compose #'not #'null get-rest)
		     (append check-s1 check-s))
	       (append check-c1 check-c)
	       (compose #'cdr get-rest)))))))
    ;;;;;	   
    (let ((ret (make-match-func-1exp pattern0 nil nil #'identity)))
      #'(lambda (x)
	  (awhen
	   (check-mapcar
	    #'(lambda (f) (funcall f x))
	    (append (reverse (first ret)) (reverse (second ret))))
	   (cons `(x ,x) 
		 (reduce #'append (remove-if-not #'listp it)
			 :from-end t)))))))
