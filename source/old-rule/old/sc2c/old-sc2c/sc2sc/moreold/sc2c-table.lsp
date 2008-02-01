;;;$B8=:_$N%V%m%C%/3,AX(B
(defvar *lev* 0)      
(defun inc-lev ()
  (incf *lev*))
(defun dec-lev ()
  (setf (get-block-index) nil)
  (decf *lev*))

;;;identifeir$B4IM}(B
(defstruct id
  name   ;;;$B<1JL;RL>J8;zNs(B
  object ;;;$B<1JL;R$N<oN`(B 
         ;;;  (type, struct-type, union-type, enum-type, struct(=union),
         ;;;   enum, e-const, variable, function)
  type   ;;;type,variable->texp  struct-type,struct->member-id-list 
         ;;;enum-type,enum->enumeration-identifier-list
         ;;;e-const->nil function->texp-list
         ;;;$B9=B$BN$N%a%s%P$O(B*block-index*$B$HF1MM$K4IM}(B
  lev    ;;;$B@k8@$N9T$o$l$?%V%m%C%/%l%Y%k(B
)  

;;; $B<1JL;R$NC5:w(B
;;; name:$B<1JL;R$NL>A0(B (all t):nil$B$K$9$k$H8=:_$N3,AX$N$_C5:w(B
(defun search-id (name &key (all t))
  (let ((endlv (if all 0 *lev*)))
    (do ((lv *lev* (1- lv)))
	((< lv endlv) nil)
      (let ((*lev* lv) getid)
	(when (setq getid (member name (get-block-index) 
				  :key 'id-name :test 'string=))
	  (return-from search-id (car getid)))))))

;;; identifier$BEPO?(B
(defun add-id-table (id)
  (if (not (id-p id)) (error "id is not struct 'id'"))
  (setf (id-lev  id) *lev*)
  (renew-block-index id)
  id)

;;; variable$BEPO?(B
;;; v-name:$BJQ?tL>(B v-type:$B7?(B
(defun entry-variable (v-name v-type)
  (if (search-id v-name :all nil)
      (let ((*standard-output* *error-output*))
	(princ "warning:previous declaration exists for ")
	(princ "'") (princ v-name) (princ "'") (princ #\Linefeed)))
  (add-id-table (make-id :name v-name :object 'variable :type v-type)))

;;; $B4X?t$N0z?t$r(Bvariable$B$H$7$FEPO?(B
;;; a-name:$B0z?tL>%j%9%H(B fn:$B4X?tEPO?%G!<%?(B
(defun entry-argument (a-name fn &aux (a-type (cdr (id-type fn))))
  (let ((lnm (length a-name)) (ltp (length a-type)))
    (if (> lnm ltp)
	(error "argument-type incompleted"))
    (if (< lnm ltp)
	(error "too few arguments")))
  (mapcar 'entry-variable a-name a-type))

;;; function$BEPO?(B
;;; f-name:$B4X?tL>(B f-type:$B7?%j%9%H(B
(defun entry-function (f-name f-type &aux (old-fn (search-id f-name)))
  (if (and old-fn (eq (id-object old-fn) 'function))
      (progn 
	(if (not (equal f-type (id-type old-fn)))
	    (let ((*standard-output* *error-output*))
	      (princ "warning:conflicting types for function ")
	      (princ "'") (princ f-name) (princ "'") (princ #\Linefeed)))
	(setf (id-type old-fn) f-type)
	old-fn)
      (add-id-table (make-id :name f-name :object 'function :type f-type))))

;;; struct,union$BEPO?(B
;;; s-spec:struct-or-union-specifier m-id:$B%a%s%P$N<1JL;R%G!<%?%j%9%H(B
(defun entry-struct (s-spec m-id &aux (s-or-u (car s-spec)) 
			    (s-name (identifier (cadr s-spec)))
			    (old-st (search-id s-name)))
  (if (and old-st (eq (id-object old-st) s-or-u) (null (id-type old-st)))
      (progn
	(setf (id-type old-st) m-id)
	old-st)
      (add-id-table (make-id :name s-name :object s-or-u :type m-id))))

;;; enum$BEPO?(B
;;; e-spec:enum-specifier ec-list:enumration-constant$B$N%j%9%H(B
(defun entry-enum (e-spec ec-list &aux (e-name (identifier (cadr e-spec))))
  (add-id-table (make-id :name e-name :object 'enum :type ec-list)))


;;; e-const$BEPO?(B
;;; ec-name:eumeration-constant$BL>(B
(defun entry-econst (ec-name)
  (add-id-table (make-id :name ec-name :object 'e-const)))

;;; type$BEPO?(B
;;; t-name:typedef$BL>(B texp:type-expression
(defun entry-type (t-name texp)
  (add-id-table (make-id :name t-name :object 'type :type texp)))

;;; struct-type, union-type$BEPO?(B
;;; t-name:typedef$BL>(B s-or-u:struct-or-union m-id:$B%a%s%P$N<1JL;R%G!<%?%j%9%H(B
(defun entry-struct-type (t-name s-or-u m-id)
  (let ((obj (if (eq s-or-u 'struct) 'struct-type 'union-type)))
    (add-id-table (make-id :name t-name :object obj :type m-id))))

;;; enum-type$BEPO?(B
;;; t-name:typedef$BL>(B ec-list:enumeration-constant$B$N%j%9%H(B
(defun entry-enum-type (t-name ec-list)
  (add-id-table (make-id :name t-name :object 'enum-type :type ec-list)))

;;;$B%V%m%C%/%l%Y%k(Bi$B$G:G8e$K@k8@$5$l$?<1JL;R$N(B*id-table*$BMWAG$X$N%j%s%/(B
(defvar *block-index* (make-array 256))
(defmacro get-block-index ()
  `(aref *block-index* *lev*))
(defun renew-block-index (newindex)
  (setf (get-block-index) (cons newindex (get-block-index))))

