/* S E T C R S */
/*
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2018,2019 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * Sets up character buffer + count req'd to position curser at
 * 'POSN'.
 * The common variable CURSR is set to the i/p value.
 *
 * Facilities of the device are used to minimise chars sent to screen
 * as follows (if available in all cases):-
 *    A) BACKSPACE
 *    B) RETURN
 *    C) Get to EOL (poss return, backspace etc.)
 * One of A) or B) must be available (not checked)
 */
#include <stdio.h>
#include <memory.h>
#include "prototypes.h"
#include "scrnedit.h"
void
setcrs(int posn)
{
/*
 * LOCAL VARIABLES
 * ===============
 *
 * oldcrs - Former position of cursor
 * bsp    - # of chars if BACKSPACE
 * rtn    - # of chars if return plus fwd refresh
 * fwd    - # of chars to refresh fwd
 * ca     - # of chars to go to end of line then backspace
 * i,j &c - Indices and the like
 */
  int i, oldcrs, bsp, rtn, fwd, ca;
  int absnum;                      /* # chars for "absolute" pos'n */
  char absbuf[16];                 /* Ec seq for absnum pos'n */
  uint8_t *p;
/*
 * Here we go!
 */
  crscnt = 0;                      /* No chars req'd yet */
  oldcrs = scurs;                  /* Remember where cursor was */
  scurs = posn;                    /* When finished, it will be */
  if (scurs == oldcrs)
    return;                        /* Cursor right already */
/*
 * Fill buffer with backspaces
 */
  p = crsbuf;
  for (i = WCHRS; i > 0; i--)
    *p++ = backsp;
/* Find out how many chars to use with vt100 Ec seq, if VT100... */
  if (vt100)
  {
    sprintf(absbuf, "\r\033[%hdC", scurs);
    absnum = strlen(absbuf);
  }
  else
    absnum = 9999;                 /* Dummy high value */
  if (scurs > oldcrs)
    goto p1003;                    /* Move cursor forward */
/*
 * Move cursor backwards
 */
  bsp = oldcrs - scurs;            /* Set # bsp's req'd */
  rtn = 1 + scurs;                 /* \r + refresh */
  if (bsp <= rtn)
/* Backspace or VT100 */
  {
    if (absnum < bsp)
      goto p1101;                  /* J VT100 better */
    crscnt = bsp;
    return;                        /* Backspaces already moved in */
  }
/* F'wd refresh or VT100 */
  if (absnum < rtn)
  {
  p1101:
    memcpy((char *)crsbuf, absbuf, (size_t)absnum);
    crscnt = absnum;
  }
  else
  {
    crsbuf[0] = '\r';
    if (scurs ^= 0)
      memcpy((char *)&crsbuf[1], (char *)reqd, (size_t)scurs);
    crscnt = rtn;                  /* Set final # to do */
  }
  return;
/*
 * P1003 - Cursor to be moved forward
 */
p1003:fwd = scurs - oldcrs;        /* # chars if refresh forward */
  if (cacnt)                       /* If fast ^A available */
/* cacnt shouldn't be zero if VT100 is enabled, so no check here */
  {
    ca = cacnt + WCHRS - scurs;    /* # of chars if ^A then bsp */
    if (ca < fwd)                  /* If ^A faster */
    {
      crscnt = ca;                 /* Set no. of chars in sequence */
      memcpy((char *)crsbuf, (char *)cachrs, (size_t)cacnt);
      return;                      /* Finished */
    }
  }
  if (absnum < fwd)
    goto p1101;                    /* J VT100 is faster */
  crscnt = fwd;                    /* Set result */
  memcpy((char *)crsbuf, (char *)&reqd[oldcrs], (size_t)fwd); /* Refresh */
}
