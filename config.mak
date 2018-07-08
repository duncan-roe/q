# q configuration file

# These mostly correspond to what ./configure defaults would be
# if using autotools

# Paths for install.
# These correspond to what ./configure defaults would be if using autotools.
# Some of these are used in code - see Features below

PREFIX:=/usr/local
EPREFIX:=$(PREFIX)
BINDIR:=$(EPREFIX)/bin
DATAROOTDIR:=$(PREFIX)/share
DATADIR:=$(DATAROOTDIR)/q
DOCDIR:=$(DATAROOTDIR)/doc/q
SYSCONFDIR:=$(PREFIX)/etc
DESTDIR:=

# Features

HELP_CMD:=less
HELP_DIR:=$(DATADIR)
MACRO_DIR:=$(HELP_DIR)
ETC_DIR:=$(SYSCONFDIR)
