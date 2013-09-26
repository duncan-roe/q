/* N O T M A C
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2013 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * Gets out of obeying a macro.
 */
#include <stdio.h>
#include "alledit.h"
#include "macros.h"
#include "scrnedit.h"
#include "c1in.h"

/* Instantiate externals */

bool simulate_q = false;
int simulate_q_idx = 0;

void
notmac(int err)
{
  int i;
/* */
  if (!USING_FILE)                     /* Not in command file */
  {
    duplx5(false);                 /* Disable XOFF recognition */
    nodup = false;
    if (offline)
      simulate_q = true;
  }
  if (err && curmac >= 0)
  {
    mctrst = false;
    printf("\r\n\aStopped in macro 0%o, character %d\r\n", curmac, mcposn);
    if (scmacs[curmac])
    {
      showmac(curmac);
      putchar('\r');
    }
    else
      puts("** Macro not defined **\r");
  }
  curmac = -1;                     /* Not in a macro */
  if (err || BRIEF || NONE)
    for (i = WCHRS - 1; i >= 0; i--)
      screen[i] = '\0';            /* Force refresh */
  mcnxfr = MCDTUM;                 /* No stack */
  immnxfr = FIRST_IMMEDIATE_MACRO;
  for (i = 0; i <= stdidx; i++)
    stdinfo[i].frommac = false;
}
