/* >%---- CODE_STARTS ./getnum.c */
/* G E T N U M */
/*
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2014,2018,2019 Duncan Roe
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
/* >%---- KEEP2HERE ./getnum.c */
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "prototypes.h"
#include "edmast.h"
/* >%---- CUT_HERE ./getnum.c */

#define GIVE_UP(x) \
  do { fprintf(stderr, "%s%s", x, " # of lines"); return false; } while (0)
/* */
bool
getnum(bool okzero, bool filpos)
{
  uint8_t zbuf[14];
/* */
  if (scrdtk(2, zbuf, 13, oldcom)) /* Read poss # of lines */
  {
    GIVE_UP("too many digits in");
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
        if (trytab(zbuf, oldcom, filpos)) /* Was 'Tx' or -TO <something> (OK) */
          break;                   /* switch(oldcom->toktyp) */
        GIVE_UP("bad decimal");
      }                            /* if(!oldcom->decok) */
      if (oldcom->decval > 0)
        break;                     /* switch(oldcom->toktyp) */
      if (oldcom->decval == 0)
      {
        if (okzero)
          break;                   /* switch(oldcom->toktyp) */
        if (oldcom->toklen == 1 && zbuf[0] == '-') /* Is unary minus */
          goto tryfortaborto;
        GIVE_UP("not allowed zero");
      }
/* Otherwise it is < 0 (illegal) */
      GIVE_UP("not allowed negative");
  }                                /* switch(oldcom->toktyp) */
  return true;                     /* OK */
}
