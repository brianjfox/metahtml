/* display.h: Declarations of functions defined in display.c. */

/*  Copyright (c) 1995 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Mon Oct  9 06:09:41 1995.  */

#if defined (__cplusplus)
extern "C"
{
#endif

extern void _rl_redisplay_after_sigwinch (void);
extern void _rl_update_final (void);
extern void _rl_erase_at_end_of_line (int l);
extern void rl_reset_line_state (void);
extern void rl_clear_message (void);
extern void rl_message (char *format, ...);
extern int rl_character_len (int c, int pos);

/* Physically print C on rl_outstream.  This is for functions which know
   how to optimize the display.  Return the number of characters output. */
extern int rl_show_char (int c);
extern void _rl_move_vert (int to);

/* Move the cursor from _rl_last_c_pos to NEWPOS, which are buffer indices.
   DATA is the contents of the screen line of interest; i.e., where
   the movement is being done. */
extern void _rl_move_cursor_relative (int newpos, char *data);

/* Actually update the display, period. */
extern void rl_forced_update_display (void);
extern void rl_on_new_line (void);
extern void rl_redisplay (void);
extern int rl_expand_prompt (char *prompt);

#if defined (__cplusplus)
}
#endif
