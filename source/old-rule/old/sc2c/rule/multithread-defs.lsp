(provide "MULTITHREAD-DEFS")
(in-package "MULTITHREAD")
(require "SC-UTIL")
(use-package "SC-TRANSFORMER")
(use-package "SC-MISC")
(use-package "SC-UTIL")

(defvar *current-func* nil)
(defvar *used-identifier* nil)
(defconstant *nthr-id* 'nthr)
(defconstant *nestfunc-tag* 'fn) ;; fn or lightweight

;(<���x�� �ϐ�><����q�֐���><���[�J���ϐ�><�X���b�h�̔��f(�������p����)>)
(defstruct finfo
  ;; �֐���? <- ������B���Ԃ񗛂���̊��Ⴂ
  ;; �֐����Ƃ��Ďg�����Ƃɂ���
  id
  ;; �Ԃ�l�̌^
  rettype 
  ;; �֐��̐擪�ɒǉ�����錾�̃��X�g
  decl
  ;; �v�Z���ǂ��܂ŏI����������������x�����̃��X�g
  ;; list of ( <label-id> <retval-id> )
  ;; �o������push
  label
  ;; ����q�֐��̖��O
  nfunc-id
  ;; ���ɂ���ln�̒l?
  ;; ln)
  )

; make list of "case <num>: goto L<num>" in initial switch statement
; for rsn_cont of nested-function
(defun switch-cont (label-list)
  `(switch
    ln
    ,@(let ((i 0))
	(mapcan #'(lambda (x)
		    (list `(case ,(incf i)) `(goto ,(car x))))
		label-list))))

; make list of "case <num>: return t<num>" in initial switch statement 
; for rsn_retval of nested-function
(defun switch-retval (label-list)
  `(switch
    ln
    ,@(let ((i 0))
	(mapcan #'(lambda (x)
		    (incf i)
		    (when (second x)
		      (list
		       `(case ,i)
		       `(return (cast (ptr void) (ptr ,(second x)))))))
		label-list))))

; make identifier of nested-function from identifier of parent-func
(defun make-nestfunc-id (f-id)  
  (generate-id (string+ (par-identifier f-id) "_c")
	       *used-identifier*))

; separate declarations from the others in block-item-list
(defun separate-decl (bil &aux dcl-list oth-list)
  (dolist (bi bil (list (reverse dcl-list) (reverse oth-list)))
    (if (and (listp bi)
	     (or (par-storage-class-specifier (first bi))
		 (eq 'deftype (first bi)))
	     (par-identifier (second bi)))
	(push bi dcl-list)
      (push bi oth-list))))

;; ����q�֐��̒�`�𐶐�
(defun make-nestfunc (nf-body)
  #|
  (setq label-l (get-id-from-string (string+ "@L" (write-to-string ln))))
  (setq returninfos `((label ,label-l nil)
  (= (fref cp -> c) c_p)
  (= (fref cp -> stat) thr_runnable)
		      (return)))
  |#
  (let ((nfunc-id (finfo-nfunc-id *current-func*))
	(label-list (reverse (finfo-label *current-func*))))
    `(def (,nfunc-id cp rsn) (@nestfunc-tag (ptr void) thst_ptr reason)
	  (switch rsn 
		  (case rsn_cont)
		  ,(switch-cont label-list)
		  (return)
		  (case rsn_retval)
		  ,(switch-retval label-list)
		  (return))
	  (return)
	  
	  ;;<����q�֐��̕�>����
	  ,@nf-body
	  ;;�֐��̍Ō��return���Ȃ��ꍇ�������ƌĂяo�����̃X���b�h��
	  ;;runnable�ɂ��Ă���X�P�W���[����return
	  (= (fref cp -> c) c_p)
	  (= (fref cp -> stat) thr_runnable)
	  (return))
	  #|
	  ,@(cond 
	     ((eq `thr_c flag)
	      `((return)))
	     ((eq `thr_s flag)
	      returninfos)
	     (t
	      returninfo))
          |#
    ))
