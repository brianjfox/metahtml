/* Engine.c: Meta-HTML Engine for the Macintosh.  Parts of this code
   were cribbed from various free packages found on the net.  Thanks
   to all who contributed in this way. */

#include <AppleEvents.h>
#include <MenuBar.h>
#include <stdarg.h>
#include <sys/types.h>
#include <bprintf/bprintf.h>
#include <xmalloc/xmalloc.h>
#include "pages.h"
#include "session_data.h"
#include "pagefuncs.h"

#define ENGINE_IS_ASYNCHRONOUS 1
extern char *strdup (const char *string);

/* PowerPC doesn't define these (or I couldn't find them), so here they are. */
#if !defined (LoWord)
#  define LoWord(x) (x & 0x0000FFFF)
#  define HiWord(x) ((x & 0xFFFF0000) >> 16)
#endif

#define menu_Apple 128
#define menu_File  menu_Apple + 1
#define item_Quit  1

static void menu_dispatch (long choice);
static void initialize_event_handlers (void);
static PAGE *run_engine (const char *filename);

/* When non-zero, our event loop is active. */
static int running = 1;

pascal OSErr HandQuitApp (AppleEvent *, AppleEvent *, long);
pascal OSErr event_engine (AppleEvent *, AppleEvent *, long);

static void StatusSetUp ();
static void UpdateScreen ();
static void AboutBox ();

MenuHandle MenuApple;
WindowPtr MyWindow;
WindowRecord MyWindArea;
PicHandle MyPic;

/* Do the required initializations for Macintosh.  Ugh. */
static void
initialize_macintosh_toolbox (void)
{
  InitGraf (&qd.thePort);
  InitFonts ();
  FlushEvents (everyEvent, 0);
  InitWindows ();
  InitMenus ();
  TEInit ();
  InitDialogs ((long)0);
  InitCursor ();
}

static void
initialize_menubar (void)
{
  Handle MenuBar;
  MenuHandle MenuFile;
  
  MenuBar = GetNewMBar (menu_Apple);
  SetMenuBar (MenuBar);
  MenuApple = GetMHandle (menu_Apple);
  MenuFile = GetMenu (menu_File);
  AddResMenu (MenuApple, 'DRVR');	
  DrawMenuBar ();
}

#define POST_BUFFER_SIZE 32767

void
main (void)
{
  EventRecord event;

  /* Okay, we're up and running now. */
  running = 1;

  /* Do some Mac shit. */
  initialize_macintosh_toolbox ();
  initialize_menubar ();
  initialize_event_handlers ();

  StatusSetUp ();

  while (running)
    {
      WaitNextEvent (everyEvent, &event, (long)0, (long)0);

      switch (event.what)
	{
	case mouseDown:
	  {
	    WindowPtr window;
	    short int location;

	    location = FindWindow (event.where, &window);

	    if (location == inMenuBar)
	      menu_dispatch (MenuSelect (event.where));
	  }
	  break;

	case keyDown:
	  {
	    int command_key_p = ((int)event.modifiers) & cmdKey;

	    if (command_key_p > 0)
	      {
		long which = MenuKey (((int)event.message) & charCodeMask);
		menu_dispatch (which);
	      }
	  }
	  break;

	case updateEvt:
	  if ((WindowPtr)event.message == MyWindow)
	    {
	      BeginUpdate (MyWindow);
	      UpdateScreen ();
	      EndUpdate (MyWindow);
	    }
	  break;

	  /* What we actually care about. */
	case kHighLevelEvent:
	  {
	    OSErr status;

	    status = AEProcessAppleEvent (&event);

	    if (status != noErr)
	      SysBeep (2);
	  }
	  break;
	}
    }
}

static void
menu_dispatch (long choice)
{
  int menu, item;
  Str255 deskacc_name;

  if (choice != 0)
    {
      menu = HiWord (choice);
      item = LoWord (choice);

      switch (menu)
	{
	case menu_Apple:
	  if (item == 1)
	    {
	      AboutBox ();
	    }
	  else
	    {
	      GetItem (MenuApple, item, deskacc_name);
	      OpenDeskAcc (deskacc_name);
	    }
	  break;

	case menu_File:
	  switch (item)
	    {
	    case item_Quit:
	      running = 0;
	      break;
	    }

	  break;
	}

      HiliteMenu (0);
    }
}

static pascal OSErr
event_ignore_handler (AppleEvent *req, AppleEvent *rep, long which)
{
  UpdateScreen ();
  return (noErr);
}

pascal OSErr
HandQuitApp (AppleEvent *TheRequest, AppleEvent *TheReply, long Reference)
{
  UpdateScreen ();
  running = 0;

  return (noErr);
}

/* The largest chunk of information that we can get in an Apple Event.
   How's that for random? */
#define MAX_BYTES 32768

static void
get_event_string (AppleEvent *e, unsigned long which, char *buffer,
		  int max_bytes)
{
  DescType dt;
  Size size;

  AEGetParamPtr (e, which, typeChar, &dt, buffer, max_bytes, &size);
  buffer[size] = '\0';
}

#define get_parm(name) get_event_string (req, name, buffer, MAX_BYTES);
#define apple_to_mhtml(apple_name, metahtml_name) \
  do { \
        get_parm (apple_name); \
	pagefunc_set_variable (metahtml_name, buffer); \
     } while (0)

typedef struct
{
  unsigned long apple_name;	/* Name used to get parameter. */
  char *mhtml_name;		/* Variable name in Meta-HTML. */
} MAPPING;

static MAPPING apple_to_mhtml_mappings[] =
{
  { '----', "env::path_info" },		/* Args after '$' in the URL. */
  { 'addr', "env::remote_host" },	/* Client's hostname or address. */
  { 'Agnt', "env::http_user_agent" },	/* Client's User-Agent. */
  { 'kfor', "env::query_string" },	/* Query String (GET args). */
  { 'meth', "env::method" },		/* METHOD: GET, POST, etc. */
  { 'frmu', "env::user_email" },	/* User's e-mail address.  Sure. */
  { 'user', "server::authorized-username" }, /* Name from authorization. */
  { 'pass', "server::authorized-password" }, /* Pass from authorization. */
  { 'svnm', "env::server_name" },	/* Server name. */
  { 'svpt', "env::server_port" },	/* Server Port. */
  { 'refr', "env::http_referer" },	/* HTTP_REFERER */
  { 'ctyp', "server::content-type-encoding" },  /* Encoding for POSTs. */
  { 'Kact', "server::action-name" },	/* Action Name as taken from MacHTTP.
					   Should be "METAHTML". */
  { 'Kapt', "server::action-path" },	/* The full path to this code. */
  { 'Kcip', "env::remote_addr" },	/* Client's IP address? */
  { 'Kfrq', "mac::request-text[]" },	/* Everything the client sent. */
  { 'scnm', "env::script_name" },	/* Relative path to the current doc. */
  { (unsigned long)0, (char *)NULL }
};

pascal OSErr
event_engine (AppleEvent *req, AppleEvent *rep, long reference)
{
  register int i;
  PAGE *page = (PAGE *)NULL;
  char *document;
  static char *buffer = (char *)NULL;

  if (buffer == (char *)NULL)
    buffer = (char *)xmalloc (MAX_BYTES);

  /* This is the Macintosh platform, isn't it. */
  pagefunc_set_variable ("mhtml::platform", "Macintosh; PPC");

  /* Get and install all of the known parameters with Meta-HTML analogues. */
  for (i = 0; apple_to_mhtml_mappings[i].mhtml_name; i++)
    apple_to_mhtml (apple_to_mhtml_mappings[i].apple_name,
		    apple_to_mhtml_mappings[i].mhtml_name);
		    
   /* Get the posted arguments. */
  {
    Package *package = (Package *)NULL;
    char *temp;
   	   
#if defined (ENGINE_IS_ASYNCHRONOUS)
    package = symbol_lookup_package ("POSTED");
    if (package != (Package *)NULL)
      symbol_destroy_package (package);
    package = (Package *)NULL;
#endif

    get_parm ('post');
    if (buffer[0] != '\0')
      {
	package = symbol_get_package ("POSTED");

	forms_parse_data_string ((const char *)buffer, package);
      }
    
    temp = pagefunc_get_variable ("env::query_string");

    if (temp != (char *)NULL)
      {
	if (!package)
	  package = symbol_get_package ("POSTED");

    	forms_parse_data_string ((const char *)temp, package);
      }

    /* If we got some data items, put them in DEFAULT also. */
    if (package != (Package *)NULL)
      {
	Package *default_package = symbol_get_package ("DEFAULT");
	symbol_copy_package (package, default_package);
      }
  }

  document = pagefunc_get_variable ("env::script_name");

  if (document != (char *)NULL)
    page = run_engine (document);

  if (page == (PAGE *)NULL)
    {
      page = page_create_page ();
      bprintf (page, "<html><body>");
      bprintf (page, "<h2>Run-Engine failed to return a page!</h2><br>");
      bprintf (page, "</body></html>");
    }

  AEPutParamPtr (rep, keyDirectObject, typeChar, page->buffer, page->bindex);

  page_free_page (page);

#if defined (ENGINE_IS_ASYNCHRONOUS)
  pagefunc_destroy_package ("env");
  pagefunc_destroy_package ("server");
  pagefunc_destroy_package ("mhtml");
  pagefunc_destroy_package ("default");
#else
  running = 0;
#endif

  UpdateScreen ();
  return (noErr);
}

static char *
substring (char *string, int start, int end)
{
  char *result = (char *)xmalloc (1 + end - start);

  strncpy (result, string + start, end - start);
  result[end - start] = '\0';

  return (result);
}

static char *
gobble_cookie (char *string)
{
  register int i, start = 0;
  char *sid = (char *)NULL;
  int cookie_index = 0;

  /* Gobble up any SID out of STRING. */
  if (*string == '/') start = 1;
  for (i = start; string[i] != '\0'; i++)
    {
      if (string[i] == '/')
	{
	  sid = substring (string, start, i);
	  memmove (string, string + i, (1 + strlen (string)) - i);
	  break;
	}

      if (!isdigit (string[i]))
	break;
    }

  return (sid);
}

static PAGE *
run_engine (const char *immutable_path)
{
  PAGE *page = (PAGE *)NULL;
  char *path = (char *)NULL;
  char *filename = immutable_path ? strdup (immutable_path) : (char *)NULL;
  char *temp;
  static char *include_prefix = (char *)NULL;
  static called_once = 0;

  if (!called_once)
    {
      PAGE *init = page_read_template ("meta-html.conf");
      BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
      char *action_dir = pagefunc_get_variable ("server::action-path");

      if (action_dir)
	{
	  register int i;

	  bprintf (buffer, "/%s", action_dir);
	  temp = strrchr (buffer->buffer, ':');
	  if (temp) *temp = 0;

	  for (i = 0; buffer->buffer[i] != '\0'; i++)
	    if (buffer->buffer[i] == ':')
	      buffer->buffer[i] = '/';

	  pagefunc_set_variable ("mhtml::include-prefix", buffer->buffer);
	  bprintf_free_buffer (buffer);
	}

      if (init != (PAGE *)NULL)
	{
	  page_process_page (init);
	  called_once++;

	  if (init != (PAGE *)NULL)
	    {
	      page_free_page (init);
	      temp = pagefunc_get_variable ("mac::document-root");
	      if (temp)
		include_prefix = strdup (temp);
	    }
	}
    }


  /* Set up the page variables that must be set. */
  {
    BPRINTF_BUFFER *buffer = bprintf_create_buffer ();
    char *server_name = pagefunc_get_variable ("env::server_name");
    char *server_port = pagefunc_get_variable ("env::server_port");
    char *cookie = (char *)NULL;
    char *offset;

    if (include_prefix)
      pagefunc_set_variable ("mhtml::include-prefix", include_prefix);

    bprintf (buffer, "http://%s", server_name ? server_name : "");

    if ((server_port != (char *)NULL) && (strcmp (server_port, "80") != 0))
      bprintf (buffer, ":%s", server_port);

    pagefunc_set_variable ("mhtml::http-to-host", buffer->buffer);
    pagefunc_set_variable ("mhtml::http-prefix-sans-sid", buffer->buffer);
    temp = strdup (buffer->buffer);

    bprintf (buffer, "%s", filename);
    pagefunc_set_variable ("mhtml::full-url", buffer->buffer);
    pagefunc_set_variable ("mhtml::current-url", buffer->buffer);

    /* We have to gobble up the cookie if it is present. */
    if (filename != (char *)NULL)
      cookie = gobble_cookie (filename);

    if (cookie != (char *)NULL)
      {
	pagefunc_set_variable ("SID", cookie);

	/* Set HTTP-PREFIX. */
	buffer->bindex = 0;
	bprintf (buffer, "%s/%s", temp, cookie);
	pagefunc_set_variable ("mhtml::http-prefix", buffer->buffer);

	/* URL-TO-DIR. */
	bprintf (buffer, "%s", filename);
	offset = strrchr (buffer->buffer, '/');
	if (offset != (char *)NULL)
	  *offset = '\0';

	pagefunc_set_variable ("mhtml::url-to-dir", buffer->buffer);

	/* URL-TO-DIR-SANS-SID. */
	buffer->bindex = 0;
	bprintf (buffer, "%s%s", temp, filename);
	offset = strrchr (buffer->buffer, '/');
	if (offset != (char *)NULL)
	  *offset = '\0';
	pagefunc_set_variable ("mhtml::url-to-dir-sans-sid", buffer->buffer);


	/* Set FULL-URL-SANS-SID. */
	buffer->bindex = 0;
	bprintf (buffer, "%s%s", temp, filename);
	pagefunc_set_variable ("mhtml::full-url-sans-sid", buffer->buffer);
	pagefunc_set_variable ("mhtml::current-url-sans-sid", buffer->buffer);
      }
    else
      {
	pagefunc_set_variable ("mhtml::http-prefix", temp);

	/* URL-TO-DIR. */
	buffer->bindex = 0;
	bprintf (buffer, "%s%s", temp, filename);
	offset = strrchr (buffer->buffer, '/');
	if (offset != (char *)NULL)
	  *offset = '\0';
	pagefunc_set_variable ("mhtml::url-to-dir", buffer->buffer);
	pagefunc_set_variable ("mhtml::url-to-dir-sans-sid", buffer->buffer);

	pagefunc_set_variable ("mhtml::full-url-sans-sid", buffer->buffer);
	pagefunc_set_variable ("mhtml::current-url-sans-sid", buffer->buffer);
      }

    free (temp);
    bprintf_free_buffer (buffer);
  }

  /* If there is a prologue document, run it now. */
  temp = pagefunc_get_variable ("mac::prologue-document");

  if (temp != (char *)NULL)
    {
      PAGE *prologue = page_read_template (temp);

      if (prologue)
	{
	  page_process_page (prologue);
	  page_free_page (prologue);
	}
    }

  /* Now get the document that the user requested. */
  if (filename && *filename)
    {
      char *path = (char *)xmalloc (1024);
      char *relative_prefix = (char *)NULL;
      BPRINTF_BUFFER *contents = bprintf_create_buffer ();

      if (include_prefix)
	sprintf (path, "%s", include_prefix);
      else
	sprintf (path, "/mxm/WebStar");

      /* Get the relative prefix from the document name. */
      temp = strrchr (filename, '/');

      if ((temp != (char *)NULL) && (temp != filename))
	{
	  relative_prefix = (char *)xmalloc (temp - filename);
	  strncpy (relative_prefix, filename, (temp - filename));
	  relative_prefix[temp - filename] = '\0';
	  pagefunc_set_variable ("mhtml::relative-prefix", relative_prefix);
	}

      /* Get document name. */
      if (temp)
	{
	  temp++;
	  pagefunc_set_variable ("mhtml::current-doc", temp);
	}

      strcat (path, filename);

      page = page_read_template (path);

      if (page == (PAGE *)NULL)
	{
	  page = page_create_page ();
	  			
	  bprintf (page, "Couldn't find the file %s\n", path);
	}
      else
	{
	  /* Set up Meta-HTML variables which we find useful. */

	  page_process_page (page);
	  if (page == (PAGE *)NULL)
	    {
	      page = page_create_page ();
	      bprintf
		(page, "Failed on %s: page_process_page () returned NULL",
		 path);
	    }

	  page_clean_up (page);
	}

      if (relative_prefix) free (relative_prefix);
      free (path);
      bprintf_free_buffer (contents);
    }

  if (filename)
    free (filename);

  return (page);
}

static void
initialize_event_handlers (void)
{
  OSErr res;
  AEEventHandlerUPP upp;

  upp = NewAEEventHandlerProc (event_ignore_handler);
  res = AEInstallEventHandler
    (kCoreEventClass, kAEOpenApplication, upp, 0, FALSE);

  /* We should actually allow Meta-HTML to open some documents directly.
     Oh, well, that will be in version 1. */
  res = AEInstallEventHandler
    (kCoreEventClass, kAEOpenDocuments, upp, 1, FALSE);

  /* upp = NewAEEventHandlerProc (HandPrintDoc); */
  res = AEInstallEventHandler
    (kCoreEventClass, kAEPrintDocuments, upp, 2, FALSE);

  upp = NewAEEventHandlerProc (HandQuitApp);
  res = AEInstallEventHandler
    (kCoreEventClass, kAEQuitApplication, upp, 0, FALSE);

  upp = NewAEEventHandlerProc (event_engine);
  res = AEInstallEventHandler ('WWW‡', 'sdoc', upp, 0, FALSE);
}

static void
StatusSetUp (void)
{
  Rect rect;

  rect.top = 40;
  rect.bottom = 178;
  rect.left = 20;
  rect.right = 322;
  MyWindow = NewWindow (&MyWindArea, &rect,
			"\pMeta-HTML Engine", TRUE, (int)0, (WindowPtr)(-1),
			FALSE, (long)5);
  ShowWindow (MyWindow);
  SetPort (MyWindow);

  MyPic = GetPicture (128);
  UpdateScreen ();
}

static void
UpdateScreen (void)
{
  Rect rect;

  if (MyPic != 0)
    {
      rect = (*MyPic)->picFrame;
      DrawPicture (MyPic, &rect);
    }
}

static void
AboutBox (void)
{
  Rect WindowRect;
  int Continue;
  EventRecord Action;
  PicHandle MyPic;
  Rect PicRect;
  int Height,Width;
  WindowPtr DLGWindow;
  WindowRecord DLGWindArea;

  MyPic = GetPicture (129);

  if (MyPic != 0)
    {
      PicRect = (*MyPic)->picFrame;
      Height = PicRect.bottom - PicRect.top;
      Width = PicRect.right - PicRect.left;
      WindowRect.top =
	qd.screenBits.bounds.top +
	  ((qd.screenBits.bounds.bottom - qd.screenBits.bounds.top) / 2)
	    - (Height / 2);
    WindowRect.bottom = WindowRect.top + Height;

      WindowRect.left =
	qd.screenBits.bounds.left +
	  ((qd.screenBits.bounds.right - qd.screenBits.bounds.left) / 2)
	    - (Width / 2);
      WindowRect.right = WindowRect.left + Width;

      DLGWindow = NewWindow (&DLGWindArea, &WindowRect, "\pAbout", false,
			     2, (WindowPtr)(-1), false, 5);
      SetPort (DLGWindow);
      SetRect (&PicRect, 0, 0, Width, Height);
      ShowWindow (DLGWindow);
      Continue = FALSE;

      while (Continue == FALSE)
	{
	  WaitNextEvent (everyEvent, &Action, 0L, 0L);
	  switch (Action.what)
	    {
	    case mouseDown:
	      Continue = TRUE;
	      break;

	    case keyDown:
	      Continue = TRUE;
	      break;

	    case updateEvt:
	      if ((WindowPtr)Action.message == DLGWindow)
		{
		  BeginUpdate (DLGWindow);
		  DrawPicture (MyPic, &PicRect);
		  EndUpdate (DLGWindow);
		}
	      break;
	    }
	}

      CloseWindow (DLGWindow);
      SetPort (MyWindow);
      ReleaseResource((Handle)MyPic);
      UpdateScreen ();
  }
}

/* Some support routines for handling output to stdout, etc. */

BPRINTF_BUFFER *console_buffer = (BPRINTF_BUFFER *)NULL;

void
WriteCharsToConsole (void)
{
  if (console_buffer == (BPRINTF_BUFFER *)NULL)
    console_buffer = bprintf_create_buffer ();

  /* Now simply write the characters into our buffer. */
}

void
ReadCharsFromConsole (void)
{
  /* Can't be done.  I guess we should return some characters. */
}

void
RemoveConsole (void)
{
  /* The characters in the console buffer have already been placed where
     they should go.  Simply free the memory associated with this buffer. */
  if (console_buffer != (BPRINTF_BUFFER *)NULL)
    bprintf_free_buffer (console_buffer);
}

void
InstallConsole (void)
{
  /* Don't do anything.  The act of writing to the console buffer with
     WriteCharsToConsole creates the buffer if it doesn't exist. */
}

