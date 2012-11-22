/* P O P _ S T D I N . C */
/*
 * Copyright (C) 1993, Duncan Roe & Associates P/L
 * Copyright (C) 2012, Duncan Roe
 *
 * This function pops stdin 1 level
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "termio5.hl"
#include "alledit.h"
#include "c1in.h"

bool
pop_stdin()
{
  int retcod;
  bool devnull_currently;

  if (!USING_FILE)
  {
    printf("%s", "Only allowed from U-Use file");
    return false;
  }                                /* if (!USING_FILE) */

  devnull_currently = devnullstack[stdinstkptr];

  do
    retcod = dup2(stdinstack[stdinstkptr], 0);
  while (retcod == -1 && errno == EINTR);
  if (retcod == -1)
  {
    fprintf(stderr, "\r\n%s. (dup2(%d, 0))\r\n", strerror(errno),
      stdinstack[stdinstkptr]);
    refrsh(NULL);
  }                                /* if (retcod == -1) */
  else
  {
    do
      retcod = close(stdinstack[stdinstkptr]);
    while (retcod == -1 && errno == EINTR);
    stdinstkptr--;
  }                                /* if (retcod == -1) else */
  if (!USING_FILE)
    duplx5(false);                 /* don't want XOFF */

/* Change DEVNULL setting if needed */
  if (USING_FILE)
  {
    if (devnull_currently != devnullstack[stdinstkptr])
    {
      if (devnullstack[stdinstkptr])
        devnull_stdout();
      else
        restore_stdout();
    }                  /* if (devnull_currently != devnullstack[stdinstkptr]) */
  }                                /* if(USING_FILE) */
  else if (devnull_currently)
    restore_stdout();

/* LATER - deal with stacked saved input (reading 1 char at a time for now) */
  return true;
}
