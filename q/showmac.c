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
#include "alu.h"
void
showmac(int i, FILE *stream)
{
  macro5 *p;                       /* Points to current macro */
  int k, l;                        /* Scratch */
  uint16_t ch;                     /* Character of interest */
  bool ctl_n_pending = false;
  char tbuf[16];

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
      uint16_t ch017000 = ch & 017000;

      ctl_n_pending = false;
      if (ch >= FIRST_ALU_OP && ch < FIRST_ALU_OP + num_ops)
      {
        char *q = opcode_defs[alu_table_index[ch - FIRST_ALU_OP]].name;
        char *r = tbuf;

        while (*q)
          *r++ = toupper((uint8_t)*q++);
        *r = 0;
        fprintf(stream, "^<%s>", tbuf);
        continue;
      }             /* if (ch >= FIRST_ALU_OP && ch < FIRST_ALU_OP + num_ops) */
      else if (ch017000 == 05000)
      {
        fprintf(stream, "^<PSH %o>", ch & 0777);
        continue;
      }                            /* else if (ch017000 == 05000) */
      else if (ch017000 == 06000)
      {
        fprintf(stream, "^<POP %o>", ch & 0777);
        continue;
      }                            /* else if (ch017000 == 06000) */
      else if (ch017000 == 011000)
      {
        fprintf(stream, "^<PSHF %o>", ch & 0777);
        continue;
      }                            /* else if (ch017000 == 011000) */
      else if (ch017000 == 012000)
      {
        fprintf(stream, "^<POPF %o>", ch & 0777);
        continue;
      }                            /* else if (ch017000 == 012000) */
      else if (ch >= FIRST_ALU_OP + num_ops &&
        ch < FIRST_ALU_OP + num_ops + NUM_TABS * 2)
      {
        bool is_pop = false;
        int tabidx = ch - FIRST_ALU_OP - num_ops;
        char tab_str[3] = { 0 };

        if (tabidx >= NUM_TABS)
        {
          tabidx -= NUM_TABS;
          is_pop = true;
        }                          /* if (tabidx >= NUM_TABS) */
        if (tabidx == 78)
        {
          *tab_str = '^';
          tab_str[1] = '?';
        }                          /* if (tabidx == 78) */
        else if (tabidx == 79)
          *tab_str = '-';
        else if (tabidx < 78)
          *tab_str = tabidx + '1';
        fprintf(stream, "^<%sTAB %s>", is_pop ? "POP" : "PSH", tab_str);
        continue;
      }                       /* else if (ch >= FIRST_ALU_OP + num_ops && ... */
      else
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
    else if (ch > 0177)
      fprintf(stream, "^<%o>", ch);
    else if (ch == 0177)
      fputs("^?", stream);
    else
      fprintf(stream, "%c%s", ch, ch == CARAT ? "*" : "");
  }
  if (ctl_n_pending)
    fputs("^N", stream);
  fputc('\n', stream);
}
