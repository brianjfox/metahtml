# Simplistic Makefile for mod_mhtml (Apache versions 1.3x)
#
APXS          = apxs
APXS_STDFLAGS = -a -c
APXS_INCFLAGS = -I ../.. -I ../../libutils
APXS_MHTML    = -L ../../libmhtml -l mhtml
APXS_SERVE    = -L ../../libserver -l server
APXS_UTILS    = -L ../../libutils -l utils
APXS_CRYPT    = -L ../../../libdes-4.04b -l des
APXS_GDBM     = -l gdbm
APXS_LIBFLAGS = $(APXS_MHTML) $(APXS_SERVE) $(APXS_UTILS) \
		$(APXS_GDBM) $(APXS_CRYPT)
APXS_EXTRAFLAGS = $(APXS_INCFLAGS) $(APXS_LIBFLAGS)
APXS_COMPILE_FLAGS = $(APXS_STDFLAGS) $(APXS_EXTRAFLAGS)
APXS_INSTALL_FLAGS = -i

all: mod_mhtml.so

mod_mhtml.so: mod_mhtml.c mod_mhtml.h
	$(APXS) $(APXS_COMPILE_FLAGS) mod_mhtml.c

install: all
	$(APXS) $(APXS_INSTALL_FLAGS) mod_mhtml.so

realclean clean distclean: FORCE
	rm -f *.so *.o *~

FORCE:
