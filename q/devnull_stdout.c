/* D E V N U L L _ S T D O U T . C */
/*
 * Copyright (C) 2012,2013 Duncan Roe
 *
 * This function switches stdout to /dev/null
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "alledit.h"
#include "c1in.h"

void
devnull_stdout()
{
  int i, j;

  if (fmode & 01000)
  {
    printf("\"fd y\" ignored (mode +v)\r\n");
  }                                /* if (fmode & 01000) */
  else
  {
/* Only switch output to /dev/null if not there already. */
/* If o/p is to /dev/null, orig_stdout has original fd */
    if (orig_stdout == -1)
    {
      do
        orig_stdout = dup(1);
      while (orig_stdout == -1 && errno == EINTR);
      if (orig_stdout == -1)
      {
        fprintf(stderr, "\r\n%s. (dup(1))\r\n", strerror(errno));
        refrsh(NULL);
        return;
      }                            /* if (orig_stdout == -1) */
      do
        i = close(1);
      while (i == -1 && errno == EINTR);
      do
        i = open("/dev/null", O_RDWR);
      while (i == -1 && errno == EINTR);
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
        do
          j = dup2(i, 1);
        while (j == -1 && errno == EINTR);
        if (j == -1)
        {
          fprintf(stderr, "\r\n%s. (dup2(%d, 1))\r\n", strerror(errno), i);
          do
            j = close(i);
          while (j == -1 && errno == EINTR);
          restore_stdout();
          return;
        }                          /* if (j == -1) */
      }                            /* if (i != 1) */
    }                              /* if (orig_stdout == -1) */
    stdinfo[stdidx].nullstdout = true;
  }                                /* if (fmode & 01000) else */
}                                  /* devnull_stdout() */
