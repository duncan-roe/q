/* S I N D N T
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012, Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * Sets the INDENT at the start of an APPEND or INSERT, if indenting.
 * In any case, sets up the previous buffer (as recalled by ^A) to the
 * previous line, unless we are inserting line 1.
 */
#include <stdio.h>
#include "alledit.h"
#include "edmast.h"
void
sindnt()
{
  int j;                           /* Scratch */
  long i4, k4;                     /* Scratch */
/*
 * In fact look back for a non-blank line
 */
  i4 = ptrpos;                     /* Remember pos'n on entry */
  k4 = 1;                          /* # of lines we are going back */
  if (modify)
    k4 = 2;                        /* To skip over line being modified */
  lstvld = true;                   /* Will be true after we finish */
p1002:
  if (i4 != k4)
    goto p1001;                    /* J not at s.o.f. */
  ndntch = 0;                      /* No indent if at s.o.f. */
  prev->bchars = 0;                /* No data in 0th line */
  prev->bcurs = 0;                 /* Cursor at line strt */
  return;
p1001:setaux(i4 - k4);
  (void)rdlin(prev, 1);
  k4 = k4 + 1;                     /* In case line empty */
  if (prev->bchars == 0)
    goto p1002;
/* J line was empty */
  if (!INDENT)
    return;                        /* Finished if no indenting */
/*
 * Set the INDENT - code copied from SCRDIT with CURR -> PREV
 */
  j = prev->bchars;
/* Finish when find non-space */
  for (ndntch = 0; ndntch < j; ndntch++)
    if (prev->bdata[ndntch] != SPACE)
      return;
}
