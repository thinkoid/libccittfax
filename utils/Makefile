# -*- mode: makefile; -*-

CURDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
TOPDIR := $(abspath $(CURDIR)/..)
include $(TOPDIR)/Makefile.common

LIBS = $(TOPDIR)/src/libccittfax.a

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

TARGETS = generate_trie mkbmp

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

%.o: %.c
	$(CC) -o $@ -c $< $(CPPFLAGS) $(CFLAGS)

%: %.o $(LIBS)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	@rm -rf $(TARGETS) $(OBJS)

realclean:
	@rm -rf $(TARGETS) $(OBJS) $(DEPENDDIR)

.PHONY: all clean realclean
