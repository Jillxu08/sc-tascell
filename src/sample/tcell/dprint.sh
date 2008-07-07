;;; Debug print�ѥޥ���
;;(%defconstant VERBOSE 1)                ; �����ȥ����Ȥ���ȥǥХå����ϤΤ���Υ����ɤ�̵����
(%ifdef* VERBOSE
 (%defmacro DEBUG-STMTS (n &rest stmts) `(if (>= option.verbose ,n) (begin ,@stmts)))
 (%defmacro DEBUG-PRINT (n &rest args)
  `(if (>= option.verbose ,n) (csym::fprintf stderr ,@args)))
 %else
 (%defmacro DEBUG-STMTS (n &rest stmts) `(begin))
 (%defmacro DEBUG-PRINT (n &rest args) `(begin)))
