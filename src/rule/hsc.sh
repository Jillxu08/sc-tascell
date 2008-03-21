(%ifndef NESTFN
	 ((%defconstant NESTFN fn))
	 )
(deftype sht (ptr (NESTFN void void)))

;;;
;; (deftype size-t long)
(deftype align-t double)

;;; (def d desc-t) �β���2�ӥåȤ�������
(%defconstant TYPE-STRUCT 0)
(%defconstant TYPE-REF-ARRAY 1)
(%defconstant TYPE-NONREF-ARRAY 3)

;;; ��¤��<sid>�ι�¤��hsc.rule����
;; ~(def (struct ,data-sid) ,@sbody-ret) ; ���ι�¤�ξ���� <sid>_data
;; ~(def (struct ,sid)     ; <sid> --> {<type_info>, <ptr to descriptor>, <sid>_data}
;;   (def d desc-t)        ; (ptr (struct desc-struct))
;;   (def data (struct ,data-sid))

(deftype size-t unsigned-int)

(def (struct desc-struct)
  (decl size size-t)
  (decl asize size-t) 
  (decl fli-len int)
  (decl fli (ptr size-t)))
(deftype descriptor (struct desc-struct))
(deftype desc-t (ptr descriptor))

(def (struct array-object)
  (decl d desc-t)   ; ����2bit�ʳ��ϥ��ߡ�
  (decl length int)
  (decl el-size size-t)
  (decl size size-t)
  (decl asize size-t)
  (decl body (array char 0)))
  
;;;
(extern-decl (getmem scan size) (fn (ptr void) sht size-t))
(extern-decl (move vp) (csym::fn (ptr void) (ptr void)))
