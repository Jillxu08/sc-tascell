
;;;
;;; $B@k8@J8$rA4$F4X?tDj5A$N@hF,$K0\F0$5$;$k(B
;;; input:translation-unit output:translation-unit
;;; (2003/5/29$BDI5-!'9=B$BN(B->$B9=B$BN$N$H$-$N(Bversion)
;;;

(defvar *local-id*)
(defvar *sync-id*)

    
;; function->hoisting$B7k2L(B
;; other->$B%V%m%C%/Cf$N@k8@J8$N=|5n!#I,MW$J$i<1JL;RJQ99(B
;; $BJVCM(B:$B=|5n$5$l$?@k8@J8$N%j%9%H(B 
(defun hoisting (s)
  (typecase s
    (decl-function (hs-decl-function s))
    (compound-stat (hs-compound-stat s))
    (label-name    (hs-label-name s))
    (if-stat       (hs-if-stat s))
    (switch-stat   (hs-switch-stat s))
    (while-stat    (hs-while-stat s))
    (do-while-stat (hs-do-while-stat s))
    (for1-stat     (hs-for1-stat s))
    (for2-stat     (hs-for2-stat s))
    (otherwise nil)))

;; $B@k8@J8Cf$N@k8@<1JL;R%j%9%H$rJV$9(B
(defun get-decl-id (s &aux idl)
  (setq idl
	(typecase s
	  (decl-variable  (decl-variable-id s))
	  (decl-function  (decl-function-id s))
	  (decl-struct    (decl-struct-id s))
	  (decl-enum  
	   (cons (decl-enum-id s)
		 (mapcar 'enumerator-id (decl-enum-elist s))))
	  (decl-defs      (decl-defs-id-list s))
	  (typedef-texp   (typedef-texp-id s)) 
	  (typedef-struct (typedef-struct-id s))
	  (typedef-enum   
	   (cons (typedef-enum-id s)
		 (mapcar 'enumerator-id (typedef-enum-elist s))))
	  (othewise nil)))
  (unless (listp idl) (setq idl (list idl)))
  idl)


;; $B@k8@J8$r<u$1<h$j!"(B
;; $B>e$K0\F0$5$;$k@k8@J8$N%j%9%H(B & $BJQ?t=i4|2=MQ$NBeF~<0(B($BJ8(B)$B$N%j%9%H(B  $B$rJV$9(B
;; $B@k8@J8$G$J$$>l9g$O!"(Bnil & '($B85$NJ8(B)  $B$rJV$9(B
(defun split-decl (s &aux val1 val2)
  (typecase s
    ((decl-function decl-struct decl-enum
		    typedef-texp typedef-struct typedef-enum)
     (setq val1 `(,s)))
    
    (decl-variable
     (if (or (null (decl-variable-init s))
	     (member "const" (type-exp-qlist (decl-variable-texp s))
		     :test 'string=))
	 (setq val1 `(,s))
	 (let* ((id       (decl-variable-id s))
		(st-class (decl-variable-st-class s))
		(texp     (decl-variable-texp s))
		(init     (decl-variable-init s))
		(bitfield (decl-variable-bitfield s)) ;;probably nil
		(initid   (create-name (string+ `(,id "_init")))))
	   (push initid *local-id*)
	   (setq val1 `(,(make-decl-variable
			  :id       id
			  :st-class st-class
			  :texp     texp
			  :init     init
			  :bitfield bitfield)
			,(make-decl-variable
			  :id       initid
			  :st-class st-class
			  :texp     ((lambda (te)
				       (let ((nte (copy-type-exp te)))
					 (push "const"
					       (type-exp-qlist nte))
					 nte))
				     texp)
			  :init     init
			  :bitfield bitfield))
		 val2 `(,(make-init-expstat id initid init))))))

    (decl-defs
     (let ((st-class  (decl-defs-st-class s))
	   (texp      (decl-defs-texp s))
	   (id-list   (decl-defs-id-list s))
	   (init-list (decl-defs-init-list s))
	   (bitfield  (decl-defs-bitfield s)))
       (do ((idl id-list (cdr idl)) (inl init-list (cdr inl)))
	   ((endp idl))
	 (let ((id (car idl)) (init (car inl)))
	   (multiple-value-bind (v1 v2)
	       (split-decl (make-decl-variable
			    :id id
			    :st-class st-class
			    :texp texp
			    :init init
			    :bitfield bitfield))
	     (setq val1 `(,@val1 ,@v1)
		   val2 `(,@val2 ,@v2)))))))

    (otherwise (setq val2 `(,s))))

  (values val1 val2))

;; $B@k8@;~$N=i4|2=;R$KAjEv$9$kBeF~<0$r@8@.(B
(defvar *aref-list* nil)
(defvar *init-exp-list* nil)
(defvar *decl-id* nil)
(defvar *init-id* nil)

(defun make-init-expstat (*decl-id* *init-id* init)
  (let ((*init-exp-list* nil) (*aref-list* nil))
    (add-init-exp init)
    (make-expression-stat
     :exp (make-exp-exps 
	   :exp-list (nreverse *init-exp-list*)))))

(defun add-init-exp (init)
  (if (eq (initializer-compoundp init) 'array)
      (let ((n 0))
	(dolist (in (initializer-init-list init))
	  (let ((*aref-list* `(,@*aref-list* ,(make-exp-constant
					       :const (write-to-string n)))))
	    (add-init-exp in))
	  (incf n)))
      (let ((exp1 (make-exp-aref
		   :exp (make-exp-identifier :id *decl-id*)
		   :aref-exp-list *aref-list*))     
	    (exp2 (make-exp-aref
		   :exp (make-exp-identifier :id *init-id*)
		   :aref-exp-list *aref-list*)))
	(if (eq 'exp-string (type-of (car (initializer-init-list init))))
	    (push (make-exp-funcall
		   :exp (make-exp-identifier :id "strcpy")
		   :arg-exp-list `(,exp1 ,exp2))
		  *init-exp-list*)
	    (push (make-exp-assignment
		   :op "="
		   :exp1 exp1 
		   :exp2 exp2
		   :asoc-lev 15)
		  *init-exp-list*)))))

;; item-list$B$r@k8@J8$H$=$l0J30$KJ,N%(B
(defun pickout-decl (item-list)
  (let ((dclist nil) (rmlist nil))
    (dolist (bli item-list (values dclist rmlist))
      (multiple-value-bind (dc rm) (split-decl bli)
	(setq dclist `(,@dclist ,@dc)
	      rmlist `(,@rmlist ,@rm))))))

;; $B=EJ#$7$J$$?7$7$$<1JL;RL>$r@8@.(B
(defun create-name (&optional (basename "__sc2c"))
  (let ((n 0) ns (ngname `(,@*local-id* ,@*sync-id*)))
    (unless (member basename ngname :test 'string=)
      (return-from create-name basename))
    (loop
     (setq ns (string+ (list basename (write-to-string n))))
     (unless (member ns ngname :test 'string=)
       (return ns))
     (incf n))))

;; $B4X?t%V%m%C%/$H$N(Bid$B$N=EJ#$r%A%'%C%/$7!"I,MW$J$i?7$7$$L>A0$KCV$-49$(!"(B
;; *sync-id*$B$K(Bid$B$rDI2C$9$k(B
(defun check-id (item-list)
  (do ((il item-list (cdr il))) ((endp il) item-list)
    (let* ((bitem (car il)) (idlist (get-decl-id bitem)))
      (dolist (id idlist)
	(if (member id *local-id* :test 'string=)
	    (let ((newid (create-name id)))
	      (exec-rename il id newid)
	      (push newid *sync-id*))
	    (push id *sync-id*))))))

(defun hs-translation-unit (s)
  (let (*local-id* *sync-id*)
    (do ((edl (translation-unit-edecl-list s) (cdr edl)))
	((endp edl) s)
      (let ((ed (car edl)))
	(setq *sync-id* `(,@*sync-id* ,@(get-decl-id ed)))
	(hoisting ed)))
    s))

(defun hs-decl-function (s)
  (let ((*local-id* `(,@*local-id* ,@*sync-id*)) ;;?
	(*sync-id* nil)
	(arg-id-list (decl-function-arg-id-list s))
	(item-list (decl-function-item-list s))
	(dclist nil) (rmlist nil))
    (setq *sync-id* `(,@arg-id-list 
		      ,@(apply 'append (mapcar 'get-decl-id item-list)))) 
    (multiple-value-setq (dclist rmlist) (pickout-decl item-list))
    (setq *local-id* `(,@*local-id* ,@*sync-id*))	
    (mapc 'hoisting dclist)
    (dolist (bli rmlist)
      (let ((dcl nil))
	(setq dcl (hoisting bli)
	      *local-id* (apply 'append *local-id* (mapcar 'get-decl-id dcl))
	      dclist `(,@dclist ,@dcl))))
    (setf (decl-function-item-list s)
	  (nconc dclist rmlist))
    nil))

;; compound-statement, while, switch $B$J$I$N(Bitem-list$B$KBP$9$k(Bhoisting
;; $BJVCM(B:$B30$N%V%m%C%/$K=P$9(Bdclist & $B8=:_$N%V%m%C%/$K;D$9(Brmlist
(defun hoisting-block (item-list)
  (let ((*sync-id* nil)
	(dclist nil) (rmlist nil))
    (check-id item-list)
    (multiple-value-setq (dclist rmlist) (pickout-decl item-list))
    (setq *local-id* `(,@*local-id* ,@*sync-id*))	
    (mapc 'hoisting dclist)
    (dolist (bli rmlist)
      (let ((dcl nil))
	(setq dcl (hoisting bli)
	      *local-id* (apply 'append *local-id* (mapcar 'get-decl-id dcl))
	      dclist `(,@dclist ,@dcl))))

    (values dclist rmlist)))

(defun hs-compound-stat (s)
  (multiple-value-bind (dclist rmlist)
      (hoisting-block (compound-stat-item-list s))
    (setf (compound-stat-item-list s) rmlist)
    dclist))


(defun hs-label-name (s)
  (hoisting (label-name-stat s)))
  
(defun hs-if-stat (s)
  (let (dclist1 dclist2)
    (setq dclist1 (hoisting (if-stat-then-stat s)))
    (setq dclist2 (hoisting (if-stat-else-stat s)))
    (nconc dclist1 dclist2)))

(defun hs-switch-stat (s)
  (multiple-value-bind (dclist rmlist)
      (hoisting-block (swicth-stat-item-list s))
    (setf (switch-stat-item-list s) rmlist)
    dclist))

(defun hs-while-stat (s)
  (multiple-value-bind (dclist rmlist)
      (hoisting-block (while-stat-item-list s))
    (setf (while-stat-item-list s) rmlist)
    dclist))

(defun hs-do-while-stat (s)
  (multiple-value-bind (dclist rmlist)
      (hoisting-block (do-while-stat-item-list s))
    (setf (do-while-stat-item-list s) rmlist)
    dclist))
 
(defun hs-for1-stat (s)
  (multiple-value-bind (dclist rmlist)
      (hoisting-block (for1-stat-item-list s))
    (setf (for1-stat-item-list s) rmlist)
    dclist))

(defun hs-for2-stat (s)
  (let ((decl-list (for2-stat-decl-list s))
	(exp2 (for2-stat-exp2 s))
	(exp3 (for2-stat-exp3 s))
	(item-list (for2-stat-item-list s))
	cstat dclist rmlist)
    (setq cstat (make-compound-stat
		 :item-list `(,@decl-list
			      ,(make-for1-stat
				:exp1-list nil
				:exp2      exp2
				:exp3      exp3
				:item-list item-list))))
    (setq dclist (hoisting cstat))
    (let* ((cilist (compound-stat-item-list cstat))
	   (ciblast (butlast cilist))
	   (cfor1 (car (last cilist))))
      (setf (for2-stat-decl-list s) (mapcar 'expression-stat-exp ciblast)
	    (for2-stat-exp2 s)      (for1-stat-exp2 cfor1)
	    (for2-stat-exp3 s)      (for1-stat-exp3 cfor1)
	    (for2-stat-item-list s) (for1-stat-item-list cfor1)
	    (for2-stat-for1p s)     t))
    dclist))
