/* History.h -- the names of functions that you can call in history. */

#if !defined (_HISTORY_H_)
#define _HISTORY_H_
#if defined (__cplusplus)
extern "C" 
{
#endif

/* The structure used to store a history entry. */
typedef struct _hist_entry
{
  char *line;
  void *data;
} HIST_ENTRY;

/* A structure used to pass the current state of the history stuff around. */
typedef struct _hist_state
{
  HIST_ENTRY **entries;		/* Pointer to the entries themselves. */
  int offset;			/* The location pointer within this array. */
  int length;			/* Number of elements within this array. */
  int size;			/* Number of slots allocated to this array. */
  int flags;
} HISTORY_STATE;

/* Flag values for the `flags' member of HISTORY_STATE. */
#define HS_STIFLED	0x01

/* Initialization and state management. */

/* Begin a session in which the history functions might be used.  This
   just initializes the interactive variables. */
extern void using_history (void);

/* Return the current HISTORY_STATE of the history. */
extern HISTORY_STATE *history_get_history_state (void);

/* Set the state of the current history array to STATE. */
extern void history_set_history_state (HISTORY_STATE *state);

/* Manage the history list. */

/* Place STRING at the end of the history list.
   The associated data field (if any) is set to NULL. */
extern void add_history (char *string);

/* A reasonably useless function, only here for completeness.  WHICH
   is the magic number that tells us which element to delete.  The
   elements are numbered from 0. */
extern HIST_ENTRY *remove_history (int which);

/* Make the history entry at WHICH have LINE and DATA.  This returns
   the old entry so you can dispose of the data.  In the case of an
   invalid WHICH, a NULL pointer is returned. */
extern HIST_ENTRY *replace_history_entry (int which, char *line, void *data);

/* Stifle the history list, remembering only MAX number of entries. */
extern void stifle_history (int max);

/* Stop stifling the history.  This returns the previous amount the
   history was stifled by.  The value is positive if the history was
   stifled, negative if it wasn't. */
extern int unstifle_history (void);

/* Return 1 if the history is stifled, 0 if it is not. */
extern int history_is_stifled (void);

/* Information about the history list. */

/* Return a NULL terminated array of HIST_ENTRY which is the current input
   history.  Element 0 of this list is the beginning of time.  If there
   is no history, return NULL. */
extern HIST_ENTRY **history_list (void);

/* Returns the number which says what history element we are now
   looking at.  */
extern int where_history (void);
  
/* Return the history entry at the current position, as determined by
   history_offset.  If there is no entry there, return a NULL pointer. */
HIST_ENTRY *current_history (void);

/* Return the history entry which is logically at OFFSET in the history
   array.  OFFSET is relative to history_base. */
extern HIST_ENTRY *history_get (int OFFSET);

/* Return the number of bytes that the primary history entries are using.
   This just adds up the lengths of the_history->lines. */
extern int history_total_bytes (void);

/* Moving around the history list. */

/* Set the position in the history list to POS. */
int history_set_pos (int pos);

/* Back up history_offset to the previous history entry, and return
   a pointer to that entry.  If there is no previous entry, return
   a NULL pointer. */
extern HIST_ENTRY *previous_history (void);

/* Move history_offset forward to the next item in the input_history,
   and return the a pointer to that entry.  If there is no next entry,
   return a NULL pointer. */
extern HIST_ENTRY *next_history (void);

/* Searching the history list. */

/* Search the history for STRING, starting at history_offset.
   If DIRECTION < 0, then the search is through previous entries,
   else through subsequent.  If the string is found, then
   current_history () is the history entry, and the value of this function
   is the offset in the line of that history entry that the string was
   found in.  Otherwise, nothing is changed, and a -1 is returned. */
extern int history_search (char *string, int direction);

/* Search the history for STRING, starting at history_offset.
   The search is anchored: matching lines must begin with string. */
extern int history_search_prefix (char *string, int direction);

/* Search for STRING in the history list, starting at POS, an
   absolute index into the list.  DIR, if negative, says to search
   backwards from POS, else forwards.
   Returns the absolute index of the history element where STRING
   was found, or -1 otherwise. */
extern int history_search_pos (char *string, int direction, int position);

/* Managing the history file. */

/* Add the contents of FILENAME to the history list, a line at a time.
   If FILENAME is NULL, then read from ~/.history.  Returns 0 if
   successful, or errno if not. */
extern int read_history (char *filename);

/* Read a range of lines from FILENAME, adding them to the history list.
   Start reading at the FROM'th line and end at the TO'th.  If FROM
   is zero, start at the beginning.  If TO is less than FROM, read
   until the end of the file.  If FILENAME is NULL, then read from
   ~/.history.  Returns 0 if successful, or errno if not. */
extern int read_history_range (char *filename, int from, int to);

/* Write the current history to FILENAME.  If FILENAME is NULL,
   then write the history list to ~/.history.  Values returned
   are as in read_history ().  */
extern int write_history (char *filename);

/* Append NELEMENT entries to FILENAME.  The entries appended are from
   the end of the list minus NELEMENTs up to the end of the list. */
int append_history (int nelements, char *filename);

/* Truncate the history file FILENAME, leaving only the last LINES lines. */
extern int history_truncate_file (char *filename, int lines);

/* History expansion. */

/* Expand the string STRING, placing the result into OUTPUT, a pointer
   to a string.  Returns:

   0) If no expansions took place (or, if the only change in
      the text was the de-slashifying of the history expansion
      character)
   1) If expansions did take place
  -1) If there was an error in expansion.
   2) If the returned line should just be printed.

  If an error ocurred in expansion, then OUTPUT contains a descriptive
  error message. */
extern int history_expand (char *string, char **output);

/* Extract a string segment consisting of the FIRST through LAST
   arguments present in STRING.  Arguments are broken up as in
   the shell. */
extern char *history_arg_extract (int first, int last, char *string);

/* Return the event specified at TEXT + OFFSET modifying OFFSET to
   point to after the event specifier.  Just a pointer to the history
   line is returned; NULL is returned in the event of a bad specifier.
   You pass STRING with *INDEX equal to the history_expansion_char that
   begins this specification.
   DELIMITING_QUOTE is a character that is allowed to end the string
   specification for what to search for in addition to the normal
   characters `:', ` ', `\t', `\n', and sometimes `?'.
   So you might call this function like:
   line = get_history_event ("!echo:p", &index, 0);  */
extern char *get_history_event (char *string, int *caller_index, int dq);

/* Return an array of tokens, much as the shell might.  The tokens are
   parsed out of STRING. */
extern char **history_tokenize (char *string);

/* Exported history variables. */
extern int history_base;
extern int history_length;
extern int max_input_history;
extern char history_expansion_char;
extern char history_subst_char;
extern char history_comment_char;
extern char *history_no_expand_chars;

#if defined (__cplusplus)
}
#endif

#endif /* HISTORY_H */
