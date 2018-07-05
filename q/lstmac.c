/* L S T M A C */
/*
 * Copyright (C) 1993, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2014,2015 Duncan Roe
 *
 * This routine lists the current non-null macros to standard output in
 * a form suitable for reinput by U. Generally, the caller will have
 * assigned standard output to a file before calling this routine...
 */
#include <stdio.h>
#include "prototypes.h"
#include "macros.h"
#include "alu.h"
void
lstmac()
{
  int i, j;
/*
 * All macros are output in the form "N ooo <def'n>" where ooo is at
 * least 3 octal digits.
 */
  if (!alu_macros_only)
  {
    for (i = 0; i <= TOPMAC; i++)
    {
      if (!scmacs[i])
        continue;                  /* J no macro in this slot */
      printf("n %03o ", i);
      showmac(i);
    }
  }                                /* if (!alu_macros_only) */
  for (i = 07000, j = 0; i <= 07777; i++, j++)
    if (ALU_memory[j])
      printf("n %o %ld\n", i, ALU_memory[j]);
  for (i = 013000, j = 0; i <= 013777; i++, j++)
    if (FPU_memory[j])
      printf("n %o %.17g\n", i, FPU_memory[j]);
}
