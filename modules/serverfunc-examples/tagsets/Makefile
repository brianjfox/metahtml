# This is the Makefile for the tagsets directory of the Jeopardy demo.
#.
# It simply builds the binary versions of the libraries from the source.
#

MCC = ../../../mhc/mhc -z ../../../utilities/mklib

all: server.lib smtp.lib

server.lib: server.mhtml commands.mhtml
	$(MCC) -o $@ server.mhtml

clean distclean realclean: FORCE
	rm -f *.lib

FORCE:

