/* xmalloc.c -- safe versions of malloc and realloc */

/* Brian J. Fox (bfox@ai.mit.edu): Wed Mar  8 12:15:38 1995 */

#if defined (ALREADY_HAVE_XMALLOC)
#else
#include <stdio.h>

#include <stdlib.h>

#if defined (HAVE_CONFIG_H)
#  include "../../config.h"
#endif

#if defined (COMPILING_WITH_RMALLOC)
#  define MALLOC_DEBUG
#  include "../rmalloc/rmalloc.h"
#endif

#if defined (__cplusplus)
extern "C"
{
#endif

static void memory_error_and_abort (char *fname);

/* **************************************************************** */
/*								    */
/*		   Memory Allocation and Deallocation.		    */
/*								    */
/* **************************************************************** */

/* Return a pointer to free()able block of memory large enough
   to hold BYTES number of bytes.  If the memory cannot be allocated,
   print an error message and abort. */
void *
xmalloc (unsigned int bytes)
{
  void *temp = (void *)malloc (bytes);

  if (!temp)
    memory_error_and_abort ("xmalloc");
  return (temp);
}

void *
xrealloc (void *pointer, unsigned int bytes)
{
  void *temp;

  if (!pointer)
    temp = (void *)malloc (bytes);
  else
    temp = (void *)realloc (pointer, bytes);

  if (!temp)
    memory_error_and_abort ("xrealloc");
  return (temp);
}

void *
xcalloc (unsigned int count, unsigned int bytes)
{
  void *temp = (void *)calloc (count, bytes);

  if (!temp)
    memory_error_and_abort ("xcalloc");
  return (temp);
}

static void
memory_error_and_abort (char *fname)
{
  fprintf (stderr, "%s: Out of virtual memory!\n", fname);
  abort ();
}
#if defined (__cplusplus)
}
#endif

#endif /* !ALREADY_HAVE_XMALLOC */

