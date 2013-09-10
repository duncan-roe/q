/* R E S T O R E _ S T D O U T . C
 *
 * Copyright (C) 1993, Duncan Roe & Associates P/L
 * Copyright (C) 2012, Duncan Roe
 *
 * This routine switches output back to the original stdout
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "alledit.h"
#include "termio5.hl"
#include "c1in.h"
void
restore_stdout()
{
  int i;

/* Only switch if the original stdout is preserved */
  if (orig_stdout != -1)
  {
    if (stdidx >= 0)
      stdinfo[stdidx].nullstdout = false;
    do
      i = dup2(orig_stdout, 1);
    while (i == -1 && errno == EINTR);
    if (i == -1)
    {
      fprintf(stderr, "\r\n%s. (dup2(%d, 1))\r\n", strerror(errno),
        orig_stdout);
    }                              /* if (i == -1) */
    else
    {
      do
        i = close(orig_stdout);
      while (i == -1 && errno == EINTR);
      orig_stdout = -1;
    }                              /* if (i == -1) else */
  }                                /* if (orig_stdout != -1) */
}
