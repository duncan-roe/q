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
  int oldcrs;                      /* Former position of cursor */
  int bsp;                         /* # of chars if backspace */
  int rtn;                         /* # of chars if return plus fwd refresh */
  int fwd;                         /* # of chars to refresh fwd */
  int ca;                   /* # of chars to go to end of line then backspace */
  int absnum;                      /* # chars for "absolute" pos'n */
  char absbuf[16];                 /* Ec seq for absnum pos'n */

  crscnt = 0;                      /* No chars req'd yet */
  oldcrs = scurs;                  /* Remember where cursor was */
  scurs = posn;                    /* When finished, it will be */
  if (scurs == oldcrs)
    return;                        /* Cursor right already */

/* Fill buffer with backspaces */
  memset(crsbuf, backsp, WCHRS);

/* Find out how many chars to use with vt100 Ec seq, if VT100... */
  if (vt100)
  {
    sprintf(absbuf, "\r\033[%hdC", scurs);
    absnum = strlen(absbuf);
  }
  else
    absnum = 9999;                 /* Dummy high value */
  if (scurs < oldcrs)
  {
/* Move cursor backwards */
    bsp = oldcrs - scurs;          /* Set # bsp's req'd */
    rtn = 1 + scurs;               /* \r + refresh */
    if (bsp <= rtn)
/* Backspace or VT100 */
    {
      if (absnum < bsp)
        memcpy(crsbuf, absbuf, crscnt = absnum); /* VT100 better */
      else
        crscnt = bsp;
      return;                      /* Backspaces already moved in */
    }
/* F'wd refresh or VT100 */
    if (absnum < rtn)
      memcpy(crsbuf, absbuf, crscnt = absnum);
    else
    {
      crsbuf[0] = '\r';
      if (scurs ^= 0)
        memcpy((char *)&crsbuf[1], (char *)reqd, (size_t)scurs);
      crscnt = rtn;                /* Set final # to do */
    }
    return;
  }                                /* if (scurs < oldcrs) */
  else
  {
/* Cursor to be moved forward */
    fwd = scurs - oldcrs;          /* # chars if refresh forward */
    if (cacnt)                     /* If fast ^A available */
/* cacnt shouldn't be zero if VT100 is enabled, so no check here */
    {
      ca = cacnt + WCHRS - scurs;  /* # of chars if ^A then bsp */
      if (ca < fwd)                /* If ^A faster */
      {
        crscnt = ca;               /* Set no. of chars in sequence */
        memcpy((char *)crsbuf, (char *)cachrs, (size_t)cacnt);
        return;                    /* Finished */
      }
    }
    if (absnum < fwd)
      memcpy(crsbuf, absbuf, crscnt = absnum); /* VT100 is faster */
    else
    {
      crscnt = fwd;                /* Set result */
      memcpy((char *)crsbuf, (char *)&reqd[oldcrs], (size_t)fwd); /* Refresh */
    }                              /* if (absnum < fwd) else */
  }                                /* if (scurs < oldcrs) else */
}
