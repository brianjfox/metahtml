/* vi_mode.h: All of the losing VI mode functions. */

/*  Copyright (c) 1995 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Mon Oct  9 06:56:26 1995.  */

#if defined (__cplusplus)
extern "C"
{
#endif
extern int rl_vi_replace (int count, int key);
extern int rl_vi_overstrike_delete (int count, int ignore);
extern int rl_vi_overstrike (int count, int key);
extern int rl_vi_subst (int count, int key);
extern int rl_vi_change_char (int count, int key);
extern int rl_vi_bracktype (int c);
extern int rl_vi_match (int ignore, int key);
extern int rl_vi_char_search (int count, int key);
extern int rl_back_to_indent (int ignore1, int ignore2);
extern int rl_vi_first_print (int count, int key);
extern int rl_vi_comment (int count, int key);
extern int rl_vi_delete (int count, int key);
extern int rl_vi_yank_to (int count, int key);
extern int rl_vi_change_to (int count, int key);
extern int rl_vi_delete_to (int count, int key);
extern int rl_vi_domove (int key, int *nextkey);
extern int rl_vi_column (int count, int key);
extern int rl_vi_check (void);
extern int rl_vi_put (int count, int key);
extern int rl_vi_change_case (int count, int ignore);
extern int rl_vi_arg_digit (int count, int c);
extern int rl_vi_movement_mode (int count, int key);
extern void _rl_vi_done_inserting (void);
extern int rl_vi_insertion_mode (int count, int key);
extern int rl_vi_append_eol (int count, int key);
extern int rl_vi_append_mode (int count, int key);
extern int rl_vi_insert_beg (int count, int key);
extern int rl_vi_eword (int count, int ignore);
extern int rl_vi_bword (int count, int ignore);
extern int rl_vi_fword (int count, int ignore);
extern int rl_vi_eWord (int count, int ignore);
extern int rl_vi_bWord (int count, int ignore);
extern int rl_vi_fWord (int count, int ignore);
extern int rl_vi_end_word (int count, int key);
extern int rl_vi_next_word (int count, int key);
extern int rl_vi_tilde_expand (int ignore, int key);
extern int rl_vi_complete (int ignore, int key);
extern int rl_vi_fetch_history (int count, int c);
extern int rl_vi_redo (int count, int c);
extern int rl_vi_textmod_command (int c);
extern void _rl_vi_set_last (int key, int repeat, int sign);
extern void _rl_vi_reset_last (void);
extern int rl_vi_eof_maybe (int count, int c);
extern int rl_vi_prev_word (int count, int c);
extern int rl_vi_search (int count, int c);
extern int rl_vi_yank_arg (int count, int c);
extern int rl_vi_search_again (int count, int c);

#if defined (__cplusplus)
}
#endif
