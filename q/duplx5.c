/* >%---- CODE_STARTS ./duplx5.c */
/* D U P L X 5 . C
 *
 * Copyright (C) 1993, Duncan Roe & Associates P/L
 * Copyright (C) 2012-2014 Duncan Roe
 */
/* >%---- KEEP2HERE ./duplx5.c */
#include <termios.h>
#include "prototypes.h"
#include "c1in.h"
/* >%---- CUT_HERE ./duplx5.c */

void
duplx5(bool enable_IXON)
{
  if (ttyfd > 0)
  {
/* Set or clear IXON as requested */
    if (enable_IXON)
      tio5.c_iflag = tio5.c_iflag | IXON;
    else
      tio5.c_iflag = tio5.c_iflag & ~(unsigned)IXON;
    change_attr(ttyfd, &tio5);
  }
}
