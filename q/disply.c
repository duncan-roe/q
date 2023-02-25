/* >%---- CODE_STARTS ./disply.c */
/* D I S P L Y */
/*
 * Copyright (C) 1981 D. C. Roe
 * Copyright (C) 2012,2014,2018,2019 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * Displays the SCREENEDIT line supplied
 */
/* >%---- KEEP2HERE ./disply.c */
#include <stdio.h>
#include "prototypes.h"
#include "scrnedit.h"
#include "fmode.h"
/* >%---- CUT_HERE ./disply.c */

void
disply(scrbuf5 *line, bool savecurs)
{
  int oldcrs;                      /* Saved cursor value */

/* INITIAL TASKS */
  cdone = 0;                       /* Won't be set by SCRSET */
  partno = 1;                      /* Won't be set by SCRSET */
  endlin = true;                   /* We are DISPLY */
  oldcrs = line->bcurs;
  line->bcurs = line->bchars;      /* Force SCRSET to o/p the lot */

  do
  {
    refrsh(line);
    newlin();
    if (NOWRAP)
      break;                       /* Only showing 1st screen width */
  }                                /* do */
  while (cdone != line->bchars);

  endlin = false;                  /* Finishing off */
  if (savecurs)
    line->bcurs = oldcrs;
}
