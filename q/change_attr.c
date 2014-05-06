/* C H A N G E _ A T T R . C
 *
 * Copyright (C) 2014 Duncan Roe
 *
 * This function tries very hard to change terminal settings
 *
 * There's nothing much we can do on an unsuccessful attempt except report to
 * the user.
 * We check for 2 possible error classes:
 * 1. An actual error return from a system call
 * 2. A good return but nothing got done.
 *
 * We do all this because of observed erratic behaviour under Cygwin.
 * Opertaion under Linux has always been flawless.
 */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include "c1in.h"
static void
report_error(int fd, char *func)
{
  fprintf(stderr, "%s. fd %d (%s)\r\n", strerror(errno), fd, func);
}                                  /* report_error() */
void
change_attr(int fd, struct termios *wanted)
{
  struct termios current = { 0 }, changed =
  {
  0};
  int retcod;

  retcod = tcgetattr(fd, &current);
  if (retcod == -1)
  {
    report_error(fd, "tcgetattr");
    return;
  }                                /* if (retcod == -1) */
  while (true)
  {
    if (!memcmp(wanted, &current, sizeof current))
      return;
    retcod = tcsetattr(fd, TCSADRAIN, wanted);
    if (retcod == -1)
    {
      report_error(fd, "tcsetattr");
      return;
    }                              /* if (retcod == -1) */
    retcod = tcgetattr(fd, &changed);
    if (retcod == -1)
    {
      report_error(fd, "tcgetattr");
      return;
    }                              /* if (retcod == -1) */
    if (!memcmp(&changed, &current, sizeof current))
    {
      errno = 0;                   /* To signal no change */
      report_error(fd, "tcsetattr");
      return;
    }
    current = changed;
  }                                /* while (true) */
}                                  /* change_attr() */
