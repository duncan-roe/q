.PHONY: clean
CFLAGS = -Wall -Wstrict-prototypes -Wmissing-prototypes

../q/disowntty : disowntty.c Makefile
	$(CC) disowntty.c -o $@ $(CFLAGS)

clean :
	rm -f ../q/disowntty
