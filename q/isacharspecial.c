/* >%---- CODE_STARTS ./isacharspecial.c */
/* I S A C H A R S P E C I A L . C
 *
 * Copyright (C) 2015, Duncan Roe
 *
 */

/* System headers */

/* >%---- KEEP2HERE ./isacharspecial.c */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Local headers */

#include "isacharspecial.h"
#include "prototypes.h"
/* >%---- CUT_HERE ./isacharspecial.c */

/* Code */

bool isacharspecial(int fd)
{
  int retcod;
  struct stat statbuf;

  retcod = fstat(fd, &statbuf);
  if (retcod == -1)
  {
    fprintf(stderr, "%s. funit %d\r\n", strerror(errno), fd);
    newlin();
    return false;
  }                                /* if (retcod == -1) */
  return S_ISCHR(statbuf.st_mode);
}                                  /* isacharspecial() */
