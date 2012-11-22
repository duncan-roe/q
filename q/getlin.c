/* G E T L I N */
/*
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012, Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * Analyses the next token (which must not be NULL or EOL) to see
 * that it is a valid line # (in range 1 - # of lines). Errors are
 * reported if REPERR is true
 * Function value is true for good line #. The # itself is in the
 * common DECVAL.
 */
#include <stdio.h>
#include <limits.h>
#ifdef ANSI5
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#endif
#include "alledit.h"
#include "edmast.h"
#ifdef ANSI5
short
getlin(int reperr, int eofok)
#else
short
getlin(reperr, eofok)
int reperr;                        /* Whether to report error here */
int eofok;                         /* EOF line is ok, MUST be 0 or 1 */
#endif
{
  unsigned char zbuf[14];
  long wanted;                     /* Lines wanted from dfread */
/* */
  lstlin = -1;                     /* In case unsuccessful */
  if (scrdtk(2, zbuf, 13, oldcom)) /* 12 digits max */
  {
    strcpy(ermess, "line # - too many digits");
    errlen = 24;
  p1003:
    if (reperr)
      (void)write(1, ermess, errlen);
    return 0;
  }
  if (oldcom->toktyp != nortok)    /* EOF or NULL */
  {
    strcpy(ermess, "line # missing or null");
  p1007:
    errlen = 22;
    goto p1003;
  }
  if (!(oldcom->decok))            /* Not decimal */
  {
    if (trytab(1, zbuf, oldcom))
      goto p1005;                  /* J was a Tx (ok) */
    if (oldcom->toklen == 3 && ((zbuf[0] & 0337) == 'E') &&
      ((zbuf[1] & 0337) == 'O') && ((zbuf[2] & 0337) == 'F'))
    {
      if (deferd)
        dfread(LONG_MAX, NULL);    /* Get real lintot */
      oldcom->decval = lintot + 1;
    }                        /* if(oldcom->toklen==3&&((zbuf[0]&0337)=='E'... */
    else
    {
      strcpy(ermess, "bad decimal line #");
      errlen = 18;
      goto p1003;
    }
  }
/* Relative line #  if signed */
  else if (oldcom->minusf || oldcom->plusf)
    oldcom->decval += ptrpos;
  if (oldcom->decval <= 0)
  {
    strcpy(ermess, "line # off start of file");
    errlen = 24;
    goto p1003;
  }
p1005:
  if (deferd && (wanted = oldcom->decval - lintot - eofok) > 0)
    dfread(wanted, NULL);
  if (oldcom->decval <= lintot + eofok)
  {
    lstlin = oldcom->decval;       /* For a possible following -TO */
    return 1;
  }
  strcpy(ermess, "line # off end of file");
  goto p1007;
}
