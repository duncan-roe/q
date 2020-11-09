/* S E T C R S */
/*
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2018-2020 Duncan Roe
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
 *    A) Backspace
 *    B) Return
 *    C) Get to EOL (poss return, backspace etc.)
 * One of A) or B) must be available (not checked)
 */
#include <stdio.h>
#include <memory.h>
#include <limits.h>
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

/* Find out how many chars to use with Ansii Ec seq, if enabled... */
  absnum =
    vt100 ? sprintf(absbuf, "\033[%d%c",
    scurs > oldcrs ? scurs - oldcrs : oldcrs - scurs,
    scurs > oldcrs ? 'C' : 'D') : INT_MAX;

  if (scurs < oldcrs)
  {
/* Move cursor backwards */
    bsp = oldcrs - scurs;          /* Set # bsp's req'd */
    rtn = 1 + scurs;               /* \r + refresh */
    if (bsp <= rtn)
    {
/* Backspace or Ansii */
      if (absnum < bsp)
        memcpy(crsbuf, absbuf, crscnt = absnum); /* Ansii better */
      else
        memset(crsbuf, backsp, crscnt = bsp);
    }                              /* if (bsp <= rtn) */
    else
    {
/* Cr + f'wd refresh or Ansii */
      if (absnum < rtn)
        memcpy(crsbuf, absbuf, crscnt = absnum);
      else
      {
        crsbuf[0] = '\r';
        if (scurs)
          memcpy(&crsbuf[1], reqd, scurs);
        crscnt = rtn;              /* Set final # to do */
      }                            /* if (absnum < rtn) else */
    }                              /* if (bsp <= rtn) else */
  }                                /* if (scurs < oldcrs) */
  else
  {
/* Cursor to be moved forward */
    fwd = scurs - oldcrs;          /* # chars if refresh forward */
    if (cacnt)                     /* If fast ^A available */
    {
      ca = cacnt + WCHRS - scurs;  /* # of chars if ^A then bsp */
      if (ca < fwd)                /* If ^A faster */
      {
        if (absnum < ca)
          memcpy(crsbuf, absbuf, crscnt = absnum);
        else
        {
          crscnt = ca;             /* Set no. of chars in sequence */
          memcpy(crsbuf, cachrs, cacnt);
          memset(crsbuf + cacnt, backsp, ca - cacnt);
        }                          /* if (absnum < ca) else */
      }                            /* if (ca < fwd) */
      else
      {
        if (absnum < fwd)
          memcpy(crsbuf, absbuf, crscnt = absnum);
        else
          memcpy(crsbuf, reqd + oldcrs, crscnt = fwd);
      }                            /* if (ca < fwd) else */
    }                              /* if (cacnt) */
    else
    {
      if (absnum < fwd)
        memcpy(crsbuf, absbuf, crscnt = absnum); /* Ansii is faster */
      else
        memcpy(crsbuf, &reqd[oldcrs], crscnt = fwd); /* Refresh */
    }                              /* if (cacnt) else */
  }                                /* if (scurs < oldcrs) else */
}
