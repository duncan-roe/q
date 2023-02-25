/* >%---- CODE_STARTS ./kbd5.c */
/* K B D 5 . C */
/* >%---- KEEP2HERE ./kbd5.c */
#include <poll.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "prototypes.h"
/* >%---- CUT_HERE ./kbd5.c */
/*
 * Copyright (C) 2012,2019 Duncan Roe
 */

bool
kbd5(void)
{
  struct pollfd p;
  int i;

  p.fd = 0;                        /* Standard input */
  p.events = POLLIN;
  SYSCALL(i, poll(&p, 1, 0));
  if (i == -1)
  {
    fprintf(stderr, "\r\n%s. (poll)", strerror(errno));
    newlin();
  }                                /* if (i == -1) */
  return (p.revents & POLLIN) != 0;
}
