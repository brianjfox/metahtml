/* stringfuncs.c: -*- C -*-  String manipulation functions for Meta-HTML. */

/*  Copyright (c) 1997 Brian J. Fox
    Author: Brian J. Fox (bfox@ai.mit.edu) Sat Jul 19 14:44:32 1997.  */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
    deployment of Internet and Intranet applications via the use
    of the Meta-HTML language.

    Copyright (c) 1995, 2001, Brian J. Fox (bfox@ai.mit.edu).

    Meta-HTML is free software; you can redistribute it and/or modify
    it under the terms of the General Public License as published
    by the Free Software Foundation; either version 1, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    FSF GPL for more details.

    You should have received a copy of the FSF General Public License
    along with this program; if you have not, you may obtain one
    electronically at the following URL:

	 http://www.metahtml.com/COPYING  */

#include "language.h"

/************************************************************/
/*							    */
/*		   String Manipulation Functions	    */
/*							    */
/************************************************************/

#if defined (__cplusplus)
extern "C"
{
#endif

static void pf_string_length (PFunArgs);
static void pf_match (PFunArgs);
static void pf_string_compare (PFunArgs);
static void pf_substring (PFunArgs);
static void pf_subst_in_var (PFunArgs);
static void pf_subst_in_string (PFunArgs);
static void pf_upcase (PFunArgs);
static void pf_downcase (PFunArgs);
static void pf_capitalize (PFunArgs);
static void pf_word_wrap (PFunArgs);
static void pf_pad (PFunArgs);
static void pf_string_eq (PFunArgs);
static void pf_string_neq (PFunArgs);
static void pf_plain_text (PFunArgs);
static void pf_base64decode (PFunArgs);
static void pf_base64encode (PFunArgs);
static void pf_char_offsets (PFunArgs);
static void pf_left_trim (PFunArgs);
static void pf_right_trim (PFunArgs);
static void pf_collapse (PFunArgs);

#if defined (HAVE_CRYPT)
#  if defined (HAVE_CRYPT_H)
#    include <crypt.h>
#  endif
  static void pf_unix_crypt (PFunArgs);
#endif

  /* Random string operations. */
static PFunDesc func_table[] =
{
  { "STRING-LENGTH",		0, 0, pf_string_length },
  { "MATCH",			0, 0, pf_match },
  { "STRING-COMPARE",		0, 0, pf_string_compare },
  { "SUBSTRING",		0, 0, pf_substring },
  { "SUBST-IN-STRING",		0, 0, pf_subst_in_string },
  { "SUBST-IN-VAR",		0, 0, pf_subst_in_var },
  { "UPCASE",			0, 0, pf_upcase },
  { "DOWNCASE",			0, 0, pf_downcase },
  { "CAPITALIZE",		0, 0, pf_capitalize },
  { "WORD-WRAP",		0, 0, pf_word_wrap },
  { "PAD",			0, 0, pf_pad },
  { "STRING-EQ",		0, 0, pf_string_eq },
  { "STRING-NEQ",		0, 0, pf_string_neq },
  { "PLAIN-TEXT",		1, 0, pf_plain_text },
  { "BASE64DECODE",		0, 0, pf_base64decode },
  { "BASE64ENCODE",		0, 0, pf_base64encode },
  { "CHAR-OFFSETS",		0, 0, pf_char_offsets },
  { "STRINGS::LEFT-TRIM",	0, 0, pf_left_trim },
  { "STRINGS::RIGHT-TRIM",	0, 0, pf_right_trim },
  { "STRINGS::COLLAPSE",	0, 0, pf_collapse },

#if defined (HAVE_CRYPT)
  { "UNIX::CRYPT",	0, 0, pf_unix_crypt },
#endif

  { (char *)NULL,	0, 0, (PFunHandler *)NULL }
};

PACKAGE_INITIALIZER (initialize_string_functions)
DEFINE_SECTION (STRING-OPERATORS, strings; characters; changing case,
"There is a single function in <meta-html> which performs pattern\n\
matching, substring extraction, and substring deletion.  For\n\
convenience, a blind substring extraction function is supplied as\n\
well.  Three functions perform the three most common case changes.\n\
Finally, the <funref \"string operators\" pad> function allows\n\
alignment of fixed-width text.  ", "")

DEFUN (pf_string_length, string,
"Returns the number of characters present in <var string>.\n\
\n\
<complete-example>\n\
<string-length \"This is an interesting string\">\n\
</complete-example>")
{
  char *string = mhtml_evaluate_string (get_positional_arg (vars, 0));
  int length = 0;

  if (string != (char *)NULL)
    length = strlen (string);

  xfree (string);

  bprintf_insert (page, start, "%d", length);
}

#define MAX_SUBEXPS 10
DEFUN (pf_match, string regex
       &key action=[delete|extract|report|startpos|endpos|length]
       caseless=true,
"Matches <var regexp> against <var string>, and then performs the\n\
indicated <var action>.  The default for <var action> is \"report\".\n\
\n\
When action is \"report\" (the default), returns \"true\" if <var\n\
regex> matched.<br>\n\
When action is \"extract\", returns the substring of <var string>\n\
matching <var regex>.<br>\n\
When action is \"delete\", returns <var string> with the matched\n\
substring removed.<br>\n\
When action is \"startpos\", returns the numeric offset of the start of\n\
the matched substring.<br>\n\
When action is \"endpos\", returns the numeric offset of the end of the\n\
matched substring.\n\
\n\
<var regexp> is an extended Unix regular expression, the complete syntax of\n\
which is beyond the scope of this document.  However, the essential\n\
basics are:\n\
<ul>\n\
<li> A period (<code>.</code>) matches any one character.\n\
<li> An asterisk (<code>*</code>) matches any number of occurrences of\n\
the preceding expression, including none.\n\
<li> A plus-sign matches one or more occurrences of the preceding expression.\n\
<li> Square brackets are used to enclose lists of characters which may\n\
match.  For example, \"[a-zA-Z]+\" matches one or more occurrences of\n\
alphabetic characters.\n\
<li> The vertical bar is used to separate alternate expressions to\n\
match against.  For example, \"foo|bar\" says to match either \"foo\"\n\
<i>or</i> \"bar\".\n\
<li> A dollar-sign (<code>$</code>) matches the end of <var STRING>.\n\
<li> Parenthesis are used to group subexpressions.\n\
</ul>\n\
\n\
Here are a few examples:\n\
\n\
<example>\n\
  <match \"foobar\" \".*\">                 --> \"true\"\n\
  <match \"foobar\" \"foo\">                --> \"true\"\n\
  <match \"foobar\" \"foo\" action=extract> --> \"foo\"\n\
  <match \"foobar\" \"oob\" action=delete>  --> \"far\"\n\
  <match \"foobar\" \"oob\" action=startpos>--> \"1\"\n\
  <match \"foobar\" \"oob\" action=endpos>  --> \"4\"\n\
  <match \"foobar\" \"oob\" action=length>  --> \"3\"\n\
  <match \"foobar\" \"[0-9]+\">             --> \"\"\n\
</example>")
{
  char *_string = get_positional_arg (vars, 0);
  char *_regex = get_positional_arg (vars, 1);
  char *result = (char *)NULL;

  if (_string && _regex)
    {
      char *string = mhtml_evaluate_string (_string);
      char *regex = mhtml_evaluate_string (_regex);
      int caseless = var_present_p (vars, "caseless");
      char *action = (char *)NULL;

      if ((string != (char *)NULL) && (regex != (char *)NULL))
	{
	  /* Only up to MAX_SUBEXPS subexpressions kept. */
	  regex_t re;
	  regmatch_t offsets[MAX_SUBEXPS];
	  int slen = strlen (string);
	  int matched;
	  int so = 0, eo = 0, len = 0;
	  char *temp = mhtml_evaluate_string (get_value (vars, "action"));
	  char *packname = mhtml_evaluate_string (get_value (vars, "package"));

	  if (!empty_string_p (temp))
	    action = temp;
	  else
	    {
	      xfree (temp);
	      action = strdup ("report");
	    }

	  regcomp (&re, regex, REG_EXTENDED | (caseless ? REG_ICASE : 0));

	  matched = (regexec (&re, string, MAX_SUBEXPS, offsets, 0) == 0);

	  if (matched)
	    {
	      so = offsets[0].rm_so;
	      eo = offsets[0].rm_eo;
	      len = eo - so;
	    }

	  /* If the caller has specified a package to receive the detailed
	     results of the match, put the information there now. */
	  if (matched && packname)
	    {
	      register int i, limit;
	      Package *p = symbol_get_package (packname);
	      Symbol *starts, *ends, *lengths;
	      Symbol *matches = (Symbol *)NULL;
	      char digitbuff[40];

	      forms_set_tag_value_in_package (p, "expr", regex);
	      starts = symbol_intern_in_package (p, "start");
	      ends = symbol_intern_in_package (p, "end");
	      lengths = symbol_intern_in_package (p, "length");
	      if (strcasecmp (action, "extract") == 0)
		matches = symbol_intern_in_package (p, "matches");

	      for (limit = MAX_SUBEXPS; limit; limit--)
		if (offsets[limit - 1].rm_so != -1)
		  break;

	      sprintf (digitbuff, "%d", limit - 1);
	      forms_set_tag_value_in_package (p, "matched", digitbuff);

	      for (i = 0; i < limit; i++)
		{
		  int sublen = offsets[i].rm_eo - offsets[i].rm_so;

		  sprintf (digitbuff, "%d", offsets[i].rm_so);
		  symbol_add_value (starts, digitbuff);
		  sprintf (digitbuff, "%d", offsets[i].rm_eo);
		  symbol_add_value (ends, digitbuff);
		  sprintf (digitbuff, "%d", sublen);
		  symbol_add_value (lengths, digitbuff);

		  if (matches != (Symbol *)NULL)
		    {
		      char *substring = (char *)xmalloc (1 + sublen);
		      strncpy (substring, string + offsets[i].rm_so, sublen);
		      substring[sublen] = '\0';
		      symbol_add_value (matches, substring);
		      free (substring);
		    }
		}
	    }

	  if (packname != (char *)NULL) free (packname);
	      
	  if (matched && strcasecmp (action, "report") == 0)
	    {
	      result = strdup ("true");
	    }
	  else if (matched && (strcasecmp (action, "extract") == 0))
	    {
	      result = (char *)xmalloc (1 + len);
	      strncpy (result, string + so, len);
	      result[len] = '\0';
	    }
	  else if (strcasecmp (action, "delete") == 0)
	    {
	      result = strdup (string);
	      if (matched)
		memmove (result + so, result + eo, (slen + 1) - eo);
	    }
	  else if ((strcasecmp (action, "startpos") == 0) ||
		   (strcasecmp (action, "endpos") == 0) ||
		   (strcasecmp (action, "length") == 0))
	    {
	      result = (char *)xmalloc (20);
	      result[0]= '\0';

	      if (matched)
		{
		  if (strcasecmp (action, "startpos") == 0)
		    sprintf (result, "%d", so);
		  else if (strcasecmp (action, "endpos") == 0)
		    sprintf (result, "%d", eo);
		  else
		    sprintf (result, "%d", len);
		}
	    }
	  regfree (&re);
	}

      xfree (string);
      xfree (regex);
      xfree (action);
    }

  if (result)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
      free (result);
    }
}

DEFUN (pf_substring, string &optional start end,
"Extracts the substring of <var string> whose first character starts\n\
at offset <var start>, and whose last character ends at offset\n\
<var end>. The indexing is zero-based, so that:\n\
\n\
<example>\n\
  <substring \"Hello\" 1 2> --> \"e\"\n\
</example>\n\
\n\
This function is useful when you know in advance which part of the\n\
string you would like to extract, and do not need the pattern matching\n\
facilities of <funref string-operators match>.\n\
\n\
If you wish to index through each character of a string, the most direct\n\
way is to convert it to an array first using\n\
<funref string-operators string-to-array>, and then use the\n\
<funref array-operators foreach> function to iterate over the members.\n\
<complete-example>\n\
<set-var s=\"This is a string.\">\n\
<string-to-array <get-var-once s> chars>\n\
<foreach character chars><get-var character>-</foreach>\n\
</complete-example>")
{
  char *str_arg = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *beg_arg = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *end_arg = mhtml_evaluate_string (get_positional_arg (vars, 2));

  if (str_arg != (char *)NULL)
    {
      register int i;
      char *temp;
      int len = strlen (str_arg);
      int beg_index = 0;
      int end_index = len;

      /* If not all digits, lookup arg as variable name. */
      if (!empty_string_p (beg_arg))
	{
	  if (!number_p (beg_arg))
	    {
	      for (i = 0; whitespace (beg_arg[i]); i++);
	      temp = pagefunc_get_variable (beg_arg + i);
	      if (temp != (char *)NULL)
		beg_index = atoi (temp);
	    }
	  else
	    beg_index = atoi (beg_arg);
	}

      if (!empty_string_p (end_arg))
	{
	  if (!number_p (end_arg))
	    {
	      for (i = 0; whitespace (end_arg[i]); i++);
	      temp = pagefunc_get_variable (end_arg + i);
	      if (temp != (char *)NULL)
		end_index = atoi (temp);
	    }
	  else
	    end_index = atoi (end_arg);
	}

      if (beg_index > end_index)
	{ i = beg_index; beg_index = end_index; end_index = i; }

      if (end_index > len) end_index = len;

      if ((beg_index != end_index) && (beg_index < len))
	{
	  if ((end_index - beg_index) < 100)
	    {
	      char buffer[100];

	      strncpy (buffer, str_arg + beg_index, end_index - beg_index);
	      buffer[end_index - beg_index] = '\0';
	      bprintf_insert (page, start, "%s", buffer);
	      *newstart += (end_index - beg_index);
	    }
	  else
	    {
	      temp = (char *)xmalloc (1 + (end_index - beg_index));
	      strncpy (temp, str_arg + beg_index, end_index - beg_index);
	      temp[end_index - beg_index] = '\0';
	      bprintf_insert (page, start, "%s", temp);
	      *newstart += (end_index - beg_index);
	      free (temp);
	    }
	}
    }

  if (str_arg) free (str_arg);
  if (beg_arg) free (beg_arg);
  if (end_arg) free (end_arg);
}

static char *
subst_in_string_internal (char *contents, Package *vars, int debug_level)
{
  char *result = (char *)NULL;

  if (contents != (char *)NULL)
    {
      int done = 0;
      int arg = 1;
      PAGE *temp = page_create_page ();
      page_set_contents (temp, contents);

      while (!done)
	{
	  char *this_string = get_positional_arg (vars, arg++);
	  char *with_that = get_positional_arg (vars, arg++);

	  if (this_string == (char *)NULL)
	    done = 1;
	  else
	    {
	      this_string = mhtml_evaluate_string (this_string);
	      with_that = mhtml_evaluate_string (with_that);

	      if (debug_level > 5)
		page_debug
		  ("<subst-in-xxx \"%s\" \"%s\" \"%s\">",
		   contents, this_string, with_that ? with_that : "");

	      if (this_string != (char *)NULL)
		page_subst_in_page (temp, this_string, with_that);

	      if (debug_level > 5)
		page_debug ("--> `%s'", temp->buffer ? temp->buffer : "");

	      xfree (this_string);
	      xfree (with_that);
	    }
	}

      result = temp->buffer;
      free (temp);
    }

  return (result);
}

DEFUN (pf_subst_in_var, varname &optional this-string with-that,
"Replaces all occurrences of <var this-string> with <var with-that> in the\n\
contents of the variable named <var varname>.  Both <var this-string> and\n\
<var with-that> are evaluated before the replacement is done. <var\n\
this-string> can be any regular expression allowed by the POSIX extended\n\
regular expression matching.  This command can be useful when parsing\n\
the output of <funref osfuncs cgi-exec>.")
{
  char *varname = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (!empty_string_p (varname))
    {
      char *contents = pagefunc_get_variable (varname);

      if (contents != (char *)NULL)
	{
	  char *result = subst_in_string_internal
	    (contents, vars, debug_level);

	  pagefunc_set_variable (varname, result);
	  xfree (result);
	}
    }
  xfree (varname);
}

DEFUN (pf_subst_in_string, string &rest regexp replacement,
"Replaces all occurrences of <var regexp> with <var replacement> in\n\
<var string>.\n\
\n\
<var regexp> can be any regular expression allowed by POSIX extended\n\
regular expression matching.\n\
\n\
In the replacement string, a backslash followed by a number <var n> is\n\
replaced with the contents of the <var n>th subexpression from <var\n\
regexp>.\n\
\n\
<example>\n\
<set-var foo=\"This is a list\">\n\
<subst-in-string <get-var foo> \"is\" \"HELLO\">\n\
     --> \"ThHELLO HELLO a lHELLOt\"\n\
.blank\n\
<subst-in-string \"abc\" \"([a-z])\" \"\\\\1 \"> --> \"a b c \"\n\
</example>")
{
  char *contents = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (contents != (char *)NULL)
    {
      char *result = subst_in_string_internal (contents, vars, debug_level);

      free (contents);

      if (result)
	{
	  bprintf_insert (page, start, "%s", result);
	  *newstart += strlen (result);
	  free (result);
	}
    }
}

DEFUN (pf_downcase, string,
"Converts all of the uppercase characters in <var string> to\n\
lowercase.\n\
\n\
<complete-example>\n\
<downcase \"This is Written in Meta-HTML\">\n\
</complete-example>")
{
  unsigned char *value = (unsigned char *)
    mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (value != (unsigned char *)NULL)
    {
      register int i;

      for (i = 0; value[i] != '\0'; i++)
	if (isupper (value[i]))
	  value[i] = tolower (value[i]);

      bprintf_insert (page, start, "%s", value);
      *newstart += i;
      free (value);
    }
}

DEFUN (pf_upcase, string,
"Converts all of the lowercase characters in <var string> to\n\
uppercase.\n\
\n\
<complete-example>\n\
<upcase \"This is Written in Meta-HTML\">\n\
</complete-example>")
{
  unsigned char *value = (unsigned char *)
    mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (value != (unsigned char *)NULL)
    {
      register int i;

      for (i = 0; value[i] != '\0'; i++)
	if (islower (value[i]))
	  value[i] = toupper (value[i]);

      bprintf_insert (page, start, "%s", value);
      *newstart += i;
      free (value);
    }
}

DEFUN (pf_capitalize, string,
"Changes the case of each character in <var string> to uppercase or\n\
lowercase depending on the surrounding characters.\n\
\n\
<complete-example>\n\
<capitalize \"This is a list\">\n\
</complete-example>\n\
\n\
Also see <funref string-operators downcase>, and\n\
<funref string-operators upcase>.")
{
  unsigned char *value = (unsigned char *)
    mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (value != (unsigned char *)NULL)
    {
      register int i;
      int capnext = 1;

      for (i = 0; value[i] != '\0'; i++)
	{
	  if (value[i] > 127)
	    continue;

	  if (!isalpha (value[i]))
	    capnext = 1;
	  else
	    {
	      if (capnext)
		{
		  if (islower (value[i]))
		    value[i] = toupper (value[i]);

		  capnext = 0;
		}
	      else
		{
		  if (isupper (value[i]))
		    value[i] = tolower (value[i]);
		}
	    }
	}

      bprintf_insert (page, start, "%s", value);
      *newstart += i;
      free (value);
    }
}

DEFUN (pf_string_compare, string1 string2 &key caseless,
"Compare the two strings <var string1> and <var string2>, and return\n\
a string which specifies the relationship between them.  The\n\
comparison is normall case-sensitive, unless the keyword argument <var\n\
caseless=true> is given.\n\
\n\
The possible return values are:\n\
<ol>\n\
<li> equal<br>\n\
The two strings are exactly alike.\n\
<li> greater<br>\n\
<var string1> is lexically greater than <var string2>.\n\
<li> less<br>\n\
<var string1> is lexically less than <var string2>.\n\
</ol>\n\
\n\
Examples:\n\
\n\
<example>\n\
<string-compare \"aaa\" \"aab\">               --> less\n\
<string-compare \"zzz\" \"aab\">               --> greater\n\
<string-compare \"zzz\" \"ZZZ\">               --> greater\n\
<string-compare \"zzz\" \"ZZZ\" caseless=true> --> equal\n\
</example>")
{
  char *string_1 = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *string_2 = mhtml_evaluate_string (get_positional_arg (vars, 1));
  int caseless_p = get_value (vars, "caseless") != (char *)NULL;
  char *result = (char *)NULL;

  /* Both strings empty? */
  if (string_1 == string_2)
    result = "equal";
  else if (string_1 == (char *)NULL)
    result = "less";
  else if (string_2 == (char *)NULL)
    result = "greater";
  else
    {
      int temp;

      if (caseless_p)
	temp = strcasecmp (string_1, string_2);
      else
	temp = strcmp (string_1, string_2);

      if (temp == 0)
	result = "equal";
      else if (temp > 0)
	result = "greater";
      else
	result = "less";
    }

  if (string_1 != (char *)NULL) free (string_1);
  if (string_2 != (char *)NULL) free (string_2);

  if (result)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart = start + strlen (result);
    }
}

DEFUN (pf_word_wrap,
       string &key width=charwidth indent=indentation skip-first=true,
"Produce paragraphs of text from the string <var string> with the text\n\
filled to a width of <var charwidth>.\n\
\n\
This is provided for convenience only, and is of use when you need to\n\
present some free form text in pre-formatted fashion, such as when\n\
sending an E-mail message, or the like.\n\
\n\
If the keyword <var indent=indentation> is supplied, it says to indent\n\
each line in the output by <var indentation>, using space characters.\n\
\n\
If the keyword <var skip-first=true> is given, it says not to indent\n\
the first line of the output -- just each successive line.  For\n\
example:\n\
<complete-example>\n\
<set-var text =\n\
   <concat \"This is provided for convenience only, and is of use \"\n\
           \"when you need to present some free form text in \"\n\
           \"pre-formatted fashion, as in this example.\">>\n\
<pre>\n\
Title Topic: <word-wrap <get-var text> 40 indent=13 skip-first=true>\n\
</pre>\n\
</complete-example>")
{
  char *width_spec = mhtml_evaluate_string (get_value (vars, "width"));
  char *indent_spec = mhtml_evaluate_string (get_value (vars, "indent"));
  int width = (width_spec != (char *)NULL) ? atoi (width_spec) : 60;
  int indent = (indent_spec != (char *)NULL) ? atoi (indent_spec) : 0;
  char *text = mhtml_evaluate_string (get_positional_arg (vars, 0));

  if (width == 0) width = 60;
  if (indent > width) indent = width;
  if (indent < 0) indent = 0;

  if (!empty_string_p (text))
    {
      BPRINTF_BUFFER *temp = bprintf_create_buffer ();

      bprintf (temp, "%s", text);
      bprintf_word_wrap (temp, width);

      if (indent)
	{
	  register int i;
	  char *indent_string = (char *)xmalloc (1 + indent);
	  char *skip_first = mhtml_evaluate_string
	    (get_value (vars, "skip-first"));
	  char *indent_char = mhtml_evaluate_string
	    (get_value (vars, "indentation-character"));
	  char indent_value = ' ';

	  if ((indent_char != (char *)NULL) && (indent_char[0] != '\0'))
	    indent_value = *indent_char;

	  for (i = 0; i < indent; i++) indent_string[i] = indent_value;
	  indent_string[i] = '\0';

	  /* Kill leading indentation on the first line. */
	  for (i = 0; (i < temp->bindex) && (whitespace (temp->buffer[i])); i++);
	  if (i) bprintf_delete_range (temp, 0, i);

	  /* Indent the first line. */
	  if (empty_string_p (skip_first))
	    bprintf_insert (temp, 0, "%s", indent_string);

	  /* Now do the rest. */
	  while (i < temp->bindex)
	    {
	      if (temp->buffer[i] == '\n')
		{
		  bprintf_insert (temp, i + 1, "%s", indent_string);
		  i += indent;
		}
	      i++;
	    }
	  xfree (skip_first);
	  xfree (indent_char);
	}
	  
      bprintf_insert (page, start, "%s", temp->buffer);
      *newstart += temp->bindex;
      bprintf_free_buffer (temp);
    }

  xfree (text);
  xfree (width_spec);
  xfree (indent_spec);
}


#define align_RIGHT  0
#define align_LEFT   1
#define align_MIDDLE 2

DEFUN (pf_pad,
       string width &key align=[left|right|middle] truncate=true pad-char=x,
"Pads <var string> to a length of <var total-size>.  <var align> can\n\
be one of <code>LEFT</code>, <code>MIDDLE</code>, or\n\
<code>RIGHT</code> (the default).\n\
\n\
<code>PAD</code> inserts the correct number of <var pad-char>acters to\n\
make the input argument take the desired number of spaces (presumably\n\
for use in a <example code><pre> ... </pre></example> statement).  The\n\
default value for <var pad-char> is a space character.\n\
\n\
If keyword argument <var truncate=true> is given, it  says to force\n\
the string to be the specified length.\n\
\n\
Before any padding is done, leading and trailing whitespace is removed\n\
from <var string>.\n\
\n\
Examples:\n\
\n\
<example>\n\
  <pad \"Hello\" 10>              --> \"     Hello\"\n\
  <pad \"Hello\" 10 align=left>   --> \"Hello     \"\n\
  <pad \"Hello\" 10 align=middle> --> \"  Hello   \"\n\
  <pad \"  Heckle  \" 4 truncate> --> \"Heck\"\n\
</example>")
{
  register int i;
  char *input = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *wtext = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *align = mhtml_evaluate_string (get_value (vars, "ALIGN"));
  int truncation = var_present_p (vars, "TRUNCATE");
  int width = wtext ? atoi (wtext) : 15;
  int alignment = align_RIGHT;
  int input_len = input ? strlen (input) : 0;
  char *pad_char = mhtml_evaluate_string (get_value (vars, "PAD-CHAR"));

  if (empty_string_p (pad_char))
    {
      xfree (pad_char);
      pad_char = strdup (" ");
    }

  if (align)
    {
       if (strcasecmp (align, "left") == 0)
	 alignment = align_LEFT;
       else if ((strcasecmp (align, "middle") == 0) ||
		(strcasecmp (align, "center") == 0))
	 alignment = align_MIDDLE;

       free (align);
     }

  if (wtext) free (wtext);

  if (!input)
    return;

  /* Strip leading and trailing whitespace from the input. */
  if (input_len)
    {
      for (i = 0; whitespace (input[i]); i++);
      if (i)
	memmove (input, input + i, (input_len - i) + 1);

      for (i = strlen (input) - 1; i > -1; i--)
	if (!whitespace (input[i]))
	  break;

      input[i + 1] = '\0';
      input_len = i + 1;
    }

  /* Handle truncation. */
  if (input_len > width)
    {
      if (truncation)
	input[width] = '\0';
    }
  else
    {
      int offset = 0;
      int left_pad = 0;
      int right_pad = 0;
      char *string = (char *)xmalloc (2 + width);

      /* Get the amount to pad on the left and right. */
      switch (alignment)
	{
	case align_LEFT:
	  right_pad = width - input_len;
	  break;

	case align_RIGHT:
	  left_pad = width - input_len;
	  break;

	case align_MIDDLE:
	  left_pad = (width - input_len) ? (width - input_len) / 2 : 0;
	  right_pad = width - (input_len + left_pad);
	  break;
	}

      /* Put the left-hand spaces in place. */
      for (offset = 0; offset < left_pad; offset++)
	string[offset] = *pad_char;

      /* Drop the input string down. */
      for (i = 0; (string[offset] = input[i]) != '\0'; i++, offset++);

      /* Put the right-hand spaces in place. */
      for (i = 0; i < right_pad; i++)
	string[offset++] = *pad_char;

      /* Terminate the string. */
      string[offset] = '\0';

      free (input);
      input = string;
    }

  if (input)
    {
      bprintf_insert (page, start, "%s", input);
      *newstart += strlen (input);
      free (input);
    }
}

DEFUN (pf_string_eq, string-1 string-2 &key caseless=true,
 "Compare <var string1> to <var string2> and return the string\n\
<code>\"true\"</code> if they are character-wise identical.\n\
\n\
The optional keyword argument <var caseless=true> indicates that no\n\
consideration should be given to the case of the characters during\n\
comparison.\n\
\n\
<example>\n\
<string-eq \"foo\" \"FOO\">               -->\n\
<string-eq \"foo\" \"foo\">               -->true\n\
<string-eq <upcase \"foo\"> \"FOO\">      -->true\n\
<string-eq \"foo\" \"FOO\" caseless=true> -->true\n\
</example>")
{
  char *arg1 = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *arg2 = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *caseless = mhtml_evaluate_string (get_value (vars, "caseless"));
  int caseless_p = 0;

  if (debug_level > 10)
    {
      page_debug ("[ arg1 = '%s', \n  arg2 = '%s' ]",
		  arg1 ? arg1 : "",
		  arg2 ? arg2 : "");
    }
		  
  if (!empty_string_p (caseless))
    caseless_p++;

  xfree (caseless);

  if (((empty_string_p (arg1)) && (empty_string_p (arg2))) ||
      ((arg1 && arg2) &&
       (((!caseless_p) && (strcmp (arg1, arg2) == 0)) ||
	((caseless_p) && (strcasecmp (arg1, arg2) == 0)))))
    {
      bprintf_insert (page, start, "true");
      *newstart = start + 4;
    }

  xfree (arg1);
  xfree (arg2);
}

DEFUN (pf_string_neq, string-1 string-2 &key caseless=true,
 "Compare <var string1> to <var string2> and return the string\n\
<code>\"true\"</code> if they are NOT character-wise identical.\n\
\n\
The optional keyword argument <var caseless=true> indicates that no\n\
consideration should be given to the case of the characters during\n\
comparison.\n\
\n\
<example>\n\
<string-neq \"foo\" \"FOO\">               -->true\n\
<string-neq \"foo\" \"foo\">               -->\n\
<string-neq <upcase \"foo\"> \"FOO\">      -->\n\
<string-neq \"foo\" \"FOO\" caseless=true> -->\n\
</example>")
{
  char *arg1 = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *arg2 = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *caseless = mhtml_evaluate_string (get_value (vars, "caseless"));
  int caseless_p = 0;

  if (!empty_string_p (caseless))
    caseless_p++;

  xfree (caseless);

  if ((empty_string_p (arg1) && !empty_string_p (arg2)) ||
      (empty_string_p (arg2) && !empty_string_p (arg1)) ||
      ((arg1 && arg2) &&
       (((!caseless_p) && (strcmp (arg1, arg2) != 0)) ||
	((caseless_p) && (strcasecmp (arg1, arg2) != 0)))))
    {
      bprintf_insert (page, start, "true");
      *newstart = start + 4;
    }

  xfree (arg1);
  xfree (arg2);
}

/* Does modifications to the plain text in BODY.  Usually, this simply
   inserts paragraph breaks where they appear, and optionally operates
   on the first character of paragraphs.  The text starts with a <P>,
   unless the variable NOBR is set.*/
DEFMACRO (pf_plain_text, &key first-char=expr nobr=true,
"Performs the following steps:\n\
\n\
<ol>\n\
  <li> Replace occurrences of pairs of newline characters with a\n\
  single <example code><P></example> tag.\n\
\n\
  <li> Applies the function <var expr> to the first character of every\n\
  paragraph, and inserts the closing tag after that character.\n\
</ol>\n\
\n\
The output will start with a <example code><P></example> tag, unless the\n\
optional argument <var nobr=true> is given.\n\
\n\
<complete-example>\n\
<plain-text first-char=<font size=\"+1\"> nobr=true>\n\
This is line 1.\n\
.blank\n\
This is line 2.\n\
</plain-text>\n\
</complete-example>")
{
  register int i;
  char *first_char = mhtml_evaluate_string (get_value (vars, "FIRST-CHAR"));
  char *nobr = mhtml_evaluate_string (get_value (vars, "NOBR"));
  char *nolower = mhtml_evaluate_string (get_value (vars, "NOLOWER"));
  BPRINTF_BUFFER *evalbody = bprintf_create_buffer ();
  char *pval = mhtml_evaluate_string ("\n<p>\n");
  int pval_len = strlen (pval);

  evalbody->buffer = mhtml_evaluate_string (body->buffer);
  evalbody->bsize = evalbody->buffer ? strlen (evalbody->buffer) : 0;
  evalbody->bindex = evalbody->bsize;

  /* Insert one blank line in the front of BODY. */
  bprintf_insert (evalbody, 0, "%s", pval);

  /* Modify blank lines in BODY such that they contain <p> instead. */
  page_subst_in_page (evalbody, "\n[ \t]*\n", pval);

  /* Modify the first character of every paragraph by inserting the
     open tag before it, and inserting a matching close tag after it. */
  if (first_char)
    {
      register int begin;
      char *closer = (char *)NULL;
      int o_len = strlen (first_char);
      int c_len = 0;
      char *buffer = (char *)NULL;

      if (*first_char == '<')
	{
	  register int c;

	  for (i = 1; whitespace (first_char[i]); i++);

	  begin = i;

	  for (i = begin; (c = first_char[i]) != '\0'; i++)
	    if ((c == '>') || (whitespace (c)))
	      break;

	  closer = (char *)xmalloc (4 + (i - begin));
	  closer[0] = '<';
	  closer[1] = '/';
	  strncpy (closer + 2, first_char + begin, i - begin);
	  closer[(i - begin) + 2] = '>';
	  closer[(i - begin) + 3] = '\0';
	  c_len = strlen (closer);
	}

      buffer = (char *)xmalloc (3 + o_len + c_len);
      strcpy (buffer, first_char);
      if (c_len)
	{
	  strcpy (buffer + o_len + 1, closer);
	  free (closer);
	}
      else
	buffer[o_len + 1] = '\0';
      
      /* Now quickly find occurences of "<p>" in EVALBODY. */
      begin = 0;

      while ((begin = page_search (evalbody, pval, begin)) != -1)
	{
	  begin += pval_len;

	  while ((begin < evalbody->bindex) &&
		 (whitespace (evalbody->buffer[begin])))
	    begin++;

	  if ((begin < evalbody->bindex) &&
	      (isalnum (evalbody->buffer[begin])) &&
	      ((empty_string_p (nolower)) ||
	       (isupper (evalbody->buffer[begin]))))
	    {
	      char *temp;

	      buffer[o_len] = evalbody->buffer[begin];
	      temp = mhtml_evaluate_string (buffer);
	      bprintf_delete_range (evalbody, begin, begin + 1);
	      bprintf_insert (evalbody, begin, "%s", temp);
	      begin += strlen (temp);
	      free (temp);
	    }
	}
      free (buffer);
    }

  /* Insert the modified evalbody. */
  {
    int length = evalbody->bindex;
    int offset = 0;

    if (nobr)
      {
	offset = pval_len;
	length -= pval_len;
      }
    bprintf_insert (page, start, "%s", evalbody->buffer + offset);
    *newstart += length;
  }

  xfree (nobr);
  xfree (nolower);
  xfree (first_char);
  xfree (pval);
  bprintf_free_buffer (evalbody);
}

DEFUN (pf_base64encode, varname &key shortlines=[true|value],
"Performs the translation operation commonly known as <i>Base64\n\
Encoding</i> on the contents of the binary variable referenced by <var\n\
varname>, and returns the results of that encoding.\n\
\n\
Base64 encoding is a common transfer encoding for binary data and for\n\
Basic Authorization values -- this function can be used to turn data\n\
from its original pre-encoded state to an encoded one.\n\
\n\
If the keyword argument <var shortlines=value> is supplied, then the result\n\
of encoding is broken up into lines containing <var value> characters\n\
each, rounded up to the nearest multiple of 4.  This format is\n\
commonly implemented by many base 64 encoding programs.  A value of\n\
<code>true</code> uses the Meta-HTML default value of 64.  If the\n\
argument is not supplied, the data is returned in a single line of data.\n\
\n\
The following code reads a GIF image into a binary variable, and then\n\
displays the base64 encoded version:\n\
<example>\n\
<dir::read-file /tmp/image.gif gif>	==> true\n\
<base64encode gif>			==> [base64 encoded data]\n\
</example>")
{
  char *varname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *result = (char *)NULL;
  char *sl_argstring = mhtml_evaluate_string
    (get_one_of (vars, "shortlines", "short-lines", (char *)NULL));
  int shortlines = 0;

  if (!empty_string_p (sl_argstring))
    {
      shortlines = atoi (sl_argstring);
      if (!shortlines) shortlines = 64;

      shortlines = ((shortlines + 3) / 4) * 4;
    }

  xfree (sl_argstring);

  if (!empty_string_p (varname))
    {
      Symbol *sym = symbol_lookup (varname);

      if (sym != (Symbol *)NULL)
	{ 
	  switch (sym->type)
	    {
	    case symtype_BINARY:
	      {
		Datablock *block = (Datablock *)(sym->values);
		result = mhtml_base64encode (block->data,
					     block->length, shortlines);
	      }
	    break;

	    case symtype_STRING:
	      {
		char *string = pagefunc_get_variable (varname);

		if (string != (char *)NULL)
		  {
		    int length = strlen (string);
		    result = mhtml_base64encode (string, length, shortlines);
		  }
	      }
	    break;
	    }
	}
    }

  xfree (varname);

  if (result)
    {
      bprintf_insert (page, start, "%s", result);
      *newstart += strlen (result);
      free (result);
    }
}

DEFUN (pf_base64decode, string &optional varname,
"Performs the translation operation commonly known as <i>Base64\n\
Decoding</i> on <var STRING>, and returns the results of that\n\
decoding.\n\
\n\
If the optional <var varname> is supplied, the results of decoding are\n\
placed into the binary variable named <var varname> instead of being\n\
returned in the page.  This allows random binary data to be decoded,\n\
and perhaps written to the output stream using <funref\n\
stream-operators stream-put-contents>.\n\
\n\
Base64 encoding is a common transfer encoding for binary data and for\n\
Basic Authorization values -- this function can be used to turn such\n\
strings into their original, pre-encoded state.\n\
\n\
<complete-example>\n\
<set-var the-data = \"YmZveDpmcm9ibml0eg==\">\n\
<base64decode <get-var the-data>>\n\
</complete-example>")
{
  char *string = mhtml_evaluate_string (get_positional_arg (vars, 0));
  
  if (!empty_string_p (string))
    {
      int length = 0;
      char *result = mhtml_base64decode (string, &length);
      char *varname = mhtml_evaluate_string (get_positional_arg (vars, 1));

      if (empty_string_p (varname))
	{
	  /* Manually insert the data instead of letting bprintf
	     do it for us.  This is because the data could contain
	     null characters, and then the buffer wouldn't necessarily
	     reflect the length of what was inserted. */
	  if ((length + page->bindex) >= page->bsize)
	    page->buffer = (char *)xrealloc
	      (page->buffer, (page->bsize += (length + 100)));

	  memmove (page->buffer + start + length, page->buffer + start,
		   (page->bindex + 1) - start);

	  memcpy (page->buffer + start, result, length);
	  page->bindex += length;
	  *newstart += length;
	}
      else
	{
	  Symbol *sym = symbol_remove (varname);
	  Datablock *block;

	  symbol_free (sym);

	  sym = symbol_intern (varname);
	  block = datablock_create (result, length);
	  sym->type = symtype_BINARY;
	  sym->values = (char **)block;
	}

      xfree (varname);
      xfree (result);
    }

  xfree (string);
}

DEFUN (pf_char_offsets, string ch &key caseless,
"Return an array of numbers,  each one representing the offset from the\n\
start of <var string> of <var ch>.  This function is useful for finding\n\
candidate locations for word-wrapping, for example.\n\
Here is a complete example:\n\
<complete-example>\n\
<char-offsets \"This is a list\" \" \">\n\
</complete-example>")
{
  char *string = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *ch = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char *caseless_arg = mhtml_evaluate_string (get_value (vars, "CASELESS"));
  int caseless_p = !empty_string_p (caseless_arg);

  if ((string != (char *)NULL) && (ch != (char *)NULL))
    {
      register int i;

      for (i = 0; string[i] != '\0'; i++)
	{
	  if ((string[i] == *ch) ||
	      (caseless_p && (tolower (string[i]) == tolower (*ch))))
	    {
	      static char digits[40];
	      sprintf (digits, "%d", i);
	      bprintf_insert (page, start, "%s\n", digits);
	      start += 1 + strlen (digits);
	    }
	}
    }

  *newstart = start;
  xfree (string);
  xfree (ch);
  xfree (caseless_arg);
}

void
mhtml_left_trim (char *str, char *trimchars)
{
  if ((str != (char *)NULL) && (trimchars != (char *)NULL))
    {
      register int i;

      for (i = 0; ((str[i] != '\0') &&
		   (strchr (trimchars, str[i]) != (char *)NULL)); i++);

      if (i != 0)
	{
	  int len = strlen (str + i);
	  memmove (str, str + i, len + 1);
	}
    }
}

void
mhtml_right_trim (char *str, char *trimchars)
{
  if ((str != (char *)NULL) && (trimchars != (char *)NULL))
    {
      register int i = strlen (str);

      while (i > 0)
	{
	  if (strchr (trimchars, str[i - 1]) == (char *)NULL)
	    break;
	  else
	    i--;
	}

      str[i] = '\0';
    }
}

void
mhtml_collapse_string (char *str, char *collapse_chars)
{
  if ((str != (char *)NULL) && (collapse_chars != (char *)NULL))
    {
      register int i, j;
      char *newstr = (char *)xmalloc (1 + strlen (str));

      newstr[0] = '\0';
      i = 0;
      j = 0;

      while (str[i] != '\0')
	{
	  char c = str[i];

	  if (strchr (collapse_chars, str[i]) != (char *)NULL)
	    {
	      c = collapse_chars[0];
	      i++;
	      while ((str[i] != '\0') &&
		     (strchr (collapse_chars, str[i]) != (char *)NULL)) i++;
	    }
	  else
	    i++;

	  newstr[j++] = c;
	}

      newstr[j] = '\0';

      strcpy (str, newstr);
    }
}
      
#define MHTML_TRIM_LEFT  1
#define MHTML_TRIM_RIGHT 2
#define MHTML_COLLAPSE   3

static void
pf_trim_generic (PFunArgs, int which)
{
  char *varname = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *trimchars = mhtml_evaluate_string (get_positional_arg (vars, 1));
  static char *whitespace_chars = " \t\r\n";

  if (trimchars == (char *)NULL)
    trimchars = whitespace_chars;

  if (!empty_string_p (varname))
    {
      Symbol *sym = symbol_lookup (varname);

      if (sym != (Symbol *)NULL)
	{
	  switch (sym->type)
	    {
	      case symtype_STRING:
		{
		  if (sym->values_index > 0)
		    {
		      switch (which)
			{
			case MHTML_TRIM_LEFT:
			  mhtml_left_trim (sym->values[0], trimchars);
			  break;

			case MHTML_TRIM_RIGHT:
			  mhtml_right_trim (sym->values[0], trimchars);
			  break;

			case MHTML_COLLAPSE:
			  mhtml_collapse_string (sym->values[0], trimchars);
			  break;
			}
		    }
		}
		break;

	    default:
	      break;
	    }
	}
    }

  if ((trimchars != (char *)NULL) && (trimchars != whitespace_chars))
    free (trimchars);
  xfree (varname);
}

DEFUNX (strings::left-trim, varname &optional trim-chars,
"Trims the characters specified in <var trim-chars> from the\n\
\"left-hand\" side of the string stored in <var varname>, replacing\n\
the contents of that variable with the trimmed string.  If <var trim-chars>\n\
is not specified, it defaults to the set of whitespace characters, i.e.,\n\
Space, Tab, CR, and Newline.\n\
<complete-example>\n\
<set-var foo=\"    string with whitespace on the left\">\n\
String: [<get-var-once foo>]\n\
<strings::left-trim foo>\n\
String: [<get-var-once foo>]\n\
</complete-example>")

static void
pf_left_trim (PFunArgs)
{
  pf_trim_generic (PassPFunArgs, MHTML_TRIM_LEFT);
}

DEFUNX (strings::right-trim, varname &optional trim-chars,
"Trims the characters specified in <var trim-chars> from the\n\
\"right-hand\" side of the string stored in <var varname>, replacing\n\
the contents of that variable with the trimmed string.  If <var trim-chars>\n\
is not specified, it defaults to the set of whitespace characters, i.e.,\n\
Space, Tab, CR, and Newline.\n\
<complete-example>\n\
<set-var foo=\"string with whitespace on the right     \">\n\
String: [<get-var-once foo>]\n\
<strings::right-trim foo>\n\
String: [<get-var-once foo>]\n\
</complete-example>")

static void
pf_right_trim (PFunArgs)
{
  pf_trim_generic (PassPFunArgs, MHTML_TRIM_RIGHT);
}

DEFUNX (strings::collapse, varname &optional collapsible-chars,\n\
"Collapses multiple occurrences of any of the characters specified in\n\
<var collapsible-chars> into a single occurrence of the first character\n\
in <var collapsible-chars>, throughout the string stored in <var varname>.\n\
If <var collapsible-chars> is not specified, it defaults to the set of\n\
whitespace characters, with a Space character as the first element, i.e.,\n\
Space, Tab, CR, and Newline.\n\
<complete-example>\n\
<set-var foo=\" string with    whitespace     in   various Spots \">\n\
String: [<get-var-once foo>]\n\
<strings::collapse foo>\n\
String: [<get-var-once foo>]\n\
</complete-example>")
static void
pf_collapse (PFunArgs)
{
  pf_trim_generic (PassPFunArgs, MHTML_COLLAPSE);
}

#if defined (HAVE_CRYPT)
/* Create a password from cleartext. */
static char *
create_password (char *clear, char *salt)
{
  int length = (13 * ((strlen (clear) + 7) / 8));
  char *encrypted = (char *)xmalloc (1 + length);
  char *clear_p = clear;

  encrypted[0] = '\0';

  while (length > 0)
    {
      char chunk[9];
      char *temp;

      strncpy (chunk, clear_p, 8);
      chunk[8] = (char)0;

      temp = crypt (chunk, salt);
      strcat (encrypted, temp);

      clear_p += 8;
      length -= 13;
    }

  return (encrypted);
}

DEFUNX (pf_unix::crypt, string &optional salt,
"Return <var string> encrypted using the local system's <code>crypt()</code>\n\
function with the salt <var salt>.\n\
\n\
<var salt> is a two character string -- if you wish to compare the\n\
cleartext value that a user has entered against a Unix-style encrypted\n\
password, such as one from <code>/etc/passwd</code>, or from a\n\
<code>.htaccess</code> file, use the first two characters of the\n\
existing encrypted password as the salt, and then compare the\n\
resulting strings.\n\
\n\
For example, if the variable <var existing-pass> contains a previously\n\
encypted password, and the variable <var entered-pass> contains the\n\
cleartext that the user has just entered, you may encrypt the user's\n\
password and compare it with the existing one with the following code:\n\
\n\
<example>\n\
<set-var encrypted-pass =\n\
   <unix::crypt <get-var entered-pass>\n\
                <substring <get-var existing-pass> 0 2>>>\n\
.blank\n\
<when <string-eq <get-var encrypted-pass>\n\
                 <get-var existing-pass>>>\n\
   <set-session-var logged-in=true>\n\
   <redirect members-only.mhtml>\n\
</when>\n\
.blank\n\
<h3>Please enter your password again.  It didn't match.</h3>\n\
</example>")

static void
pf_unix_crypt (PFunArgs)
{
  char *string = mhtml_evaluate_string (get_positional_arg (vars, 0));
  char *saltarg = mhtml_evaluate_string (get_positional_arg (vars, 1));
  char salt[3] = { 'c', 'd', '\0' }; /* Anything, right? */

  if (!empty_string_p (saltarg))
    {
      salt[1] = '\0';
      salt[0] = saltarg[0];
      if (saltarg[1]) salt[1] = saltarg[1];
    }

  if (!empty_string_p (string))
    {
      char *insertion = create_password (string, salt);
      bprintf_insert (page, start, "%s", insertion);
      *newstart += strlen (insertion);
      free (insertion);
    }

  xfree (string);
  xfree (saltarg);
}
#endif /* HAVE_CRYPT */

#if defined (__cplusplus)
}
#endif
