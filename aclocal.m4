dnl
dnl Meta-HTML specific tests
dnl
dnl Check wether `sig_t' is a typedef.
AC_DEFUN(AC_HAVE_TYPE_SIG_T,
[AC_REQUIRE([AC_TYPE_SIGNAL])
AC_MSG_CHECKING(whether sig_t is defined)
AC_CACHE_VAL(ac_cv_have_type_sig_t,
    [
      AC_TRY_LINK([
#include <sys/types.h>
#include <signal.h>

static void foo (void) { }
static void bar (void) { signal (SIGINT, (sig_t)foo); }
],[], ac_cv_have_type_sig_t=yes, ac_cv_have_type_sig_t=no )
    ])
AC_MSG_RESULT($ac_cv_have_type_sig_t)
if test "$ac_cv_have_type_sig_t" = yes; then
AC_DEFINE(HAVE_TYPE_SIG_T)
fi
])
dnl
dnl Check for `time_t' as a type.
AC_DEFUN(AC_HAVE_TYPE_TIME_T,[
AC_MSG_CHECKING(whether time_t is defined)
AC_CACHE_VAL(ac_cv_have_type_time_t,
    [
      AC_TRY_LINK([
	#include <sys/types.h>
	#include <time.h>
	time_t foo; ],[],
	ac_cv_have_type_time_t=yes,
	ac_cv_have_type_time_t=no )
    ])
AC_MSG_RESULT($ac_cv_have_type_time_t)
if test "$ac_cv_have_type_time_t" = yes; then
AC_DEFINE(HAVE_TYPE_TIME_T)
fi
])
