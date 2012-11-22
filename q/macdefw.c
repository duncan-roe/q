/* M A C D E F W . C
 *
 * Copyright (C) 1994, Duncan Roe & Associates P/L
 * Copyright (C) 2012, Duncan Roe
 *
 * This routine carries out the definition of a macro. Storage is
 * acquired if necessary - if inadequate storage was previously assigned
 * then it is released.
 * The input is an array of unsigned short integers, rather than
 * characters as for MACDEF
 */
#include <stdio.h>
#include <memory.h>
#ifdef ANSI5
/* Following line for system-defined malloc() & free() only */
#include <stdlib.h>
#endif
#include "alledit.h"
#include "macros.h"
/* */
#ifdef ANSI5
int
macdefw(unsigned int mcnum, unsigned short *buff, int buflen, int appnu)
#else
int
macdefw(mcnum, buff, buflen, appnu)
unsigned int mcnum;                /* Macro # to define */
unsigned short *buff;              /* The macro expansion */
int buflen;                        /* Chars in expansion */
int appnu;                         /* 1 if to append "^NU" else 0 */
#endif
{
  int i, k;                        /* Scratch */
/*
 * see if macro already defined, and big enough...
 */
  if (scmacs[mcnum] && scmacs[mcnum]->maclen < buflen + 2 * appnu)
  {
    free((char *)scmacs[mcnum]);
    scmacs[mcnum] = NULL;
  }
  if (!scmacs[mcnum])
  {
    i = BASEMAC + (buflen + 2 * appnu) * sizeof *buff; /* Bytes req'd */
/*
 * We always round up to a 16-byte multiple for malloc...
 */
    k = ((i + 15) >> 4) << 4;      /* Bytes we will ask for */
#ifdef ANSI5
    if (!(scmacs[mcnum] = (macro5 *) malloc((size_t)k)))
#else
    if (!(scmacs[mcnum] = (macro5 *) malloc(k)))
#endif
    {
      printf("MALLOC out of memory in MACDEF");
      return 0;
    }
/* Store how many chars there is actually room for... */
    scmacs[mcnum]->maclen = buflen + (k - i) / sizeof *buff + 2 * appnu;
  }
  for (i = 0; i < buflen; i++)
    scmacs[mcnum]->data[i] = buff[i];
  if (appnu)
  {
    scmacs[mcnum]->data[i++] = 016; /* ^N */
    scmacs[mcnum]->data[i++] = 0125; /* U */
  }
  scmacs[mcnum]->mcsize = i;
  return 1;
}
