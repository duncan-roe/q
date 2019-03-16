/* Y S N O 5 A
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2013,2019 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include "prototypes.h"
#include "macros.h"
#include "c1in.h"

bool
ysno5a(char *mess, int key)
{
/*
 * This function is the same as the old Primos YSNO$A
 * Parameters - as for YSNO$A except pass a string
 */
  char comlin[4];
  char *p;
  int i;
/* */
p10:
  printf("%s? ", mess);
  if (offline)
  {
    printf("%s", "y\r\n");
    return 1;
  }                                /* if (offline) */
/* Build a command line, forcing to upper case. BEL if it fills up */
/* If run off end of u-use file, clean up and re-prompt */
  if (!cl5get(comlin, 3, true, false)) /* YES is longest string */
  {
    pop_stdin();
    if (curmac > 0)
      notmac(false);
    newlin();
    goto p10;
  }
  for (p = comlin - 1;;)
  {
    if (!*++p)
      break;
    *p = toupper(*p);
  }
  if (!(i = strlen(comlin)))
  {
/*
 * DEFAULT: A$NDEF=NONE, A$DNO='NO', A$DYES='YES'
 */
    if (key == A5DYES)
      goto p30;
    if (key == A5DNO)
      goto p40;
    goto p10;
  }
/*
 * CHECK FOR 'YES', 'OK', AND 'NO'
 */
  if (strncmp(comlin, "NO", (size_t)i) == 0)
    goto p40;
  if (!(strncmp(comlin, "YES", (size_t)i) == 0 ||
    strncmp(comlin, "OK", (size_t)i) == 0))
    goto p10;
/*
 * 'YES' OR 'OK'
 */
p30:
  i = 1;
  return i;
/*
 * 'NO'
 */
p40:
  i = 0;
  return i;
}
