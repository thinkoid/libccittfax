# -*- mode: makefile; -*-

CURDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
TOPDIR := $(abspath $(CURDIR)/..)
include $(TOPDIR)/Makefile.common

LIBS = $(TOPDIR)/src/libccittfax.a

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

TARGETS = cfc cfd change trie

DEPENDDIR := ./.deps
DEPENDFLAGS = -M

all: $(TARGETS)

DEPS := $(patsubst %.o,$(DEPENDDIR)/%.d,$(OBJS))
-include $(DEPS)

DEPENDSUBDIRS := $(sort $(dir $(DEPS)))

$(DEPENDDIR)/%.d: %.c $(DEPENDDIR) $(DEPENDSUBDIRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPENDFLAGS) $< >$@

$(DEPENDDIR) $(DEPENDSUBDIRS):
	@[ ! -d $@ ] && mkdir -p $@

%: %.c

$(LIBS):
	$(MAKE) -C $(TOPDIR)/src

cfc: cfc.o image.o $(LIBS)
	$(CC) $(LDFLAGS) -o $@ $^ -lm

cfd: cfd.o file.o $(LIBS)
	$(CC) $(LDFLAGS) -o $@ $^

change: change.o $(LIBS)
	$(CC) $(LDFLAGS) -o $@ $^

trie: trie.o $(LIBS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) -o $@ -c $< $(CPPFLAGS) $(CFLAGS)

clean:
	@rm -rf $(TARGETS) $(OBJS)

realclean:
	@rm -rf $(TARGETS) $(OBJS) $(DEPENDDIR)

.PHONY: all clean realclean
