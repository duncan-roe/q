#ifndef TABS_H
#define TABS_H
/*
 * Copyright (C) 2014,2019 Duncan Roe
 *
 * This header file contains items relating to the tabs subsystem
 *
 * Tabs that reference character position are stored zero-based,
 * but the T-tabset command addresses the first tab as tab 1.
 * The value stored in a character position tab is also zero-based,
 * but the actual tab characters present them as 1-based,
 * i.e. the first character in a line is column 1.
 * Thus the command "t 36" sets tabs[0].value to 35,
 * keying a tab then skipping to column 36.
 * The ALU sees their internal representation however.
 */

/* Headers required by this header */

#include "typedefs.h"

/* Macro definitions */

#define NUM_TABS 80

/* Typedefs */

struct tabs
{
  enum
  { UNDEFINED, CHRPOS, LINENUM } tabtyp;
  long value;
};                                 /* struct tabs */

/* Function prototypes */

bool gettab(uint8_t tabid, bool filpos, long *i4, bool return_index);

/* External variables */

extern struct tabs tabs[NUM_TABS];
#endif
