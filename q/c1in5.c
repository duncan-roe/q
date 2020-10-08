/* C 1 I N 5 . C */
/*
 * Copyright (C) 1993, Duncan Roe & Associates P/L
 * Copyright (C) 2012-2014,2019-2020 Duncan Roe
 *
 * This routine gets the next character from standard input. If we
 * hit EOF on a file, revert to the TTY
 */

/* Headers */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include "prototypes.h"
#include "c1in.h"

/* Instantiate externals */

const char *const normal_end_sequence = "\033\033fq\n";
const char *end_seq;
int buf5len, buf5idx, ttyfd, stdidx;
int stdbase = 0;
bool size5 = false;
struct stdinfo stdinfo[OPEN_MAX];

/* Static variables */

static char buf5[BUF5MAX];

char
c1in5(bool *eof_encountered)
{
  int s;
  uint8_t c;

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

/* If simulating fq command for -o cmdline option, do it. */
      if (simulate_q)
      {
/* Send enough characters to quit out of anything. */
/* But in case it's not enough, send it repeatedly. */
        if (simulate_q_idx >= strlen(end_seq))
          simulate_q_idx = 0;
        return end_seq[simulate_q_idx++];
      }                            /* if (simulate_q) */

      SYSCALL(s, read(STDIN5FD, buf5, 1));
      if (s == -1)                 /* Some kind of error */
      {
        fprintf(stderr, "%s. fd %d (read)\r\n", strerror(errno), STDIN5FD);
/* Get out (atexit() will reset terminal) */
        exit(1);
      }                            /* if (s == -1) */
      if (!s)                      /* EOF from a file (we hope!) */
      {
        if (USING_FILE)
        {
          if (eof_encountered != NULL)
          {
            *eof_encountered = true;
            return '\n';
          }                        /* if (eof_encountered != NULL) */
          fprintf(stderr, "Internal Q error at %s:%d\r\n", __FILE__, __LINE__);
          exit(1);
        }                          /* if (USING_FILE) */
        else
        {
          fprintf(stderr, "%s\r\n", "!! EOF encountered unexpectedly!!");
          exit(1);
        }                          /* if (USING_FILE) else */
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
