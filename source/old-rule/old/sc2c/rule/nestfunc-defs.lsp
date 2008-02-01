;;; nestfunc.rule �Ŏg���ϐ��A�֐��̒�`
(provide "NESTFUNC-DEFS")
(in-package "NESTFUNC")

(require "SC-UTIL")
(use-package "SC-TRANSFORMER")
(use-package "SC-MISC")
(use-package "SC-UTIL")

; �g�p�ς�identifier�̃��X�g
(defvar *used-id-list* nil)
; �錾�݂̂̊֐��̃��X�g
(defvar *decl-only-list* nil)

; �֐��t���[���̍\���̏��
; ( <(���̃\�[�X�ł�)�֐���> . <�\���̏��> ) �̃��X�g
; <�\���̏��> := ( <�\���̖�> ) 
(defvar *frame-alist* nil)

; ���݂���֐��̏��
(defvar *current-func* nil)
(defstruct finfo 
  name           ;�֐���
  parent-func    ;�e�֐���finfo�\���̂ւ̎Q��
  ret-type       ;�Ԃ�l�̌^
  argp           ;argp�̗v/�s�v (=����q�֐��Ăяo���̗L��)
  ;;(<���A�ʒu���������x����> . <�t���[�����A�p�R�[�h>) �̃��X�g(�t��)
  label-list
  ;; ( <symbol> . <texp> )
  ;; frame�ɂ� var-list �� tmp-list�̗���������
  ;; �inf-list �� int-name �������j
  ;; ���f�O�ɃX�^�b�N�ɑޔ�����̂�var-list�̂�
  ;; ���A��A�񕜂���̂� var-list �� tmp-list(?) �̗���
  var-list       ;�ʏ�̃��[�J���ϐ��̃��X�g
  tmp-list       ;�ꎞ�ϐ��̃��X�g
  nf-list        ;��`���ꂽ����q�֐��̃��X�g ( <int-name> . <ext-name> )
  ;; �Ǐ����x���̃��X�g(def ,id __label__)�Œ�`�B����q�֐��E�o�p
  ;; (<���x����> . <���A�R�[�h>)�B �t��
  local-label
  ;; �ʏ�֐����ł�estack�ň������[�J���ϐ�(<id-string>)
  estack-var
  )

;estack[]�̃T�C�Y
(defconstant *estack-size* 65536)

;;;; �֐�

; �֐�����*frame-alist*����T���Ċ֐��t���[���̍\���̏��𓾂�B
; ���o�^�̏ꍇ�͓o�^���āA�o�^�����\���̏���Ԃ��B
(defun get-frame (x)
  (let* ((strx (par-identifier x))
	 (asc
	  (assoc strx *frame-alist* :test #'string=)))
    (if asc
	(cdr asc)
	(let* ((basename (string+ strx "_frame"))
	       (frame-name (generate-id basename *used-id-list*))
	       (frame-info `(,frame-name )))
	  (push (cons strx frame-info) *frame-alist*)
	  frame-info))))

; �֐���=>�֐��̃t���[���\���̖�
(defun get-frame-name (fname)
  (car (get-frame fname)))

; �^����ꂽ�֐���񂩂�t���[���\���̂̒�`�����
(defun make-frame-decl (fi)
  (let ((frame-name (get-frame-name (finfo-name fi)))
	(member-list (append (finfo-var-list fi) (finfo-tmp-list fi)
			     (mapcar #'(lambda (x)
					 (cons (car x) 'closure-t))
				     (finfo-nf-list fi)))))
    `(def (struct ,frame-name)
	  (def tmp-esp (ptr char))    ;; ����́A��΂ɐ擪
	  (def argp (ptr char))
	  (def call-id int)
	  ,@(mapcar
	     #'(lambda (x)
		 `(def ,(car x) ,(cdr x)))
	     member-list))))

; *current-func*�ɕϐ�����ǉ����āAdeclaration����Ԃ�
(defun add-local (id texp mode &key  (init nil) (finfo *current-func*))
  (let ((lw2ct-texp (lw2ct texp)))
    ; mode�F :var or :temp
    (when (and finfo mode)
      (case mode
	((:var)
	 (when (let ((ttexp (remove-type-qualifier texp)))
		 (and (listp ttexp)
		      (eq 'array (car ttexp))))
	   (push (par-identifier id) (finfo-estack-var finfo)))
	 (push (cons id lw2ct-texp) (finfo-var-list finfo)))
	((:tmp)
       (push (cons id lw2ct-texp) (finfo-tmp-list finfo)))
	(otherwise
	 (error "unexpected value of 'mode'(~s)" mode))))
    (if init
	`(def ,id ,lw2ct-texp ,init)
      `(def ,id ,lw2ct-texp))))

; lightweight => closure-t
(defun lw2ct (texp)
  (if (and texp (listp texp))
      (if (eq 'lightweight (car texp))
	  'closure-t
	(mapcar #'lw2ct texp))
    texp))


; �^����ꂽ�֐���񂩂畜�A�������s��statement�����
(defun make-resume (fi)
  (unless (or (finfo-label-list fi)
	      (finfo-local-label fi))
    (return-from make-resume
      `((label @LGOTO nil))))
  (let ((main-p (eq 'main (finfo-name fi)))
	(case-goto
	 (append
	  ;; ����q�֐��Ăяo���I����̕��A
	  (do ((ret nil)
	       (k 0 (1+ k))
	       (lb (reverse (finfo-label-list fi)) (cdr lb)))
	      ((endp lb) (apply #'append (nreverse ret)))
	    (push `((case ,k)
		   ,@(cdar lb) 
		    (goto ,(caar lb)))
		   ret))
	  ;; goto�ɂ�����q�֐�����e�֐��ւ̒E�o��
	  (do ((ret nil)
	       (k -1 (1- k))
	       (lb (reverse (finfo-local-label fi)) (cdr lb)))
	      ((endp lb) (apply #'append (nreverse ret)))
	    (push `((case ,k)
		    ,@(cdar lb) 
		    (goto ,(caar lb)))
		   ret))))
	(frame-type `(struct ,(get-frame-name (finfo-name fi)))))
    `((if ,(if main-p '0 'esp-flag)
	  (begin
	   ,@(unless main-p
	       `( (bit-xor= (cast size-t esp) esp-flag)
		  (= efp (cast (ptr ,frame-type) esp))
		  (= esp (aligned-add esp (sizeof ,frame-type)))
		  (= (mref-t (ptr char) esp) 0) ))
;;;	   ,@(when (finfo-parent-func fi)
;;;		   `( (if (and (bit-and esp-flag 2)
;;;			       (>= (fref efp -> call-id) 0))
;;;			  (return (cast (ptr char) 2)))))
	   (label @LGOTO
		  (switch (fref (mref efp) call-id) ,@case-goto))
	   ,@(when (finfo-label-list fi)
	       `( (goto ,(caar (last (finfo-label-list fi)))) )))))))

; �^����ꂽ�֐���񂩂�efp,esp,new-esp�̏������������s��statement�����B
(defun make-init (fi)
  (let ((frame-type  `(struct ,(get-frame-name (finfo-name fi)))))
    `((= efp (cast (ptr ,frame-type) esp))
      (= esp (aligned-add esp (sizeof ,frame-type)))
      (= (mref-t (ptr char) esp) 0)
      )))

; �^����ꂽ�֐���񂩂����q�֐��𐳋K������R�[�h�����
(defun make-normalize-nf (fi)
  (let ((nf-list (finfo-nf-list fi)))
    (apply #'nconc
	   (mapcar
	    #'(lambda (x) 
		`( (= (fref efp -> ,(car x) fun)
		      ,(cdr x))
		   (= (fref efp -> ,(car x) fr)
		      (cast (ptr void) efp)) ))
	    nf-list))))

;; �^����ꂽ�֐���񂩂�t���[������ۑ�����R�[�h�����
(defun make-frame-save (fi)
  (mapcan
   #'(lambda (x)
       (cond ((eq 'closure-t (cdr x))
	      nil)
	     (t
	      `( (= (fref efp -> ,(car x)) ,(car x)) ))
	     ))
   (remove-if #'(lambda (x)
		  (member (par-identifier (car x)) (finfo-estack-var fi)
			  :test #'string=))
	      (finfo-var-list fi))))

;; �^����ꂽ�֐���񂩂�t���[�����𕜊�����R�[�h�����
(defun make-frame-resume (fi)
  (mapcar
   #'(lambda (x) 
       `(= ,(car x) (fref efp -> ,(car x))))
   (remove-if #'(lambda (x)
		  (member (par-identifier (car x)) (finfo-estack-var fi)
			  :test #'string=))
	      (finfo-var-list fi))))

;; �^����ꂽ�֐���񂩂�֐����f�p��return�𐶐�����R�[�h�����
(defun make-suspend-return (fi)
  (cond ((finfo-parent-func fi)
	 ;;`(return (fref efp -> tmp-esp)))
	 (error ("make-suspend-return called in lightweight-func")))
	((eq 'void (finfo-ret-type *current-func*))
	 '(return))
	(t
	 `(return (SPECIAL ,(finfo-ret-type *current-func*))))))

;; �^����ꂽ���x�������A�e�֐��̋Ǐ����x���Ƃ��Ē�`����Ă��邩���ׂ�B
;; ��`����Ă��Ȃ����,�Ԃ�l��nil�B��`����Ă���΁A
;; (values <��������݂ĉ��Ԗڂ̐e�֐��ɒ�`����Ă�����>
;;         <���̒�` ( <label> . <���A����> )> 
;;         <���̃��x�����֐����ŉ��Ԗڂ�push���ꂽ���̂�>)
(defun find-local-label (lid fi &aux (lids (par-identifier lid)))
  (labels ((find-local-label-tail (cfi acc &aux memb)
	     (cond ((null cfi)
		     nil)
		   ((let* ((memb0 (member 
				   lids
				   (finfo-local-label cfi)
				   :test #'string=
				    :key #'(lambda (x)
					    (par-identifier (car x))))))
		      (setq memb memb0))
		    (values acc (car memb) (length memb)))
		   (t
		    (find-local-label-tail
		     (finfo-parent-func cfi) (1+ acc))))))
    (find-local-label-tail fi 0)))

; begin�Ƃ���(Nfb0 body)�̕Ԃ�l����A
; begin���̂��̂̕Ԃ�l�����B
; ((r1-1 r1-2 r1-3 r1-4) ... (rn-1 rn-2 rn-3 rn-4))
; => ( (,@r1-1 ... ,@rn-1)
;      nil nil
;      (,@prev-4th ,@r1-2 ... ,@rn-2 ,@r1-3 ,r1-4 ... ,@rn-3 ,rn-4 ) )
(defun combine-ret-list (ret-list &optional prev-4th)
  (let ((fst (mapcar #'first ret-list))
	(scd (mapcar #'second ret-list))
	(thd-4th (mapcar
		  #'(lambda (x) `(,@(third x) ,(fourth x)))
		   ret-list)))
    (list (apply #'append fst)
	  nil
	  nil
	  (remove nil (apply #'append prev-4th 
			      (apply #'append scd)
			      thd-4th)))))

; identifier, constant, string-literal
; ==> nil�ȊO  (���������ꎞ�ϐ����g���K�v���Ȃ����Ƃ����炩�Ȏ�)
(defun simple-exp-p (x)
  (or
   ;;(par-identifier x)
   (par-constant x)
   (par-string-literal x)))

