/* C 1 I N 5 . C */
/*
 * Copyright (C) 1993, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2013 Duncan Roe
 *
 * This routine gets the next character from standard input. If we
 * hit EOF on a file, revert to the TTY
 *
 */
#include <stdio.h>
#include "termio5.hl"
#include "alledit.h"
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include "c1in.h"

char
c1in5(bool *eof_encountered)
{
  int s;
  unsigned char c;

  if (eof_encountered != NULL)
    *eof_encountered = false;

/* If we have no characters to return, then get some */

  if (!buf5len)
  {
    for (;;)
    {
/* If the screen size has changed, re-size and refresh... */
      if (seenwinch)
      {
        seenwinch = false;
        setwinsz(1);
        refrsh(NULL);
      }                            /* if(seenwinch) */
      s = read(STDIN5FD, buf5, 1);
      if (s < 0)                   /* Some kind of error */
      {
        if (errno != EINTR)
        {
          perror("Blocked read");
          putchar('\r');
/* LATER Get out (atexit() will reset terminal) */
/* LATER exit(1); */
        }                          /* if(errno!=EINTR) */
        continue;                  /* for(;;) */
      }                            /* if(s<0) */
      if (!s)                      /* EOF from a file (we hope!) */
      {
        if (USING_FILE)
        {
          if (eof_encountered != NULL)
          {
            *eof_encountered = true;
            return '\n';
          }                        /* if (eof_encountered != NULL) */
          restore_stdout();
          printf("EOF reached on U-use file (it didn't end w/a \"Z\")\r\n> ");
          pop_stdin();
        }                          /* if (USING_FILE) */

/* read(2) seems to drop thru every 1/2 sec in X, so... */

        else
          puts("!! EOF encountered unexpectedly!!\r");
        continue;                  /* for(;;) */
      }                            /* if(!s) */
      buf5len = s;
      buf5idx = 0;
      break;                       /* for(;;) */
    }                              /* for(;;) */
  }                                /* if(!buf5len) */
  c = buf5[buf5idx++];
  if (USING_FILE)
    showchar(c);
  if (buf5idx == buf5len)
    buf5len = 0;
  return c;
}                                  /* c1in5() */
