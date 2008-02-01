;; (c-exp "#include<stdio.h>")

;;; �ɲä��뷿���ޥ������
(deftype nestfn-t (ptr (fn (ptr char) (ptr char) (ptr void))))

(deftype closure-t struct
  (def fun nestfn-t)
  (def fr (ptr void)))

(%defmacro special (tp)
  `(- (cast ,tp 0) 1))

(deftype Align-t double)

(%defmacro aligned-add (base size)
  `(cast (ptr char)
	 (+ (cast (ptr Align-t) ,base)
	    (/ (+ ,size (sizeof Align-t) -1) (sizeof Align-t)))))

(%defmacro aligned-sub (base size)
  `(cast (ptr char)
	 (- (cast (ptr Align-t) ,base)
	    (/ (+ ,size (sizeof Align-t) -1) (sizeof Align-t)))))

(%defmacro mref-t (tp p)
  `(mref (cast (ptr ,tp) ,p)))

(%defmacro push-arg (tp v argp)
  `(exps (= (mref-t ,tp ,argp) ,v)
	 (= ,argp (aligned-add ,argp (sizeof ,tp)))))

(%defmacro pop-arg (tp argp)
  `(exps (= ,argp (aligned-sub ,argp (sizeof ,tp)))
	 (mref-t ,tp ,argp)))

#+obsolete
(def (lw-call esp) (fn void (ptr char))
     (def tmp-esp (ptr char))
     (def clos (ptr closure-t) (mref-t (ptr closure-t) esp))
     (def new-esp (ptr char) esp)
     (while (= tmp-esp ((fref clos -> fun) new-esp (fref clos -> fr)))
       (lw-call tmp-esp)
       (= new-esp (+ esp 1))))

(decl (lw-call esp) (fn (ptr char) (ptr char)))

;;; nonlocalexit-goto ���б������Ƥߤ롥
#+obsolete
(def (lw-call esp) (fn (ptr char) (ptr char))
  (def clos (ptr closure-t) (mref-t (ptr closure-t) esp))
  ;; ����Ҵؿ���ƤӽФ���
  ;; goto ��æ�Ф��Ƥ������ϡ�æ�����efp,
  ;; �����Ǥʤ����ϡ�0 ���֤��ͤȤʤ롣 
  (return ((fref clos -> fun) esp (fref clos -> fr))))
