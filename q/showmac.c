/* S H O W M A C */
/*
 * Copyright (C) 1993, Duncan Roe & Associates P/L
 * Copyright (C) 2012, Duncan Roe
 *
 * This routine expands the body of a macro to standard output.
 */
#include <stdio.h>
#include "alledit.h"
#include "macros.h"
void
showmac(i)
int i;
{
  macro5 *p;                       /* Points to current macro */
  int k, l;                        /* Scratch */
  unsigned short ch;               /* Character of interest */
/* */
  p = scmacs[i];
  if (!p)
  {                                /* Eh? called to expand null macro */
    printf("\a !! SHOWMAC called to print null macro %03o !!\r\n", i);
    return;
  }
  if (!(l = p->mcsize))
  {
    printf("\a !! INTERNAL ERROR - macro %03o has zero length !!\r\n", i);
    return;
  }
  for (k = 0; k < l; k++)
  {
    ch = p->data[k];
    if (ch < SPACE)
      printf("^%c", ch + 0100);
    else if (ch > 0177)
      printf("^<%o>", ch);
    else if (ch == 0177)
    {
      putchar(CARAT);
      putchar('?');
    }
    else
    {
      putchar(ch);
      if (ch == CARAT)
        putchar('*');
    }
  }
  putchar('\n');
}
