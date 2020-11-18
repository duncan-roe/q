/* N O T M A C
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012-2014,2019-2020 Duncan Roe
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
notmac(notmac_action err)
{
  int i;

/* The notmac_actions ERROR and FQ_FROM_FR do the same thing most of the time */
/* so much of the code still uses err as the bool it used to be. */

/* If error, must also get out of U-use file(s) */
  if (err && USING_FILE)
  {
    while (USING_FILE)
      pop_stdin();
    if (!offline)
      fputs("(input from terminal)", stdout);
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

    if (curmac >= 0 && err == ERROR)
    {
      mctrst = false;
      visbel();
      fprintf(stderr,
        "\r\nStopped in macro 0%o, character %d\r\n", curmac, mcposn);
      if (scmacs[curmac])
      {
        showmac(curmac, stderr);
        putchar('\r');
      }
      else
        puts("** Macro not defined **\r");
    }                              /* if (curmac >= 0) */
  }                                /* if (err) */

/* If curmac was an immediate macro, */
/* that immediate macro is now available for re-use */
  if (curmac >= FIRST_IMMEDIATE_MACRO && curmac <= LAST_IMMEDIATE_MACRO)
    immnxfr = curmac;

  curmac = -1;                     /* Not in a macro */
  if (err || BRIEF || NONE)
    for (i = WCHRS - 1; i >= 0; i--)
      screen[i] = '\0';            /* Force refresh */
  if (err)
  {
/* Get out of all macros and U-USE files */
    mcnxfr = mcdtum;               /* No stack */
    immnxfr = immdtum;
    for (i = stdbase; i <= stdidx; i++)
      stdinfo[i].frommac = false;
  }                                /* if (err) */
  else
  {
/* Only unwind back to the last U-USE file, if any */
    for (; mcnxfr > mcdtum; mcnxfr--)
    {
      if (mcstck[mcnxfr - 1].u_use)
        break;
      if (mcstck[mcnxfr - 1].mcprev >= FIRST_IMMEDIATE_MACRO &&
        mcstck[mcnxfr - 1].mcprev <= LAST_IMMEDIATE_MACRO)
        immnxfr = mcstck[mcnxfr - 1].mcprev;
    }                              /* for (; mcnxfr > mcdtum; mcnxfr--) */
  }                                /* if (err) else */
}
