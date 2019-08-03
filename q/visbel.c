/* V I S B E L . C
 *
 * Copyright (C) 2019 Duncan Roe
 */

/* Pragmas */

#pragma GCC diagnostic ignored "-Wunused-result" /* Don't care about system() */

/* Headers */

#include <stdlib.h>
#include "prototypes.h"

/* Instantiate externals */

double visbel_interval = 0.15;

/* Static Variables */

static double time_last = 0;

/* ********************************* visbel ********************************* */

void
visbel()
{
  double t;

  if ((t = time_now()) - time_last >= visbel_interval)
  {
    system("tput flash 1 >&2 2>/dev/null");
    time_last = t;
  }                  /* if ((t = time_now()) - time_last  >= visbel_interval) */
}                                  /* visbel() */
