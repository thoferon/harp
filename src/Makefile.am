SUFFIXES = .c .h .y .l

AM_CFLAGS = -Wall -DSYSCONFDIR=\"${sysconfdir}\"
AM_YFLAGS = -d

BUILT_SOURCES = config_parser.h

noinst_LIBRARIES = libprimp.a
# Need to put config_lexer.l first so that config_lexer.h is generated,
# automake does not know how to deal with this file
libprimp_a_SOURCES = config_lexer.l config_parser.y options.c options.h primp_config.c primp_config.h primp_config/find.c primp_config/find.h list.c list.h connection_pool.c connection_pool.h request.c request.h response.c response.h resolve.c resolve.h memory.h memory.c worker.h worker.c loader.h loader.c log.h log.c

bin_PROGRAMS = primp
primp_SOURCES = main.c
primp_LDADD = libprimp.a

dist_primp_SOURCES = config_lexer.c config_lexer.h config_parser.c config_parser.h
# FIXME: distcheck fails if they are not cleaned
# The real problem is that even when the .h and .c files are present, it recompiles
# them inside the VPATH build directory
DISTCLEANFILES = config_lexer.c config_lexer.h config_parser.c config_parser.h
