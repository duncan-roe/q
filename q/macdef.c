/* M A C D E F . C
 *
 * Copyright (C) 1994, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2013, Duncan Roe
 *
 * This routine carries out the definition of a macro. Most of the work
 * is done by MACDEFW - all we do here is expand to 1 character / short
 */
#include <stdio.h>
#include "alledit.h"
/* */
int
macdef(unsigned int mcnum, unsigned char *buff, int buflen, bool appnu)
{
  int i;                           /* Scratch */
  unsigned short xbuf[Q_BUFSIZ];
/* */
  for (i = 0; i < buflen; i++)
    xbuf[i] = buff[i];
  return macdefw(mcnum, xbuf, buflen, appnu);
}
