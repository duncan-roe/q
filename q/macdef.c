/* M A C D E F . C
 *
 * Copyright (C) 1994 Duncan Roe & Associates P/L
 * Copyright (C) 2012-2014,2017,2019 Duncan Roe
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

/* Macros */

#define MY_RETURN(x) {buff[buflen] = saved_end; return x;}

/* Static Variables */

static uint16_t xbuf[Q_BUFSIZ];    /* Doesn't need to be on stack */

bool
macdef(uint32_t mcnum, uint8_t *buff, int buflen, bool appnu)
{
  int i;                           /* Scratch */
  uint8_t saved_end = buff[buflen];
  unsigned long ulong_result;      /* Guard against ULONG_MAX on error */
  long long_result;                /* Guard against ULONG_MAX on error */

/* Do the ALU macros here */
  if (mcnum >= 07000 && mcnum <= 07777)
  {
    int idx = mcnum & 0777;
    char *endptr;

    errno = 0;
    buff[buflen] = 0;
    long_result = strtol((char *)buff, &endptr, 0);
    if (!errno)
    {
      ALU_memory[idx] = long_result;
    check_endptr:
      if (!*endptr)
        MY_RETURN(true);
      if (buflen == 2 && toupper(buff[0]) == 'T' &&
        gettab(buff[1], false, &ALU_memory[idx], false))
        MY_RETURN(true);
      fprintf(stderr, "Illegal character '%c' in number \"%s\"", *endptr,
        (char *)buff);
      MY_RETURN(false);
    }                              /* if (!errno) */

/* On error, try unsigned conversion of hex or octal like N-NEWMACRO does */
    if (errno == ERANGE)
    {
      char *q = (char *)buff;

      while (isspace(*q))
        q++;
      if (*q == '0')
      {
        errno = 0;
        ulong_result = strtoul((char *)buff, &endptr, 0);
        if (!errno)
        {
          *(unsigned long *)(&ALU_memory[idx]) = ulong_result;
          goto check_endptr;
        }                          /* if (!errno) */
        fprintf(stderr, "%s. %s (strtoul)", strerror(errno), (char *)buff);
        MY_RETURN(false);
      }                            /* if (*q == '0') */
    }                              /* if (errno == ERANGE) */
    fprintf(stderr, "%s. %s (strtol)", strerror(errno), (char *)buff);
    MY_RETURN(false);
  }                                /* if (mcnum >= 07000 && mcnum <= 07777) */

/* Do the FPU macros here */
  if (mcnum >= 013000 && mcnum <= 013777)
  {
    int idx = mcnum & 0777;
    char *endptr;

    errno = 0;
    buff[buflen] = 0;
    FPU_memory[idx] = strtod((char *)buff, &endptr);
    if (errno)
    {
      fprintf(stderr, "%s. %s (strtod)", strerror(errno), (char *)buff);
      MY_RETURN(false);
    }                              /* if (errno) */
    if (!*endptr)
      MY_RETURN(true);
    fprintf(stderr, "Illegal character '%c' in number \"%s\"", *endptr,
      (char *)buff);
    MY_RETURN(false);
  }                                /* if (mcnum >= 013000 && mcnum <= 013777) */

  for (i = 0; i < buflen; i++)
    xbuf[i] = buff[i];
  return macdefw(mcnum, xbuf, buflen, appnu);
}
