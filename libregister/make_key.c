
#if defined (__cplusplus)
extern "C"
{
#endif

#include <stdio.h>
#include <sys/types.h>
#include <time.h>

#include "register.h"

#define MINUTES(x)	(x * 60)
#define HOURS(x)	(MINUTES(x) * 60)
#define DAYS(x)		(HOURS(x) * 24)

extern int atoi (char *);

/* Make a key, given a date in the format "12 Nov 1998". */
int
main (int argc, char *argv[])
{
  time_t now = (time_t)time ((time_t *)NULL);
  time_t then = now + DAYS (30);
  char *newkey;
  int status;
  struct tm *my_tm;

  if (argc > 1)
    {
      int days_in_future = atoi (argv[1]);
      then = now + DAYS (days_in_future);
    }

  newkey = make_expir_key (then);
  status = verify_key (newkey, &then);
  my_tm = localtime (&then);

  fprintf (stderr, "Expiration Date:\t%s", asctime (my_tm));
  fprintf (stderr, "Activation Key: \t");
  fprintf (stdout, "%s", newkey);
  fprintf (stderr, "%s", newkey);
  fprintf (stderr, "\n");
  fprintf (stderr, "Verify Key:\t\t%s\n", (status == 1) ? "OK" : "ERROR");
  fflush (stderr);
  fflush (stdout);
  return (0);
}

#if defined (__cplusplus)
}
#endif
