/* S D S P L Y
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2016 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * Displays line # only on current line - for BRIEF mode
 * The line # is in the COMMON array PROMPT
 */
/* */
#include <stdio.h>
#include "alledit.h"
#include "scrnedit.h"
void
sdsply()
{
/* */
  scrbuf5 Curr;                    /* Dummy scrnedit buffer */
  Curr.bmxch = 1;
  Curr.bchars = 0;
  Curr.bcurs = 0;
/* */
  endlin = true;                   /* Force max speed + no cursor repos */
  partno = 1;                      /* Required by SCRSET */
  cdone = 0;                       /* Required by SCRSET */
  refrsh(&Curr);                   /* Display the prompt only */
  endlin = false;
}
