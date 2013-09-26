/* C L 5 G E T
 *
 * Copyright (C) 2012,2013 Duncan Roe
 *
 * Assembles characters in buf,
 * echoing them and performing erase & kill processing,
 * but using Q's erase and cancel chars DEL and ^U.
 * bufcap is how many keyed chars there is room for:
 * there must be an extra place available for the trailing zero byte.
 * O/p BEL chars if the user's typing gets past buffer end.
 * Assume ECHOK is required as well...
 */
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "alledit.h"
#include "c1in.h"
/* */
void
cl5get(char *buf, int bufcap)
{
  unsigned char thisch;            /* Character read */
  int nchars = 0;                  /* Total chars read */
/*
 * Loop inputting characters
 */
  for (;;)
  {
    thisch = c1in5(NULL);
    if (thisch == '\n')
      break;                       /* J EOL */
    if (thisch == '\r')
      break;                       /* J EOL */
/* Only do erase & kill processing for keyboard i/p. */
    if (!USING_FILE)
    {
      if (thisch == DEL)           /* Char to be erased */
      {
        if (nchars > 0)
        {
          nchars--;
          (void)write(1, "\10 \10", 3);
        }
        continue;
      }
      if (thisch == CTL_U)         /* Line to be erased */
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
