/* Readline.h -- the names of functions callable from within readline. */

/* Copyright (C) 1987, 1989, 1992 Free Software Foundation, Inc.

   This file is part of the GNU Readline Library, a library for
   reading lines of text with interactive input and history editing.

   The GNU Readline Library is free software; you can redistribute it
   and/or modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 1, or
   (at your option) any later version.

   The GNU Readline Library is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   The GNU General Public License is often shipped with GNU software, and
   is generally kept in a file called COPYING or LICENSE.  If you do not
   have a copy of the license, write to the Free Software Foundation,
   675 Mass Ave, Cambridge, MA 02139, USA. */

#if !defined (_READLINE_H_)
#define _READLINE_H_

#include "keymaps.h"
#include "tilde.h"
#include "complete.h"
#include "signals.h"
#include "bind.h"
#include "display.h"
#include "rltty.h"

#if defined (__cplusplus)
extern "C"
{
#endif

#define DING() ding (0, 0)
#define RL_ABORT() rl_abort (0, 0)

/* Maintaining the state of undo.  We remember individual deletes and inserts
   on a chain of things to do. */

/* The actions that undo knows how to undo.  Notice that UNDO_DELETE means
   to insert some text, and UNDO_INSERT means to delete some text.   I.e.,
   the code tells undo what to undo, not how to undo it. */
enum undo_code { UNDO_DELETE, UNDO_INSERT, UNDO_BEGIN, UNDO_END };

/* What an element of THE_UNDO_LIST looks like. */
typedef struct undo_list
{
  struct undo_list *next;
  int start, end;		/* Where the change took place. */
  char *text;			/* The text to insert, if undoing a delete. */
  enum undo_code what;		/* Delete, Insert, Begin, End. */
} UNDO_LIST;

/* The current undo list for RL_LINE_BUFFER. */
extern UNDO_LIST *rl_undo_list;

typedef int __readline_function (int count, int key);

/* The data structure for mapping textual names to code addresses. */
typedef struct
{
  char *name;
  __readline_function *function;
} FUNMAP;

extern FUNMAP **funmap;

/* The functions for manipulating the text of the line within readline.
Most of these functions are bound to keys by default. */
extern int rl_tilde_expand (int count, int key);
extern int rl_beg_of_line (int count, int key);
extern int rl_backward (int count, int key);
extern int rl_delete (int count, int key);
extern int rl_end_of_line (int count, int key);
extern int rl_forward (int count, int key);
extern int ding (int count, int key);
extern int rl_backward (int count, int key);
extern int rl_newline (int count, int key);
extern int rl_kill_line (int count, int key);
extern int rl_clear_screen (int count, int key);
extern int rl_get_next_history (int count, int key);
extern int rl_get_previous_history (int count, int key);
extern int rl_quoted_insert (int count, int key);
extern int rl_reverse_search_history (int count, int key);
extern int rl_transpose_chars (int count, int key);
extern int rl_unix_line_discard (int count, int key);
extern int rl_quoted_insert (int count, int key);
extern int rl_unix_word_rubout (int count, int key);
extern int rl_yank (int count, int key);
extern int rl_rubout (int count, int key);
extern int rl_backward_word (int count, int key);
extern int rl_kill_word (int count, int key);
extern int rl_forward_word (int count, int key);
extern int rl_tab_insert (int count, int key);
extern int rl_yank_pop (int count, int key);
extern int rl_yank_nth_arg (int count, int key);
extern int rl_backward_kill_word (int count, int key);
extern int rl_backward_kill_line (int count, int key);
extern int rl_transpose_words (int count, int key);
extern int rl_complete (int count, int key);
extern int rl_possible_completions (int count, int key);
extern int rl_insert_completions (int count, int key);
extern int rl_do_lowercase_version (int count, int key);
extern int rl_kill_full_line (int count, int key);
extern int rl_digit_argument (int count, int key);
extern int rl_universal_argument (int count, int key);
extern int rl_digit_argument (int count, int key);
extern int rl_abort (int count, int key);
extern int rl_undo_command (int count, int key);
extern int rl_revert_line (int count, int key);
extern int rl_beginning_of_history (int count, int key);
extern int rl_end_of_history (int count, int key);
extern int rl_forward_search_history (int count, int key);
extern int rl_insert (int count, int key);
extern int rl_upcase_word (int count, int key);
extern int rl_downcase_word (int count, int key);
extern int rl_capitalize_word (int count, int key);
extern int rl_restart_output (int count, int key);
extern int rl_re_read_init_file (int count, int key);
extern int rl_dump_functions (int count, int key);
extern int rl_delete_horizontal_space (int count, int key);
extern int rl_history_search_forward (int count, int key);
extern int rl_history_search_backward (int count, int key);
extern int rl_tty_status (int count, int key);
extern int rl_yank_last_arg (int count, int key);

/* `Public' utility functions. */
/* Insert a string of text into the line at point.  This is the only
   way that you should do insertion.  rl_insert () calls this
   function. */
extern int rl_insert_text (char *string);

/* Delete the string between FROM and TO.  FROM is
   inclusive, TO is not. */
extern int rl_delete_text (int from, int to);

/* The way to kill something.  This appends or prepends to the last
   kill, if the last command was a kill command.  if FROM is less
   than TO, then the text is appended, otherwise prepended.  If the
   last command was not a kill command, then a new slot is made for
   this kill. */
extern int rl_kill_text (int from, int to);
extern char *rl_copy_text (int from, int to);
extern int rl_expand_prompt (char *prompt);
extern int rl_initialize (void);
extern int rl_init_argument (void);

extern int rl_read_key (void);
extern int rl_getc (FILE *stream);
extern int rl_stuff_char (int stuffer);
extern void maybe_save_line (void);
extern void maybe_unsave_line (void);
extern void maybe_replace_line (void);
extern void rl_modifying (int start, int end);

/* Remember how to undo something.  Concatenate some undos if that
   seems right. */
extern void rl_add_undo (enum undo_code what, int start, int end, char *text);
extern void rl_begin_undo_group (void);
extern void rl_end_undo_group (void);
extern void free_undo_list (void);
extern int rl_do_undo (void);

/* Not available unless readline is compiled -DPAREN_MATCHING. */
extern int rl_insert_close (int count, int invoking_key);

/* These are *both* defined even when VI_MODE is not. */
extern int rl_vi_editing_mode (int count, int key);
extern int rl_emacs_editing_mode (int count, int key);

/* Non incremental history searching. */
extern int rl_noninc_forward_search (int count, int key);
extern int rl_noninc_reverse_search (int count, int key);
extern int rl_noninc_forward_search_again (int count, int key);
extern int rl_noninc_reverse_search_again (int count, int key);

/* Things for vi mode. Not available unless readline is compiled -DVI_MODE. */
#if defined (VI_MODE)
#include "vi_mode.h"
#endif

/* Keyboard macro commands. */
extern int rl_start_kbd_macro (int count, int key);
extern int rl_end_kbd_macro (int count, int key);
extern int rl_call_last_kbd_macro (int count, int key);
extern int rl_arrow_keys(int count, int key);
extern int rl_refresh_line (int count, int key);

/* **************************************************************** */
/*								    */
/*			Well Published Variables		    */
/*								    */
/* **************************************************************** */

/* The name of the calling program.  You should initialize this to
   whatever was in argv[0].  It is used when parsing conditionals. */
extern char *rl_readline_name;

/* The line buffer that is in use. */
extern char *rl_line_buffer;

/* The location of point, and end. */
extern int rl_point, rl_end;

/* The name of the terminal to use. */
extern char *rl_terminal_name;

/* The input and output streams. */
extern FILE *rl_instream, *rl_outstream;

/* The basic list of characters that signal a break between words for the
   completer routine.  The initial contents of this variable is what
   breaks words in the shell, i.e. "n\"\\'`@$>". */
extern char *rl_basic_word_break_characters;

/* The list of characters that signal a break between words for
   rl_complete_internal.  The default list is the contents of
   rl_basic_word_break_characters.  */
extern char *rl_completer_word_break_characters;

/* List of characters which can be used to quote a substring of the line.
   Completion occurs on the entire substring, and within the substring   
   rl_completer_word_break_characters are treated as any other character,
   unless they also appear within this list. */
extern char *rl_completer_quote_characters;

/* List of characters that are word break characters, but should be left
   in TEXT when it is passed to the completion function.  The shell uses
   this to help determine what kind of completing to do. */
extern char *rl_special_prefixes;

/* Pointer to the generator function for completion_matches ().
   NULL means to use filename_entry_function (), the default filename
   completer. */
extern Function *rl_completion_entry_function;

/* If rl_ignore_some_completions_function is non-NULL it is the address
   of a function to call after all of the possible matches have been
   generated, but before the actual completion is done to the input line.
   The function is called with one argument; a NULL terminated array
   of (char *).  If your function removes any of the elements, they
   must be free()'ed. */
extern Function *rl_ignore_some_completions_function;

/* Pointer to alternative function to create matches.
   Function is called with TEXT, START, and END.
   START and END are indices in RL_LINE_BUFFER saying what the boundaries
   of TEXT are.
   If this function exists and returns NULL then call the value of
   rl_completion_entry_function to try to match, otherwise use the
   array of strings returned. */
extern CPPFunction *rl_attempted_completion_function;

/* If non-zero, then this is the address of a function to call just
   before readline_internal () prints the first prompt. */
extern Function *rl_startup_hook;

/* If non-zero, then this is the address of a function to call when
   completing on a directory name.  The function is called with
   the address of a string (the current directory name) as an arg. */
extern Function *rl_directory_completion_hook;

/* Backwards compatibility with previous versions of readline. */
#define rl_symbolic_link_hook rl_directory_completion_hook

/* The address of a function to call periodically while Readline is
   awaiting character input, or NULL, for no event handling. */
extern Function *rl_event_hook;

/* Non-zero means that modified history lines are preceded
   with an asterisk. */
extern int rl_show_star;

/* Non-zero means that the results of the matches are to be treated
   as filenames.  This is ALWAYS zero on entry, and can only be changed
   within a completion entry finder function. */
extern int rl_filename_completion_desired;

/* Non-zero means that the results of the matches are to be quoted using
   double quotes (or an application-specific quoting mechanism) if the
   filename contains any characters in rl_word_break_chars.  This is
   ALWAYS non-zero on entry, and can only be changed within a completion
   entry finder function. */
extern int rl_filename_quoting_desired;

/* Non-zero means to suppress normal filename completion after the
   user-specified completion function has been called. */
extern int rl_attempted_completion_over;

/* **************************************************************** */
/*								    */
/*			Well Published Functions		    */
/*								    */
/* **************************************************************** */

/* Read a line of input.  Prompt with PROMPT.  A NULL PROMPT means none. */
extern char *readline (char *prompt);

/* Definitions available for use by readline clients. */
#define RL_PROMPT_START_IGNORE	'\001'
#define RL_PROMPT_END_IGNORE	'\002'

#if defined (__cplusplus)
}
#endif


#endif /* _READLINE_H_ */

