SUBDIRS = src utils tests

all: src

src:
	$(MAKE) -C src

utils:
	$(MAKE) -C utils

tests:
	$(MAKE) -C tests

clean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done

realclean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d realclean; done

.PHONY: clean $(SUBDIRS)
