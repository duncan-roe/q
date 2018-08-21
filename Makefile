# default paths

include config.mak

SHELL := /bin/bash

.PHONY: q clean install install_help install_doc install_etc uninstall
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
cp --preserve=timestamps q/README* q/TODO $(DESTDIR)$(DOCDIR)/q/ && \
cp --preserve=timestamps q/.qrc $(DESTDIR)$(DOCDIR)/q/dot_qrc && \
cp --preserve=timestamps help/.qrc $(DESTDIR)$(DOCDIR)/help/dot_qrc

install_etc:
	mkdir -p $(DESTDIR)$(ETC_DIR) && cp -a etc/* $(DESTDIR)$(ETC_DIR)

uninstall:
	rm $(DESTDIR)$(BINDIR)/q; \
for i in help/*; do rm $(DESTDIR)$(HELP_DIR)/$$(basename $$i); done; \
rmdir $(DESTDIR)$(HELP_DIR); \
for i in q/README*; do rm $(DESTDIR)$(DOCDIR)/q/$$(basename $$i); done; \
rm $(DESTDIR)$(DOCDIR)/q/{TODO,dot_qrc}; \
rmdir $(DESTDIR)$(DOCDIR)/q; \
rm $(DESTDIR)$(DOCDIR)/help/dot_qrc; \
rmdir $(DESTDIR)$(DOCDIR)/help; \
rmdir $(DESTDIR)$(DOCDIR); \
for i in etc/*; do rm $(DESTDIR)$(ETC_DIR)/$$(basename $$i); done; \

