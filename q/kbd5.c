#include <poll.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "alledit.h"
/*
 * Copyright (C) 2012, Duncan Roe
 */

bool
kbd5(void)
{
  struct pollfd p;
  int i;
  
  p.fd = 0;                        /* Standard input */
  p.events = POLLIN;
  do i = poll(&p, 1, 0); while (i == -1 && errno == EINTR);
  if (i == -1)
  {
    fprintf(stderr, "\r\n%s. (poll)", strerror(errno));
    newlin();
  }                                /* if (i == -1) */
  return (p.revents & POLLIN) != 0;
}
