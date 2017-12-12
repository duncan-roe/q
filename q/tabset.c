/* T A B S E T
 *
 * Copyright (C) 1981 D. C. Roe
 * Copyright (C) 2012,2014 Duncan Roe
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
#include <sys/types.h>
#include <unistd.h>
#include "prototypes.h"
#include "scrnedit.h"
#include "tabs.h"

/* Macros */

#define GIVE_UP return false

bool
tabset(scrbuf5 *scbuf)
{
  int i;                           /* Scratch */
/* Temporary storage for tab values as they are being read */
  long temp[NUM_TABS];
/* */
  for (i = 0; i < NUM_TABS; i++)
  {
    scrdtk(1, 0, 0, scbuf);
    if (scbuf->toktyp == eoltok)
      break;
    if (scbuf->toktyp == nultok)
    {
      (void)write(1, "null tabs not allowed", 21);
      GIVE_UP;
    }                              /* if (scbuf->toktyp == nultok) */
    if ((scbuf->decok) == 0)
    {
      (void)write(1, "bad decimal number", 18);
      GIVE_UP;
    }                              /* if ((scbuf->decok) == 0) */
/* LATER - insert a check for -ve tab values or zero */
    if (i > 0 && scbuf->decval <= temp[i - 1])
    {
      (void)write(1, "tabs not in ascending order", 27);
      GIVE_UP;
    }                              /* if (scbuf->decval <= temp[i - 1]) */
    temp[i] = scbuf->decval;       /* Remember this tab */
  }
/*
 * Drop thru ok if no more params
 */
  if (i == NUM_TABS)
  {
    scrdtk(1, 0, 0, scbuf);
    if (scbuf->toktyp != eoltok)
    {
      fprintf(stderr, "%d tabs maximum", NUM_TABS);
      GIVE_UP;
    }                              /* if (scbuf->toktyp == eoltok) */
  }                                /* if (i == NUM_TABS) */
  tabcnt = i;
  if (!tabcnt)
  {
    tabcnt = 1;
    tabs[0].value = 0;
    tabs[0].tabtyp = CHRPOS;
  }                                /* if (!tabcnt) */
  else
    for (i = tabcnt - 1; i >= 0; i--)
    {
      tabs[i].value = temp[i] - 1; /* Zero-based cols internally */
      tabs[i].tabtyp = CHRPOS;
    }                              /* Sets up the common TABS array */
  return true;
}
