# This is the Makefile for the GSQL directory of Meta-HTML.
# It simply builds the binary versions of the libraries from the source.
#
# Copyright (c) 1996 Brian J. Fox
# Author: Brian J. Fox (bfox@ai.mit.edu) Sun Apr 13 08:57:54 1997.
#

MCC = mklib

.SUFFIXES: .lib .mhtml

.mhtml.lib:
	$(MCC) -o $@ $<

all: gsql.lib tour.lib

clean distclean realclean: FORCE
	rm -f *.lib

FORCE:

