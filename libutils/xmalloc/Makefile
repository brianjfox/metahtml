# This Makefile for the XMALLOC library is in -*- Text -*- mode for Emacs.
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

srcdir = .

#
# ALL_SUBDIRS is the list of all of the subdirectories within which we
# would like to run various `make's, or ".", which means that there
# are no subdirectories.
ALL_SUBDIRS = .

VERSION = 1.0
DISTNAME = xmalloc-$(VERSION)

CC = gcc
OPTIMIZE_FLAGS = -O69
# DEBUG_FLAGS = -g
CFLAGS = -Wall -Wstrict-prototypes  $(OPTIMIZE_FLAGS) $(DEBUG_FLAGS)
LDFLAGS = $(CFLAGS)
INCLUDES = -I../..

LIB_CSRC = xmalloc.c
LIB_HSRC = xmalloc.h
LIB_OBJS = xmalloc.o

TARGET_LIB = libxmalloc.a
TEST_PROGS = 

REQUIRED_LIBS = 

TARGETS = $(TARGET_LIB) $(TEST_PROGS)

.c.o:
	@echo Compiling $@ from $<
	@$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c $<

all: $(TARGETS)

$(TARGET_LIB): $(LIB_OBJS)
	$(RM) $@
	$(AR) cq $@ $(LIB_OBJS)
	if [ "$(RANLIB)" ]; then $(RANLIB) $@; fi

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

tags TAGS: FORCE
	etags *.c *.h

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
