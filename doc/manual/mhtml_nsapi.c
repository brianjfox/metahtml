Content-Type: text/plain

/* mhtml_nsapi.c: -*- C -*-  

    Copyright (c) 1996 Universal Access Inc.
    Author: Henry Minsky (hqm@ua.com) Tue Dec  3 15:12:41 1996.  
    Version 1.01

    A sample obj.conf file is included at the end of this file.

    This file contains several NSAPI modules which make it easier to call
    the Meta-HTML CGI Engine conveniently through the Netscape server.

    ================================================================
	      What The Functions In This File Do For You
    ================================================================
    The following features which provided via the functions
    mhtml_fixup_path and handle_mhtml_sid:

    mhtml_fixup_path allows you to:

    1. Configure the Netscape server to run only files which end in .mhtml
    (or the file-suffix of your choice) through the CGI engine.  

    2. Hide the location of the CGI Engine in your URL, so that instead of
    needing to use a URL like
       http://yourhost.com/cgi-bin/nph-engine/your/url.mhtml
    you can use 
       http://yourhost.com/your/url.mhtml

    3. Pass the correct PATH-INFO information to the Meta-HTML CGI Engine.

    handle_mhtml_sid provides automatic handling of session state
    information for older browsers which do not properly handle
    HTTP/Netscape cookies.

    If a numeric SID is found as the first component of the URL, then that
    is stripped off and stuck in the request-headers structure, which is
    passed to the CGI programs in the header URL-COOKIE

    ================================================================
	  How To Compile mhtml_engine.c as an NSAPI Shared Library
    ================================================================

    Options for linking 
    
    System       Compile options

    IRIX         ld -shared t.o u.o -o test.so

    SunOS        ld -assert pure-text t.o u.o -o test.so

    Solaris      ld -G t.o u.o -o test.so

    OSF/1        ld -all -shared -expect_unresolved "*" t.o u.o -o test.so

    HP-UX        ld -b t.o u.o -o test.so When compiling your code, you must also
                 use the use the +z flag to the HP C compiler. 

    AIX          cc -bM:SRE -berok t.o u.o -o test.so -bE:ext.exp -lc The
                 ext.exp file must be a text file with the name of a 
		 function that is externally accessible for each line. 


*/

#include <netsite.h> 
#include "base/pblock.h"
#include "base/session.h"
#include "frame/req.h"
#include "frame/func.h"
#include "frame/log.h"

/*

   Usage:
   At the beginning of obj.conf:
      Init fn=load-modules shlib=<path-to-library>/mhtml_nsapi.so funcs=handle-mhtml-sid,mhtml_fixup_path
   Inside the default object in obj.conf:
     NameTrans fn=handle-mhtml-sid 

   <ext> = so on UNIX
   <ext> = dll on NT.


   And add to obj.conf

   PathCheck index-names="index.mhtml,home.mhtml,welcome.mhtml" fn="find-index" 
   PathCheck fn=mhtml_fixup_path document-root=/www/pixis/docs engine=/www/pixis/docs/cgi-bin/nph-engine ext=".mhtml"
   ...
   ...
   Service fn="send-cgi" type="magnus-internal/cgi"


   PathChck mhtml_fixup_path takes the *absolute* pathname of the cgi-engine
   on your filesyste,, and the absolute document-root directory pathname.

   This NameTrans should appear before other Nametrans directives in the
   default object.

   Note that you can load more than one function using one load-modules
   command. If you want to try another function from this file, you can load
   it by separating the names to the funcs parameter with commas.

 */

static int DBG_MHTML_CGI = 0;

#define DBG_LOG_MSG(fun,msg) if (DBG_MHTML_CGI)  { log_error(LOG_INFORM, fun, sn, rq, msg); dbg_hdrs (fun, pb, sn, rq); } 


static void dbg_hdrs (char *fun, pblock *pb, Session *sn, Request *rq)
{
  if (DBG_MHTML_CGI)
    {
      /* Server variables */
      char *reqvarstr = pblock_pblock2str(rq->vars, NULL);
      char *reqpbstr = pblock_pblock2str(rq->reqpb, NULL);
      char *reqhdrstr = pblock_pblock2str(rq->headers, NULL);
      char *pbstr =  pblock_pblock2str(pb, NULL);
      char *srvhdrs = pblock_pblock2str(rq->srvhdrs, NULL);
      char *client = pblock_pblock2str(sn->client, NULL);

      log_error(LOG_INFORM, fun, sn, rq, "rq->vars: %s\n", reqvarstr);
      log_error(LOG_INFORM, fun, sn, rq, "rq->reqpb: %s\n", reqpbstr);
      log_error(LOG_INFORM, fun, sn, rq, "rq->headers: %s\n", reqhdrstr);
      log_error(LOG_INFORM, fun, sn, rq, "pb: %s\n", pbstr);
      log_error(LOG_INFORM, fun, sn, rq, "rq->srvhdrs: %s\n", srvhdrs);
      log_error(LOG_INFORM, fun, sn, rq, "sn->client: %s\n", client);
    }
}


#ifdef XP_WIN32
#define NSAPI_PUBLIC __declspec(dllexport)
#else /* !XP_WIN32 */
#define NSAPI_PUBLIC
#endif /* !XP_WIN32 */

#include <string.h>           /* strchr */
#include "frame/log.h"        /* log_error */

/* If URI starts with a string of all digits, then treat it as an
   mhtml SID, remove it from the partial path, and stick it into an environment
   variable. 
   */

NSAPI_PUBLIC int handle_mhtml_sid (pblock *pb, Session *sn, Request *rq)
{ 

  int i;

  /* Server variables */
  char *ppath = pblock_findval("ppath", rq->vars);

  DBG_LOG_MSG("handle_mhtml_sid", "Entering handle_mhtml_sid")
  
  /* Does this URL start with digits? */
  if ((ppath != (char *)NULL) &&
       ((ppath[0] == '/') && (isdigit (ppath[1]))))
    {

      /* New path, containing URL with Session ID stripped out.  */
      char *new_ppath = malloc(strlen(ppath) + 1); 

      DBG_LOG_MSG ("handle_mhtml_sid","processing URL with cookie prefix")

      for (i = 1; isdigit (ppath[i]); i++);

      /* If the URI contains a cookie, then gobble it, stuff it into
	 rq->headers , and move along. */
      if (ppath[i] == '\0' || ppath[i] == '/')
	{
	  int j;
	  char *value = malloc(strlen(ppath) + 4 + 1); 

	  /* Remove SID from path */
	  strcpy (value, "SID=");
	  strncpy (value+4, ppath + 1, i - 1);
	  value[i + 4 - 1] = '\0';
	      
	  /* Now stuff the value into a the headers, and hope it gets
	     passed by the CGI interface. */
	  pblock_nvinsert ("URL-COOKIE", value, rq->headers); 
	  free(value);

	  for (j = 0; ppath[i] != '\0'; j++, i++)
	    ppath[j] = ppath[i];

	  ppath[j] = '\0';
	}
      strcat (new_ppath, ppath);

      /* Replace partial path with session ID stripped out. */
      pblock_nvinsert("ppath", new_ppath, rq->vars); 
      free(new_ppath);
    }

  /* Let Netscape continue to do name translations. */
  return REQ_NOACTION;
}

/* If the client has requested a URL which ends with a directory
   rather than a filename (for example "http://www.foo.com/bar" or
   "http://www.foo.com/bar/") then that will not be picked up in the
   NameTrans phase of the server's request handling, and some special
   hacking needs to be done to make it process a Meta-HTML file.

   The Netscape builtin PathCheck function "find-index" is responsible
   for probing for various well known file names in the directory. An 
   example from obj.conf is

   PathCheck index-names="index.html,home.html,index.mhtml" fn="find-index" 

   If we add the standard meta-html index files
   {index.mhtml,welcome.mhtml,Welcome.mhtml} then we need to be able
   to force netscape to then run the file through the cgi-engine,
   instead of simply delivering it. Unfortunately, we cannot simply
   run another NameTrans mapping function, because Netscape does not
   seem to allow any NameTrans functions to be run after a PatchCheck
   function has been run.

   PathCheck index-names="index.mhtml,home.mhtml,welcome.mhtml" fn="find-index" 
   PathCheck fn=mhtml_fixup_path document-root=/www/pixis/docs engine=/www/pixis/docs/cgi-bin/nph-engine ext=".mhtml"

   */




NSAPI_PUBLIC int mhtml_fixup_path (pblock *pb, Session *sn, Request *rq)
{

  /* User server config param, says where engine is located. */
  char *engine_path = pblock_findval("engine", pb);
  char *docroot = pblock_findval("document-root", pb);

  /* Server variables */
  char *path = pblock_findval("path", rq->vars);
  char *mhtml_suffix = pblock_findval("ext", pb);

  DBG_LOG_MSG ("mhtml_fixup_path", "entering")

  if (!mhtml_suffix)
    mhtml_suffix = ".mhtml";
  
  /* If this path contains .mhtml, then splice the cgi-engine path
     into it, and return success. */
  if (strstr (path, mhtml_suffix))
    {

      DBG_LOG_MSG ("mhtml_fixup_path", "processing .mhtml file")

      pblock_remove ("path", rq->vars); 
      pblock_remove ("path-info", rq->vars); 
      pblock_remove ("content-type", rq->srvhdrs); 

      pblock_nvinsert("path", engine_path, rq->vars); 

      pblock_nvinsert("content-type", "magnus-internal/cgi", rq->srvhdrs); 

      /* Strip out document root, and just put filename in path-info. */
      pblock_nvinsert("path-info", path+strlen(docroot), rq->vars); 

    } 
  
  return REQ_PROCEED; 

}

#if 0

   # Netscape Communications Corporation - obj.conf
   # You can edit this file, but comments and formatting changes
   # might be lost when the admin server makes changes.
   
   Init format.access="%Ses->client.ip% - %Req->vars.auth-user% [%SYSDATE%] \"%Req->reqpb.clf-request%\" %Req->srvhdrs.clf-status% %Req->srvhdrs.content-length%" fn="flex-init" access="/export/home/www/ns-home/httpd-pixis/logs/access"
   Init fn="load-types" mime-types="mime.types"
   Init fn=load-modules shlib=/www/src/VERSIONS/metahtml-5.01/nsapi/mhtml_nsapi.so funcs=handle-mhtml-sid,mhtml_fixup_path
   
   <Object name="default">
   NameTrans fn=handle-mhtml-sid
   NameTrans from="/cgi-bin" fn="pfx2dir" dir="/www/pixis/docs/cgi-bin" name="cgi"
   NameTrans from="/ns-icons" fn="pfx2dir" dir="/export/home/www/ns-home/ns-icons"
   NameTrans from="/mc-icons" fn="pfx2dir" dir="/export/home/www/ns-home/ns-icons"
   NameTrans root="/www/pixis/docs" fn="document-root"
   PathCheck fn="unix-uri-clean"
   PathCheck fn="find-pathinfo"
   PathCheck index-names="index.html,home.html,index.mhtml,welcome.mhtml" fn="find-index" 
   PathCheck fn=mhtml_fixup_path document-root=/www/pixis/docs engine=/www/pixis/docs/cgi-engine/nph-engine ext=".mhtml" 
   ObjectType fn="type-by-extension"
   ObjectType fn="force-type" type="text/plain"
   Service fn="send-cgi" type="magnus-internal/cgi"
   Service fn="imagemap" method="(GET|HEAD)" type="magnus-internal/imagemap"
   Service fn="index-common" method="(GET|HEAD)" type="magnus-internal/directory"
   Service fn="send-file" method="(GET|HEAD)" type="*~magnus-internal/*"
   AddLog fn="flex-log" name="access"
   </Object>

   <Object name="cgi">
   ObjectType fn="force-type" type="magnus-internal/cgi"
   Service fn="send-cgi"
   </Object>


#endif
