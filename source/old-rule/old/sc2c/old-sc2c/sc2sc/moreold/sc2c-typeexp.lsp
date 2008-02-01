(defvar *type-specifier-list*
  '((void "void") (char "char") (signed-char "signed char") 
    (unsigned-char "unsigned char") (short "short")
    (signed-short "signed short") (unsigned-short "unsigned short")
    (int "int") (signed-int "signed int") (unsigned-int "unsigned int")
    (long "long") (signed-long "signed long") (unsigned-long "unsigned long")
    (long-long "long long") (signed-long-long "signed long long")
    (unsigned-long-long "unsigned long long") (float "float")
    (double "double") (long-double "long double")))

;;; x:list
;;; $BJVCM!'(B(str1 str2 str3) 
;;;     str1 --- $B@k8@$N@hF,$KCV$/J8;zNs(B
;;;     str2 --- $B<1JL;R$ND>A0$KCV$/J8;zNs(B
;;;     str3 --- $B<1JL;R$ND>8e$KCV$/J8;zNs(B
(defun type-expression (x &aux (str (list "" "" "")) ret1 ret2)
  (cond
    ((setq ret1 (type-specifier x))                                 ;;;t-spcf
     (setf (first str) ret1))
    ((not (listp x)) (return-from type-expression nil))
    ((and (setq ret1 (type-qualifier-list (butlast x 1)))
	  (setq ret2 (type-expression (car (last x)))))             ;;;t-qual
     (setq str ret2)
     (setf (first str) (concatenate 'string (string+ ret1 " ") 
				    " " (first str))))
    ((and (eq (car x) 'array)
	  (setq ret1 (type-expression (cadr x)))
	  (or (endp (cddr x))
	      (setq ret2 (array-subscription-list (cddr x)))))      ;;;array
     (setq str ret1)
     (if (endp (cddr x)) (setq ret2 "[]"))
     (setf (third str) (concatenate 'string ret2 (third str))))
    ((and (eq (car x) 'ptr)
	  (setq ret1 (type-expression (cadr x))))                   ;;;ptr
     (setq str ret1)
     (setf (second str) (concatenate 'string (second str) "(*"))
     (setf (third str) (concatenate 'string ")" (third str))))
    ((and (eq (car x) 'fn)
	  (setq ret1 (function-type-list (cdr x))))                 ;;;fn  
     (setf (first str) (first (first ret1))
	   (second str) (second (first ret1))
	   (third str) (concatenate 'string "( "
				    (string+
				     (mapcar 'make-declstr (cdr ret1)) ", ")
				    ")" (third (first ret1)))))
     (t (setq str nil)))
  str)
  


;;; x:list
;;; $BJVCM(B:type-expression-list + "..."_opt
(defun function-type-list (x &aux ret1 ret2)
  (if (listp x)
      (if (eq (car (last x)) 'va-arg)
	  (setq ret2 (list "...")
		ret1 (type-expression-list (butlast x 1)))
	  (setq ret2 nil
		ret1 (type-expression-list x)))
      (return-from function-type-list))
  (if ret1
      (nconc ret1 ret2)))

;;; x:list
;;; $BJVCM(B:type-expression$B$NJVCM$N%j%9%H(B
(defun type-expression-list (x &aux (ret1 nil) ret2)
  (if (listp x)
      (dolist (a x ret1)
	(if (setq ret2 (type-expression a))
	    (setq ret1 (nconc ret1 (list ret2)))
	    (return nil)))))


;;: x:list
;;; $BJVCM(B:C$B%3!<%IJ8;zNs(B
(defun type-specifier (x &aux ret)
  (cond
    ((setq ret (cadar (member x *type-specifier-list* :key 'car))) ret)
    ((setq ret (struct-or-union-specifier x)) ret)
    ((setq ret (enum-specifier x)) ret)
    ((setq ret (typedef-name x)) ret)
    (t nil)))

;;; x:list
;;; $BJVCM(B:C$B%3!<%IJ8;zNs(B
(defun array-subscription-list (x &aux ret)
  (if (setq ret (expression-list x))
      (string+ ret "][" "[" "]")))
  
;;; x:list
;;; $BJVCM(B:C$B%3!<%IJ8;zNs(B
(defun struct-or-union-specifier (x &aux ret1 ret2)
  (if (and (listp x)
	   (setq ret1 (struct-or-union (car x)))
	   (setq ret2 (identifier (cadr x)))
	   (endp (cddr x)))
      (concatenate 'string ret1 " " ret2)))

;;; x:list
;;; $BJVCM(B:C$B%3!<%IJ8;zNs(B
(defun struct-or-union (x)
  (case x
    ('struct "struct")
    ('union "union")
    (otherwise nil)))

;;; x:list
;;; $BJVCM(B:C$B%3!<%IJ8;zNs(B
(defun enum-specifier (x &aux ret)
  (if (and (listp x)
	   (eq (car x) 'enum)
	   (setq ret (identifier (cadr x)))
	   (endp (cddr x)))
      (concatenate 'string "enum " ret)))
	      
;;; x:list
;;; $BJVCM(B:C$B%3!<%IJ8;zNs$N%j%9%H(B
(defun type-qualifier-list (x &aux (ret1 nil) ret2)
  (if (listp x)
      (dolist (a x ret1)
	(if (setq ret2 (type-qualifier a))
	    (setq ret1 (nconc ret1 (list ret2)))
	    (return nil)))))

;;; x:list
;;; $BJVCM(B:C$B%3!<%IJ8;zNs(B
(defun type-qualifier (x)
  (case x
    ('const "const")
    ('restrict "restrict")
    ('volatile "volatile")
    (otherwise nil)))

;;; x:list
;;; $BJVCM(B:C$B%3!<%IJ8;zNs(B
(defun typedef-name (x)
  (identifier x))
	  