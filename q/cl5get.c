/* C L 5 G E T
 *
 * Copyright (C) 2012, Duncan Roe
 *
 * Assembles characters in BUF, echoing them and performing erase &
 * kill processing. LATER get erase & kill chars from TERMIO - assume
 * B/S & ^X (CAN) for now.
 * BUFMAX is how many keyed chars there is rrom for, there must be an
 * extra place available for the trailing zero byte.
 * We o/p BEL chars if the user's typing gets past buffer end. Assume
 * ECHOK is required as well...
 */
#include <stdio.h>
#ifdef ANSI5
#include <sys/types.h>
#include <unistd.h>
#endif
#include "termio5.hl"
#include "alledit.h"
#include "c1in.h"
/* */
void
cl5get(buf, bufcap)
char *buf;
int bufcap;
{
  unsigned char thisch;            /* Character read */
  int nchars = 0;                  /* Total chars read */
/*
 * Loop inputting characters
 */
  for (;;)
  {
    thisch = c1in5();
    if (thisch == '\n')
      break;                       /* J EOL */
    if (thisch == '\r')
      break;                       /* J EOL */
/* Only do erase & kill processing for keyboard i/p. Use canonical
 * characters */
/* Don't use canonical chars after al - they seem to reset to defaults
 * at Linux 1.0 */
    if (!USING_FILE)
    {
      if (thisch ==
/* tio5save.c_cc[VERASE] */
        0177)                      /* Char to be erased */
      {
        if (nchars > 0)
        {
          nchars--;
          (void)write(1, "\10 \10", 3);
        }
        continue;
      }
      if (thisch ==
/* tio5save.c_cc[VKILL] */
        '\25')                     /* Line to be erased */
      {
        nchars = 0;
        puts("\r");
        continue;
      }
    }
    if (nchars == bufcap)
    {
      putchar('\a');               /* Bell */
      continue;
    }
    if (!USING_FILE)
      showchar(thisch);
    buf[nchars++] = thisch;        /* A character in */
  }
/*
 * Processing at EOL
 */
  buf[nchars] = '\0';              /* Zero-byte terminate */
  if (!USING_FILE)
    puts("\r");
}
