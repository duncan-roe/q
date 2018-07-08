include ../config.mak
SRCS = $(wildcard *.c)
LIBS = -lm
OBJ = $(SRCS:.c=.o)
CPPFLAGS = $(shell getconf LFS_CFLAGS 2>/dev/null)\
-DHELP_CMD=\"$(HELP_CMD)\"\
-DHELP_DIR=\"$(HELP_DIR)\"\
-DMACRO_DIR=\"$(MACRO_DIR)\"\
-DETC_DIR=\"$(ETC_DIR)\"
CFLAGS = -g3 -ggdb\
-Wall -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wstrict-prototypes \
-Wmissing-prototypes
q : $(OBJ)
	$(CC) $^ -o $@ $(LIBS)
clean :
	rm -f *.o *.d q

# Auto dependency stuff (from info make)
%.d: %.c
	$(CC) -MM -MT $(@:.d=.o) -MT $@ $(CPPFLAGS) $< -o $@
ifneq ($(MAKECMDGOALS),clean)
-include $(SRCS:.c=.d)
endif
