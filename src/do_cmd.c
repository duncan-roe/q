/* D O _ C M D
 *
 * Obey SHELL command or change working directory
 *
 * Copyright (C) 1995, Duncan Roe & Associates P/L
 * Copyright (C) 2002, Duncan Roe
 * Copyright (C) 2012,2014,2017 Duncan Roe
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "prototypes.h"
#include "edmast.h"
#include "c1in.h"
int
do_cmd()
{
  int result;

  if (!scrdtk(1, (unsigned char *)buf, 3, oldcom)) /* Next token 2 chars max */
    if (!strcmp("CD", buf))
    {
      if (!scrdtk(2, (unsigned char *)buf, BUFMAX, oldcom)) /* Directory name */
      {
        if (!oldcom->toklen)
        {
          buf[0] = '~';
          buf[1] = '\0';
        }
        tildexpn(buf);
        if (chdir(buf))
        {
          printf("%s. %s (chdir)\r\n", strerror(errno), buf);
          return 1;
        }
        printf("[From Q]: \"cd %s\" done.\r\n", buf);
        return 0;
      }
    }                 /* if (scrdtk(2, (unsigned char *)buf, BUFMAX, oldcom)) */
  (void)scrdtk(3, (unsigned char *)NULL, 0, oldcom); /* Reset to line start */
  (void)scrdtk(1, (unsigned char *)NULL, 0, oldcom); /* Skip over '!' */
  (void)scrdtk(4, (unsigned char *)buf, BUFMAX, oldcom); /* Get shell command */
  final5();
  result = cmd(buf);
  init5();
  return result;
}
