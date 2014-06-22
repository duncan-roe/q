/* D U M P _ R E G I S T E R S . C
 *
 * Copyright (C) 2014 Duncan Roe
 */
#include <stdio.h>
#include "alu.h"

void
dump_registers(bool append_newline)
{
  int i;

  fprintf(stderr, "Index next PSH / POP is %s\r\n",
    index_next ? "TRUE" : "FALSE");
  fprintf(stderr, "Skip next 2 macro chars is %s\r\n",
    alu_skip ? "TRUE" : "FALSE");
  if (rsidx < 0)
    fprintf(stderr, "R is empty\r\n");
  else
  {
    fprintf(stderr, "R has %d entr%s:-\r\n", rsidx + 1,
      rsidx ? "ies (top first)" : "y");
    for (i = rsidx; i >= 0; i--)
      fprintf(stderr, "%ld\r\n", rs[i]);
  }                                /* if (rsidx < 0) else */
  fprintf(stderr, "X = %ld%s", xreg, append_newline ? "\r\n" : "");
}                                  /* dump_registers()  */
