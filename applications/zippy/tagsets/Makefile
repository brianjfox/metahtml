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

all: filter.lib zippy-filter.lib filter.api zippy-filter.api

zippy-filter.mhtml: zippy-lines.txt

clean: FORCE
	rm -f *.lib *.api

distclean realclean: clean
	rm -f *~

install: all
	$(DBINSTALL)

FORCE:

