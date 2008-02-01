;;;; 1^2 + 2^2 + ... + n^2 $B$N7W;;(B
;;;; $B7k2L$,(Blimit$B$rD6$($?$i(B -1 $B$rJV$9!#(B
(def (square-sum n) (fn int int)
  (def @O-FLOW __label__)  ; $B$3$N@k8@$O(Bgcc$B$G$bI,?\!"(B2$B%Q%92r@O$9$l$P$O$:$;$k(B?
  (static limit int 8192)

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
  
  (return (square-sum-t n 0))
     
  (label @O-FLOW (return -1)))

(def (main) (fn int)
  (return (square-sum 100)))
