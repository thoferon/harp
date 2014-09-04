check_PROGRAMS = check_memory_usage

check_memory_usage_SOURCES = check_memory_usage.c memory_tracking.h memory_tracking.c
check_memory_usage_CFLAGS = -Wall -I${top_srcdir}/include -I${top_srcdir}/common -I$(top_srcdir)/src -DTESTSMEMORYDIR=\"$(abs_srcdir)\" -static
check_memory_usage_LDADD = $(top_builddir)/src/libharpapp.a ${top_builddir}/common/libcommon.la ${top_builddir}/libharp/libharp.la
