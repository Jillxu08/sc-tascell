(eval-when (:compile-toplevel :load-toplevel :execute)
  (require "SC-MISC" "../../../sc-misc.lsp"))
#+sc-system (use-package "SC-MISC")
(use-package :socket)

(defvar *remote-io-host* "localhost")
(defvar *remote-io-port* 8888)

(defun exec-with-remote-io (command &key (host *remote-io-host*) (port *remote-io-port*))
  (with-open-stream (sock (make-socket :remote-host host :remote-port port))
    (multiple-value-bind (cmd-input cmd-output cmd-error process)
	(excl:run-shell-command
	 command :input :stream :output :stream :error-output :stream
	 :separate-streams t :wait nil)
      (unwind-protect 
	   (let ((finish-send-server nil)
		 (finish-send-command nil))
	     (loop
		;; ��������В����钼���� -> ��ג�풥������˒�ϒ��
		(unless finish-send-command
		  (input-buffer-to-output sock cmd-input)
		  (force-output cmd-input)
		  (when (eof-p sock)
		    (setq finish-send-command t)
		    (finish-output cmd-input)
		    (shutdown sock :direction :input)
		    (close cmd-input)))
		;; ��ג�풥���������钤Β�В�� -> ��������В�ؒ�����
		(unless finish-send-server
		  (input-buffer-to-output cmd-output sock *standard-output*)
		  (force-output sock)
		  (force-output)
		  (when (eof-p cmd-output)
		    (setq finish-send-server t)
		    (finish-output sock)
		    (shutdown sock :direction :output)))
	       ;; �����������Ȓ�򒤫������
	       ;;(sleep 1/2)
	       (mp:wait-for-input-available (list sock cmd-output))
	       ;; ��ܒ³�����ڒ�钤쒤���钽��λ
		(unless (open-stream-p sock)
		  (sys:reap-os-subprocess :pid process :wait t)
		  (return))
		))
	;; Protected form
	(close cmd-input)
	(close cmd-output)
	(close cmd-error))
      )))

(let ((*procs* '()))
  (defun start-procs (n &rest args)
    (loop for i from 1 to n
	do (let ((new-proc
		  (mp:process-run-function (symbol-name (gensym "PROC"))
		    #'apply #'exec-with-remote-io args)))
	     (push new-proc *procs*)))
    *procs*)
  (defun kill-procs ()
    (prog1
	(mapc #'mp:process-kill *procs*)
      (setq *procs* '()))))

(defun exec-with-remote-io-server-terminal
    (&key (port *remote-io-port*) (input *standard-input*) (output *standard-output*))
  (with-open-stream (p-sock (make-socket :connect :passive :local-port port :reuse-address t))
    (loop
       (with-open-stream (strm (accept-connection p-sock))
	 (format *error-output* "~&[Start: ~S]~%" strm)
	 (let ((finish-send-client nil))
	   (loop
	      ;; �ü��������钤Β����ϒ�򒥯��钥�������Ȓ�˒��������
	      ;; '.' ��ǒ�ϒ�ޒ�뒹Ԓ�ϒ�Ò�̒�ʒ�Ւ̣
	      (unless finish-send-client
		(do ((line (read-line input) (read-line input)))
		    ((cond ((string= "." line)     ; �����ޒ�ǒ�Β����ϒ�������
			    (force-output strm)
			    t)
			   ((string= ".end" line)  ; EOF
			    (setq finish-send-client t)
			    (finish-output strm)
			    (shutdown strm :direction :output)
			    t)
			   ((and (>= (length line) 1)
				 (eql #\. (aref line 0)))
			    (format strm "~A~%" (string-left-ntrim line 1))
			    nil)
			   (t
			    (format strm "~A~%" line)
			    nil))
		     )))
	      ;; �����钥�������Ȓ����钤Β�В�ϒ��output��˒�В��
	      (input-buffer-to-output strm output)
	      (force-output output)
	      (when (eof-p strm)
		(shutdown strm :direction :input)
		(return))))
	 (format *error-output* "~&[Finish]~%"))
       )))
