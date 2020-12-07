/* D E V N U L L _ S T D O U T . C */
/*
 * Copyright (C) 2012-2014,2019 Duncan Roe
 *
 * This function switches stdout to /dev/null
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "prototypes.h"
#include "fmode.h"
#include "c1in.h"

void
devnull_stdout()
{
  int i, j;

  if (fmode & FM_PLUS_V_BIT)
  {
    fputs("\"fd y\" ignored (mode +v)\r\n", stdout);
  }                                /* if (fmode & FM_PLUS_V_BIT) */
  else
  {
/* Only switch output to /dev/null if not there already. */
/* If o/p is to /dev/null, orig_stdout has original fd */
    if (orig_stdout == -1)
    {
      SYSCALL(orig_stdout, dup(1));
      if (orig_stdout == -1)
      {
        fprintf(stderr, "\r\n%s. (dup(1))\r\n", strerror(errno));
        refrsh(NULL);
        return;
      }                            /* if (orig_stdout == -1) */
      SYSCALL(i, close(1));
      SYSCALL(i, open("/dev/null", O_RDWR));
      if (i == -1)
      {
        fprintf(stderr, "\r\n%s. /dev/null (open)\r\n", strerror(errno));
        refrsh(NULL);
        restore_stdout();
        return;
      }                            /* if (i == -1) */

/* Verify we opened fd 1. Attempt to rectify if not */
      if (i != 1)
      {
        SYSCALL(j, dup2(i, 1));
        if (j == -1)
        {
          fprintf(stderr, "\r\n%s. (dup2(%d, 1))\r\n", strerror(errno), i);
          SYSCALL(j, close(i));
          restore_stdout();
          return;
        }                          /* if (j == -1) */
      }                            /* if (i != 1) */
    }                              /* if (orig_stdout == -1) */
    stdinfo[stdidx].nullstdout = true;
  }                                /* if (fmode & FM_PLUS_V_BIT) else */
}                                  /* devnull_stdout() */
