# The Makefile for this Meta-HTML product allows the packing or  unpacking
# of the associated databases.
#
# Copyright (c) 1996 Brian J. Fox
# Copyright (c) 1996 Universal Access Inc.
# Author: Brian J. Fox (bfox@ai.mit.edu) Thu Aug 29 09:36:07 1996.
#
# This file is part of <Meta-HTML>(tm), a system for the rapid deployment
# of Internet and Intranet applications via the use of the Meta-HTML
# language.
#
#  Copyright (c) 1995, 1996, Brian J. Fox (bfox@ai.mit.edu).
#  Copyright (c) 1996, Universal Access Inc. (http://www.ua.com).
#
# Meta-HTML is free software; you can redistribute it and/or modify
# it under the terms of the UAI Free Software License as published
# by Universal Access Inc.; either version 1, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# UAI Free Software License for more details.
#
# You should have received a copy of the UAI Free Software License
# along with this program; if you have not, you may obtain one by
# writing to:
#
# Universal Access Inc.
# 129 El Paseo Court
# Santa Barbara, CA
# 93101

APPLICATION	= sql-rolodex
VERSION		= 1.1
DISTNAME	= $(APPLICATION)-$(VERSION)
TARFILE		= $(DISTNAME).tar
GZTARFILE	= $(TARFILE).Z
GZTARFILE	= $(TARFILE).gz

RM	= rm -f
FIND	= find
MKDIR	= mkdir
LN	= ln
CP	= cp
GREP	= grep
TAR	= tar
GZIP	= gzip
DBPACK	= dbpack

all: FORCE
	@echo ""
	@echo "                  PLEASE READ THIS!!"
	@echo ""
	@echo "You may type one of \`make pack', \`make unpack', \`make dist',"
	@echo "or \`make clean'."
	@echo ""
	@echo "    Typing 'make pack' creates the system independent versions"
	@echo "    of the databases which are present in $(APPLICATION).  This"
	@echo "    allows the data to be moved between binary platforms."
	@echo ""
	@echo "    Typing 'make unpack' creates the system dependent versions"
	@echo "    of the databases which are present in $(APPLICATION).  This"
	@echo "    is how you re-install $(APPLICATION) on a new (or the same)"
	@echo "    platform."
	@echo ""
	@echo "    Typing 'make dist' first packs the databases, and then"
	@echo "    creates a gzipped tar file of $(APPLICATION).  The tar"
	@echo "    file can then be ftp'ed to another system, and unpacked"
	@echo "    there."
	@echo ""
	@echo "    Typing 'make clean' removes all Emacs backup files."

pack: FORCE
	find . -name "*.db" -exec $(DBPACK) \{\} \; -print

unpack: FORCE
	find . -name "*.packed" -exec $(DBPACK) \{\} \; -print

clean: FORCE
	@echo "Removing Emacs backup files..."
	@find . -name "*~" -exec $(RM) \{\} \; -print
	@find . -name "#*" -exec $(RM) \{\} \; -print
	$(RM) DISTFILES

distclean realclean: clean
	@echo "Removing binary databases..."
	@find . -name "*.db" -exec $(RM) \{\} \; -print

dist: pack DISTFILES
	$(RM) -r $(TARFILE) $(GZTARFILE) $(ZTARFILE) $(DISTNAME)
	$(MKDIR) $(DISTNAME)
	for d in `find . -type d -print | $(GREP) -v '='`; do \
	  if [ "$$d" != "./$(DISTNAME)" -a "$$d" != "." ]; then \
	    echo "Making directory $(DISTNAME)/$$d"; \
	    mkdir $(DISTNAME)/$$d; \
	  fi; \
	done
	for f in `cat DISTFILES`; do \
	   $(LN) "./$$f" $(DISTNAME)/"$$f" || \
		{ echo copying $$f; cp -rp ./$$f $(DISTNAME)/$$f ; } \
	done
	-(cd $(DISTNAME); $(MAKE) distclean)
	$(TAR) chvf - $(DISTNAME) | $(GZIP) >$(GZTARFILE)
	$(RM) -r $(DISTNAME)

# Gets rid of most of the unwanted files.  Verify manually (if necessary)
# that this produces a list of all the files desired in the distribution. 
DISTFILES: FORCE
	$(RM) -rf $(DISTNAME)
	(find . ! -type d -print) \
	| sed  '/\/RCS\//d; \
		/\.tar.*/d; \
		/\#*#$$/d; \
		/~$$/d; /\.o$$/d; /\/.*\.a$$/d; \
		/\/.*\.BAK$$/d; \
		/\/TAGS$$/d; \
		/\/core$$/d; /\/[a-z]*\.core$$/d; /\/core\.[a-z]*$$/d; \
		/\/a.out$$/d; \
		/\/=/d; \
		/\/DISTFILES$$/d; \
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
