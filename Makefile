# default paths

ETCDIR:=/etc
DESTDIR:=

.PHONY: q clean
q:
	cd q && $(MAKE) ETCDIR=$(ETCDIR)
clean:
	cd  q && $(MAKE) clean
