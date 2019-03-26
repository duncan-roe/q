/* O R D C H
 *
 * Copyright (C) 1993, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2014,2017,2019 Duncan Roe
 *
 * This routine processes a normal chr (or a special that was preceded
 * by ^C), overwriting/inserting as appropriate.
 */
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <memory.h>
#include "prototypes.h"
#include "macros.h"
#include "fmode.h"
#include "cmndcmmn.h"
#include "scrnedit.h"

void
ordch(uint8_t chr, scrbuf5 *scbuf)
{
/*
 * Parameters
 * ==========
 *
 * chr   - the character
 * scbuf  - pointer to the scrnedit buffer
 */
  int i, k;                        /* Scratch */
  uint8_t *p, *q;
  bool done = false;
/*
 * Apart from SCRDIT on encountering a tab (^I),
 * this is the only Q function that can increase the value of the indent point.
 * Do so now, if at the indent point and we have a space.
 */
  if (INDENT && scbuf->bcurs == ndntch && isspace(chr))
  {
    ndntch++;
/*
 * Move indent over any spaces which now follow, unless inserting ...
 */
    if (!insert)
    {
      bool first = true;
      for (;;)
      {
        if (scbuf->bcurs == scbuf->bchars)
          break;                   /* B no more chars follow */
        if (!isspace(scbuf->bdata[scbuf->bcurs + 1]))
          break;                   /* B not a space following */
        if (first)
        {
          scbuf->bdata[scbuf->bcurs] = chr;
          done = true;
          first = false;
        }                          /* if(first) */
        scbuf->bcurs++;
        ndntch++;
      }
    }
  }
  if (scbuf->bcurs == scbuf->bchars || insert)
  {
    if (scbuf->bchars == scbuf->bmxch)
    {
      fprintf(stderr, "%s",
        curmac >= 0 ? "\r\nNo room in line for insert" : "");
      visbel();
      mctrst = false;
      cmover = true;               /* Warn notmac in case input from U-use */
      notmac(true);                /* Want macro diagnostics */
      return;
    }                              /* if (scbuf->bchars == scbuf->bmxch) */
    if (scbuf->bcurs < scbuf->bchars) /* A real insert */
    {
/* We have to zot the r/h portion of line (incl. cursor chr) up 1. */
/* Use a FOR loop to do the overlapping r/h move */
      p = &scbuf->bdata[scbuf->bchars - 1]; /* Last pick up char */
      q = &scbuf->bdata[scbuf->bchars]; /* Last set down char */
      k = scbuf->bchars - scbuf->bcurs; /* # chars to move */
      for (i = 0; i < k; i++)
        *q-- = *p--;
    }
    scbuf->bchars++;               /* Increase line length */
    if (mxchrs < scbuf->bchars)
      mxchrs = scbuf->bchars;      /* For ^R */
  }
  if (!done)
    scbuf->bdata[scbuf->bcurs] = chr; /* Store chr */
  scbuf->bcurs++;                  /* Up cursor */
  return;                          /* Finished */
}
