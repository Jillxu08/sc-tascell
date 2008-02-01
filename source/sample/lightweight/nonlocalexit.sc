(%rule (:nestfunc-sc1 :nestfunc-type :nestfunc-temp :nestfunc :untype))

(decl (csym::printf) (fn int (ptr (const char)) va-arg))
(deftype size-t unsigned-int)

;;;; 1^2 + 2^2 + ... + n^2 �η׻�
;;;; ��̤�limit��Ķ������ -1 ���֤���
(def (square-sum n pflag) (fn int int (ptr int))
  (def @O-FLOW __label__)  ; ���������gcc�Ǥ�ɬ�ܡ�2�ѥ����Ϥ���ФϤ�����?
  (def limit int 8192)

  (def (square-sum-t n acc) (lightweight int int int)
    (def (square-t n acc) (lightweight int int int)
      (if (== n 0)
	  (return acc)
	  (if (> acc limit)	
	      (goto @O-FLOW)
	      (return (square-t (- n 1) (+ acc -1 (* 2 n)))))))

    (if (== n 0)
	(return acc)
	(if (> acc limit)
	    (goto @O-FLOW)
	    (return
	     (square-sum-t (- n 1) (+ acc (square-t n 0)))))))

  (= (mref pflag) 0)
  (return (square-sum-t n 0))
     
  (label @O-FLOW
    (begin
     (= (mref pflag) 1)
     (return -1))) )

(def (main) (fn int)
  (def flag int)
  (csym::printf "1^2 + ... + 100^2 =%d~%"
                (square-sum 100 (ptr flag)))
  (return flag) )
