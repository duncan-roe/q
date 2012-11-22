/* Q U T H A N . C
 *
 * Copyright (C) 1994, Duncan Roe & Associates P/L
 * Copyright (C) 2012, Duncan Roe
 *
 * This routine handles the SIGINT condition.
 *
 * Action: re-establish condition handler and set flag for rest of
 * program
 */
#include <stdio.h>
#include <signal.h>
#include "alledit.h"
#ifdef ANSI5
void
quthan(int i)
#else
void
quthan(i)
int i;
#endif
{
  signal(SIGINT, quthan);
  cntrlc = true;
  return;
}
