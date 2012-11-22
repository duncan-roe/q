/* T I L D E X P N
 *
 * Copyright (C) 1994, Duncan Roe & Associates P/L
 * Copyright (C) 2012, Duncan Roe
 *
 * This routine expands the string A1 to an absolute pathname if it
 * starts with the metapathname "~". The expansion is done in-situ,
 * i.e. A1 is modified if appropriate.
 * If we can't do the expansion (due to size limitations &c), we leave
 * the input string unchanged - failure will occur when it is used.
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef ANSI5
#include <stdlib.h>
#endif
#include "alledit.h"
#ifdef ANSI5
void
tildexpn(char *path)
#else
void
tildexpn(path)
char *path;
#endif
{
  int ln;                          /* I/p string length */
  char temp[Q_BUFSIZ];               /* Holding buffer */
  char *hompth;                    /* Pathname of home dir */
  if (path[0] != '~')
    return;
  if ((ln = strlen(path)) >= BUFMAX)
    return;                        /* R too big to massage */
  if (!(hompth = getenv("HOME")))
    return;                        /* R can't get home pathname */
  if (strlen(hompth) + ln > BUFMAX)
    return;                        /* R not room for full pathname */
  strcpy(temp, path + 1);          /* Take copy of rest of path */
  strcpy(path, hompth);            /* Copy in home path */
  strcat(path, temp);              /* Append original remainder */
  return;                          /* Finished */
}
