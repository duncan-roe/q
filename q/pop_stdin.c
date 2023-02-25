/* >%---- CODE_STARTS ./pop_stdin.c */
/* P O P _ S T D I N . C */
/*
 * Copyright (C) 1993, Duncan Roe & Associates P/L
 * Copyright (C) 2012-2013,2019-2020 Duncan Roe
 *
 * This function pops stdin 1 level
 */
/* >%---- KEEP2HERE ./pop_stdin.c */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "prototypes.h"
#include "c1in.h"
/* >%---- CUT_HERE ./pop_stdin.c */

bool
pop_stdin()
{
  int retcod;
  bool devnull_currently;

  if (!USING_FILE)
  {
    fputs("Only allowed from U-Use file", stdout);
    return false;
  }                                /* if (!USING_FILE) */

  devnull_currently = stdinfo[stdidx].nullstdout;

  SYSCALL(retcod, dup2(stdinfo[stdidx].funit, STDIN5FD));
  if (retcod == -1)
  {
    fprintf(stderr, "\r\n%s. (dup2(%d, %d))\r\n", strerror(errno),
      stdinfo[stdidx].funit, STDIN5FD);
    refrsh(NULL);
  }                                /* if (retcod == -1) */
  else
  {
    SYSCALL(retcod, close(stdinfo[stdidx].funit));
    stdidx--;
  }                                /* if (retcod == -1) else */
  if (!USING_FILE)
    duplx5(false);                 /* don't want XOFF */

/* Change DEVNULL setting if needed */
  if (USING_FILE)
  {
    if (devnull_currently != stdinfo[stdidx].nullstdout)
    {
      if (stdinfo[stdidx].nullstdout)
        devnull_stdout();
      else
        restore_stdout();
    }                 /* if (devnull_currently != stdinfo[stdidx].nullstdout) */
  }                                /* if(USING_FILE) */
  else if (devnull_currently)
    restore_stdout();

/* LATER - deal with stacked saved input (reading 1 char at a time for now) */
  return true;
}
