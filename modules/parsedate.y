%{  /* Yet another yacc parser for date formats. */

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
%}

%union { time_t Number; }

/* Tokens which are simply words, like "hours" and "from", or "ago". */
%token t_YEARS t_MONTHS t_DAYS t_HOURS t_MINUTES t_SECONDS
%token t_AT t_FROM t_AGO t_NOW t_NEXT t_LAST t_AM t_PM t_TH t_BEFORE t_AFTER
%token t_CHRISTMAS t_EASTER t_THANKSGIVING t_CHANUKAH t_NEWYEARS

/* More words, specific days, months, and timezones. */
%token t_WEEKDAY t_MONTH t_DSTZONE t_NDSTZONE

/* Fundamental tokens. */
%token t_NUMBER t_YEARNUM t_DATESEP t_TIMESEP t_RELATIVE t_BIGNUM

%type <Number> t_WEEKDAY t_MONTH t_DSTZONE t_NDSTZONE t_TH
%type <Number> t_NUMBER t_YEARNUM t_RELATIVE t_BIGNUM

/* Expect exactly 2 shift/reduce conflicts.
   They appear in the zone_spec declarations, and I don't see an easy way
   to make them go away.  Since both yacc and bison do the right thing,
   don't alarm the consumer by printing messages about the conflict. */
%expect 2

%%

spec: /* EMPTY */ | spec item;

item:     date_spec { date_found++; }
	| time_spec { time_found++; }
        | useless_word
	| zone_spec
	| rel_spec;

time_spec:
	t_NUMBER t_TIMESEP t_NUMBER {
	  DEBUGGING_OUTPUT ("TIME_SPEC 1\n");
	  input_date.tm_hour = $1;
	  input_date.tm_min = $3;
	}
	| t_NUMBER t_TIMESEP t_NUMBER t_TIMESEP t_NUMBER {
	  DEBUGGING_OUTPUT ("TIME_SPEC 2\n");
	  input_date.tm_hour = $1;
	  input_date.tm_min = $3;
	  input_date.tm_sec = $5;
	}
/*	| t_NUMBER t_TIMESEP t_NUMBER t_TIMESEP t_NUMBER t_NUMBER {
	  int year = $6;

	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  input_date.tm_hour = $1;
	  input_date.tm_min = $3;
	  input_date.tm_sec = $5;
	  DEBUGGING_OUTPUT ("TIME_SPEC 2.1\n");
	  } */
	| t_NUMBER t_AM {
	  DEBUGGING_OUTPUT ("TIME_SPEC 3\n");
	  input_date.tm_hour = $1;
	  input_date.tm_min = 0;
	  input_date.tm_sec = 0;
	}
	| t_NUMBER t_PM {
	  DEBUGGING_OUTPUT ("TIME_SPEC 4\n");
	  input_date.tm_hour = $1 + 12;
	  input_date.tm_min = 0;
	  input_date.tm_sec = 0;
	}
	| t_NUMBER t_TIMESEP t_NUMBER t_AM {
	  DEBUGGING_OUTPUT ("TIME_SPEC 5\n");
	  input_date.tm_hour = $1;
	  input_date.tm_min = $3;
	  input_date.tm_sec = 0;
	}
	| t_NUMBER t_TIMESEP t_NUMBER t_PM {
	  DEBUGGING_OUTPUT ("TIME_SPEC 6\n");
	  input_date.tm_hour = $1 + 12;
	  input_date.tm_min = $3;
	  input_date.tm_sec = 0;
	}
	;

date_spec:
	t_NUMBER t_DATESEP t_NUMBER t_DATESEP t_NUMBER {
	  DEBUGGING_OUTPUT ("DATE_SPEC 1");
	  if ($1 > 1900)
	    {
	      DEBUGGING_OUTPUT ("DATE_SPEC 1.1 YYYY.MM.DD\n");
	      input_date.tm_year = $1 - 1900;
	      input_date.tm_mon = $3 - 1;
	      input_date.tm_mday = $5;
	    }
	  else
	    {
	      int year = $5;
	      DEBUGGING_OUTPUT ("DATE_SPEC 1.1 MM/DD/YY[yy]\n");
	      input_date.tm_mon = $1 - 1;
	      input_date.tm_mday = $3;
	      if (year < 50) year += 2000;
	      else if (year < 100) year += 1900;
	      input_date.tm_year = year - 1900;
	    }
	}
	/* MM/DD (current year) */
	| t_NUMBER t_DATESEP t_NUMBER {
	  DEBUGGING_OUTPUT ("DATE_SPEC 2\n");
	  input_date.tm_mon = $1 - 1;
	  input_date.tm_mday = $3;
	}
	| t_MONTH t_NUMBER {
	  DEBUGGING_OUTPUT ("DATE_SPEC 3\n");
	  input_date.tm_mon = $1 - 1;
	  input_date.tm_mday = $2;
	}
	| t_NUMBER t_MONTH {
	  DEBUGGING_OUTPUT ("DATE_SPEC 4\n");
	  input_date.tm_mday = $1;
	  input_date.tm_mon = $2 - 1;
	}
	| t_NUMBER t_DATESEP t_MONTH t_DATESEP t_NUMBER {
	    int year = $5;

	    if (year < 50) year += 2000;
	    else if (year < 100) year += 1900;
	    input_date.tm_year = year - 1900;
	    input_date.tm_mon = $3 - 1;
	    input_date.tm_mday = $1;
	    DEBUGGING_OUTPUT ("DATE_SPEC 4.1 (12-Jan-96)\n");
	}
	| t_YEARNUM t_DATESEP t_NUMBER t_DATESEP t_NUMBER {
	    int year = $1;

	    input_date.tm_year = year - 1900;
	    input_date.tm_mon = $3 - 1;
	    input_date.tm_mday = $5;
	    DEBUGGING_OUTPUT ("DATE_SPEC 4.11 (1999-03-04)\n");
	}
	| t_CHRISTMAS {
	  input_date.tm_mon = 11;
	  input_date.tm_mday = 25;
	}
/* | t_WEEKDAY { } */
	| t_YEARNUM {
	  int year = $1;
	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  DEBUGGING_OUTPUT ("DATE_SPEC 5 (t_YEARNUM)\n");
	}
	| t_WEEKDAY t_YEARNUM t_MONTH t_NUMBER time_spec zone_spec {
	  int year = $4;
	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  input_date.tm_mon = $3 - 1;
	  input_date.tm_mday = $2;
	  DEBUGGING_OUTPUT ("DATE_SPEC 5.1");
	}
	| t_BIGNUM {
	  DEBUGGING_OUTPUT ("DATE_SPEC 6\n");
	  if ($1 > 10000)
	    {
	      /* YYYYMMDD */
	      time_t date = $1;
	      time_t year = date / 10000;
	      input_date.tm_year = year - 1900;
	      date -= year * 10000;
	      input_date.tm_mday = date % 100;
	      input_date.tm_mon = (date / 100) - 1;
	    }
	}
	| t_MONTH t_YEARNUM {
	  int year = $2;
	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  input_date.tm_mon = $1 - 1;
	  DEBUGGING_OUTPUT ("DATE_SPEC 7\n");
	}
	;

useless_word:
	t_TH { /* Ignored at all times. */ }
	;

zone_spec:
	t_DSTZONE {
	  DEBUGGING_OUTPUT ("ZONE_SPEC 1");
	  zone_handle ($1, 1);
	}
	| t_NDSTZONE {
	  DEBUGGING_OUTPUT ("ZONE_SPEC 2");
	  zone_handle ($1, 0);
	}
	| t_DSTZONE t_NUMBER {
	  int year = $2;
	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  zone_handle ($1, 1);
	  DEBUGGING_OUTPUT ("ZONE_SPEC 3");
	}
	| t_NDSTZONE t_NUMBER {
	  int year = $2;
	  if (year < 50) year += 2000;
	  else if (year < 100) year += 1900;
	  input_date.tm_year = year - 1900;
	  zone_handle ($1, 0);
	  DEBUGGING_OUTPUT ("ZONE_SPEC 4");
	}
	;

rel_spec:
	t_NUMBER t_RELATIVE {
	  DEBUGGING_OUTPUT ("REL_SPEC 1\n");
	  relative_seconds += ($1 * $2);
	}
	| t_NUMBER t_RELATIVE t_AGO {
	  DEBUGGING_OUTPUT ("REL_SPEC 2\n");
	  relative_seconds -= ($1 * $2);
	}
	| t_NEXT t_RELATIVE {
	  DEBUGGING_OUTPUT ("REL_SPEC 3\n");
	  relative_seconds += $2;
	}
	| t_NEXT t_WEEKDAY {
	  int days;

	  if ($2 == input_date.tm_wday)
	    days = 7;
	  else if ($2 < input_date.tm_wday)
	    days = 7 - (input_date.tm_wday - $2);
	  else
	    days = $2 - input_date.tm_wday;

	  relative_seconds += days * seconds_per_day;
	  DEBUGGING_OUTPUT ("REL_SPEC 4\n");
	}
	| t_LAST t_WEEKDAY {
	  int days = (input_date.tm_wday - $2) % 7;
	  if (days == 0)
	    days = 7;
	  else if (days < 0)
	    days = 7 + days;
	  relative_seconds -= days * seconds_per_day;
	  DEBUGGING_OUTPUT ("REL_SPEC 5\n");
	}
/*	| t_FROM t_WEEKDAY {
	  int days;

	  if ($2 == input_date.tm_wday)
	    days = 7;
	  else if ($2 < input_date.tm_wday)
	    days = 7 - (input_date.tm_wday - $2);
	  else
	    days = $2 - input_date.tm_wday;

	  relative_seconds += days * seconds_per_day;
	  DEBUGGING_OUTPUT ("REL_SPEC 6\n");
	  } */
	| t_FROM date_spec { /* Nothing special happens here. */
	  DEBUGGING_OUTPUT ("REL_SPEC 6.1\n");
	}
	| t_AFTER date_spec { /* Nothing special happens here. */
	  DEBUGGING_OUTPUT ("REL_SPEC 6.2\n");
	}
	| t_BEFORE date_spec {
	  DEBUGGING_OUTPUT ("REL_SPEC 6.3\n");
	  relative_seconds = -relative_seconds;
	}
	| t_FROM t_LAST t_WEEKDAY {
	  int days = (input_date.tm_wday - $3) % 7;
	  if (days == 0)
	    days = 7;
	  else if (days < 0)
	    days = 7 + days;
	  relative_seconds -= days * seconds_per_day;
	  DEBUGGING_OUTPUT ("REL_SPEC 7\n");
	}
	| t_FROM t_NEXT t_WEEKDAY {
	  int days;

	  if ($3 == input_date.tm_wday)
	    days = 7;
	  else if ($3 < input_date.tm_wday)
	    days = 7 - (input_date.tm_wday - $3);
	  else
	    days = $3 - input_date.tm_wday;

	  relative_seconds += days * seconds_per_day;
	  DEBUGGING_OUTPUT ("REL_SPEC 8\n");
	}
	| t_RELATIVE {
	  DEBUGGING_OUTPUT ("REL_SPEC 9\n");
	  relative_seconds += $1;
	}
	| t_LAST t_RELATIVE {
	  DEBUGGING_OUTPUT ("REL_SPEC 10\n");
	  relative_seconds -= $2;
	}
	| t_FROM t_RELATIVE {
	  DEBUGGING_OUTPUT ("REL_SPEC 11\n");
	  relative_seconds += $2;
	}
	| t_BEFORE t_RELATIVE {
	  DEBUGGING_OUTPUT ("REL_SPEC 12\n");
	  relative_seconds += $2;
	}
	| t_AFTER t_RELATIVE {
	  DEBUGGING_OUTPUT ("REL_SPEC 13\n");
	  relative_seconds += $2;
	}
	;
%%

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
