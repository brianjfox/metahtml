/* complete.h: Declarations of functions defined in complete.c */

/*  Copyright (c) 1995 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Mon Oct  9 05:32:05 1995.  */

#if !defined (_COMPLETE_H_)
#define _COMPLETE_H_

#if defined (__cplusplus)
extern "C" {
#endif

/* A function for simple tilde expansion. */
extern int rl_tilde_expand (int ignore, int key);

/* Okay, now we write the entry_function for filename completion.  In the
   general case.  Note that completion in the shell is a little different
   because of all the pathnames that must be followed when looking up the
   completion for a command. */
extern char *filename_completion_function (char *text, int state);

/* Return an array of (char *) which is a list of completions for TEXT.
   If there are no completions, return a NULL pointer.
   The first entry in the returned array is the substitution for TEXT.
   The remaining entries are the possible completions.
   The array is terminated with a NULL pointer.

   ENTRY_FUNCTION is a function of two args, and returns a (char *).
     The first argument is TEXT.
     The second is a state argument; it should be zero on the first call, and
     non-zero on subsequent calls.  It returns a NULL pointer to the caller
     when there are no more matches.
 */
extern char **completion_matches (char *text, Function *entry_function);

/* A completion function for usernames.
   TEXT contains a partial username preceded by a random
   character (usually `~').  */
extern char *username_completion_function (char *text, int state);

/* Complete the word at or before point.
   WHAT_TO_DO says what to do with the completion.
   `?' means list the possible completions.
   TAB means do standard completion.
   `*' means insert all of the possible completions.
   `!' means to do standard completion, and list all possible completions if
   there is more than one. */
extern void rl_complete_internal (int what_to_do);

/* Complete the word at or before point.  You have supplied the function
   that does the initial simple matching selection algorithm (see
   completion_matches ()).  The default is to do filename completion. */
extern int rl_complete (int ignore, int invoking_key);

/* List the possible completions.  See description of rl_complete (). */
extern int rl_possible_completions (int ignore, int invoking_key);

extern int rl_insert_completions (int ignore, int invoking_key);

#if defined (__cplusplus)
}
#endif

#endif /* COMPLETE_H */
