# This Makefile simply builds the binary versions of the mandefs library.
#
# Copyright (c) 1996 Brian J. Fox
# Author: Brian J. Fox (bfox@ai.mit.edu) Sun Apr 13 08:57:54 1997.
#

MCC = ../../mhc/mhc ../../utilities/mklib

.SUFFIXES: .lib .mhtml

.mhtml.lib:
	$(MCC) -o $@ $<

all: mandefs.lib

clean distclean realclean: FORCE
	rm -f *.lib

FORCE:

