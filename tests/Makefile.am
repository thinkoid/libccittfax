# -*- mode: makefile -*-

EXTRA_DIST = 
include $(top_srcdir)/Makefile.common

LIBS += $(top_srcdir)/src/libccittfax.la

bin_PROGRAMS = mkbmp

TESTS = test
check_PROGRAMS = test

test_SOURCES = test.c
test_LDADD = $(LIBS)

mkbmp_SOURCES = mkbmp.c
mkbmp_LDADD =
