TESTS = check_primp
check_PROGRAMS = check_primp

check_primp_SOURCES = check_primp.c check_list.c check_primp_config.c primp_config/check_find.c check_connection_pool.c check_request.c check_resolve.c check_response.c fixtures/full.conf fixtures/deep.conf
check_primp_CFLAGS = @CHECK_CFLAGS@ -I$(srcdir)/../src -DFIXTURESDIR=\"$(srcdir)/fixtures\"
check_primp_LDADD = $(top_builddir)/src/libprimp.a @CHECK_LIBS@
