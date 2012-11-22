/* W I N C H H A N . C
 *
 * Copyright (C) 1994, Duncan Roe & Associates P/L
 * Copyright (C) 2012, Duncan Roe
 *
 * This routine handles the SIGWINCH condition.
 *
 * Action: re-establish condition handler and set flag for rest of
 * program. Also, send SIGWINCH to our parent process.
 */
#include <stdio.h>
#include <signal.h>
#ifdef ANSI5
#include <unistd.h>
#endif
#include "alledit.h"
void
winchhan(i)
int i;
{
  signal(SIGWINCH, winchhan);
  seenwinch = true;
  if (kill(getppid(), SIGWINCH))
  {
    perror("kill");
    newlin();
  }
  return;
}
