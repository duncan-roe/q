/* N E W M A C 2
 *
 * Copyright (C) 2012, Duncan Roe
 *
 * Generates a 16-bit-char macro from the string in buf
 */
#include <stdio.h>
#include "alledit.h"
#include "edmast.h"
#include "macros.h"

#define GIVE_UP return false

bool
newmac2(int mcchrs, bool appnu)
{
  unsigned short xpnsion[Q_BUFSIZ];
  int i, k, m, l;
  unsigned short thisch;           /* Character being examined */

  m = 0;                           /* Accumulates macro size */
  for (i = 0; i < mcchrs; i++)     /* Loop on chars stored */
  {
    thisch = buf[i];
    if (thisch != CARAT)
      goto p1003;                  /* J not precursor of a control */
    if (++i == mcchrs)             /* If nothing to precurse (error) */
    {                              /* (and point to next i/p char) */
    p1201:
      printf("Macro specification ends mid-sequence");
      GIVE_UP;
    }
    thisch = buf[i];               /* Get control */
/* Allow control characters to be spec'd as lower case letters ... */
    if ((thisch < 'a' || thisch > 'z') && (thisch < 0100 || thisch > 0137))
      goto p1005;
    thisch = (thisch & 037);       /* Convert to a control */
    goto p1003;                    /* J to store control char */
/*
 * P1005 - Not a normal control. May have ^* (^) or ^? (rubout)
 *         May also have up to 16-bit octal value ^<ooo...>
 */
  p1005:
    if (thisch == LT)
    {                              /* Try for an octal sequence */
      for (thisch = 0;;)
      {                            /* thisch will accumulate result */
        if (++i == mcchrs)
          goto p1201;              /* J ran out of i/p (error) */
        k = (l = buf[i]) - '0';    /* Get octal digit */
        if (k < 0)
          goto p1104;              /* J wasn't octal */
        if (k > 7)
          break;                   /* B wasn't octal, may be ">" */
        if (thisch & 0160000)
        {                          /* Would overflow */
          printf("Octal macro char out of range (>177777)");
          GIVE_UP;
        }
        thisch = (thisch << 3) + k; /* Accumulate digit */
      }
      if (l == GT)
        goto p1003;                /* J was trailing ">", store char */
    p1104:
      printf("Bad octal character (%c) in macro definition", l);
      GIVE_UP;
    }
    if (thisch == ASTRSK)
      thisch = CARAT;
    else if (thisch == QM)
      thisch = 0177;               /* ^? -> DEL */
    else
    {
      printf("unrecognised control character: ^%c", thisch);
      GIVE_UP;
    }
/* */
  p1003:xpnsion[m++] = thisch;
  }                                /* end of for() */
/* Now try to store macro */
  if (!macdefw(verb, xpnsion, m, appnu))
    GIVE_UP;
/*
 * If the current macro has just redefined itself, get out of it now
 * We need to do this so Q massage will work on the first arg, with
 * certain .qrc's
 */
  if (curmac == verb)
    notmac(0);
  return true;                     /* Successful end */
}
