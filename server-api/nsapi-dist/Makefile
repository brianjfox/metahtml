# Defines for example NSAPI programs running under SOLARIS

CC_CMD=cc -DHPUX -D_REENTRANT 
# If you have gcc, use this 
#CC_CMD=gcc -DSOLARIS -D_REENTRANT -Wall

LD_SHAREDCMD=ld -b

all:
prepare:

INCLUDEDIR=../include

INCLUDE_FLAGS=-I$(INCLUDEDIR) -I$(INCLUDEDIR)/base -I$(INCLUDEDIR)/frame
COMMON_DEFS=-DMCC_HTTPD -DXP_UNIX

all: mhtml_nsapi.so

mhtml_nsapi.so:  mhtml_nsapi.o
	$(LD_SHAREDCMD) mhtml_nsapi.o -o mhtml_nsapi.so $(EXTRA_LDDEFINES)

.c.o:
	$(CC_CMD) $(COMMON_DEFS) $(INCLUDE_FLAGS) -c $< 

clean:
	rm -f mhtml_nsapi.o mhtml_nsapi.so $(EXTRA_CLEAN)
