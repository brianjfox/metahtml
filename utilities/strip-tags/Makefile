# Makefile for strip-tags is in -*- text -*- mode.
#

# The real locations of programs.
AR	= ar
RANLIB	= ranlib
MKDIR	= mkdir
TAR	= tar
LN	= ln
CP	= cp
RM	= rm -f
INSTALL = $(CP)
MYINSTALL= $(CP)

MHTML_SRCDIR = $(TOPDIR)

CC		= gcc $(GCC_WARN)
GCC_WARN	= -Wall -Wstrict-prototypes -Wshadow
# PROFILE_FLAGS	= -pg
# DEBUG_FLAGS	= -g $(PROFILE_FLAGS)
OPTIMIZE_FLAGS	= -O69 -finline-functions
INCLUDE_FLAGS	= -I$(MHTML_SRCDIR)libutils
REQLIBS		= $(MHTML_SRCDIR)libutils/libutils.a $(OS_LIBS)
CFLAGS		= $(OPTIMIZE_FLAGS) $(DEBUG_FLAGS) $(INCLUDE_FLAGS)
LDFLAGS		= $(DEBUG_FLAGS)

OBJS = strip-tags.o
PROG = strip-tags$(EXE)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -c $<

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(REQLIBS)

install: $(PROG)
	$(MYINSTALL) $(PROG) $(bindir)/$(PROG); chmod a+x $(bindir)/$(PROG)

clean: FORCE
	$(RM) *.o *.a $(PROG)

distclean: clean

dist: FORCE
	rm -rf strip-tags-1.0
	mkdir strip-tags-1.0
	ln *.c Makefile strip-tags-1.0
	tar -zcf strip-tags-1.0.tar.gz strip-tags-1.0
	rm -rf strip-tags-1.0

FORCE:
