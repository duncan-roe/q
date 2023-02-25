/* >%---- CODE_STARTS ./lsub5a.c */
/* L S U B 5 A
 *
 * Copyright (C) 1993 Duncan Roe & Associates P/L
 * Copyright (C) 2014,2018-2020 Duncan Roe
 *
 * This routine searches for a string. The search is case independent
 * iff CASDEP is 1.
 * XLATABLE is set up by XLASET if not in the required state.
 *
 *  Arguments:-
 *
 *  A1 - String to look for
 *  A2 - Length of string
 *  A3 - Array ctg string to be searched
 *  A4 - 1st char posn of string to search
 *  A5 - Length of string
 *  A6 - (Returned) starting char posn
 *  A7 - (Returned) ending char posn
 *
 *  The function result is true iff a match is found.
 */
/* >%---- KEEP2HERE ./lsub5a.c */
#include <stdio.h>
#include "prototypes.h"
#include "fmode.h"
/* >%---- CUT_HERE ./lsub5a.c */

bool
lsub5a(uint8_t *srchstr, int srchlen, uint8_t *string,
  int first, int len, int *strtpos, int *endpos)
{
  uint8_t cfirst,                  /* 1st char to look for, u/c if casind */
   *p, *q, *r;                     /* Scratch */
  int i, k;                        /* Scratch */

  if (first >= len)
  {
    visbel();
    fprintf(stderr,
      "First position out of range: first=%d, len=%d (lsub5a)\r\n", first, len);
    return false;
  }
  if (tbstat != CASDEP)
    xlateset();                    /* Get table right */
  cfirst = xlatable[srchstr[0]];   /* Get 1st char */

/*  Look for match on 1st char */
  p = string + first;
  for (i = len - first - srchlen + 1; i > 0; i--) /* # possible 1st matches */
  {
    if (xlatable[*p++] != cfirst)  /* Xlate for case (in)sensitive */
      continue;                    /* No match */

/*  Have match on 1st char, try the rest */
    q = p;                         /* 1st of the rest */
    r = srchstr + 1;               /* 2nd char in search string */
    for (k = srchlen - 1; k > 0; k--)
      if (xlatable[*q++] != xlatable[*r++])
        break;
    if (k)
      continue;

/* Have match. Set return variables and leave... */
    *strtpos = p - string - 1;
    if (endpos)
      *endpos = *strtpos + srchlen - 1;
    return true;
  }                               /* for (i = len - first - srchlen + 1; ...) */
/* Not found string if get here */
  return false;
}
