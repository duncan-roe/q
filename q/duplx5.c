/* D U P L X 5 . C
 *
 * Copyright (C) 1993, Duncan Roe & Associates P/L
 * Copyright (C) 2012, Duncan Roe
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "termio5.hl"
#include <memory.h>
#ifdef ANSI5
#include <sys/ioctl.h>
#endif
#include "alledit.h"
#include "c1in.h"

void
duplx5(bool enable_IXON)
{
  if (ttyfd > 0)
  {
/* Set or clear IXON as requested */
    if (enable_IXON)
      tio5.c_iflag = tio5.c_iflag | IXON;
    else
      tio5.c_iflag = tio5.c_iflag & ~(unsigned)IXON;
    if (ioctl(ttyfd, TCSET5, &tio5) == -1)
    {
      perror("ioctl#6");
      putchar('\r');
    }
  }
}
