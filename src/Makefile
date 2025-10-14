# -*- mode: makefile; -*-

CURDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
TOPDIR := $(abspath $(CURDIR)/..)
include $(TOPDIR)/Makefile.common

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

DEPENDDIR := ./.deps
DEPENDFLAGS = -M

TARGET = libccittfax.a

all: $(TARGET)

DEPS := $(patsubst %.o,$(DEPENDDIR)/%.d,$(OBJS))
-include $(DEPS)

DEPENDSUBDIRS := $(sort $(dir $(DEPS)))

$(DEPENDDIR)/%.d: %.c $(DEPENDDIR) $(DEPENDSUBDIRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPENDFLAGS) $< >$@

$(DEPENDDIR) $(DEPENDSUBDIRS):
	@[ ! -d $@ ] && mkdir -p $@

%: %.c

%.o: %.c
	$(CC) -o $@ -c $< $(CPPFLAGS) $(CFLAGS)

$(TARGET): $(OBJS)
	$(AR) -crv -o $@ $^

clean:
	@rm -rf $(TARGET) $(OBJS)

realclean:
	@rm -rf $(TARGET) $(OBJS) $(DEPENDDIR)

.PHONY: all clean realclean
