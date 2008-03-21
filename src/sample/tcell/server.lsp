;;;; Tascell ����������
;;; ��ư�� (make-and-start-server) ɬ�פ˱�����keyword parameter�򻲾�
;;; ��ǽɾ���κݤϡ�  (push :tcell-no-transfer-log *features*)
;;; �򤷤ơ����ط��Υ����ɤ�̵�뤷�ƥ���ѥ��뤹�롥

(eval-when (:compile-toplevel :load-toplevel :execute)
  (unless (featurep :allegro)
    (error "Sorry! This programs work only on Allegro Common Lisp!"))
  (when (featurep :mswindows)
    (setq *locale* (find-locale "japan.EUC")))
  ;; The most debuggable (and yet reasonably fast) code, use
  (proclaim '(optimize (speed 3) (safety 1) (space 1))); (debug 3)))
  (load (compile-file-if-needed (or (probe-file "sc-misc.lsp")
                                    "../../sc-misc.lsp")
                                :output-file "sc-misc.fasl"))
  ;; Ignore logging code
  (push :tcell-no-transfer-log *features*)
  )
#+sc-system (use-package "SC-MISC")
(use-package :socket)

(deftype gate () (type-of (mp:make-gate nil)))

;;; ����Ū���Ԥ������ݡ��Ⱥ�����
(defparameter *reuse-address* t)

;;; �Ƥؤ���³���ɥ쥹:�ݡ���
(defparameter *parent-host* nil)
(defparameter *parent-port* 8888)

;;; �Ҥ������³�ۥ���/�ݡ���
(defparameter *server-host* "localhost")
(defparameter *children-port* 8888)

;;; any�Ǥʤ�treq����ž��
(defparameter *transfer-treq-always-if-notany* t)

;;; log���Ϥ�̵ͭ
(defparameter *transfer-log* t)         ; (featurep :tcell-no-transfer-log) �ξ��Ͼ��̵��
(defparameter *connection-log* t)
;;; send/recv�Υ�����Ϥ���Ĺ��
(defparameter *transfer-log-length* 70)

;;; read-line���ɤ߹����Ĺ���κ���
(defconstant *max-line-length* 128)


(defparameter *retry* 20)

(defclass host ()
  ((server :accessor host-server :type tcell-server :initarg :server)
   (host :accessor host-hostname :type string :initarg :host)
   (port :accessor host-port :type fixnum :initarg :port)
   (sock :accessor host-socket :type stream :initarg :sock)
   (sender :accessor host-sender :type sender :initform nil)
   (receiver :accessor host-receiver :type receiver :initform nil)))

(defclass parent (host)
  ((host :initform *parent-host*)
   (port :initform *parent-port*)
   (n-treq-sent :accessor parent-n-treq-sent :type fixnum :initform 0)
   (last-none-time :accessor parent-last-none-time :type integer :initform -1)
   ))

;; terminal-parent��auto-rack, auto-resend-task��¸����뤿���
;; �Ф��Ƥ���task���ޥ�ɤξ���
(defstruct task-home
  task-cmd                              ; task���ޥ�ɤ��Τ��
  rack-to                               ; rack������
  rack-task-head                        ; rack��task-head
  start-time)                           ; ���ϻ��� (get-internal-real-time)

(defclass terminal-parent (parent)
  ((host :initform "Terminal")
   (port :initform -1)
   (sock :initform (make-two-way-stream *standard-input* *standard-output*))
   (auto-rack :accessor parent-auto-rack :type boolean :initarg :auto-rack :initform nil)
                                        ; ��ưŪ��rack���֤�
   (auto-resend-task :accessor parent-auto-resend-task :type fixnum :initarg :auto-resend-task :initform 0)
                                        ; auto-rack�塤��ưŪ��Ʊ��task�������������
   (auto-resend-func :accessor parent-auto-resend-func :type function :initform #'(lambda ()))
                                        ; treq�����ä��鼫ưŪ�˼¹Ԥ��� send�¹ԥ��ޥ��
   (task-home :accessor parent-task-home :type list :initform ())
                                        ; ����¸����뤿��˿Ƥ�������줿task��Ф��Ƥ����ꥹ��
   ))

(defclass child (host)
  ((id :accessor child-id :type fixnum)
   (diff-task-rslt :accessor child-diff-task-rslt :type fixnum :initform 0)
                                        ; <task�����ä����>-<rslt���֤äƤ������>
   (work-size :accessor child-wsize :type fixnum :initform 0)
                                        ; �Ż����礭�����ܰ¡�
                                        ; task��������/���ä���(- <ndiv>)�˹���
   ;; (in-treq :accessor child-in-treq :type boolean :initform nil)
   ))

(defclass ta-entry ()                   ; elemens of treq-any-list
  ((from :accessor tae-from :type host :initarg :from)
   (head :accessor tae-head :type string :initarg :head)
   ))

(defclass buffer ()
  ((body :accessor buf-body :type list :initform (make-queue))))
   
(defclass shared-buffer (buffer)
  ((lock :accessor sbuf-lock :type mp:process-lock :initform (mp:make-process-lock))
   (gate :accessor sbuf-gate :type gate :initform (mp:make-gate nil)))) ; to notify addition

(defclass sender ()
  ((buffer :accessor send-buffer :type shared-buffer
           :initform (make-instance 'shared-buffer) :initarg :buffer)
   (writer :accessor writer :type function ; obj stream -> (write to stream)
           :initform #'write-string :initarg :writer)
   (send-process :accessor send-process :type mp:process :initform nil)
   (destination :accessor sender-dest :type stream :initform nil :initarg :dest)))
(defclass receiver ()
  ((buffer :accessor receive-buffer :type shared-buffer
           :initform (make-instance 'shared-buffer) :initarg :buffer)
   (reader :accessor reader :type function ; stream -> <obj,eof-p>
           :initform #'(lambda (strm) (aif (read-line strm nil nil) (values it nil)
                                        (values nil t)))
           :initarg :reader)
   (receive-process :accessor receive-process :type mp:process :initform nil)
   (source :accessor receiver-src :type stream :initform nil :initarg :src)))

(defclass tcell-server ()
  ((host :accessor ts-hostname :initform *server-host* :initarg :local-host)
   (message-buffer :accessor ts-buffer :type shared-buffer
                   :initform (make-instance 'shared-buffer))
   (proc-cmd-process :accessor ts-proc-cmd-process :type mp:process :initform nil)
   (parent :accessor ts-parent :type parent)
   (children-port :accessor ts-chport :initform *children-port* :initarg :children-port)
   (children-sock0 :accessor ts-chsock0 :type stream) ; �Ԥ�����
   (children :accessor ts-children :type list :initform '())
   (eldest-child :accessor ts-eldest-child :type child :initform nil)
   (n-children :accessor ts-n-children :type fixnum :initform 0)
   (child-next-id :accessor ts-child-next-id :type fixnum :initform 0)
   (treq-any-list :accessor ts-talist :type list :initform '()) ;; treq-any��Ф��Ƥ��ʤ��ꥹ��
   (accept-connection-process :accessor ts-accept-connection-process
                              :type mp:process :initform nil)
   (retry :accessor ts-retry :type fixnum :initform *retry* :initarg :retry)
   ))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Initializers
(defmethod initialize-instance :after ((sdr sender) &rest initargs)
  (declare (ignore initargs))
  (setf (send-process sdr)
    (mp:process-run-function "SEND-PROCESS"
      #'monitor-and-send-buffer
      (send-buffer sdr) (sender-dest sdr) (writer sdr))))

(defmethod initialize-instance :after ((rcvr receiver) &rest initargs)
  (declare (ignore initargs))
  (setf (receive-process rcvr)
    (mp:process-run-function "RECEIVE-PROCESS"
      #'receive-and-add-to-buffer
      (receive-buffer rcvr) (receiver-src rcvr) (reader rcvr))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Finalizers
(defmethod finalize ((sdr sender))
  (when (send-process sdr)
    (mp:process-kill (send-process sdr))
    (setf (send-process sdr) nil)))

(defmethod finalize ((rcvr receiver))
  (when (receive-process rcvr)
    (mp:process-kill (receive-process rcvr))
    (setf (receive-process rcvr) nil)))

(defmethod finalize ((sv tcell-server))
  (when (ts-accept-connection-process sv)
    (mp:process-kill (ts-accept-connection-process sv))
    (setf (ts-accept-connection-process sv) nil))
  (when (ts-proc-cmd-process sv)
    (mp:process-kill (ts-proc-cmd-process sv))
    (setf (ts-proc-cmd-process sv) nil))
  (finalize (ts-parent sv))
  (loop for chld in (ts-children sv)
      do (finalize chld))
  (close (ts-chsock0 sv)))

(defmethod finalize ((hst host))
  (finalize (host-sender hst))
  (finalize (host-receiver hst)))

(defmethod finalize ((hst (eql nil))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defmethod start-server ((sv tcell-server) (prnt parent))
  (unwind-protect
      (progn
        ;; �Ƥ���³
        (setf (ts-parent sv) (connect-to prnt))
        ;; �Ҥ�����Ԥ������ݡ��Ȥ򳫤�
        (setf (ts-chsock0 sv)
          (make-socket :connect :passive
                       :reuse-address *reuse-address*
                       :local-host (ts-hostname sv)
                       :local-port (ts-chport sv)))
        ;; �ǽ�λҤ���³���Ƥ���Τ��Ԥ�
        (format *error-output* "~&Waiting for connection to ~A:~D...~%"
          (ts-hostname sv) (ts-chport sv))
        (wait-and-add-child sv)
        ;; �Ҥ������³�����դ��ѥץ�����ư
        (activate-accept-connection-process sv)
        ;; ��å����������ѥץ�����ư
        (activate-proc-cmd-process sv)
        ;; ɸ�����Ϥ���������Ԥ�
        (let* ((msg-buf (ts-buffer sv))
               (prnt (ts-parent sv))
               (terminal-parent-p (typep prnt 'terminal-parent))
               (reader (make-receiver-reader prnt))
               (src (host-socket prnt)))
          (loop
            (multiple-value-bind (msg eof-p) (funcall reader src)
              (when terminal-parent-p
                (add-buffer msg msg-buf))
              (when eof-p (return))))
          ))
    (finalize sv)))

(defmethod print-server-status ((sv tcell-server))
  (fresh-line *error-output*)
  (pprint (list `(parent ,(hostinfo (ts-parent sv)))
          `(children ,@(mapcar #'hostinfo (ts-children sv)))
          `(eldest-child ,(awhen (ts-eldest-child sv) (hostinfo it)))
          `(n-children ,(ts-n-children sv))
          `(child-next-id ,(ts-child-next-id sv))
          `(children-port ,(ts-chport sv))
          `(treq-any-list ,@(mapcar #'ta-entry-info (ts-talist sv)))
          `(retry ,(ts-retry sv)))
          *error-output*)
  (terpri *error-output*))

(defmethod wait-and-add-child ((sv tcell-server))
  (let ((next-child (make-instance 'child :server sv)))
    (awhen (connect-from next-child (ts-chsock0 sv) :wait t)
      (add-child sv it))))

(defmethod activate-accept-connection-process ((sv tcell-server))
  (setf (ts-accept-connection-process sv)
    (mp:process-run-function "ACCEPT-CHILD-CONNECTION"
      #'(lambda () (loop (wait-and-add-child sv))))))

(defmethod activate-proc-cmd-process ((sv tcell-server))
  (setf (ts-proc-cmd-process sv)
    (mp:process-run-function "PROC-CMD"
      #'(lambda (msg-buf &aux (msg-gate (sbuf-gate msg-buf)))
          (loop
            ;; �Ƥޤ��ϻҤ���������Ԥ�
            (mp:process-wait "Waiting for a message to T-Cell server."
                             #'mp:gate-open-p msg-gate)
            ;; ��å���������
            (while (mp:gate-open-p msg-gate)
              (destructuring-bind (host . message) (delete-buffer msg-buf)
                (proc-cmd sv host message)))))
      (ts-buffer sv))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defmethod host-name-port ((hst host))
  (string+ (host-hostname hst) ":" (write-to-string (host-port hst))))
(defmethod host-name-port ((hst terminal-parent))
  "Terminal")

(defmethod hostinfo ((chld child))
  (string+ (host-name-port chld)
           " (child " (hostid chld) ")"))
(defmethod hostinfo ((prnt parent))
  (string+ (host-name-port prnt)
           " (parent)"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defmethod connect-to ((hst host))
  (setf (host-socket hst)
	(make-socket :remote-host (host-hostname hst)
                     :remote-port (host-port hst)))
  (initialize-connection hst)
  hst)

(defmethod connect-to ((hst terminal-parent))
  (assert (host-socket hst))
  (initialize-connection hst)
  hst)

(defmethod connect-from ((hst host) sock0 &key (wait t))
  (let ((sock (setf (host-socket hst)
                (accept-connection sock0 :wait wait))))
    (if sock
        (progn
          (initialize-connection hst)
          hst)
      nil)))

(defmethod connect-from :after ((hst host) sock0 &key (wait t))
  (declare (ignorable sock0 wait))
  (when (host-socket hst)
    (when *connection-log*
      (format *error-output*
        "~&Accept connection from ~A.~%" (host-name-port hst)))))

(defmethod initialize-connection ((hst host) &aux (sock (host-socket hst)))
  (setf (host-hostname hst) (ipaddr-to-hostname (remote-host sock)))
  (setf (host-port hst) (remote-port sock))
  (initialize-sender hst)
  (initialize-receiver hst)
  hst)

(defmethod initialize-connection ((hst terminal-parent))
  (initialize-sender hst)
  hst)

;; make-instance -> initialize-connection -> initialize-sender/receiver�ʤΤ�
;; initialize-instance�ǤǤ��ʤ�
(defmethod initialize-sender ((hst host) &aux (sock (host-socket hst)))
  (setf (host-sender hst)
    (make-instance 'sender :dest sock :writer #'sender-writer))
  hst)

(defmethod initialize-receiver ((hst host) &aux (sock (host-socket hst)))
  (setf (host-receiver hst)
    (make-instance 'receiver
      :src sock :reader (make-receiver-reader hst)
      :buffer (ts-buffer (host-server hst)))) ; read������Τ϶�ͭ�ΥХåե��������
  hst)

;;;
(defun sender-writer (obj dest)
  (etypecase obj
    (list (mapc #'(lambda (ob) (sender-writer ob dest)) obj))
    (character (write-char obj dest))
    (string (write-string obj dest))
    (array (write-sequence obj dest))))

(defun msg-log-string (obj)
  (with-output-to-string (s)
    (write-msg-log obj s)))

(defun write-msg-log (obj dest)
  (typecase obj
    (list (mapc #'(lambda (ob) (write-msg-log ob dest)) obj))
    (symbol (write-string (symbol-name obj) dest))
    (character (write-char obj dest))
    (string (write-string obj dest))
    (array (format dest "#<Binary data: SIZE=~D>" (length obj)))))

(defmethod make-receiver-reader ((hst host))
  (let ((getstr (make-array *max-line-length*
                            :element-type 'character :fill-pointer *max-line-length*)))
    #'(lambda (stream)
        (setf (fill-pointer getstr) *max-line-length*)
        (let* ((n-char (setf (fill-pointer getstr)
                         (excl:read-line-into getstr stream nil 0)))
               (eof-p (= 0 n-char))
               (msg (if eof-p '(:|eof|)
                      (let ((msg0 (split-string getstr)))
                        (rplaca msg0 (intern (car msg0) :keyword))
                        msg0))))
          (if (or (eq :|task| (car msg)) (eq :|rslt| (car msg)))
              (rplacd (last msg) (read-body stream)))
          #-tcell-no-transfer-log
          (when *transfer-log*          ; debug print
            (format *error-output* "Received ~S from ~S~%"
              getstr (hostinfo hst))
            (force-output *error-output*))
          (values (cons hst msg) eof-p))
        )))

;;; task, rslt��body�����ɤ߹���
(defun read-body (stream)
  (let ((ret '()))
    (loop
      (let* ((pre (read-line stream t))
             (len (length pre)))
        ;; ���Ԥǽ�λ
        (when (= len 0) (return))
        (push pre ret)
        (push #\Newline ret)
        #-tcell-no-transfer-log
        (when *transfer-log*            ; debug print
          (write-string pre *error-output*)
          (write-char #\Newline *error-output*))
        ;; �ǽ�ιԤ�#\(�Ǥ���äƤ����鼡�ιԤ�byte-header��������byte-data
        (when (and (>= len 1)
                   (char= #\( (aref pre (- len 1))))
          #-tcell-no-transfer-log
          (when *transfer-log* (terpri *error-output*)) ; debug print
          (let* ((byte-header (read-line stream t))
                 (whole-size (parse-integer byte-header :junk-allowed t)))
            (push byte-header ret)
            (push #\Newline ret)
            #-tcell-no-transfer-log
            (when *transfer-log*        ; debug print
              (write-string byte-header *error-output*)
              (terpri *error-output*))
            (let ((byte-data (make-array whole-size)))
              (read-sequence byte-data stream :end whole-size)
              (push byte-data ret)
              #-tcell-no-transfer-log
              (when *transfer-log*      ; debug print
                (write-msg-log byte-data *error-output*))
              )))))
    (nreverse ret)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; �Хåե��ƻ�->����
(defmethod monitor-and-send-buffer ((sbuf shared-buffer) dest
                                    &optional (writer #'write-string))
  (let ((gate (sbuf-gate sbuf)))
    (loop
      (mp:process-wait "Waiting for something is added to the buffer"
                       #'mp:gate-open-p gate)
      (while (mp:gate-open-p gate)
        (funcall writer (delete-buffer sbuf) dest))
      (force-output dest))))

;; ����->�Хåե����ɲ�
(defmethod receive-and-add-to-buffer ((sbuf shared-buffer) src
                                      &optional (reader #'read-line))
  (loop
    (multiple-value-bind (obj eof-p) (funcall reader src)
      (add-buffer obj sbuf)
      (when eof-p (return)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; ���Ǥؤ�accessor

;;; �Ҥ��ɲá����
(defmethod add-child ((sv tcell-server) (chld child))
  (setf (child-id chld)	(ts-child-next-id sv))
  (incf (ts-child-next-id sv))
  (incf (ts-n-children sv))
  (push chld (ts-children sv))
  (unless (ts-eldest-child sv)
    (setf (ts-eldest-child sv) chld)))

(defmethod remove-child ((sv tcell-server) (prnt parent))
  ;; do nothing
  )

(defmethod remove-child ((sv tcell-server) (chld child))
  (finalize chld)
  (setf (ts-children sv) (delete chld (ts-children sv) :count 1))
  (delete-treq-any sv chld "*")
  (when (eq (ts-eldest-child sv) chld)
    (setf (ts-eldest-child sv) (car (last (ts-children sv)))))
  (decf (ts-n-children sv))
  )

;;; (= id n) �λҤؤΥ�������
(defmethod nth-child ((sv tcell-server) n)
  (find n (ts-children sv)
        :key #'(lambda (chld) (child-id chld))
        :test #'=))

;;; ���ֻŻ����ĤäƤ����ʻ�
(defmethod most-divisible-child ((sv tcell-server))
  (let ((max nil) (maxchld nil))
    (loop for chld in (ts-children sv)
        do (when (and (> (child-diff-task-rslt chld) 0)
                      (or (null max) (> (child-wsize chld) max)))
             (setq max (child-wsize chld))
             (setq maxchld chld)))
    maxchld))

;;; �Ҥ�work-size �򹹿�
(defmethod renew-work-size ((chld child) wsize)
  (setf (child-wsize chld) wsize))

;;; treq-any-list �ؤ������ɲ�
;;; p-task-head �ϡ�"<treq��from��id>" ":" "<treq��task-head>" 
(defmethod push-treq-any0 ((sv tcell-server) (tae ta-entry))
  (push tae (ts-talist sv)))
(defmethod push-treq-any ((sv tcell-server) (from host) (p-task-head string))
  (push-treq-any0 sv (make-instance 'ta-entry :from from :head p-task-head)))

;;; treq-any-list ��pop
(defmethod pop-treq-any ((sv tcell-server))
  (aand (pop (ts-talist sv))
        (list (tae-from it) (tae-head it))))


;;; treq-any-list ����������Ǻ��
(defmethod delete-treq-any ((sv tcell-server) (from host) (p-task-head string))
  (setf (ts-talist sv)
    (delete (make-instance 'ta-entry :from from :head p-task-head)
            (ts-talist sv)
            :test #'ta-entry-match)))

;;; treq-any-list ��member��
(defmethod member-treq-any ((sv tcell-server) (from host) (p-task-head string))
  (member (make-instance 'ta-entry :from from :head p-task-head)
          (ts-talist sv)
          :test #'ta-entry-match))

;;; ta-entry ��Ʊ����
(defmethod ta-entry-match ((x ta-entry) (y ta-entry))
  (and (eq (tae-from x) (tae-from y))
       (or (string= "*" (tae-head x))
           (string= "*" (tae-head y))
           (string= (tae-head x) (tae-head y)))))

;; ta-entry �ξ���
(defmethod ta-entry-info ((tae ta-entry))
  `((from ,(hostinfo (tae-from tae)))
    (head ,(tae-head tae))))

;; buffer�ؤ��ɲ�
(defmethod add-buffer (elm (buf buffer))
  (insert-queue elm (buf-body buf)))

(defmethod add-buffer :around (elm (sbuf shared-buffer))
  (declare (ignorable elm))
  (mp:with-process-lock ((sbuf-lock sbuf))
    (prog1 (call-next-method)
      (mp:open-gate (sbuf-gate sbuf)))))

;; ��buffer
(defmethod empty-buffer-p ((buf buffer))
  (empty-queue-p (buf-body buf)))

;; buffer������Ф�
(defmethod delete-buffer ((buf buffer))
  (delete-queue (buf-body buf)))

(defmethod delete-buffer :around ((sbuf shared-buffer))
  (mp:with-process-lock ((sbuf-lock sbuf))
    (prog1 (call-next-method)
      (when (empty-buffer-p sbuf)
        (mp:close-gate (sbuf-gate sbuf))))))

;; buffer���鸡�����Ƽ��Ф�
(defmethod find-delete-buffer ((buf buffer) test &key (key #'identity))
  (find-delete-queue (buf-body buf) test :key key))

(defmethod find-delete-buffer :around ((sbuf shared-buffer) test
                                       &key (key #'identity)
                                            (wait nil))
  (declare (ignore test key))
  (block :end
    (tagbody
      :retry
      (mp:with-process-lock ((sbuf-lock sbuf))
        (let ((item (call-next-method)))
          (when item
            (when (empty-buffer-p sbuf)
              (mp:close-gate (sbuf-gate sbuf)))
            (return-from :end item))
          (unless wait
            (return-from :end nil))
          (mp:close-gate (sbuf-gate sbuf))))
      (mp:process-wait "FIND-DELETE-BUFFER-WAIT"
                       #'mp:gate-open-p (sbuf-gate sbuf))
      (go :retry))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; ���Х��ɥ쥹ʸ��������
(defmethod hostid ((chld child))
  (format nil "~D" (child-id chld)))

(defmethod hostid ((prnt parent))
  "p")

(defmethod hostid-to-host ((sv tcell-server) hostid)
  (cond
   ((string= "p" hostid)
    (ts-parent sv))
   (t
    (nth-child sv (parse-integer hostid)))))

;;; ���ɥ쥹����Ƭ���ڤ��äơ��Ĥ���ڤ��ä���Ƭ���ɥ쥹
;;; ����������ۥ��Ȥ��֤�
(defmethod head-shift ((sv tcell-server) head-string)
  (let* ((sp-head (split-string-1 head-string #\:))
         (host (hostid-to-host sv (first sp-head))))
    (unless host
      (warn "Connection from/to ~S does not exist." (first sp-head)))
    (list host (second sp-head))))

;;; ���ɥ쥹head-string����Ƭ��hst��id���ɲä�����Τ��֤�
(defmethod head-push ((hst host) head-string)
  (string+ (hostid hst) ":" head-string))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; ��å���������
(defmethod send ((to host) obj)
  (add-buffer obj (send-buffer (host-sender to))))

(defmethod send ((to (eql nil)) obj)
  (format *error-output* "Failed to send ~S~%" (msg-log-string obj)))

;; debug print
#-tcell-no-transfer-log
(defmethod send :after ((to host) obj)
  (when *transfer-log*
    (format *error-output* "~&Sent ~S to ~S~%"
      (string-right-trim-space (msg-log-string obj)) (hostinfo to))))

(defmethod send-treq (to task-head treq-head)
  (send to (list "treq " task-head #\Space treq-head #\Newline)))

;; �������ä�task�����������ǽɾ���ѡ�
;; funcall��function �� send-rslt (to terminal-parent) �ǥ��åȤ��Ƥ���
(defmethod send-treq :after ((to terminal-parent) task-head treq-head)
  (declare (ignore task-head treq-head))
  (funcall (parent-auto-resend-func to)))

(defmethod send-task (to wsize-str rslt-head task-head task-no task-body)
  (send to (list "task "
                 wsize-str #\Space
                 rslt-head #\Space
                 task-head #\Space
                 task-no #\Newline
                 task-body #\Newline)))
(defmethod send-task :after ((to child) wsize-str rslt-head task-head task-no task-body)
  (declare (ignore rslt-head task-head task-no task-body))
  (incf (child-diff-task-rslt to))
  (let ((wsize (parse-integer wsize-str)))
    (renew-work-size to (- wsize))))

(defmethod send-rslt (to rslt-head rslt-body)
  (send to (list "rslt " rslt-head #\Newline rslt-body #\Newline)))

(defmethod send-rslt :after ((to terminal-parent) rslt-head rslt-body)
  (declare (ignore rslt-body))
  ;; rack��ư���������task��ư�����ν�����task����������ǽɾ���ѡ�
  (when (parent-auto-rack to)
    (let ((end-time (get-internal-real-time)))
      (aif (find rslt-head (parent-task-home to) :test #'string=
                 :key #'(lambda (x) (third (task-home-task-cmd x))))
           (let* ((cmd (task-home-task-cmd it))
                  (rslt-head (third cmd))
                  (rslt-no (parse-integer rslt-head :junk-allowed t))
                  (rack-to (task-home-rack-to it))
                  (rack-task-head (task-home-rack-task-head it))
                  (start-time (task-home-start-time it)))
             ;; rack�ֿ�
             (format *error-output*
               "~&Time: ~S~%~
                Auto-send \"rack ~A\" to ~S~%"
               (/ (- end-time start-time)
                  internal-time-units-per-second 1.0)
               rack-task-head (hostinfo rack-to))
             (send-rack rack-to rack-task-head)
             ;; n-resend�����å���task��ư�������ؿ��򥻥å�
             ;; rslt-no��rslt-head�κǸ�ο����ˤ����ä�����Υ�����Ȥ����
             (if (< rslt-no (parent-auto-resend-task to))
                 (let ((new-rslt-head (format nil "~D" (1+ rslt-no))))
                   (setf (caddr cmd) new-rslt-head)
                   ;; treq�������鼫ưŪ��task�����������褦�˴ؿ��򥻥å�
                   (setf (parent-auto-resend-func to)
                     #'(lambda ()
                         (sleep 1)
                         (excl:gc t)
                         (format *error-output* "~&auto-resend-task~%")
                         (setf (task-home-start-time it) (get-internal-real-time))
                         (send-task rack-to ; rack���������Ʊ���Ǥ褤���Ȥ���
                                    (second cmd) (head-push to new-rslt-head)
                                    rack-task-head (fifth cmd) (nthcdr 5 cmd))
                         (setf (parent-auto-resend-func to) #'(lambda ())))))
               ;; task-home�Υ���ȥ���
               (delete it (parent-task-home to) :count 1)))
           (warn "No task-home corresponding to rslt ~S" rslt-head)))))

(defmethod send-none (to task-head)
  (send to (list "none " task-head #\Newline)))

(defmethod send-rack (to task-head)
  (send to (list "rack " task-head #\Newline)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Dispatch
(defmethod proc-cmd ((sv tcell-server) (from host) cmd)
  (case (car cmd)
    (:|treq| (proc-treq sv from cmd))
    (:|task| (proc-task sv from cmd))
    (:|none| (proc-none sv from cmd))
    (:|rslt| (proc-rslt sv from cmd))
    (:|rack| (proc-rack sv from cmd))
    (:|eof|  (remove-child sv from))
    (:|log|  (proc-log sv from cmd))
    (:|stat| (print-server-status sv))
    (:|eval| (print (eval (read-from-string (strcat (cdr cmd) #\Space)))))
    (otherwise (warn "Unknown Command:~S" cmd))))

;;; treq
(defmethod proc-treq ((sv tcell-server) (from host) cmd)
  (let ((p-task-head (head-push from (second cmd))) ; �������׵��
        (treq-head (third cmd)))        ; �׵���
    (unless
        (if (string= "any" treq-head)
            (try-send-treq-any sv from p-task-head)
          (destructuring-bind (hst0 s-treq-head)
              (head-shift sv treq-head)
            (if *transfer-treq-always-if-notany*
                (send-treq hst0 p-task-head s-treq-head)
              (try-send-treq sv hst0 p-task-head s-treq-head))))
      ;; treq������ʤ��ä����
      (refuse-treq sv from p-task-head))))

(defmethod try-send-treq-any ((sv tcell-server) (from host) p-task-head)
  (or (awhen (most-divisible-child sv)
        (try-send-treq sv it p-task-head "any"))
      ;; ��ʬ�ΤȤ���˻Ż����ʤ���С�eldest�ʻҤ���ɽ���ƿƤ�ʹ���ˤ���
      (and (eq (ts-eldest-child sv) from)
           (try-send-treq sv (ts-parent sv)
                          p-task-head "any"))))

(defmethod try-send-treq ((sv tcell-server) (to host) p-task-head s-treq-head)
  (send-treq to p-task-head s-treq-head)
  t)

;; �����������Ƥ����send
(defmethod try-send-treq :around ((sv tcell-server) (to child) p-task-head s-treq-head)
  (declare (ignore p-task-head s-treq-head))
  (if (> (child-diff-task-rslt to) 0)
      (call-next-method)
    nil))

(defmethod refuse-treq ((sv tcell-server) (from host) p-task-head)
  (if (member-treq-any sv from p-task-head)
      (send-none from (second (head-shift sv p-task-head)))
    (push-treq-any sv from p-task-head)))

;; treq-any-list�ˤ������Ǥ� try���ʤ���
(defmethod retry-treq ((sv tcell-server))
  (loop
      for n-sent upfrom 0
      do (aif (pop-treq-any sv)
             (destructuring-bind (from head) it
               (unless (try-send-treq-any sv from head)
                 (push-treq-any sv from head)
                 (loop-finish)))
           (loop-finish))
      finally (return n-sent)))

;;; task
(defmethod proc-task ((sv tcell-server) (from host) cmd)
  (destructuring-bind (to s-task-head)
      (head-shift sv (fourth cmd))      ; ������������
    (let ((wsize-str (second cmd))      ; �Ż����礭��
          (p-rslt-head (head-push from (third cmd))) ; ����ֿ���
          (task-no (fifth cmd))         ; �������ֹ� ��fib, lu, ...��
          (task-body (nthcdr 5 cmd)))
      (send-task to wsize-str p-rslt-head s-task-head task-no
                 task-body)
      (retry-treq sv)
      )))

(defmethod proc-task :before ((sv tcell-server) (from child) cmd)
  (let ((wsize (parse-integer (second cmd))))
    (renew-work-size from (- wsize))))

;; rack��ư�����Τ���˼�����ä�task��Ф��Ƥ���
(defmethod proc-task :after ((sv tcell-server) (from terminal-parent) cmd)
  (when (parent-auto-rack from)
    (destructuring-bind (to s-task-head)
        (head-shift sv (fourth cmd))
      (let ((th-entry (make-task-home
                       :task-cmd cmd
                       :rack-to   to    ; rack������
                       :rack-task-head s-task-head ; rack��task-head
                       :start-time 0))) ; ���ϻ���ʤ��Ȥǡ�
        (push th-entry (parent-task-home from))
        (setf (task-home-start-time th-entry) (get-internal-real-time))))))

;;; none
(defmethod proc-none ((sv tcell-server) (from host) cmd)
  (destructuring-bind (to s-task-head)
      (head-shift sv (second cmd))      ; none������
    (send-none to s-task-head)))

;;; rslt
(defmethod proc-rslt ((sv tcell-server) (from host) cmd)
  (destructuring-bind (to s-rslt-head)
      (head-shift sv (second cmd))      ; rslt������
    (let ((rslt-body (cddr cmd)))
      (send-rslt to s-rslt-head rslt-body))))

(defmethod proc-rslt :before ((sv tcell-server) (from child) cmd)
  (declare (ignore cmd))
  (when (< (decf (child-diff-task-rslt from)) 0)
    (warn "~S: diff-task-rslt less than 0!" (hostinfo from))))

;;; rack
(defmethod proc-rack ((sv tcell-server) (from host) cmd)
  (destructuring-bind (to s-task-head)
      (head-shift sv (second cmd))      ; rack������
    (send-rack to s-task-head)))

;;; log
(defmethod proc-log ((sv tcell-server) (from host) cmd)
  (loop
      for str in (cdr cmd)
      as mode = (read-from-string str)
      as setter in (list #'toggle-transfer-log #'toggle-connection-log)
      do (funcall setter mode))
  (show-log-mode))

(defun toggle-transfer-log (mode)
  (setq *transfer-log* (not (not mode))))

(defun toggle-connection-log (mode)
  (setq *connection-log* (not (not mode))))

(defun show-log-mode ()
  (pprint `((*transfer-log* #-tcell-no-transfer-log ,*transfer-log*
                            #+tcell-no-transfer-log :invalidated-by-features)
            (*connection-log* ,*connection-log*))
          *error-output*)
  (terpri *error-output*))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun make-and-start-server (&key 
                              (local-host *server-host*)
                              (children-port *children-port*)
                              (retry *retry*)
                              (terminal-parent t)
                              (auto-rack t) ; for terminal paren
                              (auto-resend-task 0) ; for terminal-parent
                              (parent-host *parent-host* ph-given)
                              (parent-port *parent-port*))
  (when ph-given (setq terminal-parent nil))
  (let* ((sv (make-instance 'tcell-server
               :local-host local-host
               :children-port children-port
               :retry retry))
         (prnt (if terminal-parent
                   (make-instance 'terminal-parent
                     :server sv :auto-rack auto-rack
                     :auto-resend-task auto-resend-task)
                 (make-instance 'parent :server sv
                                :host parent-host
                                :port parent-port))))
    (start-server sv prnt)))


;; ά��
(defun ms (&rest args)
  (apply #'make-and-start-server args))

;; gero�Ǥ�ɾ����
(defun gs ()
  (make-and-start-server :auto-resend-task 4 :local-host "gero00"))
