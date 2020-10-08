/* I N I T 5 . C */
/*
 * Copyright (C) 1993, Duncan Roe & Associates P/L
 * Copyright (C) 2011-2014,2019-2020 Duncan Roe
 *
 * This routine initialises the terminal i/o system.
 */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include "prototypes.h"
#include "c1in.h"
/* */
struct termios tio5save = { 0 }, tio5 =
{
0};
void
init5()
{
  static bool first = true;
  int i;

  if (first)
  {
    first = false;
    buf5len = 0;

/* Set output to be non-buffered so we can mix write() and printf() */
/* to either stdout or stderr. */
    if (setvbuf(stdout, NULL, _IONBF, 0))
      fprintf(stderr, "%s. stdout (setvbuf)\r\n", strerror(errno));
    if (setvbuf(stderr, NULL, _IONBF, 0))
      fprintf(stderr, "%s. stderr (setvbuf)\r\n", strerror(errno));
/* See if we have access to a tty */
    ttyfd = 0;
    for (i = STDIN5FD; i <= STDERR5FD; i++)
      if (isatty(i))
      {
        SYSCALL(ttyfd, dup(i));
        break;
      }
    if (ttyfd == 0)
      ttyfd = -1;                  /* No tty found */
    else if (ttyfd == -1)
      fprintf(stderr, "%s. fd %d (dup)\r\n", strerror(errno), i);
    else
    {
      fcntl(ttyfd, F_SETFD, fcntl(ttyfd, F_GETFD) | FD_CLOEXEC);
/* Get the termio structure and save it */
      if (tcgetattr(ttyfd, &tio5save) == -1)
      {
        fprintf(stderr, "%s. fd %d (tcgetattr)\r\n", strerror(errno), ttyfd);
        ttyfd = -1;
      }
      else
      {
        tio5 = tio5save;
/* Ensure terminal will be reset on exit */
        atexit(final5);
/* Enable the ISTRIP flag */
        tio5.c_iflag = (tio5.c_iflag & ~(unsigned)IXON) | ISTRIP;
/* Disable the ICANON and ECHO flags */
        tio5.c_lflag = tio5.c_lflag & ~(unsigned)(ICANON | ECHO
#ifdef IEXTEN
          | IEXTEN
#endif
          );
/* Disable output of CR on NL */
        tio5.c_oflag &= ~ONLCR;

/* Disable most special characters except interrupt */
#ifdef VQUIT
        tio5.c_cc[VQUIT] = '\0';
#else
        tio5.c_cc[1] = '\0';
#endif
#ifdef VSUSP
        tio5.c_cc[VSUSP] = '\0';
#ifdef VREPRINT
        tio5.c_cc[VREPRINT] = '\0';
#ifdef VDISCARD
        tio5.c_cc[VDISCARD] = '\0';
        tio5.c_cc[VWERASE] = '\0';
#endif
        tio5.c_cc[VLNEXT] = '\0';
#endif
#endif
/* Read(2) returns if any characters ready */
        tio5.c_cc[VMIN] = 1;
/* No wait for characters */
        tio5.c_cc[VTIME] = 0;
      }                            /* tcgetattr OK */
    }                              /* if (ttyfd > 0) */
  }                                /* if (first) */

/* Get the screen size, whether or not we have a tty */
      setwinsz(0);                 /* No o/p this time */

/* Change the termio structure for stdin */
  if (ttyfd > 0)
    change_attr(ttyfd, &tio5);
}
