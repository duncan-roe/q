.PHONY: all clean
all: q README_DEBUG
include ../config.mak
SRCS = $(wildcard *.c)
LIBS = -lm -pthread
OBJ = $(SRCS:.c=.o)
CPPFLAGS = $(shell getconf LFS_CFLAGS 2>/dev/null)\
-DHELP_CMD=\"$(HELP_CMD)\"\
-DHELP_DIR=\"$(HELP_DIR)\"\
-DMACRO_DIR=\"$(MACRO_DIR)\"\
-DETC_DIR=\"$(ETC_DIR)\"
CFLAGS := $(CFLAGS) -g3 -gdwarf-4\
-Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wstrict-prototypes \
-Wmissing-prototypes -Wstrict-aliasing
q : $(OBJ)
	$(CC) $^ -o $@ $(LIBS)
clean :
	rm -f *.o *.d q README_DEBUG

# Auto dependency stuff (from info make)
%.d: %.c
	$(CC) -MM -MT $(@:.d=.o) -MT $@ $(CPPFLAGS) $< -o $@
ifneq ($(MAKECMDGOALS),clean)
-include $(SRCS:.c=.d)
endif

README_DEBUG: README_DEBUG.in Makefile
	sed -e 's+%DATADIR+$$DATADIR+' -e s'/\$$\(type -p q\)/q/' README_DEBUG.in > README_DEBUG
