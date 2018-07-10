# default paths

include config.mak

SHELL := /bin/bash

.PHONY: q clean install install_help install_doc install_etc
q:
	cd q && $(MAKE) \
ETC_DIR=$(ETC_DIR) \
HELP_CMD=$(HELP_CMD) \
HELP_DIR=$(HELP_DIR) \
MACRO_DIR=$(MACRO_DIR)

clean:
	cd  q && $(MAKE) clean

install: q install_help install_doc install_etc
	mkdir -p $(DESTDIR)$(BINDIR) && cp -a q/q $(DESTDIR)$(BINDIR)

install_help:
	mkdir -p $(DESTDIR)$(HELP_DIR) && cp -a help/* $(DESTDIR)$(HELP_DIR)

install_doc:
	mkdir -p $(DESTDIR)$(DOCDIR)/{q,help} && \
cp -a q/README* q/.qrc $(DESTDIR)$(DOCDIR)/q && \
cp -a help/.qrc $(DESTDIR)$(DOCDIR)/help/

install_etc:
	mkdir -p $(DESTDIR)$(ETC_DIR) && cp -a etc/* $(DESTDIR)$(ETC_DIR)
