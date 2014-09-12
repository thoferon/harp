SUFFIXES = .c .h .y .l

AM_CFLAGS = -Wall -I${top_srcdir}/include -I${top_srcdir}/common
AM_YFLAGS = -d

BUILT_SOURCES = config_parser.h

lib_LTLIBRARIES = libharp.la
libharp_la_SOURCES = list.c config_lexer.l config_parser.y harp_config.c harp_config/write.c error.c lexer_extra.c lexer_extra.h
libharp_la_CFLAGS = $(AM_CFLAGS)
libharp_la_LDFLAGS = -version-info 1:0:0
libharp_la_LIBADD = ${top_builddir}/common/libcommon.la

EXTRA_DIST = config_lexer.c config_lexer.h config_parser.c config_parser.h
# FIXME: distcheck fails if they are not cleaned
# The real problem is that even when the .h and .c files are present, it recompiles
# them inside the VPATH build directory
DISTCLEANFILES = config_lexer.c config_lexer.h config_parser.c config_parser.h
