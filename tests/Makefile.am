# -*- mode: makefile -*-

EXTRA_DIST = 
include $(top_srcdir)/Makefile.common

LIBS += $(top_srcdir)/src/libccittfax.la

bin_PROGRAMS = cfc cfd

cfc_SOURCES = cfc.c image.c
cfc_LDADD = $(LIBS) -lm

cfd_SOURCES = cfd.c file.c
cfd_LDADD = $(LIBS)
