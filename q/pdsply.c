/* P D S P L Y
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012, Duncan Roe
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
#ifdef ANSI5
void
pdsply(scrbuf5 *buf, unsigned char *prm, int pch)
#else
void
pdsply(buf, prm, pch)
scrbuf5 *buf;
unsigned char *prm;
int pch;
#endif
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
    goto p1002;                    /* J BRIEF display wanted */
  disply(buf, 0);
p1003:return;
p1002:forych = false;              /* YCH sets it every line */
  sdsply();
  goto p1003;
}
