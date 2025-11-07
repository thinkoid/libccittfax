SUBDIRS = src examples utils tests

all: src examples

src:
	$(MAKE) -C src

examples:
	$(MAKE) -C examples

utils:
	$(MAKE) -C utils

tests:
	$(MAKE) -C tests

clean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done

realclean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d realclean; done

.PHONY: clean $(SUBDIRS)
