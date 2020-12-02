/* C L 5 G E T
 *
 * Copyright (C) 2012,2013,2019-2020 Duncan Roe
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
#include <unistd.h>
#include <sys/types.h>
#include "prototypes.h"
#include "macros.h"
#include "fmode.h"
#include "c1in.h"

bool
cl5get(char *buf, int bufcap, bool action_eof, bool read_macros)
{
  uint8_t thisch;                  /* Character read */
  int nchars = 0;                  /* Total chars read */
/*
 * Loop inputting characters
 */
  for (;;)
  {
    if (read_macros && curmac >= 0)
    {
      if (mcposn >= scmacs[curmac]->maclen)
      {
        notmac(NORMAL);
        continue;
      }                            /* if (mcposn >= scmacs[curmac]->maclen) */
      thisch = scmacs[curmac]->data[mcposn++];
    }                              /* if (read_macros && curmac >= 0) */
    else if (action_eof)
    {
      bool eof_encountered;

      thisch = c1in5(&eof_encountered); /* Read 1 char */
      if (eof_encountered)
        return false;
    }                              /* if (action_eof) */
    else
      thisch = c1in5(NULL);
    LOG(thisch);
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
          fputs("\b \b", stdout);
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
    if (nchars == bufcap - 1)      /* -1 since we nul-terminate */
    {
      visbel();                    /* Bell */
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
  return true;
}
