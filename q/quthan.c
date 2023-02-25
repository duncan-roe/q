/* >%---- CODE_STARTS ./quthan.c */
/* Q U T H A N . C
 *
 * Copyright (C) 1994, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2014 Duncan Roe
 *
 * This function handles the SIGINT, SIGTERM & SIGWINCH conditions.
 */
/* >%---- KEEP2HERE ./quthan.c */
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "prototypes.h"
#include "q_pipe.h"
/* >%---- CUT_HERE ./quthan.c */
void
quthan(int signum, siginfo_t *siginfo, void *ucontext)
{
  switch (signum)
  {
    case SIGINT:
      if (!piping)
      {
        cntrlc = true;
        return;
      }                            /* if (!piping) */
      break;
    case SIGWINCH:
      seenwinch = true;
      if (kill(getppid(), SIGWINCH))
        fprintf(stderr, "%s. (kill)\r\n", strerror(errno));
      return;
  }                                /* switch (signum) */

/* If drop through the switch, need to exit in order to clean up. */
/* (Includes signals not in the switch, e.g. SIGTERM) */
  exit(1);
}
