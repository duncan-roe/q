/* T Y P M A C
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2014 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * This function types a list of current non-null macros
 */
#include <stdio.h>
#include "prototypes.h"
#include "macros.h"
#include "alu.h"

void
typmac(void)
{
  int i, j;
  bool gotone = false;             /* At least 1 macro defined */

  duplx5(true);                    /* Enable ^S */
/*
 * Start typing control char macros
 * */
  if (!alu_macros_only)
  {
    for (i = 0; i < 32; i++)
    {
      if (cntrlc)
        break;                     /* User has interrupted */
      if (!scmacs[i])
        continue;                  /* J no macro in this slot */
      gotone = true;
      printf("^%c : ", i + 0100);
      showmac(i);
      putchar('\r');
    }
    for (i = 32; i < 64; i++)
    {
      if (cntrlc)
        break;                     /* User has interrupted */
      if (!scmacs[i])
        continue;                  /* J no macro in this slot */
      gotone = true;
      printf("%c : ", i);
      showmac(i);
      putchar('\r');
    }
    for (i = 128; i <= TOPMAC; i++)
    {
      if (cntrlc)
        break;                     /* User has interrupted */
      if (!scmacs[i])
        continue;                  /* J no macro in this slot */
      gotone = true;
      printf("%03o ", i);
      showmac(i);
      putchar('\r');
    }
  }                                /* if (!alu_macros_only) */
  for (i = 07000, j = 0; i <= 07777; i++, j++)
  {
    if (cntrlc)
      break;                       /* User has interrupted */
    if (!ALU_memory[j])
      continue;
    gotone = true;
    printf("%o 0%0*lo % *ld 0x%0*lX\r\n", i, /* Macro number */
      sizeof(long) - 4 ? 22 : 11, ALU_memory[j], /* Octal */
      sizeof(long) - 4 ? 20 : 11, ALU_memory[j], /* Decimal */
      (int)(sizeof(long) * 2), ALU_memory[j]); /* Hex */
  }                           /* for (i = 07000, j = 0; i <= 07777; i++, j++) */
  for (i = 013000, j = 0; i <= 013777; i++, j++)
  {
    if (cntrlc)
      break;                       /* User has interrupted */
    if (FPU_memory[j] == 0.0)
      continue;
    gotone = true;
    printf("%o %.17e ", i, FPU_memory[j]);
    printf(FPformat, FPU_memory[j]);
    printf("\r\n");
  }                           /* for (i = 07000, j = 0; i <= 07777; i++, j++) */
  if (!gotone && !cntrlc)
  {
    if (alu_macros_only)
      printf("No nonzero memory locations\r\n");
    else
      printf("No macros currently defined\r\n");
  }                                /* if (!gotone && !cntrlc) */
  duplx5(false);
}
