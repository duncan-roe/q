/* S E T W I N S Z . C
 *
 * Copyright (C) 1994, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2013,2015 Duncan Roe
 *
 * This routine sets up the line width and # lines currently prevailing
 *
 * If msg is nonzero, the new size is o/p, else a flag is set
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "prototypes.h"
#include "scrnedit.h"
#include "c1in.h"
void
setwinsz(msg)
int msg;
{
/* Get the screen size. LATER - may try environment variables if no
 * TIOCGWINSZ */
#ifdef TIOCGWINSZ
  if (ttyfd > 0)
  {
    struct winsize window;
    if (ioctl(ttyfd, TIOCGWINSZ, &window) == -1)
    {
      perror("ioctl#3");
      row5 = 24;                   /* Default screen rows */
      col5 = 80;                   /* Default screen columns */
      newlin();
    }
    else
    {
      row5 = window.ws_row;
      col5 = window.ws_col;
/*
 * Allow for being returned defaults of ZERO (e.g. from telnet logins):
 * these would make q loop if left...
 */
      if (!row5)
        row5 = 24;
      if (!col5)
        col5 = 80;
      if (msg)
      {
        printf("\r\nNoted screen dimensions %u x %u", col5, row5);
        newlin();
      }
      else
        size5 = 1;
    }
  }
  else
  {
    row5 = 24;
    col5 = 80;
  }
#else
  row5 = 24;                       /* Default screen rows */
  col5 = 80;                       /* Default screen columns */
#endif
/* Guard against huge screens */
  if (col5 > sizeof screen)
    col5 = sizeof screen;
}
