#if defined (__cplusplus)
extern "C"
{
#endif

#include <stdio.h>
#include <sys/types.h>
#include <time.h>

#include "register.h"


/* Decode and verify a key. */
int
main (int argc, char *argv[])
{
  time_t then;
  char *key;
  int status;

  if (argc == 1)
    {
      printf ("usage: check_key keystring\n");
      exit (0);
    }

  key = argv[1];
  status = verify_key (key, &then);

  printf ("Expiration Date:\t%s", ctime (&then));
  printf ("Activation Key: \t%s\n", key);
  printf ("Verify Key:\t\t%d (%s)\n", status, (status == 1) ? "OK" : "ERROR");

  fprintf (stderr, "%ld", then);

  check_activation_key (key);
  return (0);
}
#if defined (__cplusplus)
}
#endif
