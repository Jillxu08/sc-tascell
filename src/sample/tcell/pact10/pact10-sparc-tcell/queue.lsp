;;;; queue���饹�����
(defclass queue ()
  ((body :accessor queue-body :type list :initform (misc:make-queue))))

(defclass shared-queue (queue)
  ((lock :accessor sq-lock :type mp:process-lock :initform (mp:make-process-lock))
   (gate :accessor sq-gate :type gate :initform (mp:make-gate nil)))) ; to notify addition


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; sc-misc.lsp �Υ��塼��Ϣ�δؿ��Ȥξ��ͤ��ɤ�
(eval-when (:compile-toplevel :load-toplevel :execute)
  (shadow :empty-queue-p)
  (shadow :delete-queue)
  (shadow :find-delete-queue))

;; queue�ؤ��ɲ�
(defmethod add-queue (elm (q queue))
  (misc:insert-queue elm (queue-body q)))

(defmethod add-queue :around (elm (sq shared-queue))
  (declare (ignorable elm))
  (mp:with-process-lock ((sq-lock sq))
    (prog1 (call-next-method)
      (mp:open-gate (sq-gate sq)))))

;; ��queue
(defmethod empty-queue-p ((q queue))
  (misc:empty-queue-p (queue-body q)))

;; queue������Ф�
(defmethod delete-queue ((q queue))
  (misc:delete-queue (queue-body q)))

(defmethod delete-queue :around ((sq shared-queue))
  (mp:with-process-lock ((sq-lock sq))
    (prog1 (call-next-method)
      (when (empty-queue-p sq)
        (mp:close-gate (sq-gate sq))))))

;; queue���鸡�����Ƽ��Ф�
(defmethod find-delete-queue ((q queue) test &key (key #'identity))
  (misc:find-delete-queue (queue-body q) test :key key))

(defmethod find-delete-queue :around ((sq shared-queue) test
                                       &key (key #'identity)
                                            (wait nil))
  (declare (ignore test key))
  (block :end
    (tagbody
      :retry
      (mp:with-process-lock ((sq-lock sq))
        (let ((item (call-next-method)))
          (when item
            (when (empty-queue-p sq)
              (mp:close-gate (sq-gate sq)))
            (return-from :end item))
          (unless wait
            (return-from :end nil))
          (mp:close-gate (sq-gate sq))))
      (mp:process-wait "FIND-DELETE-QUEUE-WAIT"
                       #'mp:gate-open-p (sq-gate sq))
      (go :retry))))
