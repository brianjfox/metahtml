/* termdefs.h: -*- C -*-  DESCRIPTIVE TEXT. */

/*  Copyright (c) 1995 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Mon Oct  9 08:15:45 1995.  */

#if defined (__cplusplus)
extern "C"
{
#endif

extern char *tgetstr (char *id, char **area);
extern char *tgoto (char *cm, int destcol, int destline);
extern int tputs (char *cp, int affcnt, int (*outc)(int));
extern int tgetent (char *bp, char *name);
extern int tgetnum (char *id);
extern int tgetflag (char *id);
extern char PC;
extern char *BC, *UP;

#if defined (__cplusplus)
}
#endif
