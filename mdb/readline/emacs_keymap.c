/* emacs_keymap.c -- the keymap for emacs_mode in readline (). */

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

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#if !defined (BUFSIZ)
#include <stdio.h>
#endif /* !BUFSIZ */

#include "readline.h"

/* An array of function pointers, one for each possible key.
   If the type byte is ISKMAP, then the pointer is the address of
   a keymap. */

KEYMAP_ENTRY_ARRAY emacs_standard_keymap = {

  /* Control keys. */
  { ISFUNC, (Function *)0x0 },		/* Control-@ */
  { ISFUNC, (Function *)rl_beg_of_line },		/* Control-a */
  { ISFUNC, (Function *)rl_backward },		/* Control-b */
  { ISFUNC, (Function *)0x0 },		/* Control-c */
  { ISFUNC, (Function *)rl_delete },		/* Control-d */
  { ISFUNC, (Function *)rl_end_of_line },		/* Control-e */
  { ISFUNC, (Function *)rl_forward },		/* Control-f */
  { ISFUNC, (Function *)rl_abort },			/* Control-g */
  { ISFUNC, (Function *)rl_rubout },		/* Control-h */
  { ISFUNC, (Function *)rl_complete },		/* Control-i */
  { ISFUNC, (Function *)rl_newline },		/* Control-j */
  { ISFUNC, (Function *)rl_kill_line },		/* Control-k */
  { ISFUNC, (Function *)rl_clear_screen },		/* Control-l */
  { ISFUNC, (Function *)rl_newline },		/* Control-m */
  { ISFUNC, (Function *)rl_get_next_history },	/* Control-n */
  { ISFUNC, (Function *)0x0 },		/* Control-o */
  { ISFUNC, (Function *)rl_get_previous_history },	/* Control-p */
  { ISFUNC, (Function *)rl_quoted_insert },		/* Control-q */
  { ISFUNC, (Function *)rl_reverse_search_history }, /* Control-r */
  { ISFUNC, (Function *)rl_forward_search_history }, /* Control-s */
  { ISFUNC, (Function *)rl_transpose_chars },	/* Control-t */
  { ISFUNC, (Function *)rl_unix_line_discard },	/* Control-u */
  { ISFUNC, (Function *)rl_quoted_insert },		/* Control-v */
  { ISFUNC, (Function *)rl_unix_word_rubout },	/* Control-w */
  { ISKMAP, (Function *)emacs_ctlx_keymap },	/* Control-x */
  { ISFUNC, (Function *)rl_yank },			/* Control-y */
  { ISFUNC, (Function *)0x0 },		/* Control-z */
  { ISKMAP, (Function *)emacs_meta_keymap }, /* Control-[ */
  { ISFUNC, (Function *)0x0 },		/* Control-\ */
  { ISFUNC, (Function *)0x0 },		/* Control-] */
  { ISFUNC, (Function *)0x0 },		/* Control-^ */
  { ISFUNC, (Function *)rl_undo_command },		/* Control-_ */

  /* The start of printing characters. */
  { ISFUNC, (Function *)rl_insert },	/* SPACE */
  { ISFUNC, (Function *)rl_insert },	/* ! */
  { ISFUNC, (Function *)rl_insert },	/* " */
  { ISFUNC, (Function *)rl_insert },	/* # */
  { ISFUNC, (Function *)rl_insert },	/* $ */
  { ISFUNC, (Function *)rl_insert },	/* % */
  { ISFUNC, (Function *)rl_insert },	/* & */
  { ISFUNC, (Function *)rl_insert },	/* ' */
  { ISFUNC, (Function *)rl_insert },	/* ( */
#if defined (PAREN_MATCHING)
  { ISFUNC, (Function *)rl_insert_close },	/* ) */
#else
  { ISFUNC, (Function *)rl_insert },	/* ) */
#endif /* !PAREN_MATCHING */
  { ISFUNC, (Function *)rl_insert },	/* * */
  { ISFUNC, (Function *)rl_insert },	/* + */
  { ISFUNC, (Function *)rl_insert },	/* , */
  { ISFUNC, (Function *)rl_insert },	/* - */
  { ISFUNC, (Function *)rl_insert },	/* . */
  { ISFUNC, (Function *)rl_insert },	/* / */

  /* Regular digits. */
  { ISFUNC, (Function *)rl_insert },	/* 0 */
  { ISFUNC, (Function *)rl_insert },	/* 1 */
  { ISFUNC, (Function *)rl_insert },	/* 2 */
  { ISFUNC, (Function *)rl_insert },	/* 3 */
  { ISFUNC, (Function *)rl_insert },	/* 4 */
  { ISFUNC, (Function *)rl_insert },	/* 5 */
  { ISFUNC, (Function *)rl_insert },	/* 6 */
  { ISFUNC, (Function *)rl_insert },	/* 7 */
  { ISFUNC, (Function *)rl_insert },	/* 8 */
  { ISFUNC, (Function *)rl_insert },	/* 9 */

  /* A little more punctuation. */
  { ISFUNC, (Function *)rl_insert },	/* : */
  { ISFUNC, (Function *)rl_insert },	/* ; */
  { ISFUNC, (Function *)rl_insert },	/* < */
  { ISFUNC, (Function *)rl_insert },	/* = */
  { ISFUNC, (Function *)rl_insert },	/* > */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* @ */

  /* Uppercase alphabet. */
  { ISFUNC, (Function *)rl_insert },	/* A */
  { ISFUNC, (Function *)rl_insert },	/* B */
  { ISFUNC, (Function *)rl_insert },	/* C */
  { ISFUNC, (Function *)rl_insert },	/* D */
  { ISFUNC, (Function *)rl_insert },	/* E */
  { ISFUNC, (Function *)rl_insert },	/* F */
  { ISFUNC, (Function *)rl_insert },	/* G */
  { ISFUNC, (Function *)rl_insert },	/* H */
  { ISFUNC, (Function *)rl_insert },	/* I */
  { ISFUNC, (Function *)rl_insert },	/* J */
  { ISFUNC, (Function *)rl_insert },	/* K */
  { ISFUNC, (Function *)rl_insert },	/* L */
  { ISFUNC, (Function *)rl_insert },	/* M */
  { ISFUNC, (Function *)rl_insert },	/* N */
  { ISFUNC, (Function *)rl_insert },	/* O */
  { ISFUNC, (Function *)rl_insert },	/* P */
  { ISFUNC, (Function *)rl_insert },	/* Q */
  { ISFUNC, (Function *)rl_insert },	/* R */
  { ISFUNC, (Function *)rl_insert },	/* S */
  { ISFUNC, (Function *)rl_insert },	/* T */
  { ISFUNC, (Function *)rl_insert },	/* U */
  { ISFUNC, (Function *)rl_insert },	/* V */
  { ISFUNC, (Function *)rl_insert },	/* W */
  { ISFUNC, (Function *)rl_insert },	/* X */
  { ISFUNC, (Function *)rl_insert },	/* Y */
  { ISFUNC, (Function *)rl_insert },	/* Z */

  /* Some more punctuation. */
  { ISFUNC, (Function *)rl_insert },	/* [ */
  { ISFUNC, (Function *)rl_insert },	/* \ */
#if defined (PAREN_MATCHING)
  { ISFUNC, (Function *)rl_insert_close },	/* ] */
#else
  { ISFUNC, (Function *)rl_insert },	/* ] */
#endif /* !PAREN_MATCHING */
  { ISFUNC, (Function *)rl_insert },	/* ^ */
  { ISFUNC, (Function *)rl_insert },	/* _ */
  { ISFUNC, (Function *)rl_insert },	/* ` */

  /* Lowercase alphabet. */
  { ISFUNC, (Function *)rl_insert },	/* a */
  { ISFUNC, (Function *)rl_insert },	/* b */
  { ISFUNC, (Function *)rl_insert },	/* c */
  { ISFUNC, (Function *)rl_insert },	/* d */
  { ISFUNC, (Function *)rl_insert },	/* e */
  { ISFUNC, (Function *)rl_insert },	/* f */
  { ISFUNC, (Function *)rl_insert },	/* g */
  { ISFUNC, (Function *)rl_insert },	/* h */
  { ISFUNC, (Function *)rl_insert },	/* i */
  { ISFUNC, (Function *)rl_insert },	/* j */
  { ISFUNC, (Function *)rl_insert },	/* k */
  { ISFUNC, (Function *)rl_insert },	/* l */
  { ISFUNC, (Function *)rl_insert },	/* m */
  { ISFUNC, (Function *)rl_insert },	/* n */
  { ISFUNC, (Function *)rl_insert },	/* o */
  { ISFUNC, (Function *)rl_insert },	/* p */
  { ISFUNC, (Function *)rl_insert },	/* q */
  { ISFUNC, (Function *)rl_insert },	/* r */
  { ISFUNC, (Function *)rl_insert },	/* s */
  { ISFUNC, (Function *)rl_insert },	/* t */
  { ISFUNC, (Function *)rl_insert },	/* u */
  { ISFUNC, (Function *)rl_insert },	/* v */
  { ISFUNC, (Function *)rl_insert },	/* w */
  { ISFUNC, (Function *)rl_insert },	/* x */
  { ISFUNC, (Function *)rl_insert },	/* y */
  { ISFUNC, (Function *)rl_insert },	/* z */

  /* Final punctuation. */
  { ISFUNC, (Function *)rl_insert },	/* { */
  { ISFUNC, (Function *)rl_insert },	/* | */
#if defined (PAREN_MATCHING)
  { ISFUNC, (Function *)rl_insert_close },	/* } */
#else
  { ISFUNC, (Function *)rl_insert },	/* } */
#endif /* !PAREN_MATCHING */
  { ISFUNC, (Function *)rl_insert },	/* ~ */
  { ISFUNC, (Function *)rl_rubout },	/* RUBOUT */

#if KEYMAP_SIZE > 128
  /* Pure 8-bit characters (128 - 159).
     These might be used in some
     character sets. */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */
  { ISFUNC, (Function *)rl_insert },	/* ? */

  /* ISO Latin-1 characters (160 - 255) */
  { ISFUNC, (Function *)rl_insert },	/* No-break space */
  { ISFUNC, (Function *)rl_insert },	/* Inverted exclamation mark */
  { ISFUNC, (Function *)rl_insert },	/* Cent sign */
  { ISFUNC, (Function *)rl_insert },	/* Pound sign */
  { ISFUNC, (Function *)rl_insert },	/* Currency sign */
  { ISFUNC, (Function *)rl_insert },	/* Yen sign */
  { ISFUNC, (Function *)rl_insert },	/* Broken bar */
  { ISFUNC, (Function *)rl_insert },	/* Section sign */
  { ISFUNC, (Function *)rl_insert },	/* Diaeresis */
  { ISFUNC, (Function *)rl_insert },	/* Copyright sign */
  { ISFUNC, (Function *)rl_insert },	/* Feminine ordinal indicator */
  { ISFUNC, (Function *)rl_insert },	/* Left pointing double angle quotation mark */
  { ISFUNC, (Function *)rl_insert },	/* Not sign */
  { ISFUNC, (Function *)rl_insert },	/* Soft hyphen */
  { ISFUNC, (Function *)rl_insert },	/* Registered sign */
  { ISFUNC, (Function *)rl_insert },	/* Macron */
  { ISFUNC, (Function *)rl_insert },	/* Degree sign */
  { ISFUNC, (Function *)rl_insert },	/* Plus-minus sign */
  { ISFUNC, (Function *)rl_insert },	/* Superscript two */
  { ISFUNC, (Function *)rl_insert },	/* Superscript three */
  { ISFUNC, (Function *)rl_insert },	/* Acute accent */
  { ISFUNC, (Function *)rl_insert },	/* Micro sign */
  { ISFUNC, (Function *)rl_insert },	/* Pilcrow sign */
  { ISFUNC, (Function *)rl_insert },	/* Middle dot */
  { ISFUNC, (Function *)rl_insert },	/* Cedilla */
  { ISFUNC, (Function *)rl_insert },	/* Superscript one */
  { ISFUNC, (Function *)rl_insert },	/* Masculine ordinal indicator */
  { ISFUNC, (Function *)rl_insert },	/* Right pointing double angle quotation mark */
  { ISFUNC, (Function *)rl_insert },	/* Vulgar fraction one quarter */
  { ISFUNC, (Function *)rl_insert },	/* Vulgar fraction one half */
  { ISFUNC, (Function *)rl_insert },	/* Vulgar fraction three quarters */
  { ISFUNC, (Function *)rl_insert },	/* Inverted questionk mark */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter a with grave */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter a with acute */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter a with circumflex */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter a with tilde */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter a with diaeresis */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter a with ring above */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter ae */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter c with cedilla */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter e with grave */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter e with acute */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter e with circumflex */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter e with diaeresis */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter i with grave */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter i with acute */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter i with circumflex */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter i with diaeresis */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter eth (Icelandic) */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter n with tilde */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter o with grave */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter o with acute */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter o with circumflex */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter o with tilde */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter o with diaeresis */
  { ISFUNC, (Function *)rl_insert },	/* Multiplication sign */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter o with stroke */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter u with grave */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter u with acute */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter u with circumflex */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter u with diaeresis */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter Y with acute */
  { ISFUNC, (Function *)rl_insert },	/* Latin capital letter thorn (Icelandic) */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter sharp s (German) */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter a with grave */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter a with acute */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter a with circumflex */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter a with tilde */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter a with diaeresis */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter a with ring above */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter ae */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter c with cedilla */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter e with grave */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter e with acute */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter e with circumflex */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter e with diaeresis */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter i with grave */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter i with acute */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter i with circumflex */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter i with diaeresis */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter eth (Icelandic) */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter n with tilde */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter o with grave */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter o with acute */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter o with circumflex */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter o with tilde */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter o with diaeresis */
  { ISFUNC, (Function *)rl_insert },	/* Division sign */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter o with stroke */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter u with grave */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter u with acute */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter u with circumflex */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter u with diaeresis */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter y with acute */
  { ISFUNC, (Function *)rl_insert },	/* Latin small letter thorn (Icelandic) */
  { ISFUNC, (Function *)rl_insert }		/* Latin small letter y with diaeresis */
#endif /* KEYMAP_SIZE > 128 */
};

KEYMAP_ENTRY_ARRAY emacs_meta_keymap = {

  /* Meta keys.  Just like above, but the high bit is set. */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-@ */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-a */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-b */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-c */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-d */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-e */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-f */
  { ISFUNC, (Function *)rl_abort },		/* Meta-Control-g */
  { ISFUNC, (Function *)rl_backward_kill_word },	/* Meta-Control-h */
  { ISFUNC, (Function *)rl_tab_insert },	/* Meta-Control-i */
  { ISFUNC, (Function *)rl_vi_editing_mode }, /* Meta-Control-j */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-k */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-l */
  { ISFUNC, (Function *)rl_vi_editing_mode }, /* Meta-Control-m */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-n */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-o */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-p */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-q */
  { ISFUNC, (Function *)rl_revert_line },	/* Meta-Control-r */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-s */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-t */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-u */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-v */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-w */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-x */
  { ISFUNC, (Function *)rl_yank_nth_arg },	/* Meta-Control-y */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-z */

  { ISFUNC, (Function *)rl_complete },	/* Meta-Control-[ */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-\ */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-] */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-^ */
  { ISFUNC, (Function *)0x0 },	/* Meta-Control-_ */

  /* The start of printing characters. */
  { ISFUNC, (Function *)0x0 },	/* Meta-SPACE */
  { ISFUNC, (Function *)0x0 },	/* Meta-! */
  { ISFUNC, (Function *)0x0 },	/* Meta-" */
  { ISFUNC, (Function *)0x0 },	/* Meta-# */
  { ISFUNC, (Function *)0x0 },	/* Meta-$ */
  { ISFUNC, (Function *)0x0 },	/* Meta-% */
  { ISFUNC, (Function *)rl_tilde_expand },	/* Meta-& */
  { ISFUNC, (Function *)0x0 },	/* Meta-' */
  { ISFUNC, (Function *)0x0 },	/* Meta-( */
  { ISFUNC, (Function *)0x0 },	/* Meta-) */
  { ISFUNC, (Function *)0x0 },	/* Meta-* */
  { ISFUNC, (Function *)0x0 },	/* Meta-+ */
  { ISFUNC, (Function *)0x0 },	/* Meta-, */
  { ISFUNC, (Function *)rl_digit_argument }, /* Meta-- */
  { ISFUNC, (Function *)rl_yank_last_arg},	/* Meta-. */
  { ISFUNC, (Function *)0x0 },	/* Meta-/ */

  /* Regular digits. */
  { ISFUNC, (Function *)rl_digit_argument }, /* Meta-0 */
  { ISFUNC, (Function *)rl_digit_argument }, /* Meta-1 */
  { ISFUNC, (Function *)rl_digit_argument }, /* Meta-2 */
  { ISFUNC, (Function *)rl_digit_argument }, /* Meta-3 */
  { ISFUNC, (Function *)rl_digit_argument }, /* Meta-4 */
  { ISFUNC, (Function *)rl_digit_argument }, /* Meta-5 */
  { ISFUNC, (Function *)rl_digit_argument }, /* Meta-6 */
  { ISFUNC, (Function *)rl_digit_argument }, /* Meta-7 */
  { ISFUNC, (Function *)rl_digit_argument }, /* Meta-8 */
  { ISFUNC, (Function *)rl_digit_argument }, /* Meta-9 */

  /* A little more punctuation. */
  { ISFUNC, (Function *)0x0 },		/* Meta-: */
  { ISFUNC, (Function *)0x0 },		/* Meta-; */
  { ISFUNC, (Function *)rl_beginning_of_history },	/* Meta-< */
  { ISFUNC, (Function *)0x0 },		/* Meta-= */
  { ISFUNC, (Function *)rl_end_of_history },	/* Meta-> */
  { ISFUNC, (Function *)rl_possible_completions },	/* Meta-? */
  { ISFUNC, (Function *)0x0 },		/* Meta-@ */

  /* Uppercase alphabet. */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-A */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-B */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-C */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-D */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-E */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-F */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-G */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-H */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-I */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-J */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-K */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-L */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-M */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-N */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-O */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-P */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-Q */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-R */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-S */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-T */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-U */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-V */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-W */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-X */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-Y */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Meta-Z */

  /* Some more punctuation. */
  { ISFUNC, (Function *)0x0 },		/* Meta-[ */	/* was rl_arrow_keys */
  { ISFUNC, (Function *)rl_delete_horizontal_space },	/* Meta-\ */
  { ISFUNC, (Function *)0x0 },		/* Meta-] */
  { ISFUNC, (Function *)0x0 },		/* Meta-^ */
  { ISFUNC, (Function *)rl_yank_last_arg },		/* Meta-_ */
  { ISFUNC, (Function *)0x0 },		/* Meta-` */

  /* Lowercase alphabet. */
  { ISFUNC, (Function *)0x0 },	/* Meta-a */
  { ISFUNC, (Function *)rl_backward_word },	/* Meta-b */
  { ISFUNC, (Function *)rl_capitalize_word }, /* Meta-c */
  { ISFUNC, (Function *)rl_kill_word },	/* Meta-d */
  { ISFUNC, (Function *)0x0 },	/* Meta-e */
  { ISFUNC, (Function *)rl_forward_word },	/* Meta-f */
  { ISFUNC, (Function *)0x0 },	/* Meta-g */
  { ISFUNC, (Function *)0x0 },	/* Meta-h */
  { ISFUNC, (Function *)0x0 },	/* Meta-i */
  { ISFUNC, (Function *)0x0 },	/* Meta-j */
  { ISFUNC, (Function *)0x0 },	/* Meta-k */
  { ISFUNC, (Function *)rl_downcase_word },	/* Meta-l */
  { ISFUNC, (Function *)0x0 },	/* Meta-m */
  { ISFUNC, (Function *)rl_noninc_forward_search },	/* Meta-n */
  { ISFUNC, (Function *)0x0 },	/* Meta-o */	/* was rl_arrow_keys */
  { ISFUNC, (Function *)rl_noninc_reverse_search },	/* Meta-p */
  { ISFUNC, (Function *)0x0 },	/* Meta-q */
  { ISFUNC, (Function *)rl_revert_line },	/* Meta-r */
  { ISFUNC, (Function *)0x0 },	/* Meta-s */
  { ISFUNC, (Function *)rl_transpose_words }, /* Meta-t */
  { ISFUNC, (Function *)rl_upcase_word },	/* Meta-u */
  { ISFUNC, (Function *)0x0 },	/* Meta-v */
  { ISFUNC, (Function *)0x0 },	/* Meta-w */
  { ISFUNC, (Function *)0x0 },	/* Meta-x */
  { ISFUNC, (Function *)rl_yank_pop },	/* Meta-y */
  { ISFUNC, (Function *)0x0 },	/* Meta-z */

  /* Final punctuation. */
  { ISFUNC, (Function *)0x0 },	/* Meta-{ */
  { ISFUNC, (Function *)0x0 },	/* Meta-| */
  { ISFUNC, (Function *)0x0 },	/* Meta-} */
  { ISFUNC, (Function *)rl_tilde_expand },	/* Meta-~ */
  { ISFUNC, (Function *)rl_backward_kill_word }, /* Meta-rubout */

#if KEYMAP_SIZE > 128
  /* Undefined keys. */
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 }
#endif /* KEYMAP_SIZE > 128 */
};

KEYMAP_ENTRY_ARRAY emacs_ctlx_keymap = {

  /* Control keys. */
  { ISFUNC, (Function *)0x0 },		/* Control-@ */
  { ISFUNC, (Function *)0x0 },		/* Control-a */
  { ISFUNC, (Function *)0x0 },		/* Control-b */
  { ISFUNC, (Function *)0x0 },		/* Control-c */
  { ISFUNC, (Function *)0x0 },		/* Control-d */
  { ISFUNC, (Function *)0x0 },		/* Control-e */
  { ISFUNC, (Function *)0x0 },		/* Control-f */
  { ISFUNC, (Function *)rl_abort },			/* Control-g */
  { ISFUNC, (Function *)0x0 },		/* Control-h */
  { ISFUNC, (Function *)0x0 },		/* Control-i */
  { ISFUNC, (Function *)0x0 },		/* Control-j */
  { ISFUNC, (Function *)0x0 },		/* Control-k */
  { ISFUNC, (Function *)0x0 },		/* Control-l */
  { ISFUNC, (Function *)0x0 },		/* Control-m */
  { ISFUNC, (Function *)0x0 },		/* Control-n */
  { ISFUNC, (Function *)0x0 },		/* Control-o */
  { ISFUNC, (Function *)0x0 },		/* Control-p */
  { ISFUNC, (Function *)0x0 },		/* Control-q */
  { ISFUNC, (Function *)rl_re_read_init_file },	/* Control-r */
  { ISFUNC, (Function *)0x0 },		/* Control-s */
  { ISFUNC, (Function *)0x0 },		/* Control-t */
  { ISFUNC, (Function *)rl_undo_command },		/* Control-u */
  { ISFUNC, (Function *)0x0 },		/* Control-v */
  { ISFUNC, (Function *)0x0 },		/* Control-w */
  { ISFUNC, (Function *)0x0 },		/* Control-x */
  { ISFUNC, (Function *)0x0 },		/* Control-y */
  { ISFUNC, (Function *)0x0 },		/* Control-z */
  { ISFUNC, (Function *)0x0 },		/* Control-[ */
  { ISFUNC, (Function *)0x0 },		/* Control-\ */
  { ISFUNC, (Function *)0x0 },		/* Control-] */
  { ISFUNC, (Function *)0x0 },		/* Control-^ */
  { ISFUNC, (Function *)0x0 },		/* Control-_ */

  /* The start of printing characters. */
  { ISFUNC, (Function *)0x0 },		/* SPACE */
  { ISFUNC, (Function *)0x0 },		/* ! */
  { ISFUNC, (Function *)0x0 },		/* " */
  { ISFUNC, (Function *)0x0 },		/* # */
  { ISFUNC, (Function *)0x0 },		/* $ */
  { ISFUNC, (Function *)0x0 },		/* % */
  { ISFUNC, (Function *)0x0 },		/* & */
  { ISFUNC, (Function *)0x0 },		/* ' */
  { ISFUNC, (Function *)rl_start_kbd_macro },	/* ( */
  { ISFUNC, (Function *)rl_end_kbd_macro  },	/* ) */
  { ISFUNC, (Function *)0x0 },		/* * */
  { ISFUNC, (Function *)0x0 },		/* + */
  { ISFUNC, (Function *)0x0 },		/* , */
  { ISFUNC, (Function *)0x0 },		/* - */
  { ISFUNC, (Function *)0x0 },		/* . */
  { ISFUNC, (Function *)0x0 },		/* / */

  /* Regular digits. */
  { ISFUNC, (Function *)0x0 },		/* 0 */
  { ISFUNC, (Function *)0x0 },		/* 1 */
  { ISFUNC, (Function *)0x0 },		/* 2 */
  { ISFUNC, (Function *)0x0 },		/* 3 */
  { ISFUNC, (Function *)0x0 },		/* 4 */
  { ISFUNC, (Function *)0x0 },		/* 5 */
  { ISFUNC, (Function *)0x0 },		/* 6 */
  { ISFUNC, (Function *)0x0 },		/* 7 */
  { ISFUNC, (Function *)0x0 },		/* 8 */
  { ISFUNC, (Function *)0x0 },		/* 9 */

  /* A little more punctuation. */
  { ISFUNC, (Function *)0x0 },	/* : */
  { ISFUNC, (Function *)0x0 },	/* ; */
  { ISFUNC, (Function *)0x0 },	/* < */
  { ISFUNC, (Function *)0x0 },	/* = */
  { ISFUNC, (Function *)0x0 },	/* > */
  { ISFUNC, (Function *)0x0 },	/* ? */
  { ISFUNC, (Function *)0x0 },	/* @ */

  /* Uppercase alphabet. */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* A */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* B */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* C */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* D */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* E */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* F */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* G */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* H */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* I */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* J */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* K */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* L */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* M */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* N */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* O */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* P */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Q */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* R */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* S */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* T */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* U */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* V */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* W */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* X */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Y */
  { ISFUNC, (Function *)rl_do_lowercase_version },	/* Z */

  /* Some more punctuation. */
  { ISFUNC, (Function *)0x0 },		/* [ */
  { ISFUNC, (Function *)0x0 },		/* \ */
  { ISFUNC, (Function *)0x0 },		/* ] */
  { ISFUNC, (Function *)0x0 },		/* ^ */
  { ISFUNC, (Function *)0x0 },		/* _ */
  { ISFUNC, (Function *)0x0 },		/* ` */

  /* Lowercase alphabet. */
  { ISFUNC, (Function *)0x0 },		/* a */
  { ISFUNC, (Function *)0x0 },		/* b */
  { ISFUNC, (Function *)0x0 },		/* c */
  { ISFUNC, (Function *)0x0 },		/* d */
  { ISFUNC, (Function *)rl_call_last_kbd_macro },	/* e */
  { ISFUNC, (Function *)0x0 },		/* f */
  { ISFUNC, (Function *)0x0 },		/* g */
  { ISFUNC, (Function *)0x0 },		/* h */
  { ISFUNC, (Function *)0x0 },		/* i */
  { ISFUNC, (Function *)0x0 },		/* j */
  { ISFUNC, (Function *)0x0 },		/* k */
  { ISFUNC, (Function *)0x0 },		/* l */
  { ISFUNC, (Function *)0x0 },		/* m */
  { ISFUNC, (Function *)0x0 },		/* n */
  { ISFUNC, (Function *)0x0 },		/* o */
  { ISFUNC, (Function *)0x0 },		/* p */
  { ISFUNC, (Function *)0x0 },		/* q */
  { ISFUNC, (Function *)0x0 },		/* r */
  { ISFUNC, (Function *)0x0 },		/* s */
  { ISFUNC, (Function *)0x0 },		/* t */
  { ISFUNC, (Function *)0x0 },		/* u */
  { ISFUNC, (Function *)0x0 },		/* v */
  { ISFUNC, (Function *)0x0 },		/* w */
  { ISFUNC, (Function *)0x0 },		/* x */
  { ISFUNC, (Function *)0x0 },		/* y */
  { ISFUNC, (Function *)0x0 },		/* z */

  /* Final punctuation. */
  { ISFUNC, (Function *)0x0 },		/* { */
  { ISFUNC, (Function *)0x0 },		/* | */
  { ISFUNC, (Function *)0x0 },		/* } */
  { ISFUNC, (Function *)0x0 },		/* ~ */
  { ISFUNC, (Function *)rl_backward_kill_line },	/* RUBOUT */

#if KEYMAP_SIZE > 128
  /* Undefined keys. */
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 },
  { ISFUNC, (Function *)0x0 }
#endif /* KEYMAP_SIZE > 128 */
};
