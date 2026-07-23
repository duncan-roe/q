# default paths
include config.mak

#SHELL := $(shell for i in /bin/bash /usr/bin/bash /usr/local/bin/bash /bin/sh;\
#do [ -x $$i ] && { echo $$i; break; }; done)

.PHONY: q clean install install_help install_doc install_etc install_bin \
  uninstall install_debug

q:
	cd q && $(MAKE)

clean:
	cd q && $(MAKE) clean

install: install_help install_doc install_etc install_bin install_debug

install_bin: q
	mkdir -p $(DESTDIR)$(BINDIR) && \
cp bin/qm q/q $(DESTDIR)$(BINDIR)

install_debug: q/disowntty
	mkdir -p $(DESTDIR)$(DATADIR)/debug && \
(cd q; for i in *.gdb disowntty ggdb macro_debug_sample pipe_demo.txt; do \
cp $$i $(DESTDIR)$(DATADIR)/debug; done; \
sed "s+%DATADIR+$(DATADIR)+" README_DEBUG.in >\
$(DESTDIR)$(DATADIR)/debug/README_DEBUG; \
cp README_DEBUG_PIPE.in $(DESTDIR)$(DATADIR)/debug/README_DEBUG_PIPE; \
for i in README*; do [ $$(basename $$i .in) = $$i ] && [ ! -r $$i.in ] && \
cp $$i $(DESTDIR)$(DATADIR)/debug; done || :); \
mkfifo -m666 $(DESTDIR)$(DATADIR)/debug/fifo || :

install_doc:
	mkdir -p $(DESTDIR)$(DOCDIR); \
(cd q; cp TODO DONE $(DESTDIR)$(DOCDIR); \
cp .qrc $(DESTDIR)$(DOCDIR)/q_dot_qrc); \
cp help/.qrc $(DESTDIR)$(DOCDIR)/help_dot_qrc; \
cp INSTALL INSTALL_OPENBSD LICENSE README $(DESTDIR)$(DOCDIR); \
for i in man/man*; do mkdir -p $(DESTDIR)$(MANDIR)/$$(basename $$i); \
cp $$i/*.? $(DESTDIR)$(MANDIR)/$$(basename $$i); done

install_etc:
	mkdir -p $(DESTDIR)$(ETC_DIR); cp etc/* $(DESTDIR)$(ETC_DIR)

install_help:
	mkdir -p $(DESTDIR)$(HELP_DIR); \
cp -r help/* $(DESTDIR)$(HELP_DIR)

uninstall:
	rm $(DESTDIR)$(BINDIR)/q $(DESTDIR)$(BINDIR)/qm; \
rm -rf $(DESTDIR)$(DATADIR) $(DESTDIR)$(HELP_DIR); \
rm -r $(DESTDIR)$(DOCDIR)/; \
for i in etc/*; do rm $(DESTDIR)$(ETC_DIR)/$$(basename $$i); done; \
for i in man/man*; do j=$(DESTDIR)$(MANDIR)/$$(basename $$i); \
for k in $$i/*.?;do rm $$j/$$(basename $$k); done; done

q/disowntty:
	cd disowntty && $(MAKE)
