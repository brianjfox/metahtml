/* register.h: -*- C -*-  */

/* Author: Henry Minsky (hqm@ua.com) Fri Dec 20 14:53:25 1996. 

    Code to create and check activation keys */

extern char *make_expir_key (time_t when);
extern int verify_key (char *key, time_t *when);
extern int check_activation_key (char *key);
