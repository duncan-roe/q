/* N O T M A C
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012-2014 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * Gets out of obeying a macro.
 */
#include <stdio.h>
#include "prototypes.h"
#include "macros.h"
#include "scrnedit.h"
#include "fmode.h"
#include "c1in.h"
#include "alu.h"

/* Instantiate externals */

bool simulate_q = false;
int simulate_q_idx;

void
notmac(bool err)
{
  int i;

/* If error, must also get out of U-use file(s) */
  if (err && USING_FILE)
  {
    while (USING_FILE)
      pop_stdin();
    if (!offline)
      printf("(input from terminal)");
  }                                /* if (err && USING_FILE) */

  if (!USING_FILE)                 /* Not in command file */
  {
    duplx5(false);                 /* Disable XOFF recognition */
    nodup = false;
    if (offline)
      simulate_q = true;
    simulate_q_idx = 0;
  }
  if (err)
  {
/* Minimally reset the ALU */
    xreg = 0;
    rsidx = -1;
    fsidx = -1;
    index_next = false;
    alu_skip = false;

    if (curmac >= 0)
    {
      mctrst = false;
      fprintf(stderr,
        "\r\n\aStopped in macro 0%o, character %d\r\n", curmac, mcposn);
      if (scmacs[curmac])
      {
        showmac(curmac);
        putchar('\r');
      }
      else
        puts("** Macro not defined **\r");
    }                              /* if (curmac >= 0) */
  }                                /* if (err) */
  curmac = -1;                     /* Not in a macro */
  if (err || BRIEF || NONE)
    for (i = WCHRS - 1; i >= 0; i--)
      screen[i] = '\0';            /* Force refresh */
  mcnxfr = MCDTUM;                 /* No stack */
  immnxfr = FIRST_IMMEDIATE_MACRO;
  for (i = 0; i <= stdidx; i++)
    stdinfo[i].frommac = false;
}
