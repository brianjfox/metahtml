# This Makefile simply links a bunch of pages.
#

TARGETS = Results

all: $(TARGETS)

Results: FORCE
	(cd Results; $(MAKE))

clean: FORCE
	@echo "Nothing to do for CLEAN"

distclean: FORCE
	(cd Results; $(MAKE) distclean)

FORCE:
