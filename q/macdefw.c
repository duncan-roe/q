/* >%---- CODE_STARTS ./macdefw.c */
/* M A C D E F W . C
 *
 * Copyright (C) 1994, Duncan Roe & Associates P/L
 * Copyright (C) 2012-2014,2019-2020 Duncan Roe
 *
 * This routine carries out the definition of a macro. Storage is
 * acquired if necessary - if inadequate storage was previously assigned
 * then it is released.
 * The input is an array of uint16_t integers, rather than
 * characters as for MACDEF
 */

/* Headers */

/* >%---- KEEP2HERE ./macdefw.c */
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include "prototypes.h"
#include "macros.h"
/* >%---- CUT_HERE ./macdefw.c */

/* Macros */

#define GIVE_UP goto no_memory
#define Q_PAGE_SIZE 8192           /* Just a guess */

/* Static prototypes */

static bool get_scmac(uint32_t mcnum);

/* Instantiate externals */

macro5 *scmacs[TOPMAC + 1];
bool mctrst, nodup;
int curmac, mcposn, mcnxfr, immnxfr;
struct macinfo mcstck[STKSIZ];
int mcdtum = MCDTUM_BASE;
int immdtum = FIRST_IMMEDIATE_MACRO;

/* ******************************** macdefw ******************************** */

bool
macdefw(uint32_t mcnum, uint16_t *buf, int buflen, bool appnu)
{
  int i;                           /* Scratch */

/* Round up to a 16-byte multiple for malloc */
  size_t s = ((buflen + (appnu ? 2 : 0)) * sizeof(uint16_t) + 15) & ~15;

/* Enough space already available? */
  if (!scmacs[mcnum] && !get_scmac(mcnum))
    GIVE_UP;
  if (scmacs[mcnum]->maccap < buflen + (appnu ? 2 : 0) && scmacs[mcnum]->data)
  {
    free(scmacs[mcnum]->data);
    scmacs[mcnum]->data = NULL;
  }

  if (!scmacs[mcnum]->data)
  {                                /* Need to get memory for macro */
    if (!(scmacs[mcnum]->data = malloc(s)))
    {
    no_memory:
      fprintf(stderr, "MALLOC out of memory in MACDEF\r\n");
      return false;
    }
/* Store how many 16-bit chars there is actually room for... */
    scmacs[mcnum]->maccap = s / sizeof(uint16_t);
  }
  for (i = 0; i < buflen; i++)
    scmacs[mcnum]->data[i] = buf[i];
  if (appnu)
  {
    scmacs[mcnum]->data[i++] = 016; /* ^N */
    scmacs[mcnum]->data[i++] = 0125; /* U */
  }
  scmacs[mcnum]->maclen = i;
  return true;
}

/* ******************************** get_scmac ******************************* */

static bool
get_scmac(uint32_t mcnum)
{
  static macro5 *hunk;             /* Large malloc'd hunk of memory */
  static int hunk_left = 0;

  if (hunk_left < 1)
  {
    hunk = malloc(Q_PAGE_SIZE);
    if (!hunk)
      return false;
    hunk_left = Q_PAGE_SIZE / sizeof (macro5);
  }                                /* if (hunk_left < 1 */
  scmacs[mcnum] = hunk;
  ++hunk;
  --hunk_left;
  memset(scmacs[mcnum], 0, sizeof(macro5));
  return true;
}                                  /* bool get_scmac(uint32_t mcnum) */
