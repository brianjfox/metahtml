-*- Mode: Lisp; Auto-Fill: 1 -*- 

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

DEFINE_SECTION (HTML-HELPERS, HTML; helper; convenience, 
"The following functions all produce HTML as output and are defined in
order to help with the creation of forms and tables.
","")

DEFINE_SECTION (COLOR-MANIPULATION, HTML; color,
"<Meta-HTML> provides a few primitives for manipulating HTML color strings
and names.  You can convert from hexadecimal notation to decimal notation,
from color names to hex strings and back, create <code>SELECT</code>
widgets which allow the user to choose a color, and dynamically add new
colors to the (rather extensive) list of colors that <Meta-HTML> knows
about.", "")

DEFINE_SECTION (NETWORK-APPENDIX, networking; sendmail; streams,
"<Meta-HTML> contains several core functions for manipulating network
streams, such as <funref STREAM-OPERATORS with-open-stream>.  These
low level functions are exactly what is needed to allow the maximum
functionality and expressiveness in the language -- one might write a
complete Web based IMAP client using such primitives!  However, as is
often the case, it isn't always convenient to re-invent the wheel, and
so we have provided several tags in <Meta-HTML> PRO which deliver common
functionality in a general way.", "")

DEFINE_SECTION (USING-GNUPLOT, plotting; graphing; dynamic images,
"<Meta-HTML> contains a few functions which make it easy to generate plots
and graphs of data that you have available to you.

The use of the functions requires `gnuplot', a copy-lefted graphing program
widely available on the Internet (e.g., ftp://prep.ai.mit.edu/pub/gnu).

Using the <funref using-gnuplot gnuplot::plot-alists> function, you can create
PostScript and/or GIF images of a single set of data, or overlay many data
sets in a single graph.

Some terminology common to <code>gnuplot</code> may facilitate the reading
of the function documentation:  a <i>tic</i> is a mark which appears
on the axes of the final graph indicating the value along that axis --
there are tics in both the <var x> and <var y> directions.  Tics can
be labeled, you will have to read the documentation of
<code>gnuplot</code> for more information on that flexible format.", "")

DEFINE_SECTION (PAGECOUNT-APPENDIX, page counters; html helpers, 
"The PAGECOUNT::... functions allow you to place English language or
graphical page counters on any page in your site, simply by placing a
single tag on the page.

The counted pages are kept in a database (typically
<code>server-root/writable/pagecount.db</code>) so that you can write
your own programs to manipulate the collected data.", "")

DEFINE_SECTION (POWERSTRIP-SYSTEM-TAGS, , 
"The following functions are used in the implementation of the
PowerStrip System area (/System/*).

These functions are only useful to those persons who are adding
functionality to the PowerStrip system itself, or who need to
manipulate the underlying data structures and mechanisms of that
system.  ", "")

DEFINE_SECTION (AUTHORIZATION-TAGS, authorization;authentication;basic auth,
"<Meta-HTML> provides several flexible methods of authenticating a user
against a database.

The functions defined in this section all work using HTTP's Basic
Authorization scheme, but in a way that provides a much higher granularity
than that of older directory based schemes.

Because of the dynamic nature of <Meta-HTML>'s method, you can have many files
in a single directory, some of which are authenticated against a user database,
some of which are authenticated against a Unix style password file, or against
an SQL database, and some of which do not require authentication at all.", "")

DEFINE_SECTION (SERVER-VARIABLES, server; mhttpd; variables,
"The <Meta-HTML> server (<code>mhttpd</code>) is configured by reading a
file of <meta-html> code.  This allows arbitrary programming of the
server process, and means that you don't have to learn a new
configuration language in order to modify the server.

Included with the server is a set of administration pages; these pages
can be made accessible by telling the server where they are, and by
instructing the server to listen for requests on a specific port.

There are many variables which are important both to the engine and
the server.  Many of them are created on a per-page access basis by
either the engine or the server, and others are used to control the
server's behavior.", "")

DEFVAR (MHTTPD::GET-REMOTE-IDENT, "MHTTPD::GET-REMOTE-IDENT, when set,
says to use the IDENT protocol to attempt to find out the username
associated with the connecting browser.  This only has a minor effect
on server performance, but isn't especially useful for non-<Meta-HTML>
sites.")

DEFVAR (ENV::HTTP_AUTHORIZATION, "The <code>Basic Authorization</code>
string, as received by the server.  By the time that you see this
string, the user has already been authorized.")

DEFVAR (ENV::HTTP_REFERER, "A synonym for <varref mhtml::referer>.")

DEFVAR (ENV::HTTP_USER_AGENT, "The name that the connecting client
browser passed as the value of the <code>User-Agent</code> MIME
header.

For example:

<complete-example>
You are connected using: <get-var-once env::http_user_agent>
</complete-example>")

DEFVAR (ENV::QUERY_STRING, "The raw query string which was passed in
this URL.  In many cases, <meta-html> can create variables in the
<code>POSTED</code> package based upon the contents of this string.
In some cases, the string doesn't contain any variable assignments
(e.g., when the URL is an imagemap target).  For such cases, you may
need direct access to the query string.

<complete-example>
<get-var-once env::query_string>
</complete-example>")

DEFVAR (ENV::REMOTE_ADDR, "The Internet Protocol address of the
connecting client host, in \"dot\" notation.  You may wish to use
<varref env::remote_host> instead.

<complete-example>
<get-var-once env::remote_addr>
</complete-example>")

DEFVAR (ENV::REMOTE_HOST, "The host name of the connecting client.  In
cases where the connecting client does not have a host name which can
be looked up by the use of DNS, this variable contains the host's
Internet Protocol address, in \"dot\" notation.

<complete-example>
<get-var-once env::remote_host>
</complete-example>")

DEFVAR (ENV::SERVER_PORT, "The port on which the current web server is
listening for connections.

<complete-example>
<get-var-once env::server_port>
</complete-example>")

DEFVAR (MHTML::ACCESS-LOG, "The complete path to the file where the
MHTTPD server should keep the per-page access information.

The access log contains lines written in <a
href=\"http://www.w3.org/hypertext/WWW/Daemon/User/Config/Logging.html#common-logfile-format\">W3c's Common Logfile Format</a>.")

DEFVAR (MHTML::CACHEABLE, "The variable <var mhtml::cacheable>, when set to
a non-null, non-numeric value, indicates to the engine or server that the
expiration date of the current page should be in the future
(currently, about 10 days in the future).

When set to a numeric value, explicity sets the expiration date of the
page to the current time plus that value, expressed as seconds.

Thus, to cause your page to cease being a candidate for caching by browsers
within 10 minutes of the last page fetch, set <var mhtml::cacheable> to
<code>600</code> (i.e., 60 seconds times 10 minutes).")

DEFVAR (MHTML::CGI-DIRECTORIES, "This array is a list of physical
paths relative to the document root which are directories containing
CGI programs.

The paths are checked <i>after</i> path translation has been
performed.  The final slash for each directory must be specified.

<set-var mhtml::cgi-directories[0] = /cgi-bin/>
<complete-example>
<get-var-once mhtml::cgi-directories[0]>
</complete-example>")

DEFVAR (MHTML::CGI-EXTENSIONS, "An array of file name extensions that
specifies to the MHTTPD server the file name extensions which indicate
that the file is really a CGI program to be executed.

<example>
  <set-var mhtml::cgi-extensions[0]=\".pl\">
</example>")

DEFVAR (MHTML::COOKIE-COMPATIBLE, "Indicates whether or not the
current client/server pair can pass <a
href=\"http://home.netscape.com/newsref/std/cookie_spec.html\">HTTP
Cookies</a> back and forth.

When such communication is possible, this variable is set to
<code>\"true\"</code>.")

DEFVAR (MHTML::CURRENT-DOC, "The name of the current document, without
any path information, query strings, etc.

If the current URL is:

<example>
http://www.company.com/foo/bar/doc.mhtml?this=that
</example>

then <code>MHTML::CURRENT-DOC</code> is:

<example>
doc.mhtml
</example>")

DEFVAR (MHTML::CURRENT-URL, "A synonym for <varref mhtml::location>.")

DEFVAR (MHTML::CURRENT-URL-SANS-SID,
"A synonym for <varref mhtml::location-sans-sid>.")

DEFVAR (MHTML::DEBUG-LOG, "The complete path to the file where the
MHTTPD server should write server specific debugging information.
Generally, this only needs to be defined when you are debugging the
server itself.

<complete-example>
<get-var-once mhtml::debug-log>
</complete-example>")

DEFVAR (MHTML::DEBUG-POST, "When MHTML::DEBUG-LOG is set to a filename, and
this variable is non-empty, the names and values of variables that are posted
to the page are written to the debug log.")

DEFVAR (MHTML::DEFAULT-FILENAMES, "An array specifying the file names
that should be looked up when the URL to fetch ends in a directory,
instead of a document specifier.  The files are searched for in the
order in which they appear in this array.

<complete-example>
<get-var-once mhtml::default-filenames[]>
</complete-example>")

DEFVAR (MHTML::DOCUMENT-ROOT, "The complete path to the directory from
which your documents are to be served.  The final slash should not be
present.

<complete-example>
<get-var-once mhtml::document-root>
</complete-example>")

DEFVAR (MHTML::EPILOGUE-DOCUMENT, "Specifies a per-connection file of
code to be appended to the page just before processing any <Meta-HTML>
document.

<complete-example>
<get-var-once mhtml::epilogue-document>
</complete-example>")

DEFVAR (MHTML::ERROR-LOG, "MHTML::ERROR-LOG is the full path to a file
to which the server (or engine) logs errors.

<complete-example>
<get-var-once mhtml::error-log>
</complete-example>")

DEFVAR (MHTML::FULL-URL, "The URL used to connect to the current
document, including protocol, host information, session ID,
path-information, and query strings.

<complete-example>
<get-var-once mhtml::full-url>
</complete-example>")

DEFVAR (MHTML::FULL-URL-SANS-SID, "The URL used to connect to the
current document, including protocol, host information,
path-information, and query strings, but excluding the session ID.

<complete-example>
<get-var-once mhtml::full-url-sans-sid>
</complete-example>")

DEFVAR (MHTML::HTTP-PREFIX, "The portion of the current URL which
contains the protocol, host information, and the session ID.

<complete-example>
<get-var-once mhtml::http-prefix>
</complete-example>")

DEFVAR (MHTML::HTTP-PREFIX-SANS-SID, "The portion of the current URL
which contains the protocol, host information, but excluding the
session ID.

<complete-example>
<get-var-once mhtml::http-prefix-sans-sid>
</complete-example>")

DEFVAR (MHTML::HTTP-TO-HOST, "A synonym for <varref mhtml::http-prefix>.")

DEFVAR (MHTML::LOCATION, "The URL used to connect to the current
document, including path information and query strings, but without
the protocol or host information.

<complete-example>
<get-var-once mhtml::location>
</complete-example>")

DEFVAR (MHTML::LOCATION-SANS-SID, "The URL used to connect to the
current document, including path information and query strings, but
without the protocol, host information, or Session ID.

<complete-example>
<get-var-once mhtml::location-sans-sid>
</complete-example>")

DEFVAR (MHTML::METAHTML-EXTENSIONS, "An array of file name extensions
which indicate that the document to be fetched contains <meta-html>
code.

Normally, this variable is set in the server process.

<complete-example>
<get-var-once mhtml::metahtml-extensions[]>
</complete-example>")

DEFVAR (MHTML::MHTTPD-PAGES, "MHTML::MHTTPD-PAGES is the directory
containing various pages used by the server for displaying error
conditions, directory files, etc.  This is a complete path to the
directory, without the trailing slash.

<complete-example>
<get-var-once mhtml::mhttpd-pages>
</complete-example>")

DEFVAR (MHTML::PROLOGUE-DOCUMENT, "MHTML::PROLOGUE-DOCUMENT is a
per-connection file of code to be run just before processing any
<Meta-HTML> document.

<complete-example>
<get-var-once mhtml::prologue-document>
</complete-example>")

DEFVAR (MHTML::RAW-CONTENT-DATA, "Contains <i>exactly</i> what the
client browser sent in its <code>POST</code> request.

For wizard's use only!")

DEFVAR (MHTML::REFERER, "The value of the <code>HTTP</code>
<i>Referer</i> MIME header which the client browser sent along in its
request.

You might use this variable, if it is present, as the target of an
anchor:

<example>
<a href=\"<get-var-once mhtml::referer>\">
  <b>Back</b> to referring document.
</a>
</example>")

DEFVAR (MHTML::RELATIVE-PREFIX, "The sub-portion of the document path
which appears after <varref mhtml::include-prefix>.

With <code>mhtml::include-prefix</code> equal to <code>/www/docs</code>,
and a document path of<br> <i>/www/docs/admin/sections.mhtml</i>, then the
value of <code>mhtml::relative-prefix</code> would be <i>/admin</i>.

Useful when constructing new URLs based on the current URL.")

DEFVAR (MHTML::REMOTE-IDENT, "The value of the environment variable
<varref ENV::REMOTE_IDENT> as received by the server.

In the <meta-html> server, you can turn on the feature of asking the
connecting host for the username of the individual running the client
browser by setting <varref MHTTPD::GET-REMOTE-IDENT>.

There is never any gaurantee that the client will allow the server to
find this information out, so you should not write code which relies
on the presence of this variable.  However, if it is present, it can
be useful to present a default value.  For example:

<example>
<when <not <get-var-once e-mail>>>
  <set-var e-mail = <get-var-once mhtml::remote-ident>>
</when>
</example>")

DEFVAR (MHTML::REMOTE-USER, "The value of the environment variable
<varref ENV::REMOTE_USER> as received by the server.

When <code>Basic Authorization</code> is supplied by a client, this
variable contains the username which the connected user supplied.  Do
NOT confuse this with ENV::REMOTE_IDENT!")

DEFVAR (MHTML::REQUIRE-DIRECTORIES, "An array of directory names to be
searched through in order by <funref file-operators require>.  The
directories should be specified without the trailing slash.")

DEFVAR (MHTML::REQUIRE-LOADED, "The list of pathnames which have
already been loaded through <funref file-operators require>.")

DEFVAR (MHTML::SERVER-NAME, "MHTML::SERVER-NAME is the name that this
server should advertise.  If you do not set it, the value of the
current host's name is used.  On Unix systems, this is the result of
calling <code>gethostname()</code>.

<complete-example>
<get-var-once mhtml::server-name>
</complete-example>")

DEFVAR (MHTML::SERVER-PORT, "MHTML::SERVER-PORT is the port on which
this server should answer HTTP requests.  If you do not set it, the
value is <code>80</code>.")

DEFVAR (MHTML::SERVER-ROOT, "MHTML::SERVER-ROOT is the root location
for your server.  You might store the configuration, logging, and
server specific files there, for example.  You currently cannot store
your CGIs there, because that directory must be underneath <varref
MHTML::DOCUMENT-ROOT>.

The pathname should not end in a slash.")

DEFVAR (MHTML::UNPARSED-HEADERS, "When set to true, indicates to the
<meta-html> Engine that the server is expecting the engine to produce
all of the <code>HTTP</code> results, and not just the
<code>HTML</code> results.

This variable is automatically set in the <meta-html> server, and when
the Engine is run under a name beginning with <code>\"nph-\"</code>.

You can turn this variable on or off during the invocation of a
specific page to control the behavior of both the server and engine.")

DEFVAR (MHTML::URL-TO-DIR, "The complete URL to the directory
containing the current document.  This includes protocol, host,
session ID, and relative prefix information.

<complete-example>
<get-var-once mhtml::url-to-dir>
</complete-example>")

DEFVAR (MHTML::URL-TO-DIR-SANS-SID, "The complete URL to the directory
containing the current document.  This includes protocol, host, and
relative prefix information, but excludes the session ID if one were
present.

<complete-example>
<get-var-once mhtml::url-to-dir-sans-sid>
</complete-example>")

DEFINE_SECTION (GENERIC-SQL-INTERFACE, SQL; database; ODBC,
"With the plethora of ANSI SQL compatible databases present on the
marketplace, <Meta-HTML> provides a single comprehensive interface which
may be used to access any of those which are supported.

Such databases include <code>mSQL</code>, <code>mySQL</code>,
<code>Oracle</code>, <code>SyBase</code>, <code>Informix</code>, and
many other ODBC compliant databases.

Using the <Meta-HTML> Generic SQL Interface (GSQL), you can more rapidly
develop and inexpensively prototype your application -- then migrate
to full-blown production scale without writing a single line of code!

For example, you might build an in-house HR application using the
MySQL database.  Once you are happy with the application flow and GUI,
you can port the application to Oracle 8i by changing two lines of code:

<example>
<sql::set-database-type mysql>
<set-var app::dsn = \"host=localhost;database=hr\">
    ...
<sql::set-database-type odbc>
<set-var app::dsn = \"HOST=production;database=HR;SVT=Oracle 8i;...\">
</example>",
"The <Meta-HTML> Generic SQL Interface works by providing all of the
underlying functionality for each specific database type at a generic
top-level interface which never has to change.  You specify which type
of database that you would like to use with a single function call,
using <funref generic-sql-interface sql::set-database-type>.

Although there are specific low-level functions implementing the
database connectivity, the higher-level routines transparently make
extremely efficient use of them to handle your requests.

Because the low-level functions exist, you can easily write code which
copies data from one type of database (say mSQL) to another type (say
an Oracle ODBC database).")
