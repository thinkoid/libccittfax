# -*- mode: makefile -*-

EXTRA_DIST = 
include $(top_srcdir)/Makefile.common

LIBS += $(top_srcdir)/src/libccittfax.a

bin_PROGRAMS = cfc cfd change trie

cfc_SOURCES = cfc.c image.c
cfc_LDADD = $(LIBS) -lm

cfd_SOURCES = cfd.c file.c
cfd_LDADD = $(LIBS)

change_SOURCES = change.c
change_LDADD = $(LIBS)

trie_SOURCES = trie.c
trie_LDADD = $(LIBS)
