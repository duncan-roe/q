/* T R Y T A B
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
 * Tries for a T<tabid> format of a number if decimal conversion
 * fails. The # of the tab must be outside those that ^I can jump to if
 * FILPOS is true
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
trytab(short filpos, unsigned char *zbuf, scrbuf5 * scline)
#else
short
trytab(filpos, zbuf, scline)
short filpos;
unsigned char *zbuf;
scrbuf5 *scline;
#endif
{
  long savlst;                     /* Former value of LSTLIN */
/*
 * Look for -TO if number of lines being sought
 *
 */
  if (!filpos)
    goto p1003;                    /* J not poss # lines */
  switch (scline->toklen)
  {
    default:
      goto p1002;                  /* J too big for -TO or Tabid */
    case 3:
      if ((zbuf[2] & 0337) != 'O')
        goto p1002;
    case 2:
      if ((zbuf[1] & 0337) != 'T')
        goto p1003;
    case 1:
      if (zbuf[0] != '-')
        goto p1003;
  }
  if (lstlin <= 0)
    goto p1004;                    /* J -TO not legal this param */
  savlst = lstlin;                 /* Will be overwritten by GETLIN */
  if (getlin(1, 0))
    goto p1005;                    /* J got a line # */
  goto p1008;                      /* J back, error reported */
p1004:
  (void)write(1, "-TO not allowed in this position", 32);
  goto p1008;
p1005:
  scline->decval = lstlin - savlst + 1; /* C/vert to a # of lines */
  lstlin = savlst;                 /* Reinstate */
  if (scline->decval > 0)
    goto p1006;                    /* J good value */
  (void)write(1, "-TO a previous line not allowed", 31);
  goto p1008;
p1006:
  return 1;                        /* Finished - -TO successful */
/* */
p1003:
  if (scline->toklen == 2)
    goto p1001;                    /* J right length */
p1002:
  return 0;
p1001:
  if ((zbuf[0] & 0337) != 'T')
    goto p1002;                    /* J doesn't start 'T' or 't' */
  return gettab(zbuf[1], filpos, &scline->decval);
p1008:
  (void)write(1, " or ", 4);
  goto p1002;
}
