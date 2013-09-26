/* S E M I F I N L . C
 *
 * Copyright (C) 1993, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2013 Duncan Roe
 *
 * This routine restores some initial settings
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "alledit.h"
#include "c1in.h"

void
semifinl()
{
  if (ttyfd > 0)
  {
    if (ioctl(ttyfd, TCSET5, &tio5save) == -1)
    {
      perror("ioctl#7");
      putchar('\r');
    }
  }
}
