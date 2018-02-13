/* D I S P L Y */
/*
 * Copyright (C) 1981 D. C. Roe
 * Copyright (C) 2012,2014,2018 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * Displays the SCREENEDIT line supplied
 */
#include <stdio.h>
#include "prototypes.h"
#include "scrnedit.h"
#include "fmode.h"
/* */
void
disply(scrbuf5 *line, bool savecurs)
{
  int oldcrs;                      /* Saved cursor value */
/*
 * INITIAL TASKS
 */
  cdone = 0;                       /* Won't be set by SCRSET */
  partno = 1;                      /* Won't be set by SCRSET */
  endlin = true;                   /* We are DISPLY */
  oldcrs = line->bcurs;
  line->bcurs = line->bchars;      /* Force SCRSET to o/p the lot */
/* */
p1001:refrsh(line);
  newlin();
  if (FTNMOD)
    goto p1002;                    /* J only showing 1st 72 chars */
  if (cdone != line->bchars)
    goto p1001;
/* J more to do */
p1002:endlin = false;              /* Finishing off */
  if (savecurs)
    line->bcurs = oldcrs;
}
