;;; mhtml.el: -*- Emacs-Lisp -*-  mode for the Meta-HTML language.
;;;
;;; Copyright (c) 1995 Brian J. Fox
;;; Author: Brian J. Fox (bfox@ai.mit.edu) Sat Aug 26 18:18:13 1995.
;;;
;;; A first crack at mhtml mode.
;;;
;;; The base major mode for editing Meta-HTML code.
;;;
;;; To use this mode, put the following in your ~/.emacs file
;;;
;;; (autoload 'meta-html-mode "mhtml" "Edit files of Meta-HTML code." t)
;;;
;;; (if (assoc "\\.mhtml$" auto-mode-alist)
;;;     (rplacd (assoc "\\.mhtml$" auto-mode-alist) 'meta-html-mode)
;;;   (setq auto-mode-alist (cons '("\\.mhtml$" . meta-html-mode)
;;;				  auto-mode-alist)))
;;;
;;; Then, place this file where Emacs can find it, like
;;; /usr/local/share/emacs/site-lisp, or whatever.
;;;
;;; Code:

(if (assoc "\\.mhtml$" auto-mode-alist)
    (rplacd (assoc "\\.mhtml$" auto-mode-alist) 'meta-html-mode)
  (setq auto-mode-alist (cons '("\\.mhtml$" . meta-html-mode)
			      auto-mode-alist)))

(defvar mhtml-mode-syntax-table nil "")

(if (not mhtml-mode-syntax-table)
    (progn (setq mhtml-mode-syntax-table
		 (copy-syntax-table lisp-mode-syntax-table))
	   (modify-syntax-entry ?\< "(>" mhtml-mode-syntax-table)
	   (modify-syntax-entry ?\> ")<" mhtml-mode-syntax-table)
	   ;;; (modify-syntax-entry ?\( "-" mhtml-mode-syntax-table)
	   ;;; (modify-syntax-entry ?\) "-" mhtml-mode-syntax-table)
	   (modify-syntax-entry ?\; "w" mhtml-mode-syntax-table)
	   (modify-syntax-entry ?\& "w" mhtml-mode-syntax-table)
	   (modify-syntax-entry ?\| "." mhtml-mode-syntax-table)
	   (modify-syntax-entry ?\; ". 12" mhtml-mode-syntax-table)))

(defun mhtml-mode-variables (mhtml-syntax)
  (set-syntax-table mhtml-mode-syntax-table)
  (make-local-variable 'paragraph-start)
  (setq paragraph-start (concat "^$\\|" page-delimiter))
  (make-local-variable 'paragraph-separate)
  (setq paragraph-separate paragraph-start)
  (make-local-variable 'paragraph-ignore-fill-prefix)
  (setq paragraph-ignore-fill-prefix t)
  (make-local-variable 'fill-paragraph-function)
  (setq fill-paragraph-function 'fill-paragraph)
  (make-local-variable 'indent-line-function)
  (setq indent-line-function 'mhtml-indent-line)
  (make-local-variable 'indent-region-function)
  (setq indent-region-function 'mhtml-indent-region)
  (make-local-variable 'parse-sexp-ignore-comments)
  (setq parse-sexp-ignore-comments t)
  (make-local-variable 'outline-regexp)
  (setq outline-regexp ";;; \\|<....")
  (make-local-variable 'comment-start)
  (setq comment-start ";;;")
  (make-local-variable 'comment-start-skip)
  (setq comment-start-skip ";;;+ *")
  (make-local-variable 'comment-column)
  (setq comment-column 40)
  (make-local-variable 'comment-indent-function)
  (setq comment-indent-function 'mhtml-comment-indent)
  (make-local-variable 'add-log-current-defun-function)
  (setq add-log-current-defun-function 'mhtml-add-log-current-defun-function))

(defvar mhtml-mode-map ()  "Keymap for Meta-HTML mode.")

(if mhtml-mode-map
    ()
  (let ((map (make-sparse-keymap "Meta-HTML")))
    (define-key map [separator-format] '("--"))
    (define-key map [comment-region] '("Comment Out Region" . comment-region))
    (define-key map [indent-region] '("Indent Region" . indent-region))
    (define-key map "\e\C-q" 'mhtml-indent-region)
    (define-key map "\eq" 'mhtml-indent-sexp)
    (define-key map "\e\C-d" 'mhtml-show-documentation)
    (define-key map [indent-line] '("Indent Line" . indent-relative))
    (put 'eval-region 'menu-enable 'mark-active)
    (put 'comment-region 'menu-enable 'mark-active)
    (put 'indent-region 'menu-enable 'mark-active)
    (setq mhtml-mode-map map)))

;;;
;;; Some stuff for font-lock mode.
;;;
(defconst mhtml-all-keywords
  '("set-var" "get-var" "unset-var" "subst-in-var" "subst-in-string"
    "package-vars" "package-names" "package-to-alist" "alist-to-package"
    "increment" "if" "ifeq" "when" "whenelse" "elsewhen" "not" "and" "or"
    "var-case" "while"
    "defmacro" "defsubst" "defun" "define-container" "define-function"
    "define-tag" "define-variable" "defvar" "debugging-on" "match" "include"
    "replace-page" "redirect" "cgi-encode" "cgi-decode" "cgi-exec" "comment"
    "verbatim" "plain-text" "helper" "input-item" "small-caps"
    "html-quote" "subst-in-page" "set-form-input-values" "page-insert"
    "page-search" "create-session" "delete-session" "set-session-var"
    "get-session-var" "unset-session-var" "set-session-timeout"
    "require-session" "session-export" "session-import"
    "session-export-posted" "session-import-posted" "gt" "lt" "eq"
    "add" "sub" "mul" "div" "mod" "with-open-database" "with-open-stream"
    "stream-get" "stream-put" "stream-put-contents" "stream-get-contents"
    "database-save-record" "database-load-record" "database-save-package"
    "database-delete-record" "database-first-key" "database-next-key"
    "database-first-match" "database-next-match" "database-query"
    "in-package" "with-local-package" "foreach" "array-size" "array-append"
    "array-add-unique" "layout::page" "layout::left-border"
    "layout::right-border"
    "sql::with-open-database" "sql::database-exec-query"
    "sql::database-exec-sql" "sql::database-next-record"
    "sql::database-save-record" "sql::database-delete-record"
    "sql::database-load-record" "sql::database-save-package"
    "sql::number-of-rows" "sql::set-row-position" "sql::database-query"
    "sql::host-databases" "sql::database-tables" "sql::database-tables-info"
    "sql::database-table-info" "sql::database-columns"
    "sql::database-column-info" "sql::database-columns-info"
    "sql::database-query-info" "sql::database-set-options"
    "sql::cursor-get-column" "sql::query-get-column" "sql::sql-transact"
    "example" "simple-fundoc" "complex-fundoc" "prog" "server-push"))

(defun mhtml-build-font-lock-keywords (from-list)
  (let ((string "<[/]*\\("))
    (while from-list
      (setq string (concat string (car from-list))
	    from-list (cdr from-list))
      (if from-list (setq string (concat string "\\|"))))
    (setq string (concat string "\\)[ \t\n>]"))
    (list (list string '(1 font-lock-variable-name-face)))))

(defconst mhtml-font-lock-keywords
  (mhtml-build-font-lock-keywords mhtml-all-keywords))

(defun meta-html-mode ()
  (interactive)
  (mhtml-mode))

(defun mhtml-mode ()
  "Major mode for editing Meta-HTML code.
Commands:
Delete converts tabs to spaces as it moves back.
Blank lines separate paragraphs.  Three semicolons start comments.
\\{mhtml-mode-map}
Entry to this mode calls the value of `mhtml-mode-hook'
if that value is non-nil."
  (interactive)
  (kill-all-local-variables)
  (make-local-variable 'font-lock-keywords)
  (setq font-lock-keywords mhtml-font-lock-keywords)
  ;;; (make-local-variable 'font-lock-comment-start-regexp)
  (setq font-lock-comment-start-regexp ";;+")
  (use-local-map mhtml-mode-map)
  (set-syntax-table mhtml-mode-syntax-table)
  (setq major-mode 'mhtml-mode)
  (setq mode-name "Meta-HTML")
  (mhtml-mode-variables nil)
  (run-hooks 'mhtml-mode-hook))

(defvar mhtml-mode-map () "Keymap for ordinary Meta-HTML mode.")

(if mhtml-mode-map
    ()
  (setq mhtml-mode-map
	(nconc (make-sparse-keymap) indented-text-mode-map)))

(defconst mhtml-indent-offset nil "")
(defconst mhtml-indent-function 'mhtml-indent-function "")

(defun mhtml-comment-indent ()
  (current-column))

(defun mhtml-indent-line (&optional whole-exp)
  "Indent current line as Meta-HTML code.
With argument, indent any additional lines of the same expression
rigidly along with this one."
  (interactive "P")
  (let ((indent (get-mhtml-indentation)) shift-amt beg end
	(pos (- (point-max) (point))))
    (beginning-of-line)
    (setq beg (point))
    (skip-chars-forward " \t")
    (if (looking-at "\\s<\\s<\\s<")
	;; Don't alter indentation of a ;;; comment line.
	(goto-char (- (point-max) pos))
      (if (and (looking-at "\\s<") (not (looking-at "\\s<\\s<")))
	  ;; Single-semicolon comment lines should be indented
	  ;; as comment lines, not as code.
	  (progn (indent-for-comment) (forward-char -1))
	(if (listp indent) (setq indent (car indent)))
	(setq shift-amt (- indent (current-column)))
	(if (zerop shift-amt)
	    nil
	  (delete-region beg (point))
	  (indent-to indent)))
      ;; If initial point was within line's indentation,
      ;; position after the indentation.  Else stay at same point in text.
      (if (> (- (point-max) pos) (point))
	  (goto-char (- (point-max) pos)))
      ;; If desired, shift remaining lines of expression the same amount.
      (and whole-exp (not (zerop shift-amt))
	   (save-excursion
	     (goto-char beg)
	     (forward-sexp 1)
	     (setq end (point))
	     (goto-char beg)
	     (forward-line 1)
	     (setq beg (point))
	     (> end beg))
	   (indent-code-rigidly beg end shift-amt)))))

(defun build-complex-tag-regexp (from-list)
  (let ((result "<\\("))
    (while from-list
      (setq result (concat result (car from-list))
	    from-list (cdr from-list))
      (if from-list (setq result (concat result "\\|"))))
    (concat result "\\)[ \t\n>]")))

(defconst mhtml-complex-tags
  '( "when" "whenelse" "while" "with" "defsubst" "defun" "defmacro" "defstruct"
     "define-tag" "define-function" "with-open-dmaild"
     "with-open-database" "with-open-stream" "server-push"
     "comment" "verbatim" "small-caps" "plain-text" "ptext" "section"
     "ul" "ol" "dl" "table" "tr" "td" "center" "a" "form" "select" "caption"
     "applet" "subtable" "in-package" "with-local-package"
     "example" "question" "nested-items-list" "nested-item" "slide" "bullet"
     "menu" "system-menu" "foreach" "menus::popup-menu" "system-menu"
     "layout::page" "layout::left-border" "layout::right-border"
     "frameset" "msql::with-open-database" "odbc::with-open-database"
     "sql::with-open-database" "dbm::with-open-database" "def-pagelist"
     "defweakmacro" "with-safe-environment" "tab-menu" "tdbox" "description"
     "tdbox" "config::information-item" "define-db-form" "soap:envelope" "soap:body"
     "EscalatingMessageRequest" "application" "authorization" "accountNumber" "senderToken"
     "destination" "item" "message" "text" "button" "list" "selections")
  "List of tag starts which are complex.")

(defconst mhtml-complex-tag-start-regex
  (build-complex-tag-regexp mhtml-complex-tags))

(defun mhtml-count-open-tags ()
  (let* ((close-limit (save-excursion (end-of-line) (point)))
	 (open-limit (save-excursion (beginning-of-line) (point)))
	 (search-point open-limit)
	 (closers '())
	(opens 0))
    (save-excursion
      (beginning-of-defun)
      (while (setq search-point
		   (re-search-forward mhtml-complex-tag-start-regex
				      open-limit t))
	(let ((close (concat "</"
			     (buffer-substring (1+ (match-beginning 0))
					       (- (match-end 0) 1))))
	      (temp nil)
	      (done-counting nil))
	  (goto-char search-point)
	  (while (not done-counting)
	    (setq temp (re-search-forward close close-limit t))
	    (if (not temp)
		;; No matching close found.  This one is unsatisfied.
		(setq opens (1+ opens)
		      done-counting t)
	      ;; Matching close found.  This one might be satisfied, iff it
	      ;; isn't in our list of already found closes.
	      (if (not (memq temp closers))
		  (setq closers (cons temp closers)
			done-counting t)
		;; Otherwise, try again after this closer.
		(goto-char (1+ temp))))))
	(goto-char search-point)
	(forward-char 1)))
    opens))

(defvar calculate-mhtml-indent-last-sexp)

(defun get-mhtml-indentation (&optional parse-start)
  (let ((calculated (calculate-mhtml-indent parse-start)))
    (if (equal calculated 0)
	(* 2 (mhtml-count-open-tags))
      calculated)))

(defun calculate-mhtml-indent (&optional parse-start)
  "Return appropriate indentation for current line as Meta-HTML code.
In usual case returns an integer: the column to indent to.
Can instead return a list, whose car is the column to indent to.
This means that following lines at the same level of indentation
should not necessarily be indented the same way.
The second element of the list is the buffer position
of the start of the containing expression."
  (save-excursion
    (beginning-of-line)
    (let ((indent-point (point))
          state paren-depth
          ;; setting this to a number inhibits calling hook
          (desired-indent nil)
          (retry t)
          calculate-mhtml-indent-last-sexp containing-sexp)
      (if parse-start
          (goto-char parse-start)
          (beginning-of-defun))
      ;; Find outermost containing sexp
      (while (< (point) indent-point)
        (setq state (parse-partial-sexp (point) indent-point 0)))
      ;; Find innermost containing sexp
      (while (and retry
		  state
                  (> (setq paren-depth (elt state 0)) 0))
        (setq retry nil)
        (setq calculate-mhtml-indent-last-sexp (elt state 2))
        (setq containing-sexp (elt state 1))
        ;; Position following last unclosed open.
        (goto-char (1+ containing-sexp))
        ;; Is there a complete sexp since then?
        (if (and calculate-mhtml-indent-last-sexp
		 (> calculate-mhtml-indent-last-sexp (point)))
            ;; Yes, but is there a containing sexp after that?
            (let ((peek (parse-partial-sexp calculate-mhtml-indent-last-sexp
					    indent-point 0)))
              (if (setq retry (car (cdr peek))) (setq state peek)))))
      (if retry
          nil
        ;; Innermost containing sexp found
        (goto-char (1+ containing-sexp))
        (if (not calculate-mhtml-indent-last-sexp)
	    ;; indent-point immediately follows open paren.
	    ;; Don't call hook.
            (setq desired-indent (current-column))
	  ;; Find the start of first element of containing sexp.
	  (parse-partial-sexp (point) calculate-mhtml-indent-last-sexp 0 t)
	  (cond ((looking-at "\\s(")
		 ;; First element of containing sexp is a list.
		 ;; Indent under that list.
		 )
		((> (save-excursion (forward-line 1) (point))
		    calculate-mhtml-indent-last-sexp)
		 ;; This is the first line to start within the containing sexp.
		 ;; It's almost certainly a function call.
		 (if (= (point) calculate-mhtml-indent-last-sexp)
		     ;; Containing sexp has nothing before this line
		     ;; except the first element.  Indent under that element.
		     nil
		   ;; Skip the first element, find start of second (the first
		   ;; argument of the function call) and indent under.
		   (progn (forward-sexp 1)
			  (parse-partial-sexp (point)
					      calculate-mhtml-indent-last-sexp
					      0 t)))
		 (backward-prefix-chars))
		(t
		 ;; Indent beneath first sexp on same line as
		 ;; calculate-mhtml-indent-last-sexp.  Again, it's
		 ;; almost certainly a function call.
		 (goto-char calculate-mhtml-indent-last-sexp)
		 (beginning-of-line)
		 (parse-partial-sexp (point) calculate-mhtml-indent-last-sexp
				     0 t)
		 (backward-prefix-chars)))))
      ;; Point is at the point to indent under unless we are inside a string.
      ;; Call indentation hook except when overridden by mhtml-indent-offset
      ;; or if the desired indentation has already been computed.
      (let ((normal-indent (current-column)))
        (cond ((elt state 3)
               ;; Inside a string, don't change indentation.
               (goto-char indent-point)
               (skip-chars-forward " \t")
               (current-column))
              ((and (integerp mhtml-indent-offset) containing-sexp)
               ;; Indent by constant offset
               (goto-char containing-sexp)
               (+ (current-column) mhtml-indent-offset))
              (desired-indent)
              ((and (boundp 'mhtml-indent-function)
                    mhtml-indent-function
                    (not retry))
               (or (funcall mhtml-indent-function indent-point state)
                   normal-indent))
              (t
               normal-indent))))))

(defun mhtml-indent-function (indent-point state)
  (let ((normal-indent (current-column)))
    (goto-char (1+ (elt state 1)))
    (parse-partial-sexp (point) calculate-mhtml-indent-last-sexp 0 t)
    (if (and (elt state 2)
             (not (looking-at "\\sw\\|\\s_")))
        ;; car of form doesn't seem to be a a symbol
        (progn
          (if (not (> (save-excursion (forward-line 1) (point))
                      calculate-mhtml-indent-last-sexp))
              (progn (goto-char calculate-mhtml-indent-last-sexp)
                     (beginning-of-line)
                     (parse-partial-sexp (point)
					 calculate-mhtml-indent-last-sexp 0 t)))
          ;; Indent under the list or under the first sexp on the same
          ;; line as calculate-mhtml-indent-last-sexp.  Note that first
          ;; thing on that line has to be complete sexp since we are
          ;; inside the innermost containing sexp.
          (backward-prefix-chars)
          (current-column))
      (let ((function (buffer-substring (point)
					(progn (forward-sexp 1) (point))))
	    method)
	(setq method (or (get (intern-soft function) 'mhtml-indent-function)
			 (get (intern-soft function) 'mhtml-indent-hook)))
	(cond ((or (eq method 'defun)
		   (and (null method)
			(> (length function) 3)
			(string-match "\\`def" function)))
	       (mhtml-indent-defform state indent-point))
	      ((integerp method)
	       (mhtml-indent-specform method state
				     indent-point normal-indent))
	      (method
		(funcall method state indent-point)))))))

(defconst mhtml-body-indent 2
  "Number of columns to indent the second line of a `(def...)' form.")

(defun mhtml-indent-specform (count state indent-point normal-indent)
  (let ((containing-form-start (elt state 1))
        (i count)
        body-indent containing-form-column)
    ;; Move to the start of containing form, calculate indentation
    ;; to use for non-distinguished forms (> count), and move past the
    ;; function symbol.  mhtml-indent-function guarantees that there is at
    ;; least one word or symbol character following open paren of containing
    ;; form.
    (goto-char containing-form-start)
    (setq containing-form-column (current-column))
    (setq body-indent (+ mhtml-body-indent containing-form-column))
    (forward-char 1)
    (forward-sexp 1)
    ;; Now find the start of the last form.
    (parse-partial-sexp (point) indent-point 1 t)
    (while (and (< (point) indent-point)
                (condition-case ()
                    (progn
                      (setq count (1- count))
                      (forward-sexp 1)
                      (parse-partial-sexp (point) indent-point 1 t))
                  (error nil))))
    ;; Point is sitting on first character of last (or count) sexp.
    (if (> count 0)
        ;; A distinguished form.  If it is the first or second form use double
        ;; mhtml-body-indent, else normal indent.  With mhtml-body-indent bound
        ;; to 2 (the default), this just happens to work the same with if as
        ;; the older code, but it makes unwind-protect, condition-case,
        ;; with-output-to-temp-buffer, et. al. much more tasteful.  The older,
        ;; less hacked, behavior can be obtained by replacing below with
        ;; (list normal-indent containing-form-start).
        (if (<= (- i count) 1)
            (list (+ containing-form-column (* 2 mhtml-body-indent))
                  containing-form-start)
            (list normal-indent containing-form-start))
      ;; A non-distinguished form.  Use body-indent if there are no
      ;; distinguished forms and this is the first undistinguished form,
      ;; or if this is the first undistinguished form and the preceding
      ;; distinguished form has indentation at least as great as body-indent.
      (if (or (and (= i 0) (= count 0))
              (and (= count 0) (<= body-indent normal-indent)))
          body-indent
          normal-indent))))

(defun mhtml-indent-defform (state indent-point)
  (goto-char (car (cdr state)))
  (forward-line 1)
  (if (> (point) (car (cdr (cdr state))))
      (progn
	(goto-char (car (cdr state)))
	(+ mhtml-body-indent (current-column)))))


;; (put 'prog 'mhtml-indent-function 0), say, causes progn to be indented
;; like defun if the first form is placed on the next line, otherwise
;; it is indented like any other form (i.e. forms line up under first).

(put 'prog 'mhtml-indent-function 'defun)
(put 'database-query 'mhtml-indent-function 'defun)
(put 'database-load-record 'mhtml-indent-function 'defun)
(put 'database-save-record 'mhtml-indent-function 'defun)
(put 'msql::database-query 'mhtml-indent-function 'defun)
(put 'msql::database-load-record 'mhtml-indent-function 'defun)
(put 'msql::database-save-record 'mhtml-indent-function 'defun)
(put 'odbc::database-query 'mhtml-indent-function 'defun)
(put 'odbc::database-load-record 'mhtml-indent-function 'defun)
(put 'odbc::database-save-record 'mhtml-indent-function 'defun)
(put 'with-open-stream 'mhtml-indent-function 'defun)
(put 'if 'mhtml-indent-function 2)
(put 'ifeq 'mhtml-indent-function 3)
(put 'var-case 'mhtml-indent-function 'defun)
(put 'set-var 'mhtml-indent-function 'defun)
(put 'get-var 'mhtml-indent-function 'defun)
(put 'set-session-var 'mhtml-indent-function 'defun)
(put 'get-session-var 'mhtml-indent-function 'defun)
(put 'PROG 'mhtml-indent-function 'defun)
(put 'DATABASE-QUERY 'mhtml-indent-function 'defun)
(put 'DATABASE-LOAD-RECORD 'mhtml-indent-function 'defun)
(put 'DATABASE-SAVE-RECORD 'mhtml-indent-function 'defun)
(put 'MSQL::DATABASE-QUERY 'mhtml-indent-function 'defun)
(put 'MSQL::DATABASE-LOAD-RECORD 'mhtml-indent-function 'defun)
(put 'MSQL::DATABASE-SAVE-RECORD 'mhtml-indent-function 'defun)
(put 'ODBC::DATABASE-QUERY 'mhtml-indent-function 'defun)
(put 'ODBC::DATABASE-LOAD-RECORD 'mhtml-indent-function 'defun)
(put 'ODBC::DATABASE-SAVE-RECORD 'mhtml-indent-function 'defun)
(put 'WITH-OPEN-STREAM 'mhtml-indent-function 'defun)
(put 'IF 'mhtml-indent-function 2)
(put 'IFEQ 'mhtml-indent-function 3)
(put 'VAR-CASE 'mhtml-indent-function 'defun)
(put 'SET-VAR 'mhtml-indent-function 'defun)
(put 'GET-VAR 'mhtml-indent-function 'defun)
(put 'SET-SESSION-VAR 'mhtml-indent-function 'defun)
(put 'GET-SESSION-VAR 'mhtml-indent-function 'defun)

(defun mhtml-indent-sexp (&optional endpos)
  "Indent each line of the list starting just after point.
If optional arg ENDPOS is given, indent each line, stopping when
ENDPOS is encountered."
  (interactive)
  (let ((indent-stack (list nil))
	(next-depth 0)
	;; If ENDPOS is non-nil, use nil as STARTING-POINT
	;; so that calculate-mhtml-indent will find the beginning of
	;; the defun we are in.
	;; If ENDPOS is nil, it is safe not to scan before point
	;; since every line we indent is more deeply nested than point is.
	(starting-point (if endpos nil (point)))
	(last-point (point))
	last-depth bol outer-loop-done inner-loop-done state this-indent)
    (or endpos
	;; Get error now if we don't have a complete sexp after point.
	(save-excursion (forward-sexp 1)))
    (save-excursion
      (setq outer-loop-done nil)
      (while (if endpos (< (point) endpos)
	       (not outer-loop-done))
	(setq last-depth next-depth
	      inner-loop-done nil)
	;; Parse this line so we can learn the state
	;; to indent the next line.
	;; This inner loop goes through only once
	;; unless a line ends inside a string.
	(while (and (not inner-loop-done)
		    (not (setq outer-loop-done (eobp))))
	  (setq state (parse-partial-sexp (point) (progn (end-of-line) (point))
					  nil nil state))
	  (setq next-depth (car state))
	  ;; If the line contains a comment other than the sort
	  ;; that is indented like code,
	  ;; indent it now with indent-for-comment.
	  ;; Comments indented like code are right already.
	  ;; In any case clear the in-comment flag in the state
	  ;; because parse-partial-sexp never sees the newlines.
	  (if (car (nthcdr 4 state))
	      (progn (indent-for-comment)
		     (end-of-line)
		     (setcar (nthcdr 4 state) nil)))
	  ;; If this line ends inside a string,
	  ;; go straight to next line, remaining within the inner loop,
	  ;; and turn off the \-flag.
	  (if (car (nthcdr 3 state))
	      (progn
		(forward-line 1)
		(setcar (nthcdr 5 state) nil))
	    (setq inner-loop-done t)))
	(and endpos
	     (<= next-depth 0)
	     (progn
	       (setq indent-stack (append indent-stack
					  (make-list (- next-depth) nil))
		     last-depth (- last-depth next-depth)
		     next-depth 0)))
	(or outer-loop-done endpos
	    (setq outer-loop-done (<= next-depth 0)))
	(if outer-loop-done
	    (forward-line 1)
	  (while (> last-depth next-depth)
	    (setq indent-stack (cdr indent-stack)
		  last-depth (1- last-depth)))
	  (while (< last-depth next-depth)
	    (setq indent-stack (cons nil indent-stack)
		  last-depth (1+ last-depth)))
	  ;; Now go to the next line and indent it according
	  ;; to what we learned from parsing the previous one.
	  (forward-line 1)
	  (setq bol (point))
	  (skip-chars-forward " \t")
	  ;; But not if the line is blank, or just a comment
	  ;; (except for double-semi comments; indent them as usual).
	  (if (or (eobp) (looking-at "\\s<\\|\n"))
	      nil
	    (if (and (car indent-stack)
		     (>= (car indent-stack) 0))
		(setq this-indent (car indent-stack))
	      (let ((val (calculate-mhtml-indent
			  (if (car indent-stack) (- (car indent-stack))
			    starting-point))))
		(if (integerp val)
		    (setcar indent-stack
			    (setq this-indent val))
		  (setcar indent-stack (- (car (cdr val))))
		  (setq this-indent (car val)))))
	    (if (/= (current-column) this-indent)
		(progn (delete-region bol (point))
		       (indent-to this-indent)))))
	(or outer-loop-done
	    (setq outer-loop-done (= (point) last-point))
	    (setq last-point (point)))))))

;; Indent every line whose first char is between START and END inclusive.
(defun mhtml-indent-region (start end)
  (interactive "r")
  (save-excursion
    (let ((total-lines (count-lines start end))
	  (which 0))
      (goto-char start)
      (while (< which total-lines)
	(beginning-of-line)
	(delete-horizontal-space)
	(if (not (eolp))
	    (indent-for-tab-command 1))
	(forward-line 1)
	(setq which (+ which 1))))))

(provide 'mhtml-mode)

(autoload 'compile-internal "compile")

(defun mhtml-compile-file (filename)
  "Run mklib on a file (default: current buffer), and append .lib to
the output filename."
  (interactive
   (list (read-file-name "Make Meta-HTML lib from source file: "
                         (buffer-file-name)
                         (buffer-file-name)
                         t)))
  (setq filename (expand-file-name filename))
  (cd (file-name-directory filename))
  (setq file (file-name-nondirectory filename))
  (compile-internal
   (concat "mklib -o " (file-name-sans-extension file) ".lib " file)
   "No more errors."))

;; Return a function which is called by the list containing point.
;; If that gives no function, return a function whose name is around point.
;; If that doesn't give a function, return nil.
(defun mhtml-function-called-at-point ()
  (save-excursion
    (up-list -1)
    (let ((beg (+ 1 (point))))
      (goto-char beg)
      (forward-sexp)
      (buffer-substring beg (point)))))

(defun mhtml-get-doc (string)
  (save-excursion
    (let ((buffer (get-buffer-create " *meta-doc*")))
      (set-buffer buffer)
      (delete-region (point-min) (point-max))
      (call-process "/www/bin/show-doc.mhc" nil buffer nil string)
      (buffer-substring (point-min) (point-max)))))

(defun mhtml-show-documentation ()
  "Display the full documentation of the current function."
  (interactive)
  (let ((function (mhtml-function-called-at-point))
	doc)
    (if function
	(setq doc (mhtml-get-doc function)))
    (if doc
	(with-output-to-temp-buffer "*Help*"
	  (save-excursion
	    (set-buffer (get-buffer "*Help*"))
	    (insert doc))
	  (print-help-return-message)
	  (save-excursion
	    (set-buffer standard-output)
	    (help-mode)
	    ;; Return the text we displayed.
	    (buffer-string)))
      (message "You didn't specify a function"))))

(defun mhtml-add-log-current-defun-function ()
  (save-excursion
    (end-of-line)
    (let ((start (re-search-backward "\n<[Dd][Ee][Ff]" (- (point) 10000) t)))
      (if start
	  (progn
	    (forward-word 1)
	    (skip-chars-forward " \t\r\n")
	    (setq start (point))
	    (forward-sexp 1)
	    (buffer-substring start (point)))
	""))))
