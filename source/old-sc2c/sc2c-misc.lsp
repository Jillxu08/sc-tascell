;;;; ������ͭ�Ѥʴؿ�
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(provide 'sc2c-misc)


;;; debug-print
(defmacro dprint (&rest sym-list)
  `(print
    (list ,@(mapcan #'(lambda (x)
			(list `',x x))
		    sym-list))))

;;; print-and-break
(defmacro bprint (&rest print-arg)
  `(prog1
       (print ,@print-arg)
     (break)))

;;; Ĺ��n�ʾ�Υꥹ�ȤϾ�ά����print
(defun abbrev-print (object &optional (n 5) (stream *standard-output*))
  (print (abbrev-list object n) stream))

;;; ���ξ�ά��������
(defun abbrev-list (object &optional (n 5))
  (if (listp object)
      (progn
	(when (< n (length object))
	  (setq object (append (firstn object n) '(<...>))))
	(mapcar #'abbrev-list object))
      object))

;;; ʣ�����push����٤˽�
(defmacro pushs (&rest args &aux (place (car (last args))))
  `(progn
     ,@(mapcar #'(lambda (x)
		   `(push ,x ,place))
	       (butlast args))))

;;; �ꥹ�Ȥ���Ƭn���Ǥ����
(defun firstn (x &optional (n 1) &key (func #'nreverse))
  (labels ((firstn-tail (x n acc)
	     (if (or (= n 0) (null x))
		 acc
		 (firstn-tail (cdr x) (1- n) (cons (car x) acc)))))
    (funcall func (firstn-tail x n nil))))

;;; �ꥹ�Ȥ�����n���Ǥ����
(defun lastn (x &optional (n 1) &key (func #'identity))
  (funcall func (firstn (reverse x) n :func #'identity)))

;;; �ꥹ�Ȥ�����n���Ǥ����
(defun butlastn (x &optional (n 1))
  (firstn x (- (length x) n)))


;;; list �� p ���ؤ����֤�ľ���ޤǤ���ʬ�򥳥ԡ�
(defun list-until (list p)
  (labels ((l-u-tail (list acc)
	     (if (eq list p)
		 acc
		 (l-u-tail (cdr list) (cons (car list) acc)))))
    (reverse (l-u-tail list nil))))

;;; �����оݥꥹ�Ȥ����Ǥ��ꥹ�Ȥʤ餽����Ȥ⸡������member
(defun member-ex (item list &key (key #'identity) (test #'eql) test-not)
  (let (ret)
  (if (setq ret (member item list :key key :test test :test-not test-not))
      ret
      (dolist (el list nil)
	(when (and (listp el)
		   (setq ret (member-ex item el :key key 
					:test test :test-not test-not)))
	  (return-from member-ex ret))))))


;((a b c)(x y z)(m n o)...)->((a x m)(b y n)(m n o) ...) 
(defun combine-each-nth (lst &key (rev t) (n (length (car lst))))
  (labels ((c-e-n-tail (lst acc)
	     (if (endp lst)
		 acc
	       (c-e-n-tail
		(cdr lst)
		(let ((ithcdr-el
		       (firstn (car lst) n)))
		  (mapcar #'(lambda (x)
			      (prog1
				  (cons (car ithcdr-el) x)
				(setq ithcdr-el (cdr ithcdr-el))))
			  acc))))))
    (mapcar
     (if rev #'reverse #'identity)
     (c-e-n-tail lst (make-list n)))))

;;; make ((a . 1) (b . 2)) from (a b) (1 2 3)
(defun cmpd-list (a b &aux ab)
  (unless (and (listp a) (listp b))
    (error "~s or ~s is not list" a b))
  (do ((aa a (cdr aa)) (bb b (cdr bb)))
      ((endp aa) (setq ab (nreverse ab)))
    (push `(,(car aa) . ,(car bb)) ab)))      

;;; compose functions
(defun compose (&rest fns)
  (if fns
      (let ((fn1 (car (last fns)))
	    (fns (butlast fns)))
	#'(lambda (&rest args)
	    (reduce #'funcall fns
		    :from-end t
		    :initial-value (apply fn1 args))))
    #'identity))

;;; �����nil���Ǥ������Ǥ���nil���֤�mapcar
;;; multiple-value �б�
(defun check-mapcar (func list)
  (catch 'suspend
    (apply 
     #'values
     (combine-each-nth
      (mapcar 
       #'(lambda (x &aux (fx (multiple-value-list (funcall func x))))
	   (if (car fx)
	       fx 
	       (throw 'suspend nil)))
       list)))))

;;;; other iterations
(defmacro while (test &body body)
  `(do ()
    ((not ,test))
    ,@body))

(defmacro till (test &body body)
  `(do ()
    (,test)
    ,@body))

(defmacro for ((var start stop) &body body)
  (let ((gstop (gensym)))
    `(do ((,var ,start (1+ ,var))
	  (,gstop ,stop))
      ((> ,var ,gstop))
      ,@body)))

;;;; Anaphoric Variants
(defmacro aif (test-form then-form &optional else-form)
  `(let ((it ,test-form))
    (if it ,then-form ,else-form)))

(defmacro awhen (test-form &body body)
  `(aif ,test-form
    (progn ,@body)))

(defmacro aunless (test-form &body body)
  `(aif ,test-form
    nil
    (progn ,@body)))

(defmacro awhile (expr &body body)
  `(do ((it ,expr ,expr)) ((not it))
    ,@body))

(defmacro aand (&rest args)
  (cond ((null args) t)
	((null (cdr args)) (car args))
	(t `(aif ,(car args) (aand ,@(cdr args))))))

(defmacro acond (&rest clauses)
  (if (null clauses)
      nil
      (let ((cl1 (car clauses))
	    (sym (gensym)))
	`(let ((,sym ,(car cl1)))
	  (if ,sym
	      (let ((it ,sym)) ,@(cdr cl1))
	      (acond ,@(cdr clauses)))))))