/* P D S P L Y
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2018,2019 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 *
 * Sets the prompt as supplied then does a DISPLY
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "prototypes.h"
#include "scrnedit.h"
void
pdsply(scrbuf5 *buf, uint8_t *prm, int pch)
{
/* */
  pchars = pch;
  if (pchars > PRSIZ - 1)
  {
    pchars = PRSIZ - 1;
    prm[PRSIZ] = '\0';
  }
  strcpy((char *)prompt, (char *)prm); /* Move in the prompt */
  if (forych)
  {
    forych = false;                /* YCH sets it every line */
    sdsply();
  }
  else
    disply(buf, false);
  return;
}
