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

install: q install_help install_doc install_etc install_bin

install_bin: q
	mkdir -p $(DESTDIR)$(BINDIR) && \
cp $(CP_OPTION) bin/qm $(DESTDIR)$(BINDIR); \
cp $(CP_OPTION) q/q $(DESTDIR)$(BINDIR)

install_doc:
	mkdir -p $(DESTDIR)$(DOCDIR)/q && \
cp $(CP_OPTION) q/README* q/TODO q/DONE $(DESTDIR)$(DOCDIR)/q/ && \
cp $(CP_OPTION) q/*.gdb $(DESTDIR)$(DOCDIR)/q/ && \
cp $(CP_OPTION) ggdb $(DESTDIR)$(DOCDIR)/q/ && \
cp $(CP_OPTION) q/.qrc $(DESTDIR)$(DOCDIR)/q/q_dot_qrc && \
cp $(CP_OPTION) help/.qrc $(DESTDIR)$(DOCDIR)/q/help_dot_qrc && \
cp $(CP_OPTION) INSTALL INSTALL_OPENBSD LICENSE README $(DESTDIR)$(DOCDIR)/q/ && \
cp $(CP_OPTION) q/macro_debug_sample $(DESTDIR)$(DOCDIR)/q/ && \
for i in man/man*; do mkdir -p $(DESTDIR)$(MANDIR)/$$(basename $$i) &&\
cp $(CP_OPTION) $$i/*.? $(DESTDIR)$(MANDIR)/$$(basename $$i); done

install_etc:
	mkdir -p $(DESTDIR)$(ETC_DIR) && cp -a etc/* $(DESTDIR)$(ETC_DIR)

install_help:
	mkdir -p $(DESTDIR)$(HELP_DIR) && cp -a help/* $(DESTDIR)$(HELP_DIR)

uninstall:
	rm $(DESTDIR)$(BINDIR)/q; rm $(DESTDIR)$(BINDIR)/qm; \
for i in help/*; do rm $(DESTDIR)$(HELP_DIR)/$$(basename $$i); done; \
rmdir $(DESTDIR)$(HELP_DIR); \
rm -r $(DESTDIR)$(DOCDIR)/q/; \
for i in etc/*; do rm $(DESTDIR)$(ETC_DIR)/$$(basename $$i); done; \
for i in man/man*; do j=$(DESTDIR)$(MANDIR)/$$(basename $$i); \
for k in $$i/*.?;do rm $$j/$$(basename $$k); done; done
