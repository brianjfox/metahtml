
/*  A Bison parser, made from parsedate.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define	t_YEARS	257
#define	t_MONTHS	258
#define	t_DAYS	259
#define	t_HOURS	260
#define	t_MINUTES	261
#define	t_SECONDS	262
#define	t_AT	263
#define	t_FROM	264
#define	t_AGO	265
#define	t_NOW	266
#define	t_NEXT	267
#define	t_LAST	268
#define	t_AM	269
#define	t_PM	270
#define	t_TH	271
#define	t_BEFORE	272
#define	t_AFTER	273
#define	t_WEEKDAY	274
#define	t_MONTH	275
#define	t_DSTZONE	276
#define	t_NDSTZONE	277
#define	t_NUMBER	278
#define	t_YEARNUM	279
#define	t_DATESEP	280
#define	t_TIMESEP	281
#define	t_RELATIVE	282
#define	t_BIGNUM	283

#line 1 "parsedate.y"
  /* Yet another yacc parser for date formats. */

/*  Copyright (c) 1999 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Sun Aug  8 05:06:03 1999.

    This file is part of <Meta-HTML>(tm), a system for the rapid
    deployment of Internet and Intranet applications via the use of
    the Meta-HTML language.

    Copyright (c) 1999, Brian J. Fox (bfox@ai.mit.edu). */

/* This parser handles relative time, as well as absolute time.
   Sample inputs:

   next monday
   last week
   2 years, seven hours from now
   15 minutes ago
   Sun Aug  8 05:34:01 PDT 1999
   Tuesday, August 10th at 3pm
   Thursday December 8th, 1955 at 3pm
   Sun, 21 Nov 1999 04:55:21 GMT
   12/11/59 10:15
   3/13/95 22:00
   8-9-2001
   1999-08-22
   19980108 */

extern int moddate_yylex (void);
extern int moddate_yyerror (char *);
static void zone_handle (time_t seconds_from_gmt, int dst_p);

#define yyparse moddate_parse
#define yylex moddate_lex
#define yyerror moddate_error

static int moddate_error (char *error);
static int moddate_lex (void);
extern int getdate_print_parts;

#define DEBUGGING_OUTPUT(s) if (getdate_print_parts) page_debug (s)

#define seconds_per_day	(24L * 60L * 60L)
#define HOUR(x) (x * 60 * 60)

static char **input_words = (char **)NULL;
static int iw_index = 0;
static int iw_slots = 0;
static struct tm input_date;

/* Relative time. */
static int relative_seconds = 0;

/* How far along in the parsing have we gotten? */
static int date_found = 0;
static int time_found = 0;
static int zone_found = 0;

#line 60 "parsedate.y"
typedef union { time_t Number; } YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		73
#define	YYFLAG		-32768
#define	YYNTBASE	30

#define YYTRANSLATE(x) ((unsigned)(x) <= 283 ? yytranslate[x] : 37)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
    27,    28,    29
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     1,     4,     6,     8,    10,    12,    14,    18,    24,
    31,    34,    37,    42,    47,    53,    57,    60,    63,    69,
    75,    77,    79,    86,    88,    91,    93,    95,    97,    99,
   102,   105,   108,   112,   115,   118,   121,   124,   127,   130,
   133,   137,   141,   143,   146,   149,   152
};

static const short yyrhs[] = {    -1,
    30,    31,     0,    33,     0,    32,     0,    34,     0,    35,
     0,    36,     0,    24,    27,    24,     0,    24,    27,    24,
    27,    24,     0,    24,    27,    24,    27,    24,    24,     0,
    24,    15,     0,    24,    16,     0,    24,    27,    24,    15,
     0,    24,    27,    24,    16,     0,    24,    26,    24,    26,
    24,     0,    24,    26,    24,     0,    21,    24,     0,    24,
    21,     0,    24,    26,    21,    26,    24,     0,    25,    26,
    24,    26,    24,     0,    20,     0,    25,     0,    20,    25,
    21,    24,    32,    35,     0,    29,     0,    21,    25,     0,
    20,     0,    17,     0,    22,     0,    23,     0,    22,    24,
     0,    23,    24,     0,    24,    28,     0,    24,    28,    11,
     0,    13,    28,     0,    13,    20,     0,    14,    20,     0,
    10,    20,     0,    10,    33,     0,    19,    33,     0,    18,
    33,     0,    10,    14,    20,     0,    10,    13,    20,     0,
    28,     0,    14,    28,     0,    10,    28,     0,    18,    28,
     0,    19,    28,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
    83,    83,    85,    86,    87,    88,    89,    91,    97,   103,
   114,   120,   126,   132,   140,   162,   167,   172,   177,   187,
   195,   196,   203,   212,   225,   235,   239,   242,   247,   251,
   259,   269,   274,   278,   282,   295,   304,   317,   320,   323,
   327,   336,   349,   353,   357,   361,   365
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","t_YEARS",
"t_MONTHS","t_DAYS","t_HOURS","t_MINUTES","t_SECONDS","t_AT","t_FROM","t_AGO",
"t_NOW","t_NEXT","t_LAST","t_AM","t_PM","t_TH","t_BEFORE","t_AFTER","t_WEEKDAY",
"t_MONTH","t_DSTZONE","t_NDSTZONE","t_NUMBER","t_YEARNUM","t_DATESEP","t_TIMESEP",
"t_RELATIVE","t_BIGNUM","spec","item","time_spec","date_spec","useless_word",
"zone_spec","rel_spec", NULL
};
#endif

static const short yyr1[] = {     0,
    30,    30,    31,    31,    31,    31,    31,    32,    32,    32,
    32,    32,    32,    32,    33,    33,    33,    33,    33,    33,
    33,    33,    33,    33,    33,    34,    34,    35,    35,    35,
    35,    36,    36,    36,    36,    36,    36,    36,    36,    36,
    36,    36,    36,    36,    36,    36,    36
};

static const short yyr2[] = {     0,
     0,     2,     1,     1,     1,     1,     1,     3,     5,     6,
     2,     2,     4,     4,     5,     3,     2,     2,     5,     5,
     1,     1,     6,     1,     2,     1,     1,     1,     1,     2,
     2,     2,     3,     2,     2,     2,     2,     2,     2,     2,
     3,     3,     1,     2,     2,     2,     2
};

static const short yydefact[] = {     1,
     0,     0,     0,     0,    27,     0,     0,    21,     0,    28,
    29,     0,    22,    43,    24,     2,     4,     3,     5,     6,
     7,     0,     0,    21,     0,    45,    38,    35,    34,    36,
    44,    21,    46,    40,    47,    39,     0,    17,    25,    30,
    31,    11,    12,    18,     0,     0,    32,     0,    42,    41,
     0,     0,    16,     8,    33,     0,     0,     0,     0,    13,
    14,     0,     0,     0,     0,    19,    15,     9,    20,    23,
    10,     0,     0
};

static const short yydefgoto[] = {     1,
    16,    17,    18,    19,    20,    21
};

static const short yypact[] = {-32768,
     0,    13,   -19,    45,-32768,    33,    35,   -23,    22,   -18,
    -8,    24,    10,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,    46,    47,   -23,     9,-32768,-32768,-32768,-32768,-32768,
-32768,   -23,-32768,-32768,-32768,-32768,    23,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,   -16,    44,    58,    48,-32768,-32768,
    50,    49,    51,   -12,-32768,    52,    55,    56,    57,-32768,
-32768,    59,    60,    16,    26,-32768,-32768,    61,-32768,-32768,
-32768,    70,-32768
};

static const short yypgoto[] = {-32768,
-32768,    14,     5,-32768,    11,-32768
};


#define	YYLAST		85


static const short yytable[] = {    72,
    28,    37,    60,    61,    52,    40,    27,    53,    29,     2,
    34,    36,     3,     4,    62,    41,     5,     6,     7,     8,
     9,    10,    11,    12,    13,    22,    23,    14,    15,    44,
    42,    43,    24,     9,    45,    48,    25,    13,    42,    43,
    26,    15,    46,    51,    44,    38,    39,    10,    11,    45,
    46,    47,    32,     9,    32,     9,    25,    13,    25,    13,
    33,    15,    35,    15,    30,    49,    50,    54,    55,    73,
    65,    56,    31,    57,    58,    70,    59,    63,    64,    66,
    67,     0,    68,    69,    71
};

static const short yycheck[] = {     0,
    20,    25,    15,    16,    21,    24,     2,    24,    28,    10,
     6,     7,    13,    14,    27,    24,    17,    18,    19,    20,
    21,    22,    23,    24,    25,    13,    14,    28,    29,    21,
    15,    16,    20,    21,    26,    26,    24,    25,    15,    16,
    28,    29,    27,    21,    21,    24,    25,    22,    23,    26,
    27,    28,    20,    21,    20,    21,    24,    25,    24,    25,
    28,    29,    28,    29,    20,    20,    20,    24,    11,     0,
    57,    24,    28,    24,    26,    65,    26,    26,    24,    24,
    24,    -1,    24,    24,    24
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"
/* This file comes from bison-1.28.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 217 "/usr/lib/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 3:
#line 85 "parsedate.y"
{ date_found++; ;
    break;}
case 4:
#line 86 "parsedate.y"
{ time_found++; ;
    break;}
case 8:
#line 92 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("TIME_SPEC 1\n");
	  input_date.tm_hour = yyvsp[-2].Number;
	  input_date.tm_min = yyvsp[0].Number;
	;
    break;}
case 9:
#line 97 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("TIME_SPEC 2\n");
	  input_date.tm_hour = yyvsp[-4].Number;
	  input_date.tm_min = yyvsp[-2].Number;
	  input_date.tm_sec = yyvsp[0].Number;
	;
    break;}
case 10:
#line 103 "parsedate.y"
{
	  int year = yyvsp[0].Number;

	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  input_date.tm_hour = yyvsp[-5].Number;
	  input_date.tm_min = yyvsp[-3].Number;
	  input_date.tm_sec = yyvsp[-1].Number;
	  DEBUGGING_OUTPUT ("TIME_SPEC 2.1\n");
	;
    break;}
case 11:
#line 114 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("TIME_SPEC 3\n");
	  input_date.tm_hour = yyvsp[-1].Number;
	  input_date.tm_min = 0;
	  input_date.tm_sec = 0;
	;
    break;}
case 12:
#line 120 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("TIME_SPEC 4\n");
	  input_date.tm_hour = yyvsp[-1].Number + 12;
	  input_date.tm_min = 0;
	  input_date.tm_sec = 0;
	;
    break;}
case 13:
#line 126 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("TIME_SPEC 5\n");
	  input_date.tm_hour = yyvsp[-3].Number;
	  input_date.tm_min = yyvsp[-1].Number;
	  input_date.tm_sec = 0;
	;
    break;}
case 14:
#line 132 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("TIME_SPEC 6\n");
	  input_date.tm_hour = yyvsp[-3].Number + 12;
	  input_date.tm_min = yyvsp[-1].Number;
	  input_date.tm_sec = 0;
	;
    break;}
case 15:
#line 141 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("DATE_SPEC 1");
	  if (yyvsp[-4].Number > 1900)
	    {
	      DEBUGGING_OUTPUT ("DATE_SPEC 1.1 YYYY.MM.DD\n");
	      input_date.tm_year = yyvsp[-4].Number - 1900;
	      input_date.tm_mon = yyvsp[-2].Number - 1;
	      input_date.tm_mday = yyvsp[0].Number;
	    }
	  else
	    {
	      int year = yyvsp[0].Number;
	      DEBUGGING_OUTPUT ("DATE_SPEC 1.1 MM/DD/YY[yy]\n");
	      input_date.tm_mon = yyvsp[-4].Number - 1;
	      input_date.tm_mday = yyvsp[-2].Number;
	      if (year < 50) year += 2000;
	      else if (year < 100) year += 1900;
	      input_date.tm_year = year - 1900;
	    }
	;
    break;}
case 16:
#line 162 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("DATE_SPEC 2\n");
	  input_date.tm_mon = yyvsp[-2].Number - 1;
	  input_date.tm_mday = yyvsp[0].Number;
	;
    break;}
case 17:
#line 167 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("DATE_SPEC 3\n");
	  input_date.tm_mon = yyvsp[-1].Number - 1;
	  input_date.tm_mday = yyvsp[0].Number;
	;
    break;}
case 18:
#line 172 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("DATE_SPEC 4\n");
	  input_date.tm_mday = yyvsp[-1].Number;
	  input_date.tm_mon = yyvsp[0].Number - 1;
	;
    break;}
case 19:
#line 177 "parsedate.y"
{
	    int year = yyvsp[0].Number;

	    if (year < 50) year += 2000;
	    else if (year < 100) year += 1900;
	    input_date.tm_year = year - 1900;
	    input_date.tm_mon = yyvsp[-2].Number - 1;
	    input_date.tm_mday = yyvsp[-4].Number;
	    DEBUGGING_OUTPUT ("DATE_SPEC 4.1 (12-Jan-96)\n");
	;
    break;}
case 20:
#line 187 "parsedate.y"
{
	    int year = yyvsp[-4].Number;

	    input_date.tm_year = year - 1900;
	    input_date.tm_mon = yyvsp[-2].Number - 1;
	    input_date.tm_mday = yyvsp[0].Number;
	    DEBUGGING_OUTPUT ("DATE_SPEC 4.11 (1999-03-04)\n");
	;
    break;}
case 21:
#line 195 "parsedate.y"
{ ;
    break;}
case 22:
#line 196 "parsedate.y"
{
	  int year = yyvsp[0].Number;
	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  DEBUGGING_OUTPUT ("DATE_SPEC 5 (t_YEARNUM)\n");
	;
    break;}
case 23:
#line 203 "parsedate.y"
{
	  int year = yyvsp[-2].Number;
	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  input_date.tm_mon = yyvsp[-3].Number - 1;
	  input_date.tm_mday = yyvsp[-4].Number;
	  DEBUGGING_OUTPUT ("DATE_SPEC 5.1");
	;
    break;}
case 24:
#line 212 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("DATE_SPEC 6\n");
	  if (yyvsp[0].Number > 10000)
	    {
	      /* YYYYMMDD */
	      time_t date = yyvsp[0].Number;
	      time_t year = date / 10000;
	      input_date.tm_year = year - 1900;
	      date -= year * 10000;
	      input_date.tm_mday = date % 100;
	      input_date.tm_mon = (date / 100) - 1;
	    }
	;
    break;}
case 25:
#line 225 "parsedate.y"
{
	  int year = yyvsp[0].Number;
	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  input_date.tm_mon = yyvsp[-1].Number - 1;
	  DEBUGGING_OUTPUT ("DATE_SPEC 7\n");
	;
    break;}
case 26:
#line 236 "parsedate.y"
{
	  /* Does nothing by itself. */
	;
    break;}
case 27:
#line 239 "parsedate.y"
{ /* Ignored at all times. */ ;
    break;}
case 28:
#line 243 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("ZONE_SPEC 1");
	  zone_handle (yyvsp[0].Number, 1);
	;
    break;}
case 29:
#line 247 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("ZONE_SPEC 2");
	  zone_handle (yyvsp[0].Number, 0);
	;
    break;}
case 30:
#line 251 "parsedate.y"
{
	  int year = yyvsp[0].Number;
	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  zone_handle (yyvsp[-1].Number, 1);
	  DEBUGGING_OUTPUT ("ZONE_SPEC 3");
	;
    break;}
case 31:
#line 259 "parsedate.y"
{
	  int year = yyvsp[0].Number;
	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  zone_handle (yyvsp[-1].Number, 0);
	  DEBUGGING_OUTPUT ("ZONE_SPEC 4");
	;
    break;}
case 32:
#line 270 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("REL_SPEC 1\n");
	  relative_seconds += (yyvsp[-1].Number * yyvsp[0].Number);
	;
    break;}
case 33:
#line 274 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("REL_SPEC 2\n");
	  relative_seconds -= (yyvsp[-2].Number * yyvsp[-1].Number);
	;
    break;}
case 34:
#line 278 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("REL_SPEC 3\n");
	  relative_seconds += yyvsp[0].Number;
	;
    break;}
case 35:
#line 282 "parsedate.y"
{
	  int days;

	  if (yyvsp[0].Number == input_date.tm_wday)
	    days = 7;
	  else if (yyvsp[0].Number < input_date.tm_wday)
	    days = 7 - (input_date.tm_wday - yyvsp[0].Number);
	  else
	    days = yyvsp[0].Number - input_date.tm_wday;

	  relative_seconds += days * seconds_per_day;
	  DEBUGGING_OUTPUT ("REL_SPEC 4\n");
	;
    break;}
case 36:
#line 295 "parsedate.y"
{
	  int days = (input_date.tm_wday - yyvsp[0].Number) % 7;
	  if (days == 0)
	    days = 7;
	  else if (days < 0)
	    days = 7 + days;
	  relative_seconds -= days * seconds_per_day;
	  DEBUGGING_OUTPUT ("REL_SPEC 5\n");
	;
    break;}
case 37:
#line 304 "parsedate.y"
{
	  int days;

	  if (yyvsp[0].Number == input_date.tm_wday)
	    days = 7;
	  else if (yyvsp[0].Number < input_date.tm_wday)
	    days = 7 - (input_date.tm_wday - yyvsp[0].Number);
	  else
	    days = yyvsp[0].Number - input_date.tm_wday;

	  relative_seconds += days * seconds_per_day;
	  DEBUGGING_OUTPUT ("REL_SPEC 6\n");
	;
    break;}
case 38:
#line 317 "parsedate.y"
{ /* Nothing special happens here. */
	  DEBUGGING_OUTPUT ("REL_SPEC 6.1\n");
	;
    break;}
case 39:
#line 320 "parsedate.y"
{ /* Nothing special happens here. */
	  DEBUGGING_OUTPUT ("REL_SPEC 6.2\n");
	;
    break;}
case 40:
#line 323 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("REL_SPEC 6.3\n");
	  relative_seconds = -relative_seconds;
	;
    break;}
case 41:
#line 327 "parsedate.y"
{
	  int days = (input_date.tm_wday - yyvsp[0].Number) % 7;
	  if (days == 0)
	    days = 7;
	  else if (days < 0)
	    days = 7 + days;
	  relative_seconds -= days * seconds_per_day;
	  DEBUGGING_OUTPUT ("REL_SPEC 7\n");
	;
    break;}
case 42:
#line 336 "parsedate.y"
{
	  int days;

	  if (yyvsp[0].Number == input_date.tm_wday)
	    days = 7;
	  else if (yyvsp[0].Number < input_date.tm_wday)
	    days = 7 - (input_date.tm_wday - yyvsp[0].Number);
	  else
	    days = yyvsp[0].Number - input_date.tm_wday;

	  relative_seconds += days * seconds_per_day;
	  DEBUGGING_OUTPUT ("REL_SPEC 8\n");
	;
    break;}
case 43:
#line 349 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("REL_SPEC 9\n");
	  relative_seconds += yyvsp[0].Number;
	;
    break;}
case 44:
#line 353 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("REL_SPEC 10\n");
	  relative_seconds -= yyvsp[0].Number;
	;
    break;}
case 45:
#line 357 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("REL_SPEC 11\n");
	  relative_seconds += yyvsp[0].Number;
	;
    break;}
case 46:
#line 361 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("REL_SPEC 12\n");
	  relative_seconds += yyvsp[0].Number;
	;
    break;}
case 47:
#line 365 "parsedate.y"
{
	  DEBUGGING_OUTPUT ("REL_SPEC 13\n");
	  relative_seconds += yyvsp[0].Number;
	;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 543 "/usr/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}
#line 370 "parsedate.y"


/* We store the words that we recognize in a structure containing the
   word, the token type, and the numeric value this token has. */
typedef struct
{
  char *word;
  short token;
  time_t value;
} WordLookup;

/* An array of all the words that we know about. */
static WordLookup known_words[] =
{
  { "january",		t_MONTH,	1 },
  { "february",		t_MONTH,	2 },
  { "march",		t_MONTH,	3 },
  { "april",		t_MONTH,	4 },
  { "may",		t_MONTH,	5 },
  { "june",		t_MONTH,	6 },
  { "july",		t_MONTH,	7 },
  { "august",		t_MONTH,	8 },
  { "september",	t_MONTH,	9 },
  { "sept",		t_MONTH,	9 },
  { "october",		t_MONTH,	10 },
  { "november",		t_MONTH,	11 },
  { "december",		t_MONTH,	12 },
  { "sunday",		t_WEEKDAY,	0 },
  { "monday",		t_WEEKDAY,	1 },
  { "tuesday",		t_WEEKDAY,	2 },
  { "wednesday",	t_WEEKDAY,	3 },
  { "thursday",		t_WEEKDAY,	4 },
  { "friday",		t_WEEKDAY,	5 },
  { "saturday",		t_WEEKDAY,	6 },
  { "year",		t_RELATIVE,	365 * seconds_per_day },
  { "years",		t_RELATIVE,	365 * seconds_per_day },
  { "month",		t_RELATIVE,	 30 * seconds_per_day },
  { "months",		t_RELATIVE,	 30 * seconds_per_day },
  { "week",		t_RELATIVE,	  7 * seconds_per_day },
  { "weeks",		t_RELATIVE,	  7 * seconds_per_day },
  { "day",		t_RELATIVE,	  1 * seconds_per_day },
  { "days",		t_RELATIVE,	  1 * seconds_per_day },
  { "tomorrow",		t_RELATIVE,	  1 * seconds_per_day },
  { "yesterday",	t_RELATIVE,	 -1 * seconds_per_day },
  { "hour",		t_RELATIVE,	60 * 60 },
  { "hours",		t_RELATIVE,	60 * 60 },
  { "minute",		t_RELATIVE,	60 },
  { "minutes",		t_RELATIVE,	60 },
  { "second",		t_RELATIVE,	1 },
  { "seconds",		t_RELATIVE,	1 },
  { "today",		t_RELATIVE,	0 },
  { "from",		t_FROM,		0 },
  { "last",		t_LAST,		0 },
  { "now",		t_RELATIVE,	0 },
  { "next",		t_NEXT,		1 },
  { "before",		t_BEFORE,	1 },
  { "after",		t_AFTER,	1 },
  { "ago",		t_AGO,		1 },
  { "am",		t_AM,		1 },
  { "pm",		t_PM,		1 },
  { "one",		t_NUMBER,	1 },
  { "two",		t_NUMBER,	2 },
  { "three",		t_NUMBER,	3 },
  { "four",		t_NUMBER,	4 },
  { "five",		t_NUMBER,	5 },
  { "six",		t_NUMBER,	6 },
  { "seven",		t_NUMBER,	7 },
  { "eight",		t_NUMBER,	8 },
  { "nine",		t_NUMBER,	9 },
  { "ten",		t_NUMBER,	10 },
  { "th",		t_TH,		0 },
  { "st",		t_TH,		0 },
  { "nd",		t_TH,		0 },
  { "rd",		t_TH,		0 },

  { "zone-start-marker", t_TH,		0 },
  { "gmt",	t_NDSTZONE,     HOUR( 0) }, /* Greenwich Mean */
  { "ut",	t_NDSTZONE,     HOUR( 0) }, /* Universal (Coordinated) */
  { "utc",	t_NDSTZONE,     HOUR( 0) },
  { "wet",	t_NDSTZONE,     HOUR( 0) }, /* Western European */
  { "bst",	t_DSTZONE,	HOUR( 0) }, /* British Summer */
  { "wat",	t_NDSTZONE,     HOUR( 1) }, /* West Africa */
#if	0
  { "at",	t_NDSTZONE,     HOUR( 2) }, /* Azores */
  /* For completeness.  BST is also British Summer, and GST is
     also Guam Standard. */
  { "bst",	t_NDSTZONE,     HOUR( 3) }, /* Brazil Standard */
  { "gst",	t_NDSTZONE,     HOUR( 3) }, /* Greenland Standard */
#endif
#if 0
  { "nft",	t_NDSTZONE,     HOUR(3.5) }, /* Newfoundland */
  { "nst",	t_NDSTZONE,     HOUR(3.5) }, /* Newfoundland Standard */
  { "ndt",	t_DSTZONE,	HOUR(3.5) }, /* Newfoundland Daylight */
#endif
  { "ast",	t_NDSTZONE,     HOUR( 4) }, /* Atlantic Standard */
  { "adt",	t_DSTZONE,	HOUR( 4) }, /* Atlantic Daylight */
  { "est5edt",	t_NDSTZONE,     HOUR( 5) }, /* Eastern Standard */
  { "est",	t_NDSTZONE,     HOUR( 5) }, /* Eastern Standard */
  { "edt",	t_DSTZONE,	HOUR( 5) }, /* Eastern Daylight */
  { "cst6cdt",	t_NDSTZONE,     HOUR( 6) }, /* Central Standard */
  { "cst",	t_NDSTZONE,     HOUR( 6) }, /* Central Standard */
  { "cdt",	t_DSTZONE,	HOUR( 6) }, /* Central Daylight */
  { "mst7mdt",	t_NDSTZONE,     HOUR( 7) }, /* Mountain Standard */
  { "mst",	t_NDSTZONE,     HOUR( 7) }, /* Mountain Standard */
  { "mdt",	t_DSTZONE,	HOUR( 7) }, /* Mountain Daylight */
  { "pst8pdt",	t_NDSTZONE,     HOUR( 8) }, /* Pacific Standard */
  { "pst",	t_NDSTZONE,     HOUR( 8) }, /* Pacific Standard */
  { "pdt",	t_DSTZONE,	HOUR( 8) }, /* Pacific Daylight */
  { "yst",	t_NDSTZONE,     HOUR( 9) }, /* Yukon Standard */
  { "ydt",	t_DSTZONE,	HOUR( 9) }, /* Yukon Daylight */
  { "hst",	t_NDSTZONE,     HOUR(10) }, /* Hawaii Standard */
  { "hdt",	t_DSTZONE,	HOUR(10) }, /* Hawaii Daylight */
  { "cat",	t_NDSTZONE,     HOUR(10) }, /* Central Alaska */
  { "ahst",	t_NDSTZONE,     HOUR(10) }, /* Alaska-Hawaii Standard */
  { "nt",	t_NDSTZONE,     HOUR(11) }, /* Nome */
  { "idlw",	t_NDSTZONE,     HOUR(12) }, /* International Date Line West */
  { "cet",	t_NDSTZONE,     -HOUR(1) }, /* Central European */
  { "met",	t_NDSTZONE,     -HOUR(1) }, /* Middle European */
  { "mewt",	t_NDSTZONE,     -HOUR(1) }, /* Middle European Winter */
  { "mest",	t_DSTZONE,	-HOUR(1) }, /* Middle European Summer */
  { "swt",	t_NDSTZONE,     -HOUR(1) }, /* Swedish Winter */
  { "sst",	t_DSTZONE,	-HOUR(1) }, /* Swedish Summer */
  { "fwt",	t_NDSTZONE,     -HOUR(1) }, /* French Winter */
  { "fst",	t_DSTZONE,	-HOUR(1) }, /* French Summer */
  { "eet",	t_NDSTZONE,     -HOUR(2) }, /* Eastern Europe, USSR Zone 1 */
  { "bt",	t_NDSTZONE,     -HOUR(3) }, /* Baghdad, USSR Zone 2 */
#if 0
  { "it",	t_NDSTZONE,     -HOUR(3.5) }, /* Iran */
#endif
  { "zp4",	t_NDSTZONE,     -HOUR(4) }, /* USSR Zone 3 */
  { "zp5",	t_NDSTZONE,     -HOUR(5) }, /* USSR Zone 4 */
#if 0
  { "ist",	t_NDSTZONE,     -HOUR(5.5) }, /* Indian Standard */
#endif
  { "zp6",	t_NDSTZONE,     -HOUR(6) }, /* USSR Zone 5 */
#if	0
  /* For completeness.  NST is also Newfoundland Stanard, and SST is
   * also Swedish Summer. */
  { "nst",	t_NDSTZONE,     -HOUR(6.5) }, /* North Sumatra */
  { "sst",	t_NDSTZONE,     -HOUR(7) }, /* South Sumatra, USSR Zone 6 */
#endif	/* 0 */
  { "wast",	t_NDSTZONE,     -HOUR(7) }, /* West Australian Standard */
  { "wadt",	t_DSTZONE,	-HOUR(7) }, /* West Australian Daylight */
#if 0
  { "jt",	t_NDSTZONE,     -HOUR(7.5) }, /* Java (3pm in Cronusland!) */
#endif
  { "cct",	t_NDSTZONE,     -HOUR(8) }, /* China Coast, USSR Zone 7 */
  { "jst",	t_NDSTZONE,     -HOUR(9) }, /* Japan Standard, USSR Zone 8 */
#if 0
  { "cast",	t_NDSTZONE,     -HOUR(9.5) }, /* Central Australian Standard */
  { "cadt",	t_DSTZONE,	-HOUR(9.5) }, /* Central Australian Daylight */
#endif
  { "east",	t_NDSTZONE,     -HOUR(10) }, /* Eastern Australian Standard */
  { "eadt",	t_DSTZONE,	-HOUR(10) }, /* Eastern Australian Daylight */
  { "gst",	t_NDSTZONE,     -HOUR(10) }, /* Guam Standard, USSR Zone 9 */
  { "nzt",	t_NDSTZONE,     -HOUR(12) }, /* New Zealand */
  { "nzst",	t_NDSTZONE,     -HOUR(12) }, /* New Zealand Standard */
  { "nzdt",	t_DSTZONE,	-HOUR(12) }, /* New Zealand Daylight */
  { "idle",	t_NDSTZONE,     -HOUR(12) }, /* International Date Line East */
  { "zone-end-marker", t_TH,		0 },
  { (char *)NULL, 0, 0 }
};

static int
moddate_error (char *s)
{
  return (0);
}

static WordLookup *
find_word (char *string)
{
  register int i;
  WordLookup *result = (WordLookup *)NULL;

  for (i = 0; known_words[i].word != (char *)NULL; i++)
    if ((strcasecmp (known_words[i].word, string) == 0) ||
	((strncasecmp (known_words[i].word, string, 3) == 0) &&
	 (strlen (string) == 3)))
      {
	result = &known_words[i];
	break;
      }

  return (result);
}

static char **tokens_read = (char **)NULL;
static int tr_slots = 0;
static int tr_index = 0;

static void
add_token (int token)
{
  char *string = "UnknownToken";

  switch (token)
    {
    case 0: string = "EndOfInput"; break;
    case t_YEARS: string = "t_YEARS"; break;
    case t_MONTHS: string = "t_MONTHS"; break;
    case t_DAYS: string = "t_DAYS"; break;
    case t_HOURS: string = "t_HOURS"; break;
    case t_MINUTES: string = "t_MINUTES"; break;
    case t_SECONDS: string = "t_SECONDS"; break;
    case t_AT: string = "t_AT"; break;
    case t_FROM: string = "t_FROM"; break;
    case t_BEFORE: string = "t_BEFORE"; break;
    case t_AGO: string = "t_AGO"; break;
    case t_NOW: string = "t_NOW"; break;
    case t_NEXT: string = "t_NEXT"; break;
    case t_LAST: string = "t_LAST"; break;
    case t_AM: string = "t_AM"; break;
    case t_PM: string = "t_PM"; break;
    case t_WEEKDAY: string = "t_WEEKDAY"; break;
    case t_MONTH: string = "t_MONTH"; break;
    case t_DSTZONE: string = "t_DSTZONE"; break;
    case t_NDSTZONE: string = "t_NDSTZONE"; break;
    case t_NUMBER: string = "t_NUMBER"; break;
    case t_YEARNUM: string = "t_YEARNUM"; break;
    case t_DATESEP: string = "t_DATESEP"; break;
    case t_TIMESEP: string = "t_TIMESEP"; break;
    case t_RELATIVE: string = "t_RELATIVE"; break;
    case t_BIGNUM: string = "t_BIGNUM"; break;
    case t_TH: string = "t_TH"; break;
    }

  if (tr_index + 2 > tr_slots)
    tokens_read = (char **)
      xrealloc (tokens_read, (tr_slots += 10) * sizeof (char *));

  tokens_read[tr_index++] = string;
  tokens_read[tr_index] = (char *)NULL;
}

static int
moddate_lex (void)
{
  int yearnum_possible = 0;
  int token = 0;

  while (1)
    {
      char *word = input_words[iw_index++];

      if (word == (char *)NULL)
	{
	  iw_index = 0;
	  break;
	}
      else
	{
	  if (strcmp (word, ",") == 0)
	    {
	      yearnum_possible++;
	      continue;
	    }

	  if (strcmp (word, ":") == 0)
	    {
	      token = t_TIMESEP;
	      break;
	    }

	  if ((strcmp (word, "/") == 0) || (strcmp (word, "-") == 0))
	    {
	      token = t_DATESEP;
	      break;
	    }

	  if (mhtml_all_digits (word))
	    {
	      time_t number = (time_t)strtol (word, (char **)NULL, 10);
	      yylval.Number = number;

	      if (number > 10000)
		token = t_BIGNUM;
	      else if (yearnum_possible)
		token = t_YEARNUM;
	      else
		token = t_NUMBER;
	      break;
	    }
	  else
	    {
	      WordLookup *wl = find_word (word);

	      if (wl)
		{
		  yylval.Number = wl->value;
		  token = wl->token;
		  break;
		}
	    }
	}
    }

  if (getdate_print_parts > 5)
    add_token (token);

  return (token);
}

static void
zone_handle (time_t seconds_from_gmt, int dst_p)
{
#if defined (linux)
  register int i;
  static int zone_start = 0;
  static int zone_end = 0;

  if (zone_end == 0)
    {
      for (i = 0; known_words[i].word != (char *)NULL; i++)
	{
	  if (strcmp (known_words[i].word, "zone-start-marker") == 0)
	    zone_start = i;
	  else if (strcmp (known_words[i].word, "zone-end-marker") == 0)
	    {
	      zone_end = i;
	      break;
	    }
	}
    }

  for (i = zone_start + 1; i < zone_end; i++)
    if (known_words[i].value == seconds_from_gmt)
      {
	input_date.tm_zone = known_words[i].word;
	break;
      }
#endif
  input_date.tm_isdst = dst_p;
  zone_found++;
}

static char *
dump_struct_tm (struct tm *tm, char *label)
{
  BPRINTF_BUFFER *b = bprintf_create_buffer ();
  char *result = (char *)NULL;

  bprintf (b, "tm->%s {\n", label);
  bprintf (b, "   tm_sec =  %d,\n", tm->tm_sec);
  bprintf (b, "   tm_min =  %d,\n", tm->tm_min);
  bprintf (b, "   tm_hour = %d,\n", tm->tm_hour);
  bprintf (b, "   tm_mday = %d,\n", tm->tm_mday);
  bprintf (b, "   tm_mon =  %d,\n", tm->tm_mon);
  bprintf (b, "   tm_year = %d,\n", tm->tm_year);
  bprintf (b, "   tm_wday = %d,\n", tm->tm_wday);
  bprintf (b, "   tm_yday = %d,\n", tm->tm_yday);
  bprintf (b, "   tm_isdst =%d,\n", tm->tm_isdst);
#if defined (linux)
  bprintf (b, "   tm_zone = %s,\n", tm->tm_zone ? tm->tm_zone : "[none]");
#endif
  bprintf (b, "}\n");
  result = b->buffer;
  free (b);
  return (result);
}
  
time_t
get_date (char *string, char *zone_name)
{
  register int i;
  time_t result = (time_t)time ((time_t *)NULL);
  struct tm *temp_time;
  extern char **environ;
  char **saved_environ = environ;
  char *fake_environ[2];
  char use_zone[100];

  use_zone[0] = '\0'; use_zone[99] = '\0';

  /* Initialize the working time_structures. */
  if (zone_name != (char *)NULL && *zone_name != '\0')
    {
      BPRINTF_BUFFER *b = bprintf_create_buffer ();
      char *temp = (char *)NULL;

      bprintf (b, "timezone-translations::%s", zone_name);
      temp = pagefunc_get_variable (b->buffer);
      b->bindex = 0;

      if (empty_string_p (temp))
	strncpy (use_zone, zone_name, 99);
      else
	strncpy (use_zone, temp, 99);

      for (i = 0; use_zone[i] != '\0'; i++)
	if (islower (use_zone[i]))
	  use_zone[i] = toupper (use_zone[i]);

      bprintf (b, "TZ=%s", use_zone);
      fake_environ[0] = b->buffer;
      fake_environ[1] = (char *)NULL;
      environ = fake_environ;

      if (getdate_print_parts > 5)
	page_debug ("Zone Used For Defaults: %s", use_zone);

      temp_time = (struct tm *)localtime (&result);

      environ = saved_environ;
      bprintf_free_buffer (b);
    }
  else
    temp_time = (struct tm *)localtime (&result);

  input_date.tm_sec = temp_time->tm_sec;
  input_date.tm_min = temp_time->tm_min;
  input_date.tm_hour = temp_time->tm_hour;
  input_date.tm_mday = temp_time->tm_mday;
  input_date.tm_mon = temp_time->tm_mon;
  input_date.tm_year = temp_time->tm_year;
  input_date.tm_wday = temp_time->tm_wday;
  input_date.tm_yday = temp_time->tm_yday;
  input_date.tm_isdst = temp_time->tm_isdst;
#if defined (linux)
  input_date.tm_zone = (char *)NULL;
#endif

  relative_seconds = (time_t)0;
  date_found = 0;
  time_found = 0;
  zone_found = 0;

  tr_index = 0;
  if (tokens_read) tokens_read[0] = (char *)NULL;

  /* Initialize/zero out the list of input tokens. */
  if (input_words == (char **)NULL)
    {
      iw_slots = 12;
      input_words = (char **)xmalloc ((1 + iw_slots) * sizeof (char *));
      for (i = 0; i < iw_slots; i++)
	input_words[i] = (char *)NULL;
    }
  else
    {
      for (i = 0; i < iw_slots; i++)
	if (input_words[i] != (char *)NULL)
	  {
	    free (input_words[i]);
	    input_words[i] = (char *)NULL;
	  }
    }
  iw_index = 0;

  /* Create the array of words that comprise the input string. */
  i = 0; if (string == (char *)NULL) string = "";
  while (string[i] != '\0')
    {
      int beg, end;

      while (isspace (string[i])) i++;
      if (string[i] == '\0') break;

      /* Pointing at start of word.  Words are all digits, or all alpha,
	 or single punctuation. */
      beg = i;
      if (isalpha (string[i]))
	{
	  while (isalpha (string[i])) i++;
	}
      else if (isdigit (string[i]))
	{
	  while (isdigit (string[i])) i++;
	}
      else
	i++;

      end = i;

      /* Add the word to our array. */
      if (iw_index + 2 > iw_slots)
	{
	  register int j;

	  input_words = (char **)
	    xrealloc (input_words, (iw_slots += 10) * sizeof (char *));

	  for (j = iw_index; j < iw_slots; j++)
	    input_words[j] = (char *)NULL;
	}

      input_words[iw_index] = (char *)xmalloc (1 + (end - beg));
      strncpy (input_words[iw_index], string + beg, (end - beg));
      input_words[iw_index][(end - beg)] = '\0';
      iw_index++;
      input_words[iw_index] = (char *)NULL;
    }

  /* Parse the string. */
  iw_index = 0;
  moddate_parse ();

  if (getdate_print_parts > 5)
    {
      BPRINTF_BUFFER *b = bprintf_create_buffer ();

      for (i = 0; i < tr_index; i++)
	bprintf (b, "%s ", tokens_read[i]);

      page_debug ("%s", b->buffer);
      bprintf_free_buffer (b);
    }

#if defined (linux)
  if (input_date.tm_zone != (char *)NULL)
    strcpy (use_zone, input_date.tm_zone);

  for (i = 0; use_zone[i] != '\0'; i++)
    if (islower (use_zone[i]))
      use_zone[i] = toupper (use_zone[i]);
#endif

  if (use_zone != (char *)NULL && *use_zone != '\0')
    {
      BPRINTF_BUFFER *b = bprintf_create_buffer ();
      bprintf (b, "TZ=%s", use_zone);
      fake_environ[0] = b->buffer;
      fake_environ[1] = (char *)NULL;
      environ = fake_environ;

      if (getdate_print_parts > 5)
	page_debug ("Zone Used For Final: %s", use_zone);

      input_date.tm_isdst = -1;
      result = mktime (&input_date);

      environ = saved_environ;
      bprintf_free_buffer (b);
    }
  else
    result = mktime (&input_date);

  result += relative_seconds;
  return (result);
}
