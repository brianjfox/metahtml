/* register.c: -*- C -*-  */

/*  Copyright (c) 1996 Universal Access Inc.
    Author: Henry Minsky (hqm@ua.com) Fri Dec 20 15:07:26 1996.  */

#if defined (__cplusplus)
extern "C"
{
#endif

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>

#include "register.h"

static char *hexchars = "0123456789ABCDEF";

/* NOTE: This code depends on being able to cast a time_t to an int,
   and will have to be modified if this operation cannot be done. */
static void
activation_key_invalid (time_t when)
{
  struct tm *tm;

  if (when == 0)
    {
     fprintf (stderr, "You don't have an activation key.\n");
    }
  else
    {
      tm = localtime (&when);
      fprintf (stderr, "Activation key has expired on ");
      fprintf (stderr, "%s", asctime (tm));
    }

  fprintf (stderr, "Please contact Universal Access Inc. (info@ua.com).\n");
}

/* Returns 1 if key is valid, 0 if not. */
int
check_activation_key (char *key)
{
  time_t when;
  
  if ((key == (char *) NULL) || (*key == 0))
    {
      activation_key_invalid ((time_t)0);
      return (0);
    }

  if ((verify_key (key, &when)) > 0)
    {
      time_t now = time((time_t *) NULL);
      if (when < now)
	{
	  activation_key_invalid (when);
	  return (0);
	}
      else
	return (1);
    }
  else
    {
      activation_key_invalid (when);
      return (0);
    }
}

/* Generate a 16 byte printable hashcode of a string, using hex digits.
   Always returns a newly consed string. */
static char *
hash_int (int num)
{
  char src[32];
  register int i, c;
  register char *p;
  char buf[64];
  char hashbuf[9];

  sprintf (src, "%d", num);
  memset (hashbuf, 0, 9);

  if (strlen (src) <= 8)
    {
      strcpy (hashbuf, src);
    }
  else
    {
      /* Do a linear feedback shiftregister. */
      for (i = 0; i < 8; i++)
	hashbuf[i] = src[i];

      p = src + i;
      while (1)
	{
	  char tmp = hashbuf[0];

	  c = *p++;
	  if (c == 0)
	    break;

	  hashbuf[0] = hashbuf[1] ^ c;
	  hashbuf[1] = hashbuf[2] ^ tmp;;
	  hashbuf[2] = hashbuf[3] ^ tmp;
	  hashbuf[3] = hashbuf[4] ^ ~c;
	  hashbuf[4] = hashbuf[5] ^ hashbuf[3] ^ tmp;
	  hashbuf[5] = hashbuf[6] ^ hashbuf[1] ^ c;;
	  hashbuf[6] = hashbuf[7] ^ c ^ tmp;
	}
    }

  for (p = buf, i = 0; i < 8; i ++)
    {
      *p++ = hexchars[(hashbuf[i] >> 4) & 0x0F];
      *p++ = hexchars[(hashbuf[i] & 0x0F)];
    }
  *p = 0;
  return (char *) (strdup (buf));
}

/* Separate out key into hash and expirtime, and make sure
   that the hash is valid.

   Takes key of the form of a fixed length 24 char hex string 
   hash[16] . exprtime[8]

   Returns 1 if OK, or 0 if exprtime does not match its hash. */
int
verify_key (char *key, time_t *when)
{
  char *newhash;
  char oldhash[64];
  char exprstring[16];

  /* Extract first 16 bytes as hash value. */
  if (strlen (key) < 24) return (-1);

  strncpy (oldhash, key, 16);
  oldhash[16] = '\0';
  
  strncpy (exprstring, key + 16, 8);
  exprstring[8] = '\0';

  /* Parse out the 8 byte hex string which contains a time_t. */
  sscanf (exprstring, "%x", (unsigned int *) when);

  newhash = hash_int (*when);
  
 if (strcasecmp (newhash, oldhash) == 0)
   return (1);
 else
   return (0);
}

/* Called with a time_t, it returns a 24 byte key. */
char *
make_expir_key (time_t when)
{
  char newkey[64];
  char *hash = hash_int (when);
  sprintf (newkey, "%s%8X", hash, (unsigned int) when);
  return ((char *) (strdup (newkey)));
}

#if defined (__cplusplus)
}
#endif
