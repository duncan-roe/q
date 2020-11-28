/* S C R S E T
 *
 * Copyright (C) 1981 D. C. Roe
 * Copyright (C) 2012,2014,2018-2020 Duncan Roe
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

/* Macros */

#define END_LINE {end_line_wanted = true; break;}
#define FINISH_LINE {if (line->bcurs == line->bchars) cursr = icurs + 1;\
  if (cursr < 0) puts("!missed cursor (scrset)!\r"); /* Report (bug) */ return;}

void
scrset(scrbuf5 *line)
{
  bool cntrl;                      /* thisch needs >1 pos to display */
  bool phigh;                      /* thisch has parity bit set */
  bool end_line_wanted;            /* Set by END_LINE */
  int j;                           /* Loop indices */
  uint16_t thisch;                 /* Char being dealt with */
  int icurs;                       /* Pos'n on screen of last char done */

/* Initial Tasks:- */
  if (!endlin)                     /* No disply() initialising yet */
  {
    partno = 1;                    /* 1st section of line */
    cdone = 0;                     /* No chars dealt with yet */
  }                                /* if (!endlin) */
  cursr = -1;                      /* Don't know where cursor should be */
  for (;;)
  {
/* Initial tasks for this line */
    end_line_wanted = false;
    memset(reqd, SPACE, WCHRS);
    icurs = -1;                    /* At start of line */
    if (pchars != 0)
    {
      if (partno == 1)
      {
        icurs = pchars - 1;        /* Pchars chars moved */
        memcpy(reqd, prompt, pchars); /* Move in the prompt */
      }                            /* if (partno == 1) */
      else
      {
        icurs = sprintf((char *)reqd, "(P%3d)", partno); /* Move in part # */
        reqd[icurs] = ' ';         /* O/write trlg \0 */
      }                            /* if (partno == 1) else */
    }                              /* if (pchars != 0) */
    if (line->bchars == cdone)     /* Empty line */
    {
      cursr = icurs + 1;           /* Put curs strt after any prompt */
      return;                      /* Finished */
    }                              /* if (line->bchars == cdone) */
    do
    {
      if (cdone == line->bchars)
        FINISH_LINE;

/* Move char to screen. Control chars are displayed as '^'CHAR+:100.
 * Rubout is displayed as '^?'.
 * '^' is displayed as '^*' if fm+* is asserted.
 * If there is not room on a line to display * both of these,
 * display '^' & repeat on next line.
 * If the cursor is pointing at one such, then if in insert mode - ok:
 * display a space;
 * else repeat the pair on the next line with the cursor pointing as always
 * to the second of the pair.
 * Characters with the parity bit =1 are displayed as '^<ooo>'
 * where 'ooo' is their octal value. */

      icurs = icurs + 1;           /* Point to next vacant space */
      thisch = line->bdata[cdone++]; /* Get this char */
      phigh = (thisch & 0200) != 0; /* Remember high parity bit */

/* Set cntrl if multi-char display needed */
      cntrl = thisch < SPACE || phigh || (thisch == CARAT && fmode & 040) ||
        thisch == 0177;

      if (cntrl)                   /* If not guaranteed room */
      {
/* check whether room for this character as expanded, or room for the
 * insert space if at the cursor position and inserting... */
        if (icurs >= WCHRS - (phigh ? 4 : 0) - 1 &&
          !(insert && cdone == line->bcurs + 1))
        {
          cdone--;                 /* Couldn't do this one after all */
          reqd[icurs] = CARAT;     /* Put in a '^' */
          if (phigh)
            goto insert_octal_char; /* J some octal to try to store */
          END_LINE;                /* Not room */
        }
      }
      if (cdone == line->bcurs + 1) /* If this is cursor pos */
      {
        cursr = icurs;
        if (insert)                /* If displaying insert space */
        {
          reqd[icurs] = SPACE;     /* Move in space */
          if (++icurs == WCHRS)
            END_LINE;              /* Screen now full */
        }
        else
        {
          if (cntrl)
            cursr++;               /* Moves cursor to char after '^' */
          if (phigh)
            cursr++;               /* Moves cursor to 1st octal digit */
        }
      }
/* The insert space (if any) has been dealt with, and there is
 * definitely (?) room for this char, be it a control or no. */
      if (cntrl)
      {
        reqd[icurs] = CARAT;
        if (icurs + 1 == WCHRS)
          END_LINE;                /* Screen now full */
        if (phigh)                 /* If octal to store */
        {
        insert_octal_char:
          if (++icurs == WCHRS)
            END_LINE;              /* No room for anything */
          reqd[icurs] = LT;
          if (++icurs == WCHRS)
            END_LINE;              /* End of line now */
          thisch <<= 7;
          for (j = 3; j > 0; j--)
          {
            reqd[icurs] = (thisch >> 13) + '0';
            thisch <<= 3;
            if (++icurs == WCHRS)
              END_LINE;            /* End of line now */
          }
          thisch = GT;
        }
        else
        {
          icurs = icurs + 1;
          if (thisch == CARAT)
            thisch = ASTRSK;       /* O/p '*' */
          else if (thisch == 0177)
            thisch = QM;           /* O/p DEL */
          else
            thisch = thisch + 0100; /* O/p appropriate symbol */
        }
      }
      reqd[icurs] = thisch;

/* Test for internal consistency */
      if (icurs > WCHRS)
        puts("\r\n!cursor overshoot (SCRSET)!\r");

    }                              /* do */
    while (icurs + 1 < WCHRS);
    if (!end_line_wanted)
    {
      if (cdone == line->bchars)
        FINISH_LINE;
    }                              /* if (!end_line_wanted) */

/* Internal consistency check again */
    if (cdone > line->bchars)
      puts("\r\n!short record overshoot (SCRSET)!\r");

    partno++;
    if (endlin || cursr >= 0)
      return;
  }                                /* for (;;) */
}
