/* >%---- CODE_STARTS ./setwinsz.c */
/* S E T W I N S Z . C
 *
 * Copyright (C) 1994, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2013,2015,2018,2019 Duncan Roe
 *
 * This routine sets up the line width and # lines currently prevailing
 *
 * If msg is nonzero, the new size is o/p, else a flag is set */

/* Headers */

/* >%---- KEEP2HERE ./setwinsz.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "prototypes.h"
#include "scrnedit.h"
#include "c1in.h"
/* >%---- CUT_HERE ./setwinsz.c */

/* Static prototypes */

static int row_dflt(void);
static int col_dflt(void);

/* ******************************** setwinsz ******************************** */

void
setwinsz(int msg)
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
      row5 = row_dflt();           /* Default screen rows */
      col5 = col_dflt();           /* Default screen columns */
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
        row5 = row_dflt();
      if (!col5)
        col5 = col_dflt();
      if (msg)
      {
        printf("\r\nNoted screen dimensions %u x %u", col5, row5);
        newlin();
      }
      else
        size5 = true;
    }
  }
  else
  {
    row5 = row_dflt();
    col5 = col_dflt();
  }
#else
  row5 = row_dflt();               /* Default screen rows */
  col5 = col_dflt();               /* Default screen columns */
#endif
/* Guard against huge screens */
  if (col5 > sizeof screen)
    col5 = sizeof screen;
}

/* ******************************** col_dflt ******************************** */

static int
col_dflt(void)
{
/* If COLUMNS is exported to the environment then use it; */
/* otherwise return 80. */
/* Conveniently, the "watch" command exports COLUMNS */

  char *cols = getenv("COLUMNS");

  if (cols)
    return atoi(cols);
  return 80;
}                                  /* static int row_dflt(void) */

/* ******************************** row_dflt ******************************** */

static int
row_dflt(void)
{
/* If LINES is exported to the environment then use it; */
/* otherwise return 24. */
/* Conveniently, the "watch" command exports LINES */

  char *rows = getenv("LINES");

  if (rows)
    return atoi(rows);
  return 24;
}                                  /* static int row_dflt(void) */
