;;; �ޥ����Ŭ��
;;; 2003/07/08 include��ǽ���ɲ� ;; (include <�ե�����̾>)
;;; 2003/11/17 �ޥ���̾��lisp�ѥå������ε���Ⱦ��ͤ��ʤ��褦�˲���

(defvar *macro-symbol-alist* nil)
(defvar *sc2c-macro-package* 'sc2c-macro-package)
(defvar *replaced* nil)

(defun apply-macro (x)
  (let ((macp (find-package *sc2c-macro-package*)))
    (unless macp
      (make-package *sc2c-macro-package*)))
  
  (setq *macro-symbol-alist* nil)
  (push nil x)

  ;; defmacroʸ���ܤ��Ф��ƥޥ������Ͽ
  ;; includeʸ���ܤ��Ф��ƥե��������Ƥ��֤�����
  (do ((y x))
      ((endp (cdr y)) (setq x (cdr x)))
    (let ((ye (cadr y)))
      (cond ((and (listp ye)  ; defmacro
		  (eq 'defmacro (car ye)))
	     (entry-macro ye)
	     (rplacd y (cddr y)))
	    ((and (listp ye)  ; include
		  (eq 'include (car ye)))
	     (let ((fname (second ye)) (xx nil))
	       (with-open-file (istream fname :direction :input)
		 (do ((yy (read istream nil 'eof) (read istream nil 'eof)))
		     ((eq yy 'eof) (setq xx (nreverse xx)))
		   (push yy xx)))
	       (rplacd (last xx) (cddr y))
	       (rplacd y xx)))
	    (t                ; otherwise
	     (setq y (cdr y))))))
  (loop
   (setq *replaced* nil)
   (replace-macro x)
   (unless *replaced* (return x)))) ;�����ִ����Ԥ��Ƥʤ���н�λ

(defun entry-macro (dm &aux macname)
  (unwind-protect
       (progn
	 (in-package *sc2c-macro-package*)
	 (shadow (intern (symbol-name (second dm))))
	 (setq macname (intern (symbol-name (second dm))))
	 (eval `(,(car dm) ,macname ,@(cddr dm))))
    (in-package *sc2c-package*))
  (push (cons (second dm) macname) *macro-symbol-alist*))
  
(defun replace-macro (x &aux macname)
  (do ((y x (cdr y)))
      ((endp y) x)
    (let ((ye (car y)))
      (when (listp ye)
	(if (setq macname (cdr (assoc (first ye) *macro-symbol-alist*)))
	    (unwind-protect
		 (progn
		   (setq *replaced* t)
		   (in-package *sc2c-macro-package*)
		   (rplaca y (macroexpand `(,macname ,@(cdr ye)))))
	      (in-package *sc2c-package*))
	    (replace-macro ye))))))


  
      
      