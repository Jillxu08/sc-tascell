(provide "RENAME-DEFS")
(in-package "RENAME")

(scr:require "SC-UTIL")
       
; get *block-id* of id
(defun assoc-id-level (id)
  (cdr (assoc (par-identifier id)
	      *id-alist* :key #'par-identifier :test #'string=)))

; get replacement-id of old-id
(defun assoc-replacement-id (id)
  (or
   (aand (par-identifier id)
	 (cdr (assoc it 
		     *replace-alist* :key #'par-identifier :test #'string=)))
   id))

; create new identifier
(defun create-new-id (base-id &optional (used-id-list *used-id-list*))
  (let ((n 2) ns (base-idstr (par-identifier base-id)))
    (loop
     (setq ns (string+ base-idstr "__" (write-to-string n)))
     (unless (member ns used-id-list :test 'string=)
       (push ns *used-id-list*)
       (return (id-to-scid ns)))
     (incf n))))

; entry identifier
; 返り値は更新先の (values *id-alist* *replace-alist*) 
(defun entry-identifier (id)
  (let ((lev (assoc-id-level id)))
    (if lev 
	(if (= *block-id* lev)     
	    ;同一スコープで宣言済 ->そのまま
	    (values *id-alist* *replace-alist*)
	    ;別のスコープで宣言済
	    (let* ((new-id (generate-id (par-identifier id)
                                        *used-id-list*)))
	      (values
	       (cons (cons id *block-id*) *id-alist*)
	       (cons (cons id new-id) *replace-alist*))))
      ; 未宣言
      (values 
       (cons (cons id *block-id*) *id-alist*)
       (cons (cons id id) *replace-alist*)))))

; replace all identifier in s-expression
(defun repl-id (x)
  (do-all-atoms #'assoc-replacement-id x))

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

)
