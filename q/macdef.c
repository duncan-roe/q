/* M A C D E F . C
 *
 * Copyright (C) 1994 Duncan Roe & Associates P/L
 * Copyright (C) 2012-2014,2017 Duncan Roe
 *
 * This routine carries out the definition of a macro. Most of the work
 * is done by MACDEFW - all we do here is expand to 1 character / short
 */
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "typedefs.h"
#include "macros.h"
#include "tabs.h"
#include "alu.h"

bool
macdef(unsigned int mcnum, unsigned char *buff, int buflen, bool appnu)
{
  int i;                           /* Scratch */
  unsigned short xbuf[Q_BUFSIZ];
  unsigned char saved_end = buff[buflen];

/* Do the ALU macros here */
  if (mcnum >= 07000 && mcnum <= 07777)
  {
    int idx = mcnum & 0777;
    char *endptr;

    errno = 0;
    buff[buflen] = 0;
    ALU_memory[idx] = strtol((char *)buff, &endptr, 0);
    if (!errno)
    {
    check_endptr:
      if (!*endptr)
        goto r_true;
      if (buflen == 2 && toupper(buff[0]) == 'T' &&
        gettab(buff[1], false, &ALU_memory[idx], false))
        goto r_true;
      fprintf(stderr, "Illegal character '%c' in number \"%s\"", *endptr,
        (char *)buff);
      goto r_false;
    }                              /* if (!errno) */

/* On error, try unsigned conversion of hex or octal like N-NEWMACRO does */
    if (errno == ERANGE)
    {
      char *q = (char *)buff;

      while (*q == ' ')
        q++;
      if (*q == '0')
      {
        errno = 0;
        *(unsigned long *)(&ALU_memory[idx]) =
          strtoul((char *)buff, &endptr, 0);
        if (!errno)
          goto check_endptr;
      }                            /* if (*q == '0') */
    }                              /* if (errno == ERANGE) */
    fprintf(stderr, "%s. %s (strtol)", strerror(errno), (char *)buff);
    goto r_false;
  }                                /* if (mcnum >= 07000 && mcnum <= 07777) */
  for (i = 0; i < buflen; i++)
    xbuf[i] = buff[i];
  return macdefw(mcnum, xbuf, buflen, appnu);
r_true:
  buff[buflen] = saved_end;
  return true;
r_false:
  buff[buflen] = saved_end;
  return false;
}
