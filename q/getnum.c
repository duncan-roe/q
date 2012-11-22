/* G E T N U M */
/*
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012, Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 *
 * Gets the optional # of lines param. If OKZERO is true, 0 lines
 * is ok, otherwise must be 1 or more. Result in DECVAL is ok
 * returns true unless actual bad decno.
 */
#include <stdio.h>
#ifdef ANSI5
#include <sys/types.h>
#include <unistd.h>
#endif
#include "alledit.h"
#include "edmast.h"
/* */
#ifdef ANSI5
short
getnum(int okzero)
#else
short
getnum(okzero)
int okzero;
#endif
{
  unsigned char zbuf[14];
/* */
  if (scrdtk(2, zbuf, 13, oldcom)) /* Read poss # of lines */
  {
    (void)write(1, "too many digits in", 18);
  badnumlines:
    (void)write(1, " # of lines", 11);
    return 0;
  }
  switch (oldcom->toktyp)
  {
    case eoltok:
    case nultok:
      oldcom->decval = 1;          /* Assume 1 line intended */
      break;                       /* switch(oldcom->toktyp) */
    case nortok:
      if (!oldcom->decok)
      {
      tryfortaborto:
        if (trytab(1, zbuf, oldcom)) /* Was a 'Tx' or -TO <something> (OK) */
          break;                   /* switch(oldcom->toktyp) */
        (void)write(1, "bad decimal", 11);
        goto badnumlines;
      }                            /* if(!oldcom->decok) */
      if (oldcom->decval > 0)
        break;                     /* switch(oldcom->toktyp) */
      if (oldcom->decval == 0)
      {
        if (okzero)
          break;                   /* switch(oldcom->toktyp) */
        if (oldcom->toklen == 1 && zbuf[0] == '-') /* Is unary minus */
          goto tryfortaborto;
        (void)write(1, "not allowed zero", 16);
        goto badnumlines;
      }
/* Otherwise it is < 0 (illegal) */
      (void)write(1, "not allowed negative", 20);
      goto badnumlines;
  }                                /* switch(oldcom->toktyp) */
  return 1;                        /* OK */
}
