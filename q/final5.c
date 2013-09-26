/* F I N A L 5 . C
 *
 * Copyright (C) 1995, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2013 Duncan Roe
 *
 * This routine restores initial settings, prior to exiting
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "alledit.h"
#ifdef ANSI5
#include <sys/ioctl.h>
#endif
#include "c1in.h"

void
final5()
{
  if (ttyfd > 0)
  {
    if (ioctl(ttyfd, TCSET5, &tio5save) == -1)
    {
      perror("ioctl#5");
      putchar('\r');
    }
  }
  fcntl(STDIN5FD, F_SETFL, ff5save);
  fcntl(STDOUT5FD, F_SETFL, off5save);
  if (setvbuf(stdout, NULL, _IOLBF, BUFSIZ))
  {
    perror("setvbuf");
    putchar('\r');
  }
}
