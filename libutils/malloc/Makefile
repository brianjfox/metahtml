# Copyright (C) 1991, 1992, 1993 Free Software Foundation, Inc.
# This file is part of the GNU C Library.

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public License
# as published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.

# You should have received a copy of the GNU Library General Public
# License along with the GNU C Library; see the file COPYING.LIB.  If
# not, write to the Free Software Foundation, Inc., 675 Mass Ave,
# Cambridge, MA 02139, USA.

# Makefile for standalone distribution of malloc.

# Use this on System V.
#CPPFLAGS = -DUSG

AR	= ar
RANLIB	= ranlib

# You cannot compile with anything other than GNU C.
CC		 = gcc $(GCC_WARN)
GCC_WARN	= -Wall -Wstrict-prototypes -Wshadow
# PROFILE_FLAGS	= -pg
# DEBUG_FLAGS	= -g $(PROFILE_FLAGS)
OPTIMIZE_FLAGS	= -O69
CFLAGS		= $(OPTIMIZE_FLAGS) $(DEBUG_FLAGS)

.c.o:
	@echo Compiling $@ from $<
	@$(CC) -I. $(CFLAGS) $(DEFS) -c $<

all: libmalloc.a # gmalloc.o

gmalloc = malloc.c free.c realloc.c calloc.c morecore.c memalign.c
sources = malloc.c free.c realloc.c calloc.c morecore.c memalign.c valloc.c # mcheck.c mtrace.c mstats.c vm-limit.c ralloc.c malloc-find.c
objects = malloc.o free.o realloc.o calloc.o morecore.o memalign.o valloc.o # mcheck.o mtrace.o mstats.o # vm-limit.o ralloc.o malloc-find.o
headers = malloc.h

libmalloc.a: $(objects)
	$(AR) crv $@ $(objects)
	$(RANLIB) $@

$(objects): $(headers)

gmalloc.c: gmalloc-head.c $(headers) $(gmalloc) Makefile
	cat gmalloc-head.c $(headers) $(gmalloc) > $@-tmp
	mv -f $@-tmp $@
# Make it unwritable to avoid accidentally changing the file,
# since it is generated and any changes would be lost.
	chmod a-w $@

clean malloc-clean: FORCE
	-rm -f libmalloc.a *.o core

distclean realclean malloc-realclean: clean
	-rm -f TAGS tags *~

# For inside the C library.
malloc.tar malloc.tar.Z: FORCE
	$(MAKE) -C .. $@
FORCE:
