srcdir	= .
DEFS	= -DHAVE_CONFIG_H
INSTALL = .././install.sh -c
DASH_SHARED = -G
SHARED_LD = ld
REGEX_IN= -I../../libutils/regex
GDBMLIB = /usr/lib/libgdbm.a
GDBM_IN = -I/usr/include
HTTPDIN = -I/usr/include/apache
HTTPDLIB= /export/home2/build/apache_1.3.26/src/main/libmain.a /export/home2/build/apache_1.3.26/src/ap/libap.a
DESLIB	= 
DBINC	= $(GDBM_IN)
DBLIB	= $(GDBMLIB) $(DESLIB)

PLATFORM= i386-solaris2.8
EXE	= 
OS_LIBS = -lnsl -lm -ldl
prefix	= /usr
exec_prefix = ${prefix}
bindir=$(exec_prefix)/bin

WEBBASEDIR	= 
MHTML_VERSION   = 6.11
WEBDEF		= -DWEBBASEDIR='"$(WEBBASEDIR)"'
VERSDEF		= -DMHTML_VERSION_STRING='\"$(MHTML_VERSION)\"'
SERVER_MAJOR	= 4
SERVER_MINOR	= 1
SERVER_DEF	= -DSERVER_VERSION='"MHttpd/$(SERVER_MAJOR).$(SERVER_MINOR) (bfox; $(PLATFORM); Meta-HTML/$(MHTML_VERSION))"' $(VERSDEF) $(CHECK_TIMESTAMP)
SERVER_OPT	= $(KEEP_ALIVE_OPTION) $(USE_SSL_OPTION)


# The real locations of programs.
AR	= ar
RANLIB	= ranlib
LN	= ln
CP	= cp
RM	= rm -f

# You cannot compile with anything other than GNU C.
CC		 = gcc $(GCC_WARN)
GCC_WARN	= -Wall -Wstrict-prototypes -Wshadow
APXS            = /usr/sbin/apxs
# PROFILE_FLAGS	= -pg
##DEBUG_FLAGS	= -g $(PROFILE_FLAGS)
OPTIMIZE_FLAGS	= -Wc,-O4 -Wc,-march=i686 -Wc,-mcpu=i686 -Wc,-fpic -Wc,-fomit-frame-pointer
INCLUDE_FLAGS	= -I../.. -I../../libutils -I../../libmhtml -I../../libserver $(DBINC) $(REGEX_IN) $(HTTPDIN)
CFLAGS		= $(OPTIMIZE_FLAGS) $(DEBUG_FLAGS) $(VERSDEF) -DEAPI $(INCLUDE_FLAGS)
LDFLAGS		= $(DEBUG_FLAGS) 
LD_FLAGS        = -L/usr/lib -R/usr/lib
FLD_FLAGS       = -L/usr/lib -Wl,-R/usr/lib

LIBUTILS	= ../../libutils/libutils.a
LIBMHTML	= ../../libmhtml/libmhtml.a
LIBSERVER	= ../../libserver/libserver.a
DESLIB		= /www/src/libdes-4.04b/libdes.a

OBJECTS		= mod_mhtml.o

REQLIBS		= $(LIBTIMESTAMP) $(LIBREGISTER) $(LIBMHTML) \
		  $(LIBUTILS) $(LIBSERVER) ###$(LIBSSL) $(HTTPDLIB)

LIBS            = -lgdbm $(LIBMHTML) $(LIBUTILS) $(LIBSERVER) $(DESLIB)

SHLIBS          = mod_mhtml.so

TARGETS		= $(SHLIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) $(DEFS) $(ENGINE_DEF) $(ENGINE_OPT) -c $<

all:	$(TARGETS) $(SUBDIRS)

$(LIBUTILS):
	(cd ../libutils; $(MAKE) $@)

$(LIBMHTML):
	(cd ../libmhtml; $(MAKE) $@)

$(LIBSERVER):
	(cd ../libserver; $(MAKE) $@)

$(ENGINE): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(REQLIBS) $(DBLIB) $(OS_LIBS)

mod_mhtml.so: mod_mhtml.c $(LIBMHTML) $(LIBUTILS) $(LIBSERVER)
	$(APXS) -c $(CFLAGS) $(INCLUDE_FLAGS) $(DEFS) $(FLD_FLAGS) $(OS_LIBS) $(LIBS) mod_mhtml.c

clean:  FORCE
	$(RM) *.o $(TARGETS)

distclean realclean: clean
	$(RM) *.cache *.log Makefile
	-(cd fcgi; make distclean)

tags TAGS: FORCE
	(cd ..; $(MAKE) $@)

FORCE:
