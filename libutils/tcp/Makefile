# This Makefile for the TCP library is in -*- Text -*- mode for Emacs.
#
# Copyright (c) 1995 Brian J. Fox
# Author: Brian J. Fox (bfox@ai.mit.edu)

RM = rm -f
LN = ln
CP = cp
MV = mv
AR = ar
RANLIB = ranlib
MKDIR = mkdir
TAR = tar

CC = gcc
# OPTIMIZE_FLAGS = -O69
DEBUG_FLAGS = -g
CFLAGS = -Wall -Wstrict-prototypes $(DEFS) $(OPTIMIZE_FLAGS) $(DEBUG_FLAGS) \
	 -I..
INCLUDES = 

LIB_CSRC = tcp.c
LIB_HSRC = tcp.h
LIB_OBJS = tcp.o

TARGET_LIB = libtcp.a
TEST_PROGS = 

REQUIRED_LIBS = ../xmalloc/libxmalloc.a

TARGETS = $(TARGET_LIB) $(TEST_PROGS)

.c.o:
	@echo Compiling $2 from $<
	@$(CC) $(CFLAGS) $(DEFS) $(INCLUDES) -c $<

all: $(TARGETS)

$(TARGET_LIB): $(LIB_OBJS)
	$(RM) $@
	$(AR) cq $@ $(LIB_OBJS)
	if [ "$(RANLIB)" ]; then $(RANLIB) $@; fi

../xmalloc/libxmalloc.a:
	(cd ../xmalloc; $(MAKE) $(MFLAGS))

clean realclean: FORCE
	$(RM) $(LIB_OBJS) $(TARGETS) *.o

distclean: clean
	$(RM) *.o *core* *~ DISTFILES *.bak

FORCE:

