/* S P R M P T */
/*
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2018 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * Sets up master edit's prompt buffer with the number supplied
 */
#include <stdio.h>
#include <string.h>
#include "prototypes.h"
#include "edmast.h"
void
sprmpt(long number)
{
  sprintf((char *)prmpt, "%6ld ", number);
  pchrs = strlen((char *)prmpt);
}
