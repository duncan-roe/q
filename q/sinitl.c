/* S I N I T L */
/*
 * Copyright (C) 1981, 2011 D. C. Roe
 * Copyright (C) 2012, Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * Initial tasks for screenedit subsystem
 */
#include <stdio.h>
#include "alledit.h"
#include "macros.h"
#include "cmndcmmn.h"
#include "scrnedit.h"
#include "termio5.hl"
#include "c1in.h"
void
sinitl()
{
/* */
  endlin = false;
  bspace = true;
  backsp = '\b';                   /* Set variable to the default */
  rtcnt = 1;
  rtchrs = '\r';                   /* CR */
  vt100 = true;                    /* VT100 or X */
  cacnt = 0;                       /* No fast EOL */
  tabcnt = 3;
  tabs[0].value = 2;
  tabs[0].tabtyp = chrpos;
  tabs[1].tabtyp = chrpos;
  tabs[2].tabtyp = chrpos;
  tabs[1].value = 35;
  tabs[2].value = 72;
  mctrst = false;
  curmac = -1;
  mcnxfr = MCDTUM;                 /* Empty stack so far */
  cmsplt = false;                  /* No split command yet */
}
