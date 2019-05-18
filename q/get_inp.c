/* G E T _ I N P . C
 *
 * Copyright (C) 2019 Duncan Roe */

/* Implement ^<INP> and ^<INPF> run machine opcodes */

/* Headers */

#include "errno.h"
#include "string.h"
#include <ctype.h>
#include <stdlib.h>
#include "alu.h"

bool
get_inp(double *fval, long *val, long *len, char **err)
{
  char *endptr;                    /* 1st char after number */
  uint8_t lastch;                  /* Dump for char we nullify */
  int i;

/* Skip whitespace (including tabs) */
  for (i = last_Curr->bcurs; i < last_Curr->bchars; i++)
    if (!isspace(last_Curr->bdata[i]))
      break;
  if (i == last_Curr->bchars)
  {
    *err = "No number before eol";
    return false;
  }                                /* if (i == last_Curr->bchars) */
  if (!(isdigit(last_Curr->bdata[i]) || last_Curr->bdata[i] == '+' ||
    last_Curr->bdata[i] == '-' || (fval && last_Curr->bdata[i] == '.')))
  {
    *err = "Next item on line is not a number";
    return false;
  }                                /* if (!(isdigit(last_Curr->bdata[i] ... */
  last_Curr->bcurs = i;

/* Force null termination so strtol() will stop at end */
  lastch = last_Curr->bdata[last_Curr->bchars];
  last_Curr->bdata[last_Curr->bchars] = 0;

/* Get the value (cast is to conform with strtol prototype) */
  errno = 0;
  if (val)
    *val = strtol((char *)last_Curr->bdata + i, &endptr, 0);
  else
    *fval = strtod((char *)last_Curr->bdata + i, &endptr);

/* Reinstate zeroed char */
  last_Curr->bdata[last_Curr->bchars] = lastch;

/* Check for overflow. Only check errno (previously zeroed) */
/* since LONG_MAX or LONG_MIN might be returned legitimately */
  if (errno)
  {
    *err = strerror(errno);
    return false;
  }                                /* if (errno) */

/* All OK. Return length and finish */
  *len = endptr - (char *)last_Curr->bdata - i;
  return true;
}                                  /* get_inp() */
