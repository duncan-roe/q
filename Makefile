# default paths

include config.mak

#SHELL := $(shell for i in /bin/bash /usr/bin/bash /usr/local/bin/bash /bin/sh;\
#                 do [ -x $$i ] && { echo $$i; break; }; done)

CP_OPTION := $(shell cp --version >/dev/null 2>&1 && \
                     echo --preserve=timestamps,mode || echo -a)

.PHONY: q clean install install_help install_doc install_etc install_bin \
  uninstall

q:
	cd q && $(MAKE)

clean:
	cd q && $(MAKE) clean

install: install_help install_doc install_etc install_bin install_debug

install_bin: q
	mkdir -p $(DESTDIR)$(BINDIR) && \
cp $(CP_OPTION) bin/qm q/q $(DESTDIR)$(BINDIR)

install_debug: q/disowntty
	mkdir -p $(DESTDIR)$(DATADIR)/debug && \
cp $(CP_OPTION) q/*.gdb q/disowntty q/ggdb q/macro_debug_sample \
$(DESTDIR)$(DATADIR)/debug/ && \
sed "s+%DATADIR+$(DATADIR)+" q/README_DEBUG.in >\
$(DESTDIR)$(DATADIR)/debug/README_DEBUG && \
cp $(CP_OPTION) q/README_DEBUG_* $(DESTDIR)$(DATADIR)/debug/

install_doc:
	mkdir -p $(DESTDIR)$(DOCDIR) && \
cp $(CP_OPTION) q/TODO q/DONE $(DESTDIR)$(DOCDIR)/ && \
cp $(CP_OPTION) q/.qrc $(DESTDIR)$(DOCDIR)/q_dot_qrc && \
cp $(CP_OPTION) help/.qrc $(DESTDIR)$(DOCDIR)/help_dot_qrc && \
cp $(CP_OPTION) INSTALL INSTALL_OPENBSD LICENSE README $(DESTDIR)$(DOCDIR)/ && \
for i in man/man*; do mkdir -p $(DESTDIR)$(MANDIR)/$$(basename $$i) &&\
cp $(CP_OPTION) $$i/*.? $(DESTDIR)$(MANDIR)/$$(basename $$i); done

install_etc:
	mkdir -p $(DESTDIR)$(ETC_DIR) && cp $(CP_OPTION) etc/* $(DESTDIR)$(ETC_DIR)

install_help:
	mkdir -p $(DESTDIR)$(HELP_DIR) && \
cp -r $(CP_OPTION) help/* $(DESTDIR)$(HELP_DIR)

uninstall:
	rm $(DESTDIR)$(BINDIR)/q $(DESTDIR)$(BINDIR)/qm; \
rm -rf $(DESTDIR)$(DATADIR) $(DESTDIR)$(HELP_DIR); \
rm -r $(DESTDIR)$(DOCDIR)/; \
for i in etc/*; do rm $(DESTDIR)$(ETC_DIR)/$$(basename $$i); done; \
for i in man/man*; do j=$(DESTDIR)$(MANDIR)/$$(basename $$i); \
for k in $$i/*.?;do rm $$j/$$(basename $$k); done; done
