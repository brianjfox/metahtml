# This Makefile for the bprintf library is in -*- Text -*- mode for Emacs.
# Copyright (c) 1995 Brian J. Fox
# Author: Brian J. Fox (bfox@ai.mit.edu)
#
RM = rm -f
LN = ln
CP = cp
MV = mv
AR = ar
RANLIB = ranlib
MKDIR = mkdir
TAR = tar
LIBCOMPAT = ../compat/libcompat.a
EXE =
srcdir = .

#
# ALL_SUBDIRS is the list of all of the subdirectories within which we
# would like to run various `make's, or ".", which means that there
# are no subdirectories.
ALL_SUBDIRS = .

VERSION = 1.1
DISTNAME = bprintf-$(VERSION)

CC = gcc
# OPTIMIZE_FLAGS = -O69
DEBUG_FLAGS = -g
CFLAGS = -Wall -Wstrict-prototypes  $(OPTIMIZE_FLAGS) $(DEBUG_FLAGS) -I..
LDFLAGS = $(OPTIMIZE_FLAGS) $(DEBUG_FLAGS)

LIB_CSRC = bprintf.c
LIB_HSRC = bprintf.h
LIB_OBJS = bprintf.o

TARGET_LIB = libbprintf.a
TEST_PROGS = bprintf-test$(EXE)

REQUIRED_LIBS = ../xmalloc/libxmalloc.a $(LIBCOMPAT)

TARGETS = $(TARGET_LIB) # $(TEST_PROGS)

all: $(TARGETS)

$(TARGET_LIB): $(LIB_OBJS)
	$(RM) $@
	$(AR) cq $@ $(LIB_OBJS)
	if [ "$(RANLIB)" ]; then $(RANLIB) $@; fi

bprintf-test$(EXE): bprintf-test.o $(TARGET_LIB) $(REQUIRED_LIBS)
	$(CC) $(LDFLAGS) -o $@ bprintf-test.o $(TARGET_LIB) $(REQUIRED_LIBS)

../xmalloc/libxmalloc.a:
	(cd ../xmalloc; $(MAKE) $(MFLAGS))

../compat/libcompat.a:
	(cd ../compat; $(MAKE))

clean realclean: FORCE
	$(RM) $(LIB_OBJS) $(TARGETS)

distclean: clean
	$(RM) *.o *core* *~ DISTFILES *.bak

dist: DISTFILES
	$(RM) -rf $(DISTNAME)
	$(MKDIR) $(DISTNAME)
	for d in `find $(ALL_SUBDIRS) -type d -print`; do \
	  if [ "$$d" != "." -a "$$d" != "./$(DISTNAME)" ]; then \
	    mkdir $(DISTNAME)/$$d; \
	  fi \
	done
	for f in `cat DISTFILES`; do \
	   $(LN) $(srcdir)/$$f $(DISTNAME)/$$f || \
		{ echo copying $$f; cp -p $(srcdir)/$$f $(DISTNAME)/$$f ; } \
	done
	(cd $(DISTNAME); $(MAKE) $(MFLAGS) distclean)
	$(TAR) chvf - $(DISTNAME) | gzip >$(DISTNAME).tar.gz
	$(RM) -rf $(DISTNAME)

# Gets rid of most of the unwanted files.  Verify manually (if necessary)
# that this produces a list of all the files desired in the distribution. 
DISTFILES: FORCE
	$(RM) -rf $(DISTNAME)
	(cd $(srcdir); find . ! -type d -print) \
	| sed  '/\/RCS\//d; \
	       /\/EMACS-BACKUPS\//d; \
	       /\.tar.*/d; \
	       /~$$/d; /\.o$$/d; \
	       /\/config\.status$$/d; \
	       /\/.*\.BAK$$/d; \
	       /\/TAGS$$/d; \
	       /\/core$$/d; \
	       /\/[a-z]*\.core$$/d; \
	       /\/core\.[a-z]*$$/d; \
	       /\/a.out$$/d; \
	       /\/=/d; \
	       /\/conftest\.c$$/d; \
	       /\/DISTFILES$$/d; \
	       /\/xact\/xact$$/d; \
	       /\.toc$$/d; \
	       /\.aux$$/d; /\.log$$/d; \
	       /\.cps$$/d; /\.cp$$/d; \
	       /\.fns$$/d; /\.fn$$/d; \
	       /\.tps$$/d; /\.tp$$/d; \
	       /\.vrs$$/d; /\.vr$$/d; \
	       /\.pgs$$/d; /\.pg$$/d; \
	       /\.kys$$/d; /\.ky$$/d; \
	       s/^.\///; /^\.$$/d;' \
	| sort | uniq > DISTFILES


FORCE:
