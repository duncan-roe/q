/* L S T M A C */
/*
 * Copyright (C) 1993, Duncan Roe & Associates P/L
 * Copyright (C) 2012, Duncan Roe
 *
 * This routine lists the current non-null macros to standard output in
 * a form suitable for reinput by U. Generally, the caller will have
 * assigned standard output to a file before calling this routine...
 */
#include <stdio.h>
#include "alledit.h"
#include "macros.h"
void
lstmac()
{
  int i;                           /* Scratch */
/*
 * All macros are output in the form "N ooo <def'n>" where ooo is at
 * least 3 octal digits.
 */
  for (i = 0; i <= TOPMAC; i++)
  {
    if (!scmacs[i])
      continue;                    /* J no macro in this slot */
    printf("N %03o ", i);
    showmac(i);
  }
  printf("Z\r\n");
}
