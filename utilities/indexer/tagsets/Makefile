# This is a Makefile for tagsets.
#.
# It simply builds the binary versions of the libraries from the source.
#

MCC = mklib
MHC = mhc
MKAPI = mkapi

.SUFFIXES: .lib .mhtml .api

.mhtml.lib:
	$(MCC) -o $@ $<

.mhtml.api:
	$(MKAPI) -o $@ $<

all: indexer.lib

clean: FORCE
	rm -f *.lib

distclean realclean: clean
	rm -f *~

FORCE:
