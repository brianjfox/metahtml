/* rltty.h: Declarations of functions defined in rltty.c */

/*  Copyright (c) 1995 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Mon Oct  9 06:30:59 1995.  */

#if defined (__cplusplus)
extern "C"
{
#endif

extern void rltty_set_default_bindings (Keymap kmap);
extern int rl_stop_output (int count, int key);
extern int rl_restart_output (int count, int key);
extern void rl_deprep_terminal (void);
extern void rl_prep_terminal (int meta_flag);

#if defined (__cplusplus)
}
#endif
