# -*- mode: makefile -*-

EXTRA_DIST = 
include $(top_srcdir)/Makefile.common

## LIBS += $(BOOST_UNIT_TEST_FRAMEWORK_LIBS) $(top_srcdir)/src/libbs.la

TESTS = test
check_PROGRAMS = test

test_SOURCES = test.c
test_LDADD = $(LIBS)
