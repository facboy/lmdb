ACLOCAL_AMFLAGS=-I m4 -I gl/m4

SUBDIRS = gl

dist_doc_DATA = CHANGES \
		COPYRIGHT \
		LICENSE \
		intro.doc

dist_noinst_DATA = #CodingStyle

AM_LDFLAGS=$(LT_NO_UNDEFINED)

dist_noinst_HEADERS = midl.h
dist_man1_MANS = mdb_copy.1 mdb_dump.1 mdb_load.1 mdb_stat.1

bin_PROGRAMS = mdb_stat mdb_copy mdb_dump mdb_load
lib_LTLIBRARIES = liblmdb.la
check_PROGRAMS= mtest mtest2 mtest3 mtest4 mtest5 # mtest6 wasn't included in original liblmdb Makefile

LDADD = liblmdb.la

liblmdb_la_SOURCES = lmdb.h mdb.c midl.h midl.c
liblmdb_la_CFLAGS = $(AM_CFLAGS)
liblmdb_la_LDFLAGS = -version-info $(LMDB_LIBRARY_VERSION) $(AM_LDFLAGS)
liblmdb_la_LIBADD = $(LIBPTHREAD)


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = liblmdb.pc

EXTRA_DIST = gl/m4/gnulib-cache.m4

TESTS = tests/mtest1.sh
