;;; goto �ˤ������Ҵؿ�æ����
;;; (ʣ���ʥ�����)
;;; g -> f -> g1 -> g -> g1 ==goto==> g
(%rule (:nestfunc-sc1 :nestfunc-type :nestfunc-temp :nestfunc :untype))

(deftype size-t long)

(def (f pg) (fn int (ptr (lightweight void int)))
  (pg 0)
  (return 0))

;; �ƴؿ��ؤθƽФ�������Ȥ��ϥץ�ȥ����������ɬ��
(decl (g pg) (fn int (ptr (lightweight void int))))

(def (g pg) (fn int (ptr (lightweight void int)))
  (def @L1 __label__)
  (def (g1 m) (lightweight void int)
    (if (> m 0)
	(goto @L1)
	(g g1)))
  
  (if (== pg 0)
      (return (f g1))
      (begin
       (g1 1)  ; goto��ϸ�ǸƤӽФ��줿g
       (return 0)))
  
  (label @L1
	 (if (== pg 0)
	     (return 0)
	     (pg 1)  ; goto��Ϻǽ�˸ƤӽФ��줿g
	     )))
  
(def (main) (fn int)
  (return (g 0)))

