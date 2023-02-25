/* >%---- CODE_STARTS ./display_opcode.c */
/* D I S P L A Y _ O P C O D E . C
 *
 * Copyright (C) 2020 Duncan Roe
 */

/* Headers */

/* >%---- KEEP2HERE ./display_opcode.c */
#include <ctype.h>
#include "c1in.h"
#include "tabs.h"
#include "alu.h"
/* >%---- CUT_HERE ./display_opcode.c */

bool
display_opcode(uint16_t ch, FILE *stream)
{
  char tbuf[16];

  uint16_t ch017000 = ch & 017000;

  if (ch >= FIRST_ALU_OP && ch < FIRST_ALU_OP + num_ops)
  {
    char *q = opcode_defs[alu_table_index[ch - FIRST_ALU_OP]].name;
    char *r = tbuf;

    while (*q)
      *r++ = toupper((uint8_t)*q++);
    *r = 0;
    fprintf(stream, "^<%s>", tbuf);
    return true;
  }                 /* if (ch >= FIRST_ALU_OP && ch < FIRST_ALU_OP + num_ops) */
  else if (ch017000 == 05000)
  {
    fprintf(stream, "^<PSH %o>", ch & 0777);
    return true;
  }                                /* else if (ch017000 == 05000) */
  else if (ch017000 == 06000)
  {
    fprintf(stream, "^<POP %o>", ch & 0777);
    return true;
  }                                /* else if (ch017000 == 06000) */
  else if (ch017000 == 011000)
  {
    fprintf(stream, "^<PSHF %o>", ch & 0777);
    return true;
  }                                /* else if (ch017000 == 011000) */
  else if (ch017000 == 012000)
  {
    fprintf(stream, "^<POPF %o>", ch & 0777);
    return true;
  }                                /* else if (ch017000 == 012000) */
  else if (ch >= FIRST_ALU_OP + num_ops &&
    ch < FIRST_ALU_OP + num_ops + NUM_TABS * 2)
  {
    bool is_pop = false;
    int tabidx = ch - FIRST_ALU_OP - num_ops;
    char tab_str[3] = "";

    if (tabidx >= NUM_TABS)
    {
      tabidx -= NUM_TABS;
      is_pop = true;
    }                              /* if (tabidx >= NUM_TABS) */
    if (tabidx == 78)
    {
      *tab_str = '^';
      tab_str[1] = '?';
    }                              /* if (tabidx == 78) */
    else if (tabidx == 79)
      *tab_str = '-';
    else if (tabidx < 78)
      *tab_str = tabidx + '1';
    fprintf(stream, "^<%sTAB %s>", is_pop ? "POP" : "PSH", tab_str);
    return true;
  }                           /* else if (ch >= FIRST_ALU_OP + num_ops && ... */
  return false;
}                                  /* display_opcode() */
