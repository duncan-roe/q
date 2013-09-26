/* R E R D C M
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2013 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * To reread a command after an error has been detected.
 * SCRDTK will have left OLDCOM'S
 * cursor after the last parameter read, so all we have to do is
 * transfer this to NEWCOM and reissue the CMANRD.
 * Additionally however if COMINPUT is ON, put it OFF.
 * Includes coding to deal with BRIEF mode being on, also after a bad
 * error we force TTY o/p back on, and abandon any SCMACRO.
 */
#include <stdio.h>
#include <memory.h>
#include "alledit.h"
#include "edmast.h"
#include "macros.h"
#include "cmndcmmn.h"
#include "c1in.h"
/* */
void
rerdcm()
{

  int i;                           /* Scratch */
  unsigned char *p, *q;
/* */
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
  restore_stdout();                /* Force on TTY o/p */
  if (USING_FILE)                      /* Input was from file */
  {
    while (USING_FILE)
      pop_stdin();
    noRereadIfMacro = noRereadIfMacro ? 2 : 0;
    printf(", (input from terminal)");
  }
/*
 * Abandon any macro if undetectable error
 */
  if (curmac >= 0)
  {
    notmac(1);                     /* As in SCRDIT */
    noRereadIfMacro = noRereadIfMacro ? 2 : 0;
  }                                /* if(curmac>=0) */
  if (noRereadIfMacro == 2)
  {
    noRereadIfMacro = false;
    sccmnd();                      /* Get next command */
    return;                        /* Finished */
  }                                /* if(noRereadIfMacro==2) */
  noRereadIfMacro = false;
  puts(", correct the command:\r");
/*
 * If we had a split line, rejoin the 2 halves and leave the cursor
 * on 1st char from 2nd half
 */
  if (cmsplt)                      /* We had a command split */
  {
/* If command massaging means the two commands won't fit back in,
 * reinstate the original */
    if (newcom->bchars + oldcom->bchars > BUFMAX)
      memcpy((char *)oldcom, (char *)&cmthis, sizeof(scrbuf5));
/*
 * Move up the 2nd half unless null. May be an overlapping move.
 * Move the trailing zero byte as well (count is thus 1 higher)
 */
    if (newcom->bchars)
    {
      p = &newcom->bdata[newcom->bchars]; /* p = last char to pick up */
/* q = last char to set down */
      q = &newcom->bdata[newcom->bchars + oldcom->bchars];
      for (i = 0; i <= newcom->bchars; i++)
        *q-- = *p--;
    }
/*
 * Move in the 1st half
 */
    memcpy(newcom->bdata, oldcom->bdata,
#ifdef ANSI5
      (size_t)
#endif
      oldcom->bchars);
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
    memcpy((char *)newcom, (char *)oldcom, sizeof(scrbuf5));
  }
  scmnrd();                        /* Get update */
  return;
}
