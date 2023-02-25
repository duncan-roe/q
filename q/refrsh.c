/* >%---- CODE_STARTS ./refrsh.c */
/* R E F R S H
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2018,2019 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 *
 * Refreshes the screen after some edits have been done. This is done
 * in 2 stages:
 *    1) Get the right characters on the screen
 *    2) Get the cursor in the right place
 * and the routine can be interrupted (i.e. will exit) between these
 * two if there is input on the keboard unless end-of-line has been
 * encountered. A variable is set to show whether the screen was
 * completely updated or not.
 */
/* >%---- KEEP2HERE ./refrsh.c */
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "prototypes.h"
#include "scrnedit.h"
/* >%---- CUT_HERE ./refrsh.c */
/* */
void
refrsh(scrbuf5 *xline)
{
  int pos1;                       /* Position of first char needing refreshed */
  int pos2;                        /* Position of last char needing refreshed */
  scrbuf5 *line;
  static scrbuf5 *lastline;

/* if called w/out an arg, use the previous one */
  if (!xline)
    line = lastline;
  else
    lastline = line = xline;
  rfrsh = false;                   /* Screen not right */

/* If there are more characters to process, return now unless newline
 * was pressed last */
  if (!endlin && kbd5())
    return;                        /* R more chars 2do */

  scrset(line);                    /* Get image of req'd screen */

/* Find 1st discrepancy */
  for (pos1 = 0; pos1 < WCHRS; pos1++)
    if (reqd[pos1] != screen[pos1])
      break;

  if (pos1 < WCHRS)
  {
/* Find last discrepancy */
    for (pos2 = WCHRS - 1; pos2 >= pos1; pos2--)
      if (reqd[pos2] != screen[pos2])
        break;
    if (pos2 < pos1)
    {
      fputs("\r\nDiscrepancy fwd but not back (REFRSH)\r\n", stderr);
      return;
    }                              /* if (pos2 == pos1) */

    setcrs(pos1);
    if (crscnt)
      printf("%.*s", crscnt, crsbuf);
    printf("%.*s", pos2 - pos1 + 1, (char *)&reqd[pos1]);
    scurs = pos2 + 1;              /* Where screen cursor now is */
    memcpy(screen, reqd, WCHRS);
  }                                /* if (pos1 < WCHRS) */
/* All chars were ok or are now.
 * Check position of and if necessary move cursor */
  if (!endlin)
  {
    if (scurs != cursr)
    {
      setcrs(cursr);
      printf("%.*s", crscnt, crsbuf);
    }                              /* if (scurs != cursr) */
  }                                /* if (!endlin) */
  rfrsh = true;
  return;
}
