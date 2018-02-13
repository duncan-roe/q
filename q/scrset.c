/* S C R S E T
 *
 * Copyright (C) 1981 D. C. Roe
 * Copyright (C) 2012,2014,2018 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 *
 * This routine sets up an image of what the screen should look like,
 * and where the cursor should be.
 */
#include <stdio.h>
#include <memory.h>
#include "prototypes.h"
#include "scrnedit.h"
#include "fmode.h"
/* */
void
scrset(scrbuf5 *line)
{
  bool pp, cntrl, phigh;
/*
 * LOCAL VARIABLES
 * ===============
 *
 * i,j &c - Loop indices
 * partno - Section # of short record being dealt with
 * cdone  - # of source chars dealt with so far
 * ichars - # of chars on line
 * thisch - Char being dealt with
 * cntrl  - true if THISCH is a control or '^'
 * xcurs  - Logical cursor pos'n (saves array accesses)
 * phigh - true if THISCH has parity bit set
 */
  int i, j, ichars, thisch, xcurs;
  unsigned char *p;
/*
 * Initial Tasks:-
 */
  if (endlin)
    goto p1201;                    /* Some initialising done by DISPLY */
  partno = 1;                      /* 1st section of line */
  cdone = 0;                       /* No chars dealt with yet */
p1201:
  xcurs = line->bcurs;
  cursr = -1;                      /* Don't know where cursor should be */
  ichars = line->bchars;           /* Avoids some array accesses */
  pp = pchars != 0;                /* PP=>Prompt */
/*
 * Initial tasks for this line
 */
p1014:
  p = reqd;
  for (i = WCHRS; i > 0; i--)
    *p++ = SPACE;                  /* Spacefill reqd buffer */
  icurs = -1;                      /* At start of line */
  if (!pp)
    goto p1002;                    /* J no prompt or  markers */
  if (partno == 1)
    goto p1003;                    /* J 1st part */
  reqd[0] = '(';                   /* Move in '(P' */
  reqd[1] = 'P';                   /* Move in '(P' */
  sprintf((char *)&reqd[2], "%3d", partno);
/* Move in part # */
/*
 * Later, check I
 */
  reqd[5] = ')';                   /* Moves in ') ' */
  icurs = 6;
  goto p1002;                      /* Finished part# */
p1003:
  icurs = pchars - 1;              /* Pchars chars moved */
  memcpy((char *)reqd, (char *)prompt,
#ifdef ANSI5
    (size_t)
#endif
    pchars);
/* Move in the prompt */
/*
 * End line initials
 */
p1002:if (ichars != cdone)
    goto p1004;
/* J non-empty line */
  cursr = icurs + 1;               /* Put curs strt after any prompt */
  return;                          /* Finished */
p1004:if (cdone != ichars)
    goto p1005;
/* J more on line */
/* If curs after last chr, set it now */
p1101:
  if (xcurs == ichars)
    cursr = icurs + 1;
  if (cursr >= 0)
    return;                        /* J cursor was found */
  puts("!missed cursor (scrset)!\r");
/* Report (bug) */
  return;
/*
 * Move char to screen. Control chars are displayed as '^'CHAR+:100.
 * Rubout is displayed as '^?'.
 * '^' is displayed as '^*'. If there is not room on a line to display
 * both of these, display '^' & repeat on next line. If the cursor is
 * pointing at one such, then if in insert mode - ok: display a space
 * else repeat the pair on the next line with the cursor pointing as
 * always to the second of the pair.
 * Characters with the parity bit =1 are displayed as '^<xxx>'
 * where 'xxx' is their octal value.
 */
p1005:icurs = icurs + 1;           /* Point to next vacant space */
  thisch = line->bdata[cdone++];   /* Get this char */
  phigh = (thisch & 0200) != 0;    /* Remember high parity bit */
  cntrl = thisch < SPACE || phigh; /* Set CNTRL if non printing */
/* Set CNTRL if '^' 2b shown "^*" */
  if (thisch == CARAT && fmode & 040)
    cntrl = true;
  else if (thisch == 0177)
    cntrl = true;                  /* Set CNTRL if DEL */
  if (cntrl)                       /* If not guaranteed room */
  {
    i = 0;
    if (phigh)
      i = 4;                       /* I extra amount req'd for octal */
/* check whether room for this character as expanded, or room for the
 * insert space if at the cursor position and inserting... */
/* If not room */
    if (icurs >= WCHRS - i - 1 && !(insert && cdone == xcurs + 1))
    {
      cdone--;                     /* Couldn't do this one after all */
      reqd[icurs] = CARAT;         /* Put in a '^' */
      if (phigh)
        goto p1401;                /* J some octal to try to store */
      goto p1008;                  /* J to eol sequence */
    }
  }
  if (cdone == xcurs + 1)          /* If this is cursor pos */
  {
    cursr = icurs;
    if (insert)                    /* If displaying insert space */
    {
      reqd[icurs] = SPACE;         /* Move in space */
      if (++icurs == WCHRS)
        goto p1008;                /* J screen now full */
    }
    else
    {
      if (cntrl)
        cursr++;                   /* Moves cursor to char after '^' */
      if (phigh)
        cursr++;                   /* Moves cursor to 1st octal digit */
    }
  }
/*
 * The insert space (if any) has been dealt with, and there is
 * definitely (?) room for this char, be it a control or no.
 */
  if (cntrl)
  {
    reqd[icurs] = CARAT;
    if (icurs + 1 == WCHRS)
      goto p1008;                  /* J screen now full */
    if (phigh)                     /* If octal to store */
    {
/* P1401 - Insert octal char. If it all fits in, leave '>' in THISCH,
 *	   otherwise -> P1008. */
    p1401:
      if (++icurs == WCHRS)
        goto p1008;                /* J no room for anything */
      reqd[icurs] = LT;
      if (++icurs == WCHRS)
        goto p1008;                /* J end of line now */
      thisch <<= 7;
      for (j = 3; j > 0; j--)
      {
        reqd[icurs] = (thisch >> 13) + '0';
        thisch <<= 3;
        thisch &= 0177777;         /* In case 32-bit int */
        if (++icurs == WCHRS)
          goto p1008;              /* J end of line now */
      }
      thisch = GT;
    }
    else
    {
      icurs = icurs + 1;
      if (thisch == CARAT)
        thisch = ASTRSK;           /* O/p '*' */
      else if (thisch == 0177)
        thisch = QM;               /* O/p DEL */
      else
        thisch = thisch + 0100;    /* O/p appropriate symbol */
    }
  }
  reqd[icurs] = thisch;
/*
 * Test for internal consistency
 */
  if (icurs <= WCHRS)
    goto p1013;                    /* J ok */
  puts("\r\n!cursor overshoot (SCRSET)!\r");
p1013:
  if (icurs + 1 < WCHRS)
    goto p1004;                    /* J more room on screen */
  if (cdone == ichars)
    goto p1101;
/* J poss finished anyway */
/*
 * Internal consistency check
 */
  if (cdone < ichars)
    goto p1008;
  puts("\r\n!short record overshoot (SCRSET)!\r");
/*
 * P1008 - The current screen line is full. If the cursor has been
 *	  found, routine has finished. Else keep looking...
 */
p1008:partno = partno + 1;
  if (endlin)
    return;                        /* Finished if for DISPLY */
  if (cursr >= 0)
    return;
  goto p1014;
}
