/* G E T T A B
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2014,2019 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * Translates the raw tab id char in TABID to a value derived from
 * the relevant tab setting. If FILPOS is true, the tab must be a file
 * position, otherwise it can be anything.
 */
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "tabs.h"
/* Macros */
#define GIVE_UP goto errlbl

bool
gettab(uint8_t tabid, bool filpos, long *i4, bool return_index)
{
  int i;                           /* Scratch */

  if (tabid == '-')                /* Special case for tab 80 */
    i = 79;
  else
    i = tabid - '1';               /* Get a (zero based) subscript */
  if (i < 0 || i >= NUM_TABS)
  {
    fprintf(stderr, "Illegal tab id");
    GIVE_UP;
  }                                /* if (i < 0 || i >= NUM_TABS) */
  if (return_index)
  {
    *i4 = i;
    return true;
  }                                /* if (return_index) */
  if (tabs[i].tabtyp == UNDEFINED)
  {
    fprintf(stderr, "Referencing unset tab");
    GIVE_UP;
  }
  if (filpos && tabs[i].tabtyp != LINENUM)
  {
    fprintf(stderr, "Tab %c is a cursor position", tabid);
    GIVE_UP;
  }
  *i4 = tabs[i].value;             /* Get a value */
  return true;
errlbl:
  fputs(" or ", stderr);
  return false;
}
