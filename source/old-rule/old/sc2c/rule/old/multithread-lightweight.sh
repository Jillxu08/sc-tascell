(decl (struct _thstelm))

; �p���p����q�֐��̌Ăяo�����R
(deftype reason enum rsn-cont rsn-retval)

; �p���p����q�֐��̃|�C���^
(deftype cont-f (ptr (lightweight (ptr void) (ptr (struct _thstelm)) reason)))

; �X���b�h�̏��
(def (enum _stat)
     ; ��~���ŁC�ÖٓI�p�����L��
     thr-new-suspended
     ; ���s�\�ŁA�ÖٓI�p�����L��
     thr-new-runnable
     ; ��~���ŁA�����I�p���̂ݗL��
     thr-suspended
     ; ���s�\�ŁA�����I�p���̂ݗL��
     thr-runnable
     ; �����I�p��(cont-f c)�͖���
     thr-scheduled)

; �X���b�h�Ǘ��p�X�^�b�N�̗v�f
(def (struct _thstelm)
     (def c cont-f)
     (def stat (enum _stat)))

(deftype thstelm (struct _thstelm))

(deftype thst-ptr (ptr (struct _thstelm)))
(deftype cont thst-ptr)

; �X���b�h�Ǘ��p�X�^�b�N
(def thst (array thstelm 65536))

; �X���b�h�Ǘ��p�X�^�b�N�̃g�b�v
(def thst-top thst-ptr thst)

(deftype schdexit (ptr (lightweight void)))
; �X�P�W���[���̔�Ǐ��E�o��
(def cur-schd-exit schdexit 0)
(def cur-schd-thst-top thst-ptr thst)

(def scheduling (fn void)
     (def @L0 --label--)
     ;���̃X�P�W���[���̏��
     (def prev-exit schdexit cur-schd-exit)
     (def prev-thst-top thst-ptr cur-schd-thst-top)
     ;���̃X�P�W�����̏��
     (def mythst-top thst-ptr thst-top)
     (def nonlocalexit (lightweight void) (goto @L0))

     (label @L0 nil)
     (= cur-schd-exit nonlocalexit)
     (= cur-schd-thst-top (= thst-top mythst-top))
     (while 1
       (let ((cp thst-ptr))
	 ;�����Ō��̃X�P�W���[���ւ̔�Ǐ��E�o�����݂�
	 (for ((= cp prev-thst-top)
	       (< cp mythst-top)
	       (inc cp))
	    (if (!= (fref cp -> stat) thr-scheduled) (break))
	    ; �Ԃ��S�� thr-scheduled �Ȃ�
	    (if (== cp mythst-top) (if prev-exit (prev-exit)))))

       ; runnable�ȃX���b�h��T��
       (let ((cp thst-ptr)
	     (cc cont-f))
	 (for ((= cp (- thst-top 1))
	       (>= cp thst)
	       (dec cp))
	      (if (or (== (fref cp -> stat) thr-runnable)
		      (== (fref cp -> stat) thr-new-runnable))
		  (break)))
	 (if (< cp thst)
	     (begin
	      ; ������Ȃ������Ƃ��͑��̃v���Z�b�T����̗v�����������ׂ�
	      ; ����͉������Ȃ�
	      ;;(fprintf stderr "No Active thread!\\n")
	      ;;(exit 1)
	      ))
	 (do-while (== (fref cp -> stat) thr-runnable)
	    (= cc (fref cp -> c))
	    (= (fref cp -> c) 0)
	    (= (fref cp -> stat) thr-scheduled)
		   (cc cp rsn-cont)))
       ;������new-runnable�Ȃ�pop���A������ɐ�����ڂ�
       (if (and (> thst-top thst)
		(== (fref (- thst-top 1) -> stat) thr-new-runnable))
	   (begin (dec thst-top) (break))))

     ; ���̃X�P�W���[���̏���߂�
     (= cur-schd-exit prev-exit)
     (= cur-schd-thst-top prev-thst-top))

; thread-resume
(def (thr-resume cp) (fn void thst-ptr)
     (if (== (fref cp -> stat) thr-suspended)
	 (= (fref cp -> stat) thr-runnable)
       (if (== (fref cp -> stat) thr-new-suspended)
	   (= (fref cp -> stat) thr-new-runnable))))
