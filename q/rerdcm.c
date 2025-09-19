/* R E R D C M
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012-2014,2019-2020, 2025 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * To reread a command after an error has been detected.
 * scrdtk() will have left oldcom's
 * cursor after the last parameter read, so all we have to do is
 * transfer this to newcom and reissue the scmnrd().
 * Additionally however if input is from a U-use file, revert to tty.
 * Includes coding to deal with BRIEF mode being on, also after a bad
 * error we force tty o/p back on, and abandon any macro.
 */
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include "prototypes.h"
#include "cmndcmmn.h"
#include "edmast.h"
#include "macros.h"
#include "fmode.h"
#include "c1in.h"

void
rerdcm()
{
  bool use_sccmd = false;

  if (curmac < 0)
    locerr = false;                /* LOCERR irrelevant if ! in scmac */
  if (locerr)                      /* Macro can detect error */
  {
    locerr = false;                /* Reset for next time */
    if (!BRIEF)
      puts("\r");                  /* If message to o/p */
    sccmnd();                      /* Get next command */
    return;                        /* Finished */
  }

  if (offline)
  {
    fputs("\nCommand editing is rather tricky in offline mode: suggest\n"
      "touch <ESC> (will echo \"^[\"), touch <Enter> to get command prompt\n"
      "and enter \"q\" or \"fq\".\n", stderr);
  }                                /* if (offline) */

  restore_stdout();                /* Force on TTY o/p */
  if (USING_FILE)
  {
    while (USING_FILE)
      pop_stdin();
    use_sccmd = noRereadIfMacro;
    fputs(", (input from terminal)", stdout);
  }

/* Abandon any macro if undetectable error */
  if (curmac >= 0)
  {
    notmac(ERROR);                 /* As in SCRDIT */
    use_sccmd = noRereadIfMacro;
  }                                /* if(curmac>=0) */

  noRereadIfMacro = false;
  if (use_sccmd)
  {
    sccmnd();                      /* Get next command */
    return;                        /* Finished */
  }                                /* if (use_sccmd) */
  fprintf(stderr, "%s\r\n", ", correct the command:");
/*
 * If we had a split line, rejoin the 2 halves and leave the cursor
 * on 1st char from 2nd half
 */
  if (cmsplt)                      /* We had a command split */
  {
/* If command massaging means the two commands won't fit back in,
 * reinstate the original */
    if (newcom->bchars + oldcom->bchars > BUFMAX)
      memcpy(oldcom, &cmthis,
        sizeof cmthis - sizeof cmthis.bdata + cmthis.bchars);

/* Move up the 2nd half */
    memmove(&newcom->bdata[oldcom->bchars], newcom->bdata, newcom->bchars);

/* Move in the 1st half */
    memcpy(newcom->bdata, oldcom->bdata, oldcom->bchars);
    newcom->bcurs = oldcom->bchars; /* Set cursor to 1st chr 2nd 1/2 */
    newcom->bchars += oldcom->bchars; /* Set length to total */
  }
  else
  {
/*
 * Get the cursor to the start of token in error.
 * SCRDTK actually tells us where the bad token started.
 */
    oldcom->bcurs = oldcom->tokbeg;
/* Transfer whole buffer owing to massage */
    memcpy(newcom, oldcom,
      sizeof *oldcom - sizeof oldcom->bdata + oldcom->bchars);
  }
  scmnrd();                        /* Get update */
  return;
}
