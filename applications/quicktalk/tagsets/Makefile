# This is the Makefile for the quicktalk/tagsets directory of Meta-HTML.
# It simply builds the binary versions of the library from the source.
#

MCC = mklib

TARGET	= quicktalk.lib
SOURCE=	quicktalk.mhtml

.SUFFIXES: .lib .mhtml

.mhtml.lib:
	$(MCC) -o $@ $<

all: $(TARGET)

$(TARGET): $(SOURCE)

clean: FORCE
	rm -f $(TARGET)

distclean realclean: clean
	rm -rf $(LIBFILES)

FORCE:
