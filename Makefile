# default paths

include config.mak

SHELL := /bin/bash

.PHONY: q clean install install_help install_doc install_etc uninstall

q:
	cd q && $(MAKE)

clean:
	cd  q && $(MAKE) clean

install: q install_help install_doc install_etc
	mkdir -p $(DESTDIR)$(BINDIR) && \
cp --preserve=timestamps,mode q/q $(DESTDIR)$(BINDIR)

install_help:
	mkdir -p $(DESTDIR)$(HELP_DIR) && cp -a help/* $(DESTDIR)$(HELP_DIR)

install_doc:
	mkdir -p $(DESTDIR)$(DOCDIR)/q && \
cp --preserve=timestamps q/README* q/TODO $(DESTDIR)$(DOCDIR)/q/ && \
cp --preserve=timestamps q/.qrc $(DESTDIR)$(DOCDIR)/q/q_dot_qrc && \
cp --preserve=timestamps help/.qrc $(DESTDIR)$(DOCDIR)/q/help_dot_qrc && \
cp --preserve=timestamps INSTALL LICENSE README $(DESTDIR)$(DOCDIR)/q/ && \
for i in man/man*; do mkdir -p $(DESTDIR)$(MANDIR)/$$(basename $$i) &&\
cp --preserve=timestamps $$i/* $(DESTDIR)$(MANDIR)/$$(basename $$i); done

install_etc:
	mkdir -p $(DESTDIR)$(ETC_DIR) && cp -a etc/* $(DESTDIR)$(ETC_DIR)

uninstall:
	rm $(DESTDIR)$(BINDIR)/q; \
for i in help/*; do rm $(DESTDIR)$(HELP_DIR)/$$(basename $$i); done; \
rmdir $(DESTDIR)$(HELP_DIR); \
rm -r $(DESTDIR)$(DOCDIR)/q/; \
for i in etc/*; do rm $(DESTDIR)$(ETC_DIR)/$$(basename $$i); done; \
for i in man/man*; do j=$(DESTDIR)$(MANDIR)/$$(basename $$i); \
for k in $$i/*;do rm $$j/$$(basename $$k); done; done
