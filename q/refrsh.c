/* R E F R S H
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012, Duncan Roe
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
#include <stdio.h>
#ifdef ANSI5
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#endif
#include "prototypes.h"
#include "scrnedit.h"
/* */
#ifdef ANSI5
void
refrsh(scrbuf5 *xline)
#else
void
refrsh(xline)
scrbuf5 *xline;
#endif
{
/* LOCAL VARIABLES
 * ===============
 *
 * pos1   - Position 1st char needing refreshed
 * pos2   - Position last char needing refreshed
 * i,j &c - Indicies, loop variables etc.
 * xchars - # of chars o/p (determines how long to sleep */
  int i, pos1, pos2;
  scrbuf5 *line;
  static scrbuf5 *lastline;
/* if called w/out an arg, use the previous one */
  if (!xline)
    line = lastline;
  else
    lastline = line = xline;
/*
 *  CALL TNOUA(PROMPT,PCHARS)
 *  CALL TNOU(LINE(BDATA),LINE(BCHARS))
 *  RETURN */
  rfrsh = false;                   /* Screen not right */
/* If there are more characters to process, return now unless newline
 * was pressed last */
  if (!endlin)
    if (kbd5())
      return;                      /* R more chars 2do */
/* */
  scrset(line);                    /* Get image of req'd screen */
/*
 * Find 1st discrepancy
 */
  for (pos1 = 0; pos1 < WCHRS; pos1++)
    if (reqd[pos1] != screen[pos1])
      goto p1002;
/*
 * All chars ok if drop thro'
 */
  goto p1003;                      /* Check & poss move cursor */
/*
 * Find last discrepancy
 */
p1002:
  for (pos2 = WCHRS - 1; pos2 >= pos1; pos2--)
    if (reqd[pos2] != screen[pos2])
      goto p1005;
/*
 * Error if drop through!
 */
  puts("\r\nDiscrepancy fwd but not back (REFRSH)\r");
  return;
/* */
p1005:
  setcrs(pos1);
  if (crscnt == 0)
    goto p1006;                    /* Skip o/p if none */
  write(1, crsbuf,
#ifdef ANSI5
    (size_t)
#endif
    crscnt);
p1006:i = pos2 - pos1 + 1;         /* No. of chars to rewrite */
  write(1, (char *)&reqd[pos1],
#ifdef ANSI5
    (size_t)
#endif
    i);
  scurs = pos2 + 1;                /* Where screen cursor now is */
  memcpy((char *)screen, (char *)reqd,
#ifdef ANSI5
    (size_t)
#endif
    WCHRS);
/*
 * P1003 - All chars were ok or are now. Check position of and if
 *	  necessary move cursor.
 */
p1003:if (endlin)
    goto p1009;                    /* Don't cursor posn if DISPLY */
  if (scurs != cursr)
    goto p1008;
/* Move cursor */
p1009:rfrsh = true;
  return;
p1008:setcrs(cursr);
  write(1, crsbuf,
#ifdef ANSI5
    (size_t)
#endif
    crscnt);
  goto p1009;
}
