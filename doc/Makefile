# Makefile for the documentation directory of Meta-HTML.
# Copyright (c) 1995, 1996 Brian J. Fox
# Author: Brian J. Fox (bfox@ai.mit.edu)
CP = cp
RM = rm -f
DBPACK = ../dbutils/dbpack

webroot = /www/`hostname`
docroot = $(webroot)/docs
docdir  = $(docroot)/meta-html
mandeps = ../libmhtml/functions.db.packed ../modules/functions.db.packed
secdeps = ../libmhtml/sections.db.packed ../modules/sections.db.packed

all: manual/functions.db manual/sections.db

manual/functions.db: manual/functions.db.packed
	$(DBPACK) manual/functions.db.packed

manual/sections.db: manual/sections.db.packed
	$(DBPACK) manual/sections.db.packed

manual/functions.db.packed: $(mandeps)
	$(RM) manual/functions.db.packed manual/functions.db
	for i in $(mandeps); do cat $$i >> manual/functions.db.packed; done

manual/sections.db.packed: $(secdeps)
	$(RM) manual/sections.db.packed manual/sections.db
	for i in $(secdeps); do cat $$i >> manual/sections.db.packed; done

install: all
	-if [ "$(docdir)" = "$(docroot)/meta-html" ]; then \
	  mkdir $(webroot); mkdir $(docroot); \
	fi
	-mkdir $(docdir)
	$(CP) -r . $(docdir)/

dist: FORCE
	(cd manual; ../../dbutils/dbpack *.db)

distclean realclean clean: FORCE
	$(RM) *~ */*.db */*.db.packed
	(cd manual; $(RM) metahtml.dvi metahtml.newaux metahtml.ps \
		          metahtml.texi metahtml.ljet*)

FORCE:
