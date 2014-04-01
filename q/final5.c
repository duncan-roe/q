/* F I N A L 5 . C
 *
 * Copyright (C) 1995, Duncan Roe & Associates P/L
 * Copyright (C) 2012-2014 Duncan Roe
 *
 * This routine restores initial settings, prior to exiting
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "termio5.hl"
#include <sys/ioctl.h>
#include "c1in.h"

void
final5()
{
  if (ttyfd > 0)
  {
    if (ioctl(ttyfd, TCSET5, &tio5save) == -1)
    {
      perror("ioctl TCSET saved settings");
      putchar('\r');
    }
  }
}
