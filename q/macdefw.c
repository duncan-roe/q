/* M A C D E F W . C
 *
 * Copyright (C) 1994, Duncan Roe & Associates P/L
 * Copyright (C) 2012-2014 Duncan Roe
 *
 * This routine carries out the definition of a macro. Storage is
 * acquired if necessary - if inadequate storage was previously assigned
 * then it is released.
 * The input is an array of unsigned short integers, rather than
 * characters as for MACDEF
 */
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include "alledit.h"
#include "macros.h"
/* */
bool
macdefw(unsigned int mcnum, unsigned short *buff, int buflen, bool appnu)
{
  int i, k;                        /* Scratch */
/*
 * see if macro already defined, and big enough...
 */
  if (scmacs[mcnum] && scmacs[mcnum]->maclen < buflen + (appnu ? 2 : 0))
  {
    free((char *)scmacs[mcnum]);
    scmacs[mcnum] = NULL;
  }
  if (!scmacs[mcnum])
  {
    i = BASEMAC + (buflen + (appnu ? 2 : 0)) * sizeof *buff; /* Bytes req'd */
/*
 * We always round up to a 16-byte multiple for malloc...
 */
    k = ((i + 15) >> 4) << 4;      /* Bytes we will ask for */
    if (!(scmacs[mcnum] = (macro5 *)malloc((size_t)k)))
    {
      fprintf(stderr, "MALLOC out of memory in MACDEF");
      return false;
    }
/* Store how many chars there is actually room for... */
    scmacs[mcnum]->maclen = buflen + (k - i) / sizeof *buff + (appnu ? 2 : 0);
  }
  for (i = 0; i < buflen; i++)
    scmacs[mcnum]->data[i] = buff[i];
  if (appnu)
  {
    scmacs[mcnum]->data[i++] = 016; /* ^N */
    scmacs[mcnum]->data[i++] = 0125; /* U */
  }
  scmacs[mcnum]->mcsize = i;
  return true;
}
