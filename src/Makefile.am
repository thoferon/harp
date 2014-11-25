AM_CFLAGS = -Wall -DSYSCONFDIR=\"${sysconfdir}\" -I${top_srcdir}/include -I${top_srcdir}/common

noinst_LIBRARIES = libharpapp.a
libharpapp_a_SOURCES = harp_config/find.c harp_config/find.h log.c log.h options.c options.h connection_pool.c connection_pool.h request.c request.h response.c response.h resolve.c resolve.h worker.c worker.h loader.c loader.h start.c start.h utils.c utils.h

bin_PROGRAMS = harp
harp_SOURCES = main.c
harp_LDADD = ${top_builddir}/common/libcommon.la ${top_builddir}/libharp/libharp.la libharpapp.a
