dnl Process this file with autoconf to produce a configure script. -*- Bash -*-
AC_INIT(Makefile.in)
AC_CONFIG_HEADER(config.h)

dnl make sure we are using a recent autoconf version
AC_PREREQ(2.12)

dnl Default is to install in /www/bin.
AC_PREFIX_DEFAULT(/www)

dnl canonicalize the host and os so we can do some tricky things before
dnl parsing options
AC_CANONICAL_HOST

PLATFORM="${host_cpu}-${host_os}"
AC_DEFINE_UNQUOTED(MHTML_SYSTEM_TYPE, [ "${host_cpu}-${host_os}" ])

dnl Configure defaults.
with_gdbm=yes
dnl with_perl=no
with_perl=yes
with_fcgi=yes

dnl Handle OS's that don't like GNU malloc and friends.
if test "$with_gnu_malloc" = ""; then
   case "${host_os}" in
     winnt*) with_gnu_malloc=no with_fcgi=no ;;
     linux*) with_gnu_malloc=no ;;
   freebsd*) with_gnu_malloc=no ;;
   openbsd*) with_gnu_malloc=no ;;
       aix*) with_gnu_malloc=no ;;
   solaris*) with_gnu_malloc=no ;;
       osf*) with_gnu_malloc=no ;;
          *) with_gnu_malloc=yes ;;
   esac
fi

dnl Handle OS's that can't hang with the newer and faster regex.
if test "$with_regex" = ""; then
   case "${host_os}" in
     linux*)
	if test "${host_cpu}" = "alpha"; then with_gnu_regex=yes; fi
	if test "$with_gnu_regex" != "no"; then
	   with_regex=yes;
	else
	   with_regex=no;
	fi
	;;
   solaris*) with_gnu_regex=yes ;;
       osf*) with_gnu_regex=yes ;;
     esac
fi

dnl Handle version of RPM that are different between platforms (even tho the
dnl actual rpm has an identical version)!
if test "$host_cpu" != alpha; then
   i386_RPM_ARGS='--define "_topdir $$topdir"';
else
   i386_RPM_ARGS='';
fi
AC_SUBST(i386_RPM_ARGS)

[OS_NAME=`echo "$host_os" | sed -e 's/[.0-9]*[_U]*[0-9]*$//'`]

dnl Argument declarations.
AC_ARG_WITH(gnu-malloc,
	[  --with-gnu-malloc       use the GNU version of malloc],
	with_gnu_malloc=$withval)
AC_ARG_WITH(rmalloc,
	[  --with-rmalloc          use a debugging version of malloc],
	with_rmalloc=$withval)
AC_ARG_WITH(electric-fence,
	[  --with-electric-fence   use /usr/lib/libefence.a version of malloc],
	with_electric_fence=$withval)
AC_ARG_WITH(des,
	[  --with-des              use EAY's DES library for encryption],
	with_des=$withval)
AC_ARG_WITH(msql,
	[  --with-msql             allow the use of the Minerva SQL database],
	with_msql=$withval)
AC_ARG_WITH(mysql,
	[  --with-mysql            allow the use of the MySQL database],
	with_mysql=$withval)
AC_ARG_WITH(udbc,
	[  --with-udbc             allow the use of the OpenLink UDBC driver],
	with_udbc=$withval)
AC_ARG_WITH(iodbc,
	[  --with-iodbc            allow the use of the OpenLink iODBC driver],
	with_iodbc=$withval)
AC_ARG_WITH(odbc,
	[  --with-odbc             allow the use of both OpenLink drivers],
	with_odbc=$withval)
AC_ARG_WITH(perl,
	[  --with-perl             allow the use of Perl in your pages],
	with_perl=$withval)
AC_ARG_WITH(gnu-regex,
	[  --with-gnu-regex        use older version of GNU Regex to compile],
	with_gnu_regex=$withval)
AC_ARG_WITH(regex,
	[  --with-regex            only use the negative: --with-regex=no],
	with_regex=$withval)
AC_ARG_WITH(spencer-regex,
	[  --with-spencer-regex    use the Henry Spencer version of Regex],
	with_spencer_regex=$withval)
AC_ARG_WITH(fcgi,
	[ --with-fcgi              only use the negative: --with-fcgi=no],
	with_fcgi=$withval)
AC_ARG_WITH(check-activation,
	[ --with-check-activation  check for a valid activation key when 
				   using engine, server, or mhc]
	with_check_activation=$withval)
AC_ARG_WITH(everything,
	[  --with-everything       build with EVERYTHING that we support],
	with_des=yes with_msql=yes with_mysql=yes with_pgsql=yes \
	with_udbc=yes with_fcgi=${with_fcgi:=yes} with_perl=yes)

AC_PROG_CC
AC_PROG_INSTALL
AC_PATH_PROG(FIND, find, [/usr/bin/find])
AC_PATH_PROG(BIN_PWD, pwd, [ : ])
AC_CHECK_PROGS(PERL, [perl5.00401 perl5.004 perl5.003 perl5.002 perl5 perl])
AC_PROG_RANLIB
AC_PROG_YACC

dnl Checks for header files.
AC_STDC_HEADERS
AC_HEADER_DIRENT
AC_CHECK_HEADERS(unistd.h stdarg.h string.h math.h)
AC_CHECK_HEADERS(time.h sys/time.h file.h sys/file.h)
AC_CHECK_HEADERS(sys/resource.h fcntl.h sys/fcntl.h)
AC_CHECK_HEADERS(sys/ttold.h bstring.h ioctl.h sys/ioctl.h)
AC_CHECK_HEADERS(termio.h termios.h)
AC_CHECK_HEADERS(sys/select.h crypt.h)
dnl AC_CHECK_HEADER(regex.h)
AC_HEADER_TIME

dnl Do not compile in support for FCGI if this is Windows/NT.
if test "$with_fcgi" = "yes"; then
   FCGI_ENGINE=fcgi-engine
else
   FCGI_ENGINE=""
fi
AC_SUBST(FCGI_ENGINE)

dnl Get platform information into GNU Makefiles.
BUILD_CPU="$host_cpu"
BUILD_OS="$host_os"
AC_SUBST(BUILD_CPU)
AC_SUBST(BUILD_OS)

dnl Do not compile in support for activation checking if unspecified.
dnl if test "$with_check_activation" = "yes"; then
dnl    CHECK_ACTIVATION=-DCHECK_ACTIVATION
dnl    LIBREGISTERLIB=libregister/libregister.a
dnl else
dnl    CHECK_ACTIVATION=""
dnl    LIBREGISTERLIB=""
dnl fi
dnl AC_SUBST(CHECK_ACTIVATION)
dnl AC_SUBST(LIBREGISTERLIB)

dnl A magic check here, which tries hard to determine the location of Gdbm.
if test "$with_gdbm" = ""; then with_gdbm=yes; fi
if test "$with_gdbm" = "yes"; then
GDBMLIB=$with_gdbmlib
if test "$GDBMLIB" = ""; then
  echo $ac_n "Searching for libgdbm.a... "
  for dir in $prefix/lib /usr/local/lib /opt/gnu/lib /usr/lib /lib /usr/local/gnu/lib /usr/gnu/lib; do
    if test -f $dir/libgdbm.a; then
      GDBMLIB=$dir/libgdbm.a
      SUB_SUB_GDBMLIB=$dir/libgdbm.a
      break;
    fi
  done
else
  echo $ac_n "Using libgdm.a from "
fi

if test "$GDBMLIB" = ""; then
    for dir in ../gdbm*; do
       if test -f $dir/libgdbm.a; then
	 GDBMLIB=../$dir/libgdbm.a
	 SUB_SUB_GDBMLIB=../../$dir/libgdbm.a
	 break
       fi
    done
fi
[
if test "$GDBMLIB"; then
   echo $GDBMLIB
   GDBMFIRST=`echo $GDBMLIB | sed -e 's@/[^/]*$@@'`
   GDBMFIRST="$GDBMFIRST ${GDBMFIRST}/include"
else
   echo "Not Found!  Cannot complete build."
   exit 1
fi
]

echo $ac_n "Searching for gdbm.h... "
for dir in $GDBMFIRST $prefix/include /usr/local/include /opt/gnu/include /usr/local /usr/include /usr/local/gnu/include /usr/gnu/include; do
   if test -f $dir/gdbm.h; then
     GDBM_IN=-I$dir
     SUB_SUB_GDBM_IN=-I$dir
     break
   fi
done

if test "$GDBM_IN" = ""; then
    for dir in ../gdbm*; do
       if test -f $dir/gdbm.h; then
	 GDBM_IN=-I../$dir
	 SUB_SUB_GDBM_IN=-I../../$dir
	 break
       fi
    done
fi

if test "$GDBM_IN"; then
   echo $GDBM_IN/gdbm.h
else
   echo "Not Found!  Cannot complete build."
   exit 1
fi
fi

AC_SUBST(GDBMLIB)
AC_SUBST(GDBM_IN)
AC_SUBST(SUB_SUB_GDBMLIB)
AC_SUBST(SUB_SUB_GDBM_IN)

dnl Check out the OS.  We may need to link with strange libraries.

OS_LIBS=""
if test "$OS_NAME" = "solaris"; then
   OS_NAME=Solaris
   AC_DEFINE(Solaris,1)
   echo "OS is Solaris.  Good luck..."
fi

unset NEED_LIBM
AC_CHECK_LIB(m, floor, NEED_LIBM=true)
AC_CHECK_LIB(m, frexp, NEED_LIBM=true)
AC_CHECK_LIB(m, sqrt, NEED_LIBM=true)
AC_CHECK_LIB(m, cbrt, AC_DEFINE(HAVE_CBRT, 1))
if test "$NEED_LIBM" = true; then
   OS_LIBS="$OS_LIBS -lm"
fi

AC_CHECK_FUNCS(dlopen dlsym dlclose)
if test "$ac_cv_func_dlopen" = yes; then
   MODFUNCS_O=modfuncs.o
   MODULES_DIR=modules
fi

AC_CHECK_LIB(dl, dlopen, HAVE_LIBDL=true)
if test "$HAVE_LIBDL" = true; then
  OS_LIBS="$OS_LIBS -ldl"
  MODFUNCS_O=modfuncs.o
  MODULES_DIR=modules
  AC_DEFINE(HAVE_DLOPEN,1)
fi

dnl Default extension for shared libraries.
SHARED_EXT=.so

dnl Maybe this is a system using shl_load and shl_findsym?
if test "${MODULES_DIR}" = ""; then
   AC_CHECK_LIB(dld, shl_load, HAVE_LIBDLD=true)
   if test "$HAVE_LIBDLD" = true; then
      OS_LIBS="$OS_LIBS -ldld"
      MODFUNCS_O=modfuncs.o
      MODULES_DIR=modules
      SHARED_EXT=.O
      AC_DEFINE(USE_SHL_LOAD,1)
   fi
fi

if test "$MODULES_DIR"; then
  SHARED_LD=ld
  DASH_SHARED=""
  case "$host_os" in
    sunos4*)	DASH_SHARED=-Bdynamic ;;
     linux*)	DASH_SHARED=-shared; DLLDFLAGS=-rdynamic ;;
     winnt*)	SHARED_LD=./make-dll; SHARED_EXT=.dll; DLLDFLAGS=-rdynamic ;;
       osf*)	DASH_SHARED=-shared; SHARED_LD=$CC ;;
      irix*)	DASH_SHARED=-shared; SHARED_LD=$CC ;;
      bsdi*)	DASH_SHARED=-shared; DLLDFLAGS=-rdynamic ;;
   freebsd*)	DASH_SHARED=-Bshareable; GCC_FPIC=-fpic; DLLDFLAGS=-rdynamic ;;
   openbsd*)	DASH_SHARED=-Bshareable; GCC_FPIC=-fpic; DLLDFLAGS=-rdynamic ;;
   solaris*)
      # If both the GNU compiler and linker are in use, then we need
      # to add special options in order to compile the modules.
      if test "$GCC" = "yes"; then
        GCC_FPIC=-fpic;
        if test -n "`ld -v 2>&1 | grep 'GNU ld'`"; then
	  # GNU ld
	  DASH_SHARED=-shared; DLLDFLAGS="-Xlinker -E";
	  SHARED_LD='$(CC)'
	else
	  # Sun ld
	  DASH_SHARED=-G; DLLDFLAGS="";
	  SHARED_LD=ld;	  
	fi
      else
        DASH_SHARED=-G;
	SHARED_LD='$(CC)';
      fi
      ;;
   aix*) DLLDFLAGS="-bexpall -brtl" ;;

      hpux*)	DASH_SHARED="-b -E" GCC_FPIC=-fpic DLLDFLAGS="-Xlinker -E" ;;
  esac
fi
AC_SUBST(SHARED_LD)
AC_SUBST(DASH_SHARED)
AC_SUBST(MODFUNCS_O)
AC_SUBST(MODULES_DIR)
AC_SUBST(DLLDFLAGS)
AC_SUBST(SHARED_EXT)
AC_SUBST(OS_NAME)
AC_SUBST(GCC_FPIC)

if test "$ac_cv_header_math_h" = yes; then
   MODMATH_SO=modmath${SHARED_EXT}
fi
AC_SUBST(MODMATH_SO)

dnl A magic check here, which tries hard to determine the location of libmsql.
if test "$with_msql" != "no"; then
   MSQLLIB=$with_msqllib
   [
   if test "$MSQLLIB"; then
      temp=`echo $MSQLLIB | sed -e 's@/[^/]*/[^/]*$@@'`/msql
      if test -r $temp/msql.h; then
	 MSQL_IN=-I${temp}
      fi
   fi

   if test "$MSQLLIB" = ""; then
      for dir in ../msql/targets/*/lib ../msql-3*/targets/*/lib ../msql-2*/targets/*/libd ../../../../msql/targets/*/lib ../../../../msql-3*/targets/*/lib ../../../../msql-2*/targets/*/lib; do
	 if test -f $dir/libmsql${SHARED_EXT}; then
	    MSQLLIB=../$dir/libmsql${SHARED_EXT}
	    MSQL_IN=-I`echo $MSQLLIB | sed -e 's@/[^/]*/[^/]*$@@'`/msql
	    break
	 fi
	 if test -f $dir/libmsql.a; then
	    MSQLLIB=../$dir/libmsql.a
	    MSQL_IN=-I`echo $MSQLLIB | sed -e 's@/[^/]*/[^/]*$@@'`/msql
	    break
	 fi
      done
   fi

   if test "$MSQLLIB" = ""; then
      echo $ac_n "Searching for libmsql.[so,a]... "
      for dir in $prefix/lib /usr/local/Hughes/lib /usr/local/Minerva/lib /usr/local/lib /opt/gnu/lib /usr/lib /lib /usr/local/gnu/lib /usr/gnu/lib; do
	 if test -f $dir/libmsql${SHARED_EXT}; then
	    MSQLLIB=$dir/libmsql${SHARED_EXT}
	    break;
	 fi
	 if test -f $dir/libmsql.a; then
	    MSQLLIB=$dir/libmsql.a
	    break;
	 fi
      done
   else
      echo $ac_n "Using libmsql from "
   fi

   if test "$MSQLLIB"; then
      echo $MSQLLIB
      MSQLFIRST=`echo $MSQLLIB | sed -e 's@/[^/]*/[^/]*$@@'`
      MSQLFIRST="$MSQLFIRST ${MSQLFIRST}/src ${MSQLFIRST}/msql ${MSQLFIRST}/include"
   else
      echo "Not Found:  Cannot build modmsql!"
      MSQLLIB_MISSING=true
      unset MSQLLIB
      unset MSQL_IN
      unset MODMSQL
   fi
   ]

   if test "$MSQLLIB"; then
      echo $ac_n "Searching for msql.h... "
      if test "$MSQL_IN" = ""; then
	 for dir in $MSQLFIRST $prefix/include /usr/local/Hughes/include /usr/local/Minerva/include /usr/local/include /opt/gnu/include /usr/local /usr/include /usr/local/gnu/include /usr/gnu/include; do
	    if test -f $dir/msql.h; then
	       MSQL_IN="-I${dir}"
	       break
	    fi
	 done
      fi

      if test "$MSQL_IN" = ""; then
	 for dir in ../msql*; do
	    if test -f $dir/msql.h; then
	       MSQL_IN="-I../$dir"
	       break
	    fi
	 done
      fi

      if test "$MSQL_IN"; then
	 echo $MSQL_IN/msql.h
	 MODMSQL=modmsql${SHARED_EXT}
	 MSQL_IN="$MSQL_IN -DMSQL_DATABASE"
      else
	 echo "Not Found:  Cannot build modmsql!"
	 MSQLH_MISSING=true
	 unset MSQLLIB
	 unset MSQL_IN
	 unset MODMSQL
      fi
   fi
fi
AC_SUBST(MSQLLIB)
AC_SUBST(MSQL_IN)
AC_SUBST(MODMSQL)

dnl If we have the right iODBC stuff, use it to compile the iODBC module.
if test "$with_iodbc" != "no"; then
   possibles="/usr/lib /www/src/odbcsdk/lib";
   for dir in $possibles; do
      if test -f $dir/libiodbc.a; then
	 with_iodbc=yes;
	 MODIODBC=modiodbc${SHARED_EXT}
	 MODIODBCPER=modiodbcper${SHARED_EXT}
	 IODBCLIB=$dir/libiodbc.a
	 IODBC_IN=./odbc
	 break;
      fi
   done
fi

dnl If we have the right UDBC stuff, use it to compile the UDBC module.
unset MODUDBC
unset MODUDBCPER
unset MODODBC
unset MODODBCPER
if test "$with_udbc" != "no"; then
   if test -f ./modules/odbcfuncs.c -a -f ./modules/udbc/libudbc.h; then
      # Try to find the Openlink UDBC library.
      possibles="/lib /usr/lib /usr/openlink/lib /www/src/udbcsdk/lib /www/src/sdk"
      for dir in $possibles; do
	 if test -f $dir/libudbc.a; then
	    with_udbc=yes;
	    MODUDBC=mododbc${SHARED_EXT}
	    MODUDBCPER=mododbcper${SHARED_EXT}
	    UDBCLIB="$dir/libudbc.a"
	    UDBC_IN="./udbc"
	    break;
	 fi
      done

      if test "$UDBCLIB" = ""; then
	 for dir in $possibles; do
	    if test -f $dir/libudbc.so; then
	       with_udbc=yes;
	       MODUDBC=mododbc${SHARED_EXT}
	       MODUDBCPER=mododbcper${SHARED_EXT}
	       UDBCLIB="-L$dir -ludbc"
	       UDBC_IN="./udbc"
	       break;
	    fi
	 done
      fi
   fi
fi

AC_SUBST(MODUDBC)
AC_SUBST(MODUDBCPER)
AC_SUBST(UDBCLIB)
AC_SUBST(UDBC_IN)

AC_SUBST(MODIODBC)
AC_SUBST(MODIODBCPER)
AC_SUBST(IODBCLIB)
AC_SUBST(IODBC_IN)

dnl A magic check here, which tries hard to determine the location of
dnl libmysqlclient.(a,so)
if test "$with_mysql" != "no"; then
   echo $ac_n "Searching for libmysqlclient... "
   MYSQLLIB=$with_mysqllib
   [
   if test "$MYSQLLIB"; then
      temp=`echo $MYSQLLIB | sed -e 's@/[^/]*/[^/]*$@@'`/mysql
      if test -r $temp/mysql.h; then
	 MYSQL_IN=-I${temp}
      fi
   fi

   if test "$MYSQLLIB" = ""; then
      for dir in ../mysql-3*; do
	 if test -f $dir/libmysql/.libs/libmysqlclient${SHARED_EXT}; then
	    MYSQLLIB=../$dir/libmysql/.libs/libmysqlclient${SHARED_EXT}
	    MYSQL_IN=-I../$dir/include
	    break
	 fi
	 if test -f $dir/client/libmysqlclient${SHARED_EXT}; then
	    MYSQLLIB=../$dir/client/.libs/libmysqlclient${SHARED_EXT}
	    MYSQL_IN=-I../$dir/include
	    break
	 fi
	 if test -f $dir/client/.libs/libmysqlclient${SHARED_EXT}; then
	    MYSQLLIB=../$dir/client/libmysqlclient${SHARED_EXT}
	    MYSQL_IN=-I../$dir/include
	    break
	 fi
	 if test -f $dir/client/libmysqlclient.a; then
	    MYSQLLIB=../$dir/client/libmysqlclient.a
	    MYSQL_IN=-I../$dir/include
	    break
	 fi
      done
   fi

   if test "$MYSQLLIB" = ""; then
      for dir in $prefix/lib /usr/local/lib /opt/gnu/lib /usr/lib /lib /usr/local/gnu/lib /usr/gnu/lib /usr/lib/mysql /usr/local/lib/mysql; do
	 if test -f $dir/libmysqlclient${SHARED_EXT}; then
	    MYSQLLIB=$dir/libmysqlclient${SHARED_EXT}
	    break;
	 fi
	 if test -f $dir/libmysqlclient.a; then
	    MYSQLLIB=$dir/libmysqlclient.a
	    break;
	 fi
      done
   fi

   if test "$MYSQLLIB"; then
      echo $MYSQLLIB
      MYSQLFIRST=`echo $MYSQLLIB | sed -e 's@/[^/]*/[^/]*$@@'`
      MYSQLFIRST="$MYSQLFIRST ${MYSQLFIRST}/include"
   else
      MYSQLLIB_MISSING=true
      echo "Not found!"
      unset MYSQLLIB
      unset MYSQL_IN
      unset MODMYSQL
      unset MODMYSQLPER
   fi
   ]

   if test "$MYSQLLIB"; then
      echo $ac_n "Searching for mysql.h... "
      if test "$MYSQL_IN" = ""; then
	 for dir in $MYSQLFIRST $prefix/include /usr/local/include /opt/gnu/include /usr/local /usr/include /usr/local/gnu/include /usr/gnu/include; do
	    if test -f $dir/mysql/mysql.h; then
	       MYSQL_IN="-I${dir}/mysql"
	       break
	    fi
	    if test -f $dir/mysql.h; then
	       MYSQL_IN="-I${dir}"
	       break
	    fi
	 done
      fi

      if test "$MYSQL_IN" = ""; then
	 for dir in ../mysql-3*/include; do
	    if test -f $dir/mysql.h; then
	       MYSQL_IN="-I../$dir"
	       break
	    fi
	 done
      fi

      if test "$MYSQL_IN"; then
	 echo ${MYSQL_IN}/mysql.h
	 MODMYSQL=modmysql${SHARED_EXT}
	 MODMYSQLPER=modmysqlper${SHARED_EXT}
	 MYSQL_IN="$MYSQL_IN -DMYSQL_DATABASE"
      else
	 echo "Not Found -- Not building!"
	 MYSQLH_MISSING=true
	 unset MYSQLLIB
	 unset MYSQL_IN
	 unset MODMYSQL
	 unset MODMYSQLPER
      fi
   fi
   
   if test "$OS_NAME" = "Solaris" && test -n "`echo ${MYSQLLIB} | grep libmysqlclient\.so`"; then
     mysqllibpath=`echo ${MYSQLLIB} | sed -e 's/\/libmysq.*//'`;
     if test "`echo $mysqllibpath | sed -e 's/\(.\).*/\1/'`" != "/"; then 
       echo $ac_n "WARNING: Path to libmysqlclient is not absolute!"
     fi
     LDFLAGS="$LDFLAGS -R$mysqllibpath -L/usr/ucblib -R/usr/ucblib"
   fi  
fi
AC_SUBST(MYSQLLIB)
AC_SUBST(MYSQL_IN)
AC_SUBST(MODMYSQL)
AC_SUBST(MODMYSQLPER)

dnl Yet another magic check, which tries hard to find the location of libdes.a.
DESLIB=$with_deslib

if test "$DESLIB" = ""; then
   echo $ac_n "Searching for libdes.a... "
   for dir in ../libdes*; do
      if test -f $dir/libdes.a; then
	 DESLIB=../$dir/libdes.a
	 SUB_SUB_DESLIB=../../$dir/libdes.a
	 break
      fi
   done
fi

if test "$DESLIB" = ""; then
   for dir in $prefix/lib /www/src/libdes-* /www/src/libdes /usr/local/lib /opt/gnu/lib /usr/lib /lib /usr/local/gnu/lib /usr/gnu/lib; do
      if test -f $dir/libdes.a; then
	 DESLIB=$dir/libdes.a
	 SUB_SUB_DESLIB=$dir/libdes.a
	 break;
      fi
   done
else
   echo $ac_n "Using libdes.a from "
fi


[
if test "$DESLIB"; then
   echo $DESLIB
   DESFIRST=`echo $DESLIB | sed -e 's@/[^/]*$@@'`
   DESFIRST="$DESFIRST ${DESFIRST}/include"
else
   unset DESLIB
   unset SUB_SUB_DESLIB
  if test "$with_des" = "yes"; then
    echo "Not Found!  Cannot build with strong encryption."
    exit 1;
  fi
fi
]
if test "$DESLIB"; then
   echo $ac_n "Searching for des.h... "
   for dir in $DESFIRST $prefix/include /usr/local/include /opt/gnu/include /usr/local /usr/include /usr/local/gnu/include /usr/gnu/include; do
      if test -f $dir/des.h; then
	 DES_IN=-I$dir
	 break
      fi
   done

   if test "$DES_IN" = ""; then
      for dir in ../libdes*; do
	 if test -f $dir/des.h; then
	    DES_IN=-I../$dir
	    SUB_SUB_DES_IN=-I../../$dir
	    break
	 fi
      done
   fi

   if test "DES_IN"; then
      echo $DES_IN/des.h
      CRYPTFUNCS_O=cryptfuncs.o
      DES_IN="$DES_IN -DMHTML_CRYPTOGRAPHY"
   else
      unset DESLIB
      unset DES_IN
      unset SUB_SUB_DESLIB
      unset SUB_SUB_DES_IN
      unset CRYPTFUNCS_O
      if test "$with_des" = "yes"; then
	 echo "Not Found!  Cannot complete build."
	 exit 1
      fi
   fi
fi

AC_SUBST(DESLIB)
AC_SUBST(DES_IN)
AC_SUBST(SUB_SUB_DESLIB)
AC_SUBST(SUB_SUB_DES_IN)
AC_SUBST(CRYPTFUNCS_O)

dnl Another magic check here, which looks hard for Bash.
BASH_SHELL=bash
for dir in /bin /usr/bin /usr/contrib/bin /usr/local/bin /usr/gnu/bin /opt/gnu/bin /usr/local/gnu/bin /usr/local/gnubin; do
   if test -f $dir/bash; then
      BASH_SHELL=$dir/bash
      break;
   fi
done
AC_SUBST(BASH_SHELL)

dnl Check for Perl and the Perl core directory.
if test "$with_perl" != "no" && test -n "$PERL"; then
   cant_build_perl="";

   if test "$MODFUNCS_O" = ""; then
      echo "NO DYNAMIC MODULES SUPPORT, SO NO PERL WILL BE BUILT!"
      cant_build_perl=true;
   else
      if $PERL -e 'require(5.002)' > /dev/null 2>&1; then
	 echo "Using Perl binary $PERL"
      else
	 echo "PERL NOT FOUND, SO NO PERL SUPPORT WILL BE BUILT!"
	 cant_build_perl=true;
      fi
   fi
   if test "$cant_build_perl" = ""; then
      PERL_ARCH=`$PERL -MConfig -e 'print "$Config{archlib}\n"'`
      if test \! -r $PERL_ARCH/CORE/EXTERN.h || test \! -r $PERL_ARCH/CORE/libperl.a; then
	 echo "$PERL_ARCH does not look like a valid architecture-include directory!"
	 echo "SO, NO PERL SUPPORT WILL BE BUILT!"
	 cant_build_perl=true;
      elif test \! -r $PERL_ARCH/auto/DynaLoader/DynaLoader.a; then
	 echo "$PERL_ARCH does not seem to have DynaLoader built in it!"
	 echo "Without DynaLoader, NO PERL SUPPORT WILL BE BUILT!"
	 cant_build_perl=true;
      else
	 echo "Using Perl architecture includes in $PERL_ARCH"
      fi
   fi
   if test "$cant_build_perl" = ""; then
      PERL_INC="-I$PERL_ARCH/CORE"
      PERL_LIB="$PERL_ARCH/auto/DynaLoader/DynaLoader.a"
      PERL_LIB="$PERL_LIB $PERL_ARCH/CORE/libperl.a -lm"
      MODPERL_SO=modperl.so
   fi
fi
AC_SUBST(PERL_ARCH)
AC_SUBST(PERL_INC)
AC_SUBST(PERL_LIB)
AC_SUBST(MODPERL_SO)

dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_SETPGRP
AC_FUNC_VFORK
AC_CHECK_FUNCS(getcwd memmove strerror strchr strcasecmp strdup flock qsort)
AC_CHECK_FUNCS(getpwnam getgrent setuid seteuid setgid setsid)
AC_CHECK_FUNCS(srandom random initstate setstate rand srand)
AC_CHECK_FUNCS(setlocale mkfifo strstr ftime)

dnl DO NOT USE AC_CHECK_FUNC (the non-plural version) It does not define
dnl the HAVE_FUNCNAME by default.  Thanks.

AC_CHECK_FUNCS(gethostbyname)
if test $ac_cv_func_gethostbyname = no; then
    AC_CHECK_LIB(nsl, gethostbyname)
fi

AC_CHECK_FUNCS(connect)
if test $ac_cv_func_connect = no; then
    AC_CHECK_LIB(socket, connect)
fi

AC_CHECK_FUNCS(crypt)
if test $ac_cv_func_crypt = no; then
    AC_CHECK_LIB(crypt, crypt)
fi

OS_LIBS="$LIBS $OS_LIBS"
AC_SUBST(OS_LIBS)

dnl Check for specific types.
AC_TYPE_SIZE_T
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_C_CONST
AC_HAVE_TYPE_SIG_T
AC_HAVE_TYPE_TIME_T

LIBCOMPAT=compat/libcompat.a
COMPAT_H=compat/compat.h

AC_SUBST(LIBCOMPAT)
AC_SUBST(COMPAT_H)

(cd libutils; rm -rf regex)
if test "$with_gnu_regex" = yes; then
   (cd libutils; cp -r REGEX-GNU regex)
else
   if test "$with_spencer_regex" = yes; then
      (cd libutils; cp -r REGEX-SPENCER regex)
   fi
fi

if test "$with_regex" = ""; then with_regex=yes; fi
if test "$ac_cv_header_regex_h" != yes -o "$with_regex" = yes -o "$with_gnu_regex" = yes -o "$with_spencer_regex" = yes; then
   LIBREGEX=regex/libregex.a
   REGEX_H=regex/regex.h
   REGEX_IN=-I../libutils/regex
fi


if test -f /bin/sh.exe; then
   EXE=".exe"
   if test "$DESLIB" = "../../libdes/libdes.a"; then
      NTLIBS=../$DESLIB;
   elif test "$DESLIB"; then
      NTLIBS=$DESLIB;
   fi
fi
AC_SUBST(MHTML_SYSTEM_TYPE)
AC_SUBST(PLATFORM)
AC_SUBST(EXE)
AC_SUBST(NTLIBS)
AC_SUBST(LIBREGEX)
AC_SUBST(REGEX_H)
AC_SUBST(REGEX_IN)

if test "$with_gnu_malloc" = ""; then with_gnu_malloc=yes; fi
if test "$with_gnu_malloc" = yes; then
   LIBMALLOC=malloc/libmalloc.a
fi

if test "$with_rmalloc" = yes; then
   LIBMALLOC=rmalloc/librmalloc.a
   AC_DEFINE(COMPILING_WITH_RMALLOC,1)
fi

if test "$with_electric_fence" = yes; then
   echo $ac_n "Searching for libefence... "
   unset LIBEFENCE
   for dir in /lib /usr/lib /usr/local/lib; do
      if test -r $dir/libefence.a; then
	 LIBEFENCE=$dir/libefence.a
	 break
      fi
   done

   if test "$LIBEFENCE"; then
      echo "$LIBEFENCE"
      LIBMALLOC=$LIBEFENCE
   else
      echo "NOT FOUND!"
      echo ""
      echo "  WE CAN'T USE Electric Fence because you don't have the library"
      echo "  libefence.a installed in /lib, /usr/lib, or /usr/local/lib!"
      echo ""
   fi
fi

AC_SUBST(LIBMALLOC)

WEBBASEDIR=$with_webbasedir
AC_SUBST(WEBBASEDIR)

if test "$with_readline" = ""; then with_readline=yes; fi
if test "$with_readline" = "yes"; then
   AC_CHECK_LIB(readline, readline, LIBREADLINE=-lreadline)
   if test -f /usr/lib/libreadline.a ; then
     echo ""
     echo "You have the GNU Readline library already installed in your"
     echo "system.  However, since it was found in the standard location,"
     echo "we assume that it has some known broken behavior, and we aren't"
     echo "going to use it.  We are building with the local version instead."
     echo ""
     unset LIBREADLINE
   fi
   if test "$LIBREADLINE" = ""; then
      LIBREADLINE=readline/libreadline.a
      LIBHISTORY=readline/libhistory.a
   fi
   AC_CHECK_LIB(termcap, tgetflag, LIBTERMCAP=-ltermcap)
   if test "$LIBTERMCAP" = ""; then
      AC_CHECK_LIB(curses, tgetflag, LIBTERMCAP=-lcurses)
   fi
   if test "$LIBTERMCAP" = ""; then
      AC_CHECK_LIB(ncurses, tgetflag, LIBTERMCAP=-lncurses)
   fi
   if test "$LIBTERMCAP" = ""; then
      if test "$LIBREADLINE" = "readline/libreadline.a"; then
	 LIBTERMCAP=-ltermcap
      fi
   fi
fi
AC_SUBST(LIBREADLINE)
AC_SUBST(LIBHISTORY)
AC_SUBST(LIBTERMCAP)

(cd libutils;
 if test -f Makefile.in-SHIPPED; then cp Makefile.in-SHIPPED Makefile.in; fi;)

if test "$with_gnu_regex" = "yes"; then
 (cd libutils;
  cat Makefile.in-SHIPPED | sed -e 's/#gnu-regex //' |
  sed -e 's/^.*#new-regex//' >Makefile.in);
  output_extra_file=libutils/regex/Makefile;
  EXTRAINC=-I../libutils/regex
else
   if test "$with_spencer_regex" = "yes"; then
      (cd libutils;
      cat Makefile.in-SHIPPED | sed -e 's/#spencer-regex //' |
      sed -e 's/^.*#new-regex//' >Makefile.in);
      output_extra_file=libutils/regex/Makefile;
      EXTRAINC=-I../libutils/regex
   fi
fi
AC_SUBST(EXTRAINC)

if test -f ./modules/machine/Makefile.in; then
   machine_makefiles="modules/machine/Makefile"
   MODMACHINE="modmachine${SHARED_EXT}"
else
   MODMACHINE=""
   machine_makefiles=""
fi

if test -f ./modules/compiler/Makefile.in; then
   compiler_makefiles="modules/compiler/Makefile"
   MODCOMPILER=modcompiler${SHARED_EXT}
   MKLIB_COMPILE_FLAG=--compile=../modules/modcompiler.so
else
   MODCOMPILER=""
   MKLIB_COMPILE_FLAGS=""
   compiler_makefiles=""
fi
AC_SUBST(MODCOMPILER)
AC_SUBST(MODMACHINE)
AC_SUBST(MKLIB_COMPILE_FLAG)
AC_SUBST(LDFLAGS)

utility_makefiles="utilities/Makefile utilities/imagemap/Makefile"
utility_makefiles="$utility_makefiles utilities/mkpass/Makefile"
libutil_makefiles="libutils/Makefile libutils/compat/Makefile"
session_makefiles="sessions/Makefile"

AC_OUTPUT(Makefile mhc/Makefile server/Makefile engine/Makefile \
	  engine/fcgi/Makefile engine/fcgi/libfcgi/Makefile \
	  $output_extra_file mdb/Makefile mdb/readline/Makefile \
	  ${utility_makefiles} ${libutil_makefiles} ${session_makefiles} \
	  dbutils/Makefile libmhtml/Makefile libserver/Makefile \
	  modules/libgd/Makefile modules/Makefile $compiler_makefiles \
	  $machine_makefiles)

if test "$MODCOMPILER"; then
   echo ""
   echo "We will build the Meta-HTML Compiler. (includes $MODMACHINE)..."
fi

if test "$MODMACHINE" != "" -a ${MODCOMPILER} = ""; then
   echo ""
   echo "We will build the Meta-HTML Virtual Machine ($MODMACHINE))..."
fi

if test "$CRYPTFUNCS_O"; then
   echo ""
   echo "We will build strong (3DES) encryption into Meta-HTML, using:"
   echo "   Library: $DESLIB"
   echo "   Headers: $DES_IN"
fi

if test "$MODPERL"; then
   echo ""
   echo "We will build in intrinsic support for Perl ($MODPERL) using:"
   echo "   Library: $PERL_LIB"
   echo "   Headers: $PERL_INC"
fi
   
if test "$MODMYSQL"; then
   echo ""
   echo "We will build the MySQL modules ($MODMYSQL, $MODMYSQLPER) using:"
   echo "   Library: $MYSQLLIB"
   echo "   Headers: $MYSQL_IN"
else
   if test "MYSQLLIB_MISSING"; then
      echo "We are not building the MySQL module due to no libmysqlclient.so"
   else
      echo "We are not building the MySQL module due to no mysql.h"
  fi
fi

if test "$MODMSQL"; then
   echo ""
   echo "We will build the mSQL ($MODMSQL) module, using:"
   echo "   Library: $MSQLLIB"
   echo "   Headers: $MSQL_IN"
else
   if test "MSQLLIB_MISSING"; then
      echo "We are not building the mSQL module due to no libmsql.[so,a]"
   else
      echo "We are not building the mSQL module due to no msql.h"
  fi
fi

if test "$MODIODBC"; then
   echo ""
   echo "We will build OpenLink iODBC support ($MODIODBC) module, using:"
   echo "   Library: $IODBCLIB"
   echo "   Headers: $IODBC_IN"
else
   echo ""
   echo "We are not building in OpenLink iODBC support."
fi

if test "$MODUDBC"; then
   echo ""
   echo "We will build in OpenLink UDBC support ($MODUDBC, $MODUDBCPER) using:"
   echo "   Library: $UDBCLIB"
   echo "   Headers: $UDBC_IN"
else
   echo ""
   echo "We are not building in OpenLink UDBC support."
fi
