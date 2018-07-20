/* T I L D E X P N
 *
 * Copyright (C) 1994, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2018 Duncan Roe
 *
 * This routine expands the string A1 to an absolute pathname if it
 * starts with the metapathname "~". The expansion is done in-situ,
 * i.e. A1 is modified if appropriate.
 * If we can't do the expansion (due to size limitations &c), we leave
 * the input string unchanged - failure will occur when it is used.
 */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdlib.h>
#include "prototypes.h"
void
tildexpn(char *path, int pthsiz)
{
  int ln;                          /* I/p string length */
  int tildeln;                     /* Length of tilde part (incl '~') */
  int homlen;                      /* L.O. expanded HOME path (excl trlg /) */
  char *hompth = NULL;             /* Pathname of home dir */
  char *p;
  char p_saved;
  struct passwd *pwent;

  if (path[0] != '~')
    return;
  if ((ln = strlen(path)) >= pthsiz)
    return;                        /* R too big to massage */

/* get length of tilde part */
  for (p = path + 1, tildeln = 1; *p && *p != '/'; p++, tildeln++)
    ;

  if (tildeln == 1)
    hompth = getenv("HOME");       /* Bare ~, use $HOME */
  else
  {
    p_saved = *p;
    *p = 0;                        /* Null terminate user name in path */
    setpwent();                    /* Rewind /etc/passwd from last time */
    do
    {
      do
      {
        errno = 0;
        pwent = getpwent();
      }
      while (pwent == NULL && errno == EINTR);
      if (errno || !pwent)
        break;
      if (!strcmp(path + 1, pwent->pw_name))
      {
        hompth = pwent->pw_dir;
        break;
      }                            /* if (!strcmp(path + 1, pwent->pw_name) */
    }
    while (pwent);
    *p = p_saved;                  /* Restore slash or NUL */
  }                                /* if (tildeln == 1) else */
  if (!hompth)
    return;
  if ((homlen = strlen(hompth)) + ln > pthsiz)
    return;                        /* R not room for full pathname */
  memmove(path + homlen, path + tildeln, ln - tildeln + 1);
  memcpy(path, hompth, homlen);    /* Copy in home path */
  return;                          /* Finished */
}
