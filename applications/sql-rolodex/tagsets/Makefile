# This is the Makefile for the tagsets.
#.
# It simply builds the binary versions of the libraries from the source.
#

MCC   = mklib
MKAPI = mkapi

.SUFFIXES: .lib .mhtml .api

.mhtml.lib:
	$(MCC) -o $@ $<

.mhtml.api:
	$(MKAPI) -o $@ $<

all: rolodex.lib rolodex.api browser.lib

clean distclean realclean: FORCE
	rm -f *.lib *.api

FORCE:

