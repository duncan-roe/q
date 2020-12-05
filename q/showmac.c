/* S H O W M A C */
/*
 * Copyright (C) 1993, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2014,2019-2020 Duncan Roe
 *
 * This routine expands the body of a macro to standard output.
 */
#include <stdio.h>
#include <ctype.h>
#include "prototypes.h"
#include "macros.h"
#include "fmode.h"
#include "tabs.h"
#include "c1in.h"
#include "alu.h"
void
showmac(int i, FILE *stream)
{
  macro5 *p;                       /* Points to current macro */
  int k, l;                        /* Scratch */
  uint16_t ch;                     /* Character of interest */
  bool ctl_n_pending = false;

  p = scmacs[i];
  if (!p)
  {                                /* Eh? called to expand null macro */
    fprintf(stderr, "!! SHOWMAC called to print null macro %03o !!\r\n", i);
    visbel();
    return;
  }
  if (!(l = p->maclen))
  {
    fprintf(stderr, "!! INTERNAL ERROR - macro %03o has zero length !!\r\n", i);
    visbel();
    return;
  }
  for (k = 0; k < l; k++)
  {
    ch = p->data[k];
    if (ctl_n_pending)
    {
      ctl_n_pending = false;
      if (display_opcode(ch, stream))
        continue;
      fputs("^N", stream);
    }                              /* if (ctl_n_pending) */
    else
    {
      if (ch == CTL_N && INTERPRET_ALU_OPCODES)
      {
        ctl_n_pending = true;
        continue;
      }                            /* if (ch == CTL_N) */
    }                              /* if (ctl_n_pending) else */
    if (ch < SPACE)
      fprintf(stream, "^%c", ch + 0100);
    else if (ch > DEL)
      fprintf(stream, "^<%o>", ch);
    else if (ch == DEL)
      fputs("^?", stream);
    else
      fprintf(stream, "%c%s", ch, ch == CARAT ? "*" : "");
  }
  if (ctl_n_pending)
    fputs("^N", stream);
  fputc('\n', stream);
}
