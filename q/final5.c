/* >%---- CODE_STARTS ./final5.c */
/* F I N A L 5 . C
 *
 * Copyright (C) 1995, Duncan Roe & Associates P/L
 * Copyright (C) 2012-2014 Duncan Roe
 *
 * This routine restores initial settings, prior to exiting
 */
/* >%---- KEEP2HERE ./final5.c */
#include "c1in.h"
/* >%---- CUT_HERE ./final5.c */

void
final5()
{
  if (ttyfd > 0)
    change_attr(ttyfd, &tio5save);
}
