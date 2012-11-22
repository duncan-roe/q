/* G E T T A B
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012, Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * Translates the raw tab id char in TABID to a value derived from
 * the relevant tab setting. If FILPOS is true, the tab must be a file
 * position, otherwise it must not.
 */
#include <stdio.h>
#ifdef ANSI5
#include <sys/types.h>
#include <unistd.h>
#endif
#include "alledit.h"
#include "scrnedit.h"
/* */
short
gettab(tabid, filpos, i4)
int tabid, filpos;
long *i4;
{
/* */
  int i;                           /* Scratch */
/* */
  i = tabid - '1';                 /* Get a (zero based) subscript */
  if (i >= 0)
    goto p1001;                    /* J ok so far */
p1002:
  (void)write(1, "Illegal tab id", 14);
p1005:
  (void)write(1, " or ", 4);
  return 0;
p1001:
  if (i > 79)
    goto p1002;                    /* J too big for a tab */
  if (tabs[i].tabtyp == undefined)
  {
    (void)write(1, "Referencing unset tab", 21);
    goto p1005;
  }
  if (filpos)
  {
    if (tabs[i].tabtyp != linenum)
      goto p1002;
  }
  else if (tabs[i].tabtyp != chrpos)
    goto p1002;
  *i4 = tabs[i].value;             /* Get a value */
  return 1;
}
