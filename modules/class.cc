// class.cc: -*- C++ -*-  DESCRIPTIVE TEXT.
// 
//  Copyright (c) 1998 Brian J. Fox
//  Author: Brian J. Fox (bfox@ai.mit.edu) Wed May 20 10:45:50 1998.

/* Here is the C++ code.
   It implements a totally useless class with one instance variable,
   a string, which can be assigned to, retrieved, created, and destroyed. */

#include "iostream.h"
#include "strings.h"

class CC_Class
{
protected:
  char *string;

public:
  CC_Class (char *s = (char *)NULL) {}
  char *GetValue() { return (string); }
  char *SetValue (char *value)
    {
      if (string) free (string);
      string = value ? strdup (value) : (char *)NULL;
    }
};

/* A way to call our class from within C. */
extern "C"
{
  void *cc_create (void) { cerr<< "This\n"; return (void *)(new CC_Class); }
  char *cc_get (void *obj) { return (*(CC_Class *)obj).GetValue (); }
  void cc_set (void *obj, char *val) { (*(CC_Class *)obj).SetValue (val); }
  void cc_delete (void *obj) { delete obj; }
}

#if defined (TESTING)
int
main (int argc, char *argv[])
{
  CC_Class obj;

  obj.SetValue ("This is a list");
  cout << obj.GetValue ();
  return (0);
}
#endif
