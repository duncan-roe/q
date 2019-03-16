/* V I S B E L . C
 *
 * Copyright (C) 2019 Duncan Roe
 */

/* Headers */

#include <stdlib.h>
#include "prototypes.h"

/* ********************************* visbel ********************************* */

void
visbel()
{
  system("tput flash 1 >&2 2>/dev/null");
}                                  /* visbel() */
