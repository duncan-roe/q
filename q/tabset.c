/* T A B S E T
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012, Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * Implements the main editor's T-TABSET command.
 * Returns true if OK
 * Returns false otherwise and a message will have been o/p
 *
 * Tabs and columns are zero-based, but appear 1-based to the user
 */
#include <stdio.h>
#ifdef ANSI5
#include <sys/types.h>
#include <unistd.h>
#endif
#include "alledit.h"
#include "scrnedit.h"
short
tabset(scbuf)
scrbuf5 *scbuf;
{
  int i,                           /* Scratch */
    count;                         /* # of tabs accepted */
/* Temporary storage for tab values as they are being read */
  long temp[80];
/* */
  for (i = 0; i < 80; i++)         /* 80 tabs max */
  {
    scrdtk(1, 0, 0, scbuf);
    if (scbuf->toktyp == eoltok)
      goto p1002;
    if (scbuf->toktyp != nultok)
      goto p1003;                  /* J not null(ok) */
    (void)write(1, "null tabs not allowed", 21);
    goto p1005;
  p1003:
    if ((scbuf->decok) != 0)
      goto p1004;                  /*J ok decimal */
    (void)write(1, "bad decimal number", 18);
    goto p1005;
  p1004:
/* LATER - insert a check for -ve tab values or zero */
    if (i == 0)
      goto p1006;                  /* Skip seq. chk. 1st time */
    if (scbuf->decval > temp[i - 1])
      goto p1006;                  /*J ascending order ok */
    (void)write(1, "tabs not in ascending order", 27);
    goto p1005;
  p1006:
    temp[i] = scbuf->decval;       /* Remember this tab */
  }
/*
 * Drop thro' ok if no more params
 */
  scrdtk(1, 0, 0, scbuf);
  if (scbuf->toktyp == eoltok)
    goto p10065;
  (void)write(1, "80 tabs maximum", 15);
  goto p1005;
p10065:
  count = 80;
  goto p1007;
p1002:
  count = i;
  if (count == 0)
    goto p1008;                    /*J no tabs */
p1007:
  for (i = count - 1; i >= 0; i--)
  {
    tabs[i].value = temp[i] - 1;   /* Zero-based cols internally */
    tabs[i].tabtyp = chrpos;
  }                                /* Sets up the common TABS array */
p1010:
  tabcnt = count;
  return 1;
/*
 * P1008 - No tabs so set # of tabs = 1 and posn that tab at 1
 */
p1008:
  count = 1;
  tabs[0].value = 0;
  tabs[0].tabtyp = chrpos;
  goto p1010;
p1005:
  return 0;
}
