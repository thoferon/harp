AC_PREREQ([2.69])
AC_INIT([primp], [0.1], [tho.feron@gmail.com])
AC_CONFIG_SRCDIR([src/primp_config.c])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIRS([m4])

AM_INIT_AUTOMAKE([-Wall] [subdir-objects])

AC_PROG_CC
AC_PROG_RANLIB
AM_PROG_AR
AC_PROG_YACC
AC_PROG_LEX

AC_CHECK_HEADERS([fcntl.h inttypes.h libintl.h malloc.h netdb.h stddef.h stdlib.h string.h sys/socket.h unistd.h])

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

AX_PTHREAD
LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

AC_ARG_ENABLE([syslog],
        [  --disable-syslog        disable logging to syslog],
        [syslog=${enableval}], [syslog=yes])

if test "x${syslog}" = xyes
then
        AC_DEFINE([SYSLOG_LOGGING], 1, [logging with syslog enabled])
fi

AC_CONFIG_FILES([
        Makefile
        src/Makefile
        tests/Makefile
])

AC_OUTPUT
