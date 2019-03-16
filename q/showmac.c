/* S H O W M A C */
/*
 * Copyright (C) 1993, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2014,2019 Duncan Roe
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
showmac(int i)
{
  macro5 *p;                       /* Points to current macro */
  int k, l;                        /* Scratch */
  unsigned short ch;               /* Character of interest */
  bool ctl_n_pending = false;
  char tbuf[16];

  p = scmacs[i];
  if (!p)
  {                                /* Eh? called to expand null macro */
    fprintf(stderr, "!! SHOWMAC called to print null macro %03o !!\r\n", i);
    visbel();
    return;
  }
  if (!(l = p->mcsize))
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
      if (ch >= FIRST_ALU_OP && ch < FIRST_ALU_OP + num_ops)
      {
        char *q = opcode_defs[alu_table_index[ch - FIRST_ALU_OP]].name;
        char *r = tbuf;

        while (*q)
          *r++ = toupper((unsigned char)*q++);
        *r = 0;
        printf("^<%s>", tbuf);
        continue;
      }             /* if (ch >= FIRST_ALU_OP && ch < FIRST_ALU_OP + num_ops) */
      else if ((ch & 017000) == 05000)
      {
        printf("^<PSH %o>", ch & 0777);
        continue;
      }                            /* else if ((ch & 017000) == 05000) */
      else if ((ch & 017000) == 06000)
      {
        printf("^<POP %o>", ch & 0777);
        continue;
      }                            /* else if ((ch & 017000) == 06000) */
      else if ((ch & 017000) == 011000)
      {
        printf("^<PSHF %o>", ch & 0777);
        continue;
      }                            /* else if ((ch & 017000) == 011000) */
      else if ((ch & 017000) == 012000)
      {
        printf("^<POPF %o>", ch & 0777);
        continue;
      }                            /* else if ((ch & 017000) == 012000) */
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
        printf("^<%sTAB %s>", is_pop ? "POP" : "PSH", tab_str);
        continue;
      }                       /* else if (ch >= FIRST_ALU_OP + num_ops && ... */
      else
        printf("^N");
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
      printf("^%c", ch + 0100);
    else if (ch > 0177)
      printf("^<%o>", ch);
    else if (ch == 0177)
    {
      putchar(CARAT);
      putchar('?');
    }
    else
    {
      putchar(ch);
      if (ch == CARAT)
        putchar('*');
    }
  }
  if (ctl_n_pending)
    printf("^N");
  putchar('\n');
}
