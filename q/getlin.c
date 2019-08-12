/* G E T L I N */
/*
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2014,2018,2019 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * Analyses the next token (which must not be NULL or EOL) to see
 * that it is a valid line # (in range 1 - # of lines). Errors are
 * reported if REPERR is true
 * Function value is true for good line #. The # itself is in oldcom->decval
 *
 * Implementation note: uses the boolean eofok as an integer value:
 * so false must be zero and true must be 1
 */
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "prototypes.h"
#include "edmast.h"

#define GIVE_UP goto error_exit

bool
getlin(bool reperr, bool eofok)
{
  uint8_t zbuf[14];
  long wanted;                     /* Lines wanted from dfread */
/* */
  lstlin = -1;                     /* In case unsuccessful */
  if (scrdtk(2, zbuf, 13, oldcom)) /* 12 digits max */
  {
    strcpy(ermess, "line # - too many digits");
  error_exit:
    if (reperr)
      fprintf(stderr, "%s", ermess);
    return false;
  }
  if (oldcom->toktyp != nortok)    /* EOF or NULL */
  {
    strcpy(ermess, "line # missing or null");
    GIVE_UP;
  }
  if (!(oldcom->decok))            /* Not decimal */
  {
    if (trytab(zbuf, oldcom, true))
      goto have_a_number;          /* J was Tab x (ok) */
    if (oldcom->toklen == 3 && (toupper(zbuf[0]) == 'E') &&
      (toupper(zbuf[1]) == 'O') && (toupper(zbuf[2]) == 'F'))
    {
      if (deferd)
        dfread(LONG_MAX, NULL);    /* Get real lintot */
      oldcom->decval = lintot + 1;
    }               /* if (oldcom->toklen == 3 && toupper(zbuf[0]) == 'E' ... */
    else
    {
      strcpy(ermess, "bad decimal line #");
      GIVE_UP;
    }
  }
/* Relative line #  if signed */
  else if (oldcom->minusf || oldcom->plusf)
    oldcom->decval += ptrpos;
  if (oldcom->decval <= 0)
  {
    strcpy(ermess, "line # off start of file");
    GIVE_UP;
  }
have_a_number:
  if (deferd && (wanted = oldcom->decval - lintot - eofok) > 0)
    dfread(wanted, NULL);
  if (oldcom->decval <= lintot + eofok)
  {
    lstlin = oldcom->decval;       /* For a possible following -TO */
    return true;
  }
  strcpy(ermess, "line # off end of file");
  GIVE_UP;
}
