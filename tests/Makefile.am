# -*- mode: makefile -*-

EXTRA_DIST = 
include $(top_srcdir)/Makefile.common

LIBS += $(top_srcdir)/src/libccittfax.la

TESTS = cfd
check_PROGRAMS = cfd

cfd_SOURCES = cfd.c file.c
cfd_LDADD = $(LIBS)
