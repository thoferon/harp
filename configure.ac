AC_PREREQ([2.69])
AC_INIT([harp], [current], [tho.feron@gmail.com])
AC_CONFIG_SRCDIR([src/start.c])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall] [subdir-objects])

AC_PROG_CC
AM_PROG_AR
AC_PROG_YACC
AC_PROG_LEX

LT_INIT

AC_CHECK_HEADERS([fcntl.h inttypes.h libintl.h netdb.h stddef.h stdlib.h stdio.h string.h sys/socket.h sys/types.h unistd.h limits.h pwd.h grp.h signal.h])

AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset socket strdup])

AC_SEARCH_LIBS([backtrace], [execinfo], [
  AC_DEFINE([USE_EXECINFO], 1, [using libexecinfo])
], [
  AC_MSG_WARN([Can't find libexecinfo. You need it to debug memory leaks with check_memory_usage.])
])

AX_PTHREAD
LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

AC_ARG_ENABLE([syslog],
  [  --disable-syslog        disable logging to syslog],
  [syslog=${enableval}], [syslog=yes])

if test "${syslog}" = yes
then
  AC_DEFINE([SYSLOG_LOGGING], 1, [logging with syslog enabled])
fi

AC_CONFIG_FILES([
  Makefile
  include/Makefile
  common/Makefile
  libharp/Makefile
  src/Makefile
  tests/unit/Makefile
  tests/memory/Makefile
  tests/memory/memory_usage.conf
  doc/Makefile
])

AC_OUTPUT
