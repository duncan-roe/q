/* D O _ C M D
 *
 * Obey SHELL command or change working directory
 *
 * Copyright (C) 1995, Duncan Roe & Associates P/L
 * Copyright (C) 2002, Duncan Roe
 * Copyright (C) 2012,2014,2017-2019 Duncan Roe
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "prototypes.h"
#include "edmast.h"
#include "c1in.h"
int
do_cmd()
{
  int result;

  if (!scrdtk(1, (uint8_t *)ubuf, 3, oldcom)) /* Next token 2 chars max */
    if (!strcmp("CD", ubuf))
    {
      if (!scrdtk(2, (uint8_t *)ubuf, BUFMAX, oldcom))
      {                            /* cd arg (if any) in ubuf */
        if (!oldcom->toklen)
        {
          ubuf[0] = '~';
          ubuf[1] = '\0';
        }
        tildexpn(ubuf, BUFMAX);
        if (chdir(ubuf))           /* EINTR not listed  as possible */
        {
          fprintf(stderr, "%s. %s (chdir)\r\n", strerror(errno), ubuf);
          return 1;
        }
        fprintf(stderr, "[From Q]: \"cd %s\" done.\r\n", ubuf);
        return 0;
      }
    }                      /* if (scrdtk(2, (uint8_t *)ubuf, BUFMAX, oldcom)) */
  scrdtk(3, (uint8_t *)NULL, 0, oldcom); /* Reset to line start */
  scrdtk(1, (uint8_t *)NULL, 0, oldcom); /* Skip over '!' */
  scrdtk(4, (uint8_t *)ubuf, BUFMAX, oldcom); /* Get shell command */
  final5();
/* Unary exclamation mark gets interactive shell */
  if (ubuf[0])
    result = cmd(ubuf, false);
  else
  {
    if (!USING_FILE)
      puts("Exit from shell to restart");
    result = system(sh);
    if (result < 0)
      fprintf(stderr, "%s. (system(\"%s\")\n)", strerror(errno), sh);
    if (!USING_FILE)
      puts("Re-entering Q");
  }                                /* if (ubuf[0]) else */
  init5();
  return result;
}
