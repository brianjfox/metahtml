# This is the Makefile for the /tagsets directory of Meta-HTML.
# It simply builds the binary versions of the libraries from the source.
#

MCC = mklib

LIBFILES=form-to-email.lib	pagecount.lib \
	 general.lib		parse-date.lib \
	 characters.lib		locale.lib		plist.lib \
	 dirfuns.lib		sendmail.lib		dump-package.lib \
	 menus.lib		session.lib		english-numbers.lib \
	 message.lib		numbers.lib		form-to-database.lib \
	 this.lib

OTHER_FILES = Makefile README

.SUFFIXES: .lib .mhtml

.mhtml.lib:
	$(MCC) -o $@ $<

all: $(LIBFILES)

clean distclean realclean: FORCE
	rm -rf $(LIBFILES)

FORCE:
