TESTS = check_harp
check_PROGRAMS = check_harp

check_harp_SOURCES = check_harp.c check_list.c check_harp_config.c harp_config/check_find.c harp_config/check_write.c check_connection_pool.c check_request.c check_resolve.c check_response.c check_worker.c check_loader.c fixtures/full.conf fixtures/deep.conf
check_harp_CFLAGS = @CHECK_CFLAGS@ -Wall -I${top_srcdir}/include -I${top_srcdir}/common -I$(top_srcdir)/src -DFIXTURESDIR=\"$(abs_srcdir)/fixtures\" -static
check_harp_LDADD = ${top_builddir}/common/libcommon.la ${top_builddir}/libharp/libharp.la $(top_builddir)/src/libharpapp.a @CHECK_LIBS@
