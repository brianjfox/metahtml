# This is the Makefile for the coloring-book tagset.
# It simply builds the binary versions of the library from the source.
#

MCC = mklib

TARGET	= coloring-book.lib
SOURCE=	coloring-book.mhtml

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
