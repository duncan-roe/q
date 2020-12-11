/* S I N D N T
 *
 * Copyright (C) 1981 D. C. Roe
 * Copyright (C) 2012,2014,2017-2020 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * Sets the indent at the start of an Append or Insert, if indenting.
 * In any case, sets up the previous buffer (as recalled by ^A) to the
 * previous line, unless we are inserting line 1.
 */
#include <stdio.h>
#include <ctype.h>
#include "prototypes.h"
#include "edmast.h"
#include "fmode.h"

/* Externals that are not in any header */


/* ********************************* sindnt ********************************* */

uint8_t *
sindnt(bool set_indent)
{
  int j;                           /* Scratch */
  long lines_back;

  lines_back = 1;
  if (modify)
    lines_back = 2;                /* To skip over line being modified */
  lstvld = true;                   /* Will be true after we finish */
  if (INDENT && set_indent)
  {
/* Look back for a non-blank line */
    do
    {
      if (ptrpos == lines_back)
      {
/* Reached start of file: make empty previous line */
        ndntch = 0;                /* No indent if at s.o.f. */
        prev->bchars = 0;          /* No data in 0th line */
        prev->bcurs = 0;           /* Cursor at line strt */
        return prev->bdata;
      }                            /* if (ptrpos != lines_back) */
      setaux(ptrpos - lines_back);
      rdlin(prev, true);

/* Make loop interruptible (e.g. for test file with >4G blank lines) */
      if (cntrlc)
      {
        cntrlc = false;
        if (prev->bchars == 0)
        {
          fputs("\r\nInterrupt. Assuming zero indent", stdout);
          newlin();
          ndntch = 0;
          return prev->bdata;
        }                          /* if (prev->bchars == 0) */
      }                            /* if (cntrlc) */
      lines_back++;                /* In case line empty */
    }
    while (prev->bchars == 0);

/* Set the indent - code copied from scrdit with Curr+>prev */
    for (ndntch = 0, j = prev->bchars; j > 0; j--, ndntch++)
      if (!isspace(prev->bdata[ndntch]))
        break;
  }                                /* if (INDENT) && set_indent */
  else
  {
    if (ptrpos == lines_back)      /* At start of file */
      prev->bchars = prev->bcurs = 0;
    else
    {
      setaux(ptrpos - lines_back);
      rdlin(prev, true);
    }                              /* if (ptrpos == lines_back) else */
  }                                /* if (INDENT) && set_indent else */

  return prev->bdata;
}
