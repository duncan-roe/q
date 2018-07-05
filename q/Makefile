ETCDIR:=/etc
.PHONY: clean
SRCS = $(wildcard *.c)
LIBS = -lm
OBJ = $(SRCS:.c=.o)
CPPFLAGS = $(shell getconf LFS_CFLAGS 2>/dev/null)\
-DETCDIR=\"$(ETCDIR)\"
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
