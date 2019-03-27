/* M A C D E F W . C
 *
 * Copyright (C) 1994, Duncan Roe & Associates P/L
 * Copyright (C) 2012-2014 Duncan Roe
 *
 * This routine carries out the definition of a macro. Storage is
 * acquired if necessary - if inadequate storage was previously assigned
 * then it is released.
 * The input is an array of uint16_t integers, rather than
 * characters as for MACDEF
 */
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <sys/user.h>
#include "prototypes.h"
#include "macros.h"

/* Macros */

#define GIVE_UP goto no_memory

/* ******************************** get_scmac ******************************* */

static bool
get_scmac(uint32_t mcnum)
{
  static uint8_t *hunk;            /* Large malloc'd hunk of memory */
  static int hunk_left = 0;
  static size_t s = (sizeof(macro5) + sizeof(int *) - 1) & ~(sizeof(int *) - 1);

  if (hunk_left < sizeof(macro5))
  {
    hunk = malloc(PAGE_SIZE);
    if (!hunk)
      return false;
    hunk_left = PAGE_SIZE;
  }                                /* if (hunk_left < sizeof(macro5)) */
  scmacs[mcnum] = (macro5 *)hunk;
  hunk += s;
  hunk_left -= s;
  memset(scmacs[mcnum], 0, s);
  return true;
}                                  /* bool get_scmac(uint32_t mcnum) */

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
