/* >%---- CODE_STARTS ./xlateset.c */
/* X L A T E S E T
 *
 * Copyright (C) 1994 Duncan Roe & Associates P/L
 * Copyright (C) 2012,2014,2019 Duncan Roe
 *
 * This routine sets up Q's translation table for case (in)dependent L,
 * Y, FL, & FY commands.
 * When called, the table is not in the right state...
 */
/* >%---- KEEP2HERE ./xlateset.c */
#include "prototypes.h"
#include "fmode.h"
/* >%---- CUT_HERE ./xlateset.c */
/* */
uint8_t xlatable[256];
void
xlateset()
{
  int i;                           /* Scratch */
/*
 * First off, set up a case - dependent table...
 */
  for (i = 255; i >= 0; i--)
    xlatable[i] = i;
/*
 * Amend table for case - INdependent operation if req...
 */
  if (!CASDEP)                     /* Case-INdependent wanted */
    for (i = 'a'; i <= 'z'; i++)
      xlatable[i] = i - 040;
  tbstat = CASDEP;                 /* Set new status */
}
