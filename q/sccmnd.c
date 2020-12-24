/* S C C M N D */
/*
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2020 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 *
 * SCREENEDIT-reads a command. Actually most of the work outside of
 * SCRDIT is done by SCMNRD. All we have to do here is empty the
 * command buffer and  SCMNRD...
 */
#include <stdio.h>
#include "prototypes.h"
#include "cmndcmmn.h"
#include "edmast.h"
#include "fmode.h"
#include "alu.h"
void
sccmnd()
{
  if (!cmsplt)
  {                                /* NEWCOM valid if split command */
    newcom->bcurs = 0;             /* Cursor at start of line */
    newcom->bchars = 0;            /* No data yet */
  }
  scmnrd();                        /* Get a line */

  if (xmode_pending)
  {
    xmode_pending = false;
    fmode = xreg;
  }                                /* if (xmode_pending) */
}
