/* T Y P M A C */
#include <stdio.h>
#include "alledit.h"
#include "macros.h"
void
typmac(void)
{
/*
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012, Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * This routine types a list of current non-null macros
 */
  int i;                           /* Scratch */
  short gotone;                    /* At least 1 macro defined */
/* */
  gotone = 0;                      /* Haven't found a macro yet */
  duplx5(true);                    /* Enable ^S */
/*
 * Start typing control char macros
 * */
  for (i = 0; i < 32; i++)
  {
    if (cntrlc)
      goto p1001;                  /* R user has interrupted */
    if (!scmacs[i])
      continue;                    /* J no macro in this slot */
    gotone = 1;
    printf("^%c : ", i + 0100);
    showmac(i);
    putchar('\r');
  }
  for (i = 32; i < 64; i++)
  {
    if (cntrlc)
      goto p1001;                  /* R user has interrupted */
    if (!scmacs[i])
      continue;                    /* J no macro in this slot */
    gotone = 1;
    printf("%c : ", i);
    showmac(i);
    putchar('\r');
  }
  for (i = 128; i <= TOPMAC; i++)
  {
    if (cntrlc)
      goto p1001;                  /* R user has interrupted */
    if (!scmacs[i])
      continue;                    /* J no macro in this slot */
    gotone = 1;
    printf("%03o ", i);
    showmac(i);
    putchar('\r');
  }
  if (!gotone)
    printf("No macros currently defined\r\n");
p1001:
  duplx5(false);
}
