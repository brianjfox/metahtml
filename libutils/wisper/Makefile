# This Makefile for the wisper library is in -*- Text -*- mode for Emacs.
#
# Copyright (c) 1995 Brian J. Fox
# Author: Brian J. Fox (bfox@ai.mit.edu)

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

VERSION = 1.3
DISTNAME = wisper-$(VERSION)

CC = gcc
# OPTIMIZE_FLAGS = -O69
DEBUG_FLAGS = -g
CFLAGS = -Wall -Wstrict-prototypes  $(OPTIMIZE_FLAGS) $(DEBUG_FLAGS) -I..
INCLUDES = ../regex

LIB_CSRC = wisp.c
LIB_HSRC = wisp.h
LIB_OBJS = wisp.o

TARGET_LIB = libwisper.a
TEST_PROGS = wisp-test-reader$(EXE) wisp-test-assoc$(EXE) \
	     wisp-test-append$(EXE) # phone$(EXE)

REQUIRED_LIBS = ../xmalloc/libxmalloc.a

TARGETS = $(TARGET_LIB) # $(TEST_PROGS)

all: $(TARGETS)

$(TARGET_LIB): $(LIB_OBJS)
	$(RM) $@
	$(AR) cq $@ $(LIB_OBJS)
	if [ "$(RANLIB)" ]; then $(RANLIB) $@; fi

wisp-test-reader$(EXE): wisp-test-reader.o $(TARGET_LIB) $(REQUIRED_LIBS)
	$(CC) $(CFLAGS) -o $@ wisp-test-reader.o $(TARGET_LIB) $(REQUIRED_LIBS)

wisp-test-assoc$(EXE): wisp-test-assoc.o $(TARGET_LIB) $(REQUIRED_LIBS)
	$(CC) $(CFLAGS) -o $@ wisp-test-assoc.o $(TARGET_LIB) $(REQUIRED_LIBS)

wisp-test-append$(EXE): wisp-test-append.o $(TARGET_LIB) $(REQUIRED_LIBS)
	$(CC) $(CFLAGS) -o $@ wisp-test-append.o $(TARGET_LIB) $(REQUIRED_LIBS)

phone$(EXE): phone.o $(TARGET_LIB) $(REQUIRED_LIBS)
	$(CC) $(CFLAGS) -o $@ phone.o $(TARGET_LIB) $(REQUIRED_LIBS)

birthday$(EXE): birthday.o $(TARGET_LIB) $(REQUIRED_LIBS)
	$(CC) $(CFLAGS) -o $@ birthday.o $(TARGET_LIB) $(REQUIRED_LIBS)

rolodex$(EXE): phone$(EXE)
	rm -f rolodex$(EXE)
	ln phone$(EXE) rolodex$(EXE)

../xmalloc/libxmalloc.a:
	(cd ../xmalloc; $(MAKE) $(MFLAGS))

clean realclean: FORCE
	$(RM) $(LIB_OBJS) $(TARGETS) *.o

distclean: clean
	$(RM) *.o *core* *~ DISTFILES *.bak $(TEST_PROGS) rolodex phone 

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
