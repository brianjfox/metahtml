/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     t_YEARS = 258,
     t_MONTHS = 259,
     t_DAYS = 260,
     t_HOURS = 261,
     t_MINUTES = 262,
     t_SECONDS = 263,
     t_AT = 264,
     t_FROM = 265,
     t_AGO = 266,
     t_NOW = 267,
     t_NEXT = 268,
     t_LAST = 269,
     t_AM = 270,
     t_PM = 271,
     t_TH = 272,
     t_BEFORE = 273,
     t_AFTER = 274,
     t_CHRISTMAS = 275,
     t_EASTER = 276,
     t_THANKSGIVING = 277,
     t_CHANUKAH = 278,
     t_NEWYEARS = 279,
     t_WEEKDAY = 280,
     t_MONTH = 281,
     t_DSTZONE = 282,
     t_NDSTZONE = 283,
     t_NUMBER = 284,
     t_YEARNUM = 285,
     t_DATESEP = 286,
     t_TIMESEP = 287,
     t_RELATIVE = 288,
     t_BIGNUM = 289
   };
#endif
#define t_YEARS 258
#define t_MONTHS 259
#define t_DAYS 260
#define t_HOURS 261
#define t_MINUTES 262
#define t_SECONDS 263
#define t_AT 264
#define t_FROM 265
#define t_AGO 266
#define t_NOW 267
#define t_NEXT 268
#define t_LAST 269
#define t_AM 270
#define t_PM 271
#define t_TH 272
#define t_BEFORE 273
#define t_AFTER 274
#define t_CHRISTMAS 275
#define t_EASTER 276
#define t_THANKSGIVING 277
#define t_CHANUKAH 278
#define t_NEWYEARS 279
#define t_WEEKDAY 280
#define t_MONTH 281
#define t_DSTZONE 282
#define t_NDSTZONE 283
#define t_NUMBER 284
#define t_YEARNUM 285
#define t_DATESEP 286
#define t_TIMESEP 287
#define t_RELATIVE 288
#define t_BIGNUM 289




/* Copy the first part of user declarations.  */
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


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 60 "parsedate.y"
typedef union YYSTYPE { time_t Number; } YYSTYPE;
/* Line 191 of yacc.c.  */
#line 204 "y.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 216 "y.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   88

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  35
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  8
/* YYNRULES -- Number of rules. */
#define YYNRULES  45
/* YYNRULES -- Number of states. */
#define YYNSTATES  71

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   289

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     4,     7,     9,    11,    13,    15,    17,
      21,    27,    30,    33,    38,    43,    49,    53,    56,    59,
      65,    71,    73,    75,    82,    84,    87,    89,    91,    93,
      96,    99,   102,   106,   109,   112,   115,   118,   121,   124,
     128,   132,   134,   137,   140,   143
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      36,     0,    -1,    -1,    36,    37,    -1,    39,    -1,    38,
      -1,    40,    -1,    41,    -1,    42,    -1,    29,    32,    29,
      -1,    29,    32,    29,    32,    29,    -1,    29,    15,    -1,
      29,    16,    -1,    29,    32,    29,    15,    -1,    29,    32,
      29,    16,    -1,    29,    31,    29,    31,    29,    -1,    29,
      31,    29,    -1,    26,    29,    -1,    29,    26,    -1,    29,
      31,    26,    31,    29,    -1,    30,    31,    29,    31,    29,
      -1,    20,    -1,    30,    -1,    25,    30,    26,    29,    38,
      41,    -1,    34,    -1,    26,    30,    -1,    17,    -1,    27,
      -1,    28,    -1,    27,    29,    -1,    28,    29,    -1,    29,
      33,    -1,    29,    33,    11,    -1,    13,    33,    -1,    13,
      25,    -1,    14,    25,    -1,    10,    39,    -1,    19,    39,
      -1,    18,    39,    -1,    10,    14,    25,    -1,    10,    13,
      25,    -1,    33,    -1,    14,    33,    -1,    10,    33,    -1,
      18,    33,    -1,    19,    33,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,    84,    84,    84,    86,    87,    88,    89,    90,    93,
      98,   115,   121,   127,   133,   142,   163,   168,   173,   178,
     188,   196,   201,   208,   217,   230,   241,   245,   249,   253,
     261,   272,   276,   280,   284,   297,   319,   322,   325,   329,
     338,   351,   355,   359,   363,   367
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "t_YEARS", "t_MONTHS", "t_DAYS", "t_HOURS", 
  "t_MINUTES", "t_SECONDS", "t_AT", "t_FROM", "t_AGO", "t_NOW", "t_NEXT", 
  "t_LAST", "t_AM", "t_PM", "t_TH", "t_BEFORE", "t_AFTER", "t_CHRISTMAS", 
  "t_EASTER", "t_THANKSGIVING", "t_CHANUKAH", "t_NEWYEARS", "t_WEEKDAY", 
  "t_MONTH", "t_DSTZONE", "t_NDSTZONE", "t_NUMBER", "t_YEARNUM", 
  "t_DATESEP", "t_TIMESEP", "t_RELATIVE", "t_BIGNUM", "$accept", "spec", 
  "item", "time_spec", "date_spec", "useless_word", "zone_spec", 
  "rel_spec", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    35,    36,    36,    37,    37,    37,    37,    37,    38,
      38,    38,    38,    38,    38,    39,    39,    39,    39,    39,
      39,    39,    39,    39,    39,    39,    40,    41,    41,    41,
      41,    42,    42,    42,    42,    42,    42,    42,    42,    42,
      42,    42,    42,    42,    42,    42
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     1,     3,
       5,     2,     2,     4,     4,     5,     3,     2,     2,     5,
       5,     1,     1,     6,     1,     2,     1,     1,     1,     2,
       2,     2,     3,     2,     2,     2,     2,     2,     2,     3,
       3,     1,     2,     2,     2,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       2,     0,     1,     0,     0,     0,    26,     0,     0,    21,
       0,     0,    27,    28,     0,    22,    41,    24,     3,     5,
       4,     6,     7,     8,     0,     0,     0,    43,    36,    34,
      33,    35,    42,    44,    38,    45,    37,     0,    17,    25,
      29,    30,    11,    12,    18,     0,     0,    31,     0,    40,
      39,     0,     0,    16,     9,    32,     0,     0,     0,     0,
      13,    14,     0,     0,     0,     0,    19,    15,    10,    20,
      23
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     1,    18,    19,    20,    21,    22,    23
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -29
static const yysigned_char yypact[] =
{
     -29,     0,   -29,    18,   -24,   -22,   -29,    29,    40,   -29,
     -28,    27,   -25,    13,   -10,    14,   -29,   -29,   -29,   -29,
     -29,   -29,   -29,   -29,    25,    36,    15,   -29,   -29,   -29,
     -29,   -29,   -29,   -29,   -29,   -29,   -29,    38,   -29,   -29,
     -29,   -29,   -29,   -29,   -29,   -14,    39,    56,    46,   -29,
     -29,    47,    48,    49,    -8,   -29,    50,    53,    54,    55,
     -29,   -29,    57,    58,    21,    44,   -29,   -29,   -29,   -29,
     -29
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -29,   -29,   -29,    20,    32,   -29,    23,   -29
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
       2,    29,    37,    31,    40,    42,    43,    60,    61,    30,
       3,    32,    52,     4,     5,    53,    44,     6,     7,     8,
       9,    45,    46,    47,    62,    10,    11,    12,    13,    14,
      15,    24,    25,    16,    17,    28,    42,    43,     9,    34,
      36,    44,    41,    10,    11,    48,    45,    26,    15,     9,
      49,    27,    17,    46,    10,    11,    38,    39,    26,    15,
       9,    50,    33,    17,    51,    10,    11,    55,    54,    26,
      15,    12,    13,    35,    17,    56,    57,    65,     0,    58,
      59,    63,    64,    66,    67,     0,    68,    69,    70
};

static const yysigned_char yycheck[] =
{
       0,    25,    30,    25,    29,    15,    16,    15,    16,    33,
      10,    33,    26,    13,    14,    29,    26,    17,    18,    19,
      20,    31,    32,    33,    32,    25,    26,    27,    28,    29,
      30,    13,    14,    33,    34,     3,    15,    16,    20,     7,
       8,    26,    29,    25,    26,    31,    31,    29,    30,    20,
      25,    33,    34,    32,    25,    26,    29,    30,    29,    30,
      20,    25,    33,    34,    26,    25,    26,    11,    29,    29,
      30,    27,    28,    33,    34,    29,    29,    57,    -1,    31,
      31,    31,    29,    29,    29,    -1,    29,    29,    65
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    36,     0,    10,    13,    14,    17,    18,    19,    20,
      25,    26,    27,    28,    29,    30,    33,    34,    37,    38,
      39,    40,    41,    42,    13,    14,    29,    33,    39,    25,
      33,    25,    33,    33,    39,    33,    39,    30,    29,    30,
      29,    29,    15,    16,    26,    31,    32,    33,    31,    25,
      25,    26,    26,    29,    29,    11,    29,    29,    31,    31,
      15,    16,    32,    31,    29,    38,    29,    29,    29,    29,
      41
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 4:
#line 86 "parsedate.y"
    { date_found++; }
    break;

  case 5:
#line 87 "parsedate.y"
    { time_found++; }
    break;

  case 9:
#line 93 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("TIME_SPEC 1\n");
	  input_date.tm_hour = yyvsp[-2].Number;
	  input_date.tm_min = yyvsp[0].Number;
	}
    break;

  case 10:
#line 98 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("TIME_SPEC 2\n");
	  input_date.tm_hour = yyvsp[-4].Number;
	  input_date.tm_min = yyvsp[-2].Number;
	  input_date.tm_sec = yyvsp[0].Number;
	}
    break;

  case 11:
#line 115 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("TIME_SPEC 3\n");
	  input_date.tm_hour = yyvsp[-1].Number;
	  input_date.tm_min = 0;
	  input_date.tm_sec = 0;
	}
    break;

  case 12:
#line 121 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("TIME_SPEC 4\n");
	  input_date.tm_hour = yyvsp[-1].Number + 12;
	  input_date.tm_min = 0;
	  input_date.tm_sec = 0;
	}
    break;

  case 13:
#line 127 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("TIME_SPEC 5\n");
	  input_date.tm_hour = yyvsp[-3].Number;
	  input_date.tm_min = yyvsp[-1].Number;
	  input_date.tm_sec = 0;
	}
    break;

  case 14:
#line 133 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("TIME_SPEC 6\n");
	  input_date.tm_hour = yyvsp[-3].Number + 12;
	  input_date.tm_min = yyvsp[-1].Number;
	  input_date.tm_sec = 0;
	}
    break;

  case 15:
#line 142 "parsedate.y"
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
	}
    break;

  case 16:
#line 163 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("DATE_SPEC 2\n");
	  input_date.tm_mon = yyvsp[-2].Number - 1;
	  input_date.tm_mday = yyvsp[0].Number;
	}
    break;

  case 17:
#line 168 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("DATE_SPEC 3\n");
	  input_date.tm_mon = yyvsp[-1].Number - 1;
	  input_date.tm_mday = yyvsp[0].Number;
	}
    break;

  case 18:
#line 173 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("DATE_SPEC 4\n");
	  input_date.tm_mday = yyvsp[-1].Number;
	  input_date.tm_mon = yyvsp[0].Number - 1;
	}
    break;

  case 19:
#line 178 "parsedate.y"
    {
	    int year = yyvsp[0].Number;

	    if (year < 50) year += 2000;
	    else if (year < 100) year += 1900;
	    input_date.tm_year = year - 1900;
	    input_date.tm_mon = yyvsp[-2].Number - 1;
	    input_date.tm_mday = yyvsp[-4].Number;
	    DEBUGGING_OUTPUT ("DATE_SPEC 4.1 (12-Jan-96)\n");
	}
    break;

  case 20:
#line 188 "parsedate.y"
    {
	    int year = yyvsp[-4].Number;

	    input_date.tm_year = year - 1900;
	    input_date.tm_mon = yyvsp[-2].Number - 1;
	    input_date.tm_mday = yyvsp[0].Number;
	    DEBUGGING_OUTPUT ("DATE_SPEC 4.11 (1999-03-04)\n");
	}
    break;

  case 21:
#line 196 "parsedate.y"
    {
	  input_date.tm_mon = 11;
	  input_date.tm_mday = 25;
	}
    break;

  case 22:
#line 201 "parsedate.y"
    {
	  int year = yyvsp[0].Number;
	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  DEBUGGING_OUTPUT ("DATE_SPEC 5 (t_YEARNUM)\n");
	}
    break;

  case 23:
#line 208 "parsedate.y"
    {
	  int year = yyvsp[-2].Number;
	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  input_date.tm_mon = yyvsp[-3].Number - 1;
	  input_date.tm_mday = yyvsp[-4].Number;
	  DEBUGGING_OUTPUT ("DATE_SPEC 5.1");
	}
    break;

  case 24:
#line 217 "parsedate.y"
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
	}
    break;

  case 25:
#line 230 "parsedate.y"
    {
	  int year = yyvsp[0].Number;
	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  input_date.tm_mon = yyvsp[-1].Number - 1;
	  DEBUGGING_OUTPUT ("DATE_SPEC 7\n");
	}
    break;

  case 26:
#line 241 "parsedate.y"
    { /* Ignored at all times. */ }
    break;

  case 27:
#line 245 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("ZONE_SPEC 1");
	  zone_handle (yyvsp[0].Number, 1);
	}
    break;

  case 28:
#line 249 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("ZONE_SPEC 2");
	  zone_handle (yyvsp[0].Number, 0);
	}
    break;

  case 29:
#line 253 "parsedate.y"
    {
	  int year = yyvsp[0].Number;
	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  zone_handle (yyvsp[-1].Number, 1);
	  DEBUGGING_OUTPUT ("ZONE_SPEC 3");
	}
    break;

  case 30:
#line 261 "parsedate.y"
    {
	  int year = yyvsp[0].Number;
	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  zone_handle (yyvsp[-1].Number, 0);
	  DEBUGGING_OUTPUT ("ZONE_SPEC 4");
	}
    break;

  case 31:
#line 272 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("REL_SPEC 1\n");
	  relative_seconds += (yyvsp[-1].Number * yyvsp[0].Number);
	}
    break;

  case 32:
#line 276 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("REL_SPEC 2\n");
	  relative_seconds -= (yyvsp[-2].Number * yyvsp[-1].Number);
	}
    break;

  case 33:
#line 280 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("REL_SPEC 3\n");
	  relative_seconds += yyvsp[0].Number;
	}
    break;

  case 34:
#line 284 "parsedate.y"
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
	}
    break;

  case 35:
#line 297 "parsedate.y"
    {
	  int days = (input_date.tm_wday - yyvsp[0].Number) % 7;
	  if (days == 0)
	    days = 7;
	  else if (days < 0)
	    days = 7 + days;
	  relative_seconds -= days * seconds_per_day;
	  DEBUGGING_OUTPUT ("REL_SPEC 5\n");
	}
    break;

  case 36:
#line 319 "parsedate.y"
    { /* Nothing special happens here. */
	  DEBUGGING_OUTPUT ("REL_SPEC 6.1\n");
	}
    break;

  case 37:
#line 322 "parsedate.y"
    { /* Nothing special happens here. */
	  DEBUGGING_OUTPUT ("REL_SPEC 6.2\n");
	}
    break;

  case 38:
#line 325 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("REL_SPEC 6.3\n");
	  relative_seconds = -relative_seconds;
	}
    break;

  case 39:
#line 329 "parsedate.y"
    {
	  int days = (input_date.tm_wday - yyvsp[0].Number) % 7;
	  if (days == 0)
	    days = 7;
	  else if (days < 0)
	    days = 7 + days;
	  relative_seconds -= days * seconds_per_day;
	  DEBUGGING_OUTPUT ("REL_SPEC 7\n");
	}
    break;

  case 40:
#line 338 "parsedate.y"
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
	}
    break;

  case 41:
#line 351 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("REL_SPEC 9\n");
	  relative_seconds += yyvsp[0].Number;
	}
    break;

  case 42:
#line 355 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("REL_SPEC 10\n");
	  relative_seconds -= yyvsp[0].Number;
	}
    break;

  case 43:
#line 359 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("REL_SPEC 11\n");
	  relative_seconds += yyvsp[0].Number;
	}
    break;

  case 44:
#line 363 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("REL_SPEC 12\n");
	  relative_seconds += yyvsp[0].Number;
	}
    break;

  case 45:
#line 367 "parsedate.y"
    {
	  DEBUGGING_OUTPUT ("REL_SPEC 13\n");
	  relative_seconds += yyvsp[0].Number;
	}
    break;


    }

/* Line 999 of yacc.c.  */
#line 1554 "y.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 372 "parsedate.y"


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
  { "christmas",	t_CHRISTMAS,	0 },
  { "easter",		t_EASTER,	0 },
  { "chanukah",		t_CHANUKAH,	0 },
  { "thanksgiving",	t_THANKSGIVING,	0 },

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
#if defined (__linux__)
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
#endif /* __linux__ */
  input_date.tm_isdst = dst_p;
  zone_found++;
}

#if defined (PARSEDATE_DEBUGGING_STRUCT)
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
#if defined (__linux__)
  bprintf (b, "   tm_zone = %s,\n", tm->tm_zone ? tm->tm_zone : "[none]");
#endif /* __linux__ */
  bprintf (b, "}\n");
  result = b->buffer;
  free (b);
  return (result);
}
#endif
  
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

#if defined (__linux__)
  if (input_date.tm_zone != (char *)NULL)
    strcpy (use_zone, input_date.tm_zone);

  for (i = 0; use_zone[i] != '\0'; i++)
    if (islower (use_zone[i]))
      use_zone[i] = toupper (use_zone[i]);
#endif /* __linux__ */

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


