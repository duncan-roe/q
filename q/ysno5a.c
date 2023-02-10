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

  for (;;)
  {
    printf("%s? ", mess);
    if (offline)
    {
      fputs("y\r\n", stdout);
      return true;
    }                              /* if (offline) */
/* Build a command line, forcing to upper case. BEL if it fills up */
/* If run off end of u-use file, clean up and re-prompt */
    if (!cl5get(comlin, 3, true, false)) /* YES is longest string */
    {
      pop_stdin();
      if (curmac >= 0)
        notmac(NORMAL);
      newlin();
      continue;
    }                              /* if (!cl5get(comlin, 3, true, false)) */
    for (p = comlin - 1;;)
    {
      if (!*++p)
        break;
      *p = toupper(*p);
    }                              /* for (p = comlin - 1;;) */
    if (!(i = strlen(comlin)))
    {
/* DEFAULT: A5NDEF=none, A5DNO='no', A5DYES='yes' */
      if (key == A5DYES)
        return true;
      if (key == A5DNO)
        return false;
      continue;
    }                              /* if (!(i = strlen(comlin))) */
/* Check for 'YES', 'OK' and 'NO' */
    if (!strncmp(comlin, "NO", i))
      return false;
    if (!strncmp(comlin, "YES", i) || !strncmp(comlin, "OK", i))
      return true;
  }                                /* for(;;) */
}                                  /* ysno5a() */
