/* bind.h: External declarations of functions in bind.c. */

/* Author: Brian J. Fox (bfox@ai.mit.edu) Mon Oct  9 05:06:23 1995. */

#if !defined (_BIND_H_)
#define _BIND_H_

#if defined (__cplusplus)
extern "C"
{
#endif

/* Bind key sequence KEYSEQ to DEFAULT_FUNC if KEYSEQ is unbound. */
extern void _rl_bind_if_unbound (char *keyseq, Function *default_func);

/* Print all of the functions and their bindings to rl_outstream.  If
   PRINT_READABLY is non-zero, then print the output in such a way
   that it can be read back in. */
extern void rl_function_dumper (int print_readably);

/* Print all of the current functions and their bindings to
   rl_outstream.  If an explicit argument is given, then print
   the output in such a way that it can be read back in. */
extern int rl_dump_functions (int count, int key);

/* Return a NULL terminated array of strings which represent the key
   sequences that can be used to invoke FUNCTION using the current keymap. */
extern char **rl_invoking_keyseqs (Function *function);

/* Return a NULL terminated array of strings which represent the key
   sequences that are used to invoke FUNCTION in MAP. */
extern char **rl_invoking_keyseqs_in_map (Function *function, Keymap map);

/* Print the names of functions known to Readline. */
extern void rl_list_funmap_names (int count, int ignore);

extern void rl_set_keymap_from_edit_mode (void);
extern Keymap rl_get_keymap (void);
extern void rl_set_keymap (Keymap map);
extern Keymap rl_get_keymap_by_name (char *name);
extern void rl_variable_bind (char *name, char *value);

/* Read the binding command from STRING and perform it.
   A key binding command looks like: Keyname: function-name\0,
   a variable binding command looks like: set variable value.
   A new-style keybinding looks like "\C-x\C-x": exchange-point-and-mark. */
extern void rl_parse_and_bind (char *string);

/* Do key bindings from a file.  If FILENAME is NULL it defaults
   to the first non-null filename from this list:
     1. the filename used for the previous call
     2. the value of the shell variable `INPUTRC'
     3. ~/.inputrc
   If the file existed and could be opened and read, 0 is returned,
   otherwise errno is returned. */
extern int rl_read_init_file (char *filename);

/* Re-read the current keybindings file. */
extern int rl_re_read_init_file (int count, int ignore);

/* Return the function (or macro) definition which would be invoked via
   KEYSEQ if executed in MAP.  If MAP is NULL, then the current keymap is
   used.  TYPE, if non-NULL, is a pointer to an int which will receive the
   type of the object pointed to.  One of ISFUNC (function), ISKMAP (keymap),
   or ISMACR (macro). */
extern Function *rl_function_of_keyseq (char *keyseq, Keymap map, int *type);

/* Return a pointer to the function that STRING represents.
   If STRING doesn't have a matching function, then a NULL pointer
   is returned. */
extern Function *rl_named_function (char *string);

/* Translate the ASCII representation of SEQ, stuffing the values into ARRAY,
   an array of characters.  LEN gets the final length of ARRAY.  Return
   non-zero if there was an error parsing SEQ. */
extern int rl_translate_keyseq (char *seq, char *array, int *len);

/* Bind the key sequence represented by the string KEYSEQ to
   the arbitrary pointer DATA.  TYPE says what kind of data is
   pointed to by DATA, right now this can be a function (ISFUNC),
   a macro (ISMACR), or a keymap (ISKMAP).  This makes new keymaps
   as necessary.  The initial place to do bindings is in MAP. */
extern int rl_generic_bind (int type, char *keyseq, char *data, Keymap map);

/* Bind the key sequence represented by the string KEYSEQ to
   the string of characters MACRO.  This makes new keymaps as
   necessary.  The initial place to do bindings is in MAP. */
extern int rl_macro_bind (char *keyseq, char *macro, Keymap map);

/* Bind the key sequence represented by the string KEYSEQ to
   FUNCTION.  This makes new keymaps as necessary.  The initial
   place to do bindings is in MAP. */
extern int rl_set_key (char *keyseq, Function *function, Keymap map);

/* Make KEY do nothing in MAP.
   Returns non-zero in case of error. */
extern int rl_unbind_key_in_map (int key, Keymap map);

/* Make KEY do nothing in the currently selected keymap.
   Returns non-zero in case of error. */
extern int rl_unbind_key (int key);

/* Bind KEY to FUNCTION in MAP.  Returns non-zero in case of invalid KEY. */
extern int rl_bind_key_in_map (int key, Function *function, Keymap map);

/* Bind KEY to FUNCTION.  Returns non-zero if KEY is out of range. */
extern int rl_bind_key (int key, Function *function);

/* rl_add_defun (char *name, Function *function, int key)
   Add NAME to the list of named functions.  Make FUNCTION be the function
   that gets called.  If KEY is not -1, then bind it. */
extern void rl_add_defun (char *name, Function *function, int key);

extern void rl_set_keymap_from_edit_mode (void);

#if defined (__cplusplus)
}
#endif

#endif /* BIND_H */
