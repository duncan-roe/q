/* D U M P _ R E G I S T E R S . C
 *
 * Copyright (C) 2014,2017,2019 Duncan Roe
 */
#include <stdio.h>
#include "alu.h"
#include "fmode.h"

void
dump_registers(bool append_newline)
{
  int i;

  fprintf(stderr, "Index next PSH[F] / POP[F] is %s\r\n",
    index_next ? "TRUE" : "FALSE");
  fprintf(stderr, "Skip next 2 macro chars is %s\r\n",
    alu_skip ? "TRUE" : "FALSE");
  fprintf(stderr, "Storing %s-position tabs\r\n",
    STORE_FILE_POS ? "file" : "cursor");
  if (rsidx < 0)
    fprintf(stderr, "R is empty\r\n");
  else
  {
    fprintf(stderr, "R has %d entr%s:-\r\n", rsidx + 1,
      rsidx ? "ies (top first)" : "y");
    for (i = rsidx; i >= 0; i--)
      fprintf(stderr, "%ld\r\n", rs[i]);
  }                                /* if (rsidx < 0) else */
  if (fsidx < 0)
    fprintf(stderr, "F is empty\r\n");
  else
  {
    fprintf(stderr, "F has %d entr%s:-\r\n", fsidx + 1,
      fsidx ? "ies (top first)" : "y");
    for (i = fsidx; i >= 0; i--)
      fprintf(stderr, "%.17e\r\n", fs[i]);
  }                                /* if (fsidx < 0) else */
  fprintf(stderr, "X = %ld%s", xreg, append_newline ? "\r\n" : "");
  fprintf(stderr, "Q = %ld%s", qreg, append_newline ? "\r\n" : "");
}                                  /* dump_registers()  */
