/* L T O K 5 A
 *
 * Copyright (C) 1993 Duncan Roe & Associates P/L
 * Copyright (C) 2012,2014,2018,2019 Duncan Roe
 *
 * This routine searches for a string as a token. The search is case
 * independent iff CASDEP is nonzero.
 * xlatable is set up by xlateset() if not in the required state.
 *
 *  Arguments:-
 *
 *  A1 - String to look for
 *  A2 - Length of string
 *  A3 - Array ctg string to be searched
 *  A4 - 1st char posn of string to search
 *  A5 - Length of string
 *  A6 - (Returned) starting char posn
 *  A7 - (Returned) ending char posn
 *  A8 - Table of non-deliminators. This is a string.
 *
 *  The function result is true iff a match is found.
 */
#include <stdio.h>
#include <ctype.h>
#include "prototypes.h"
#include "fmode.h"

/* Static prototypes */
static bool is_delim(uint8_t d, uint8_t *ndel);

bool
ltok5a(uint8_t *srchstr, int srchlen, uint8_t *string,
  int first, int len, int *strtpos, int *endpos, uint8_t *ndel)
{
  uint8_t cfirst;                  /* 1st char to look for, u/c if casind */
  uint8_t *p, *q, *r;              /* Scratch */
  uint8_t d;                       /* Putative token delimiter */
  int i, k;                        /* Scratch */

  if (first >= len)
  {
    visbel();
    printf("First position out of range: first=%d, len=%d (lsub5a)\r\n",
      first, len);
    return false;
  }                                /* if (first >= len) */
  if (tbstat != CASDEP)
    xlateset();                    /* Get table right */
  cfirst = xlatable[srchstr[0]];   /* Get 1st char */

/*  Look for match on 1st char */
  p = string + first;
  for (i = len - first - srchlen + 1; i > 0; i--)       /* # possible 1st matches */
  {
    if (xlatable[*p++] != cfirst)  /* Xlate for case (in)sensitive */
      continue;                    /* J no match */

/*  Match on 1st char. Ensure preceding character is tokdel, or this is
 *  1st char in string */
    if (p != string + 1)
    {
      d = *(p - 2);                /* Char before match */
      if (!is_delim(d, ndel))
        continue;
    }                              /* if (p != string + 1) */
/* 1st-char-match preceded by delimiter or at start of range. Check the rest */
    q = p;                         /* 1st of the rest */
    r = srchstr + 1;               /* 2nd char in search string */
    for (k = srchlen - 1; k > 0; k--)   /* J rest doesn't match */
      if (xlatable[*q++] != xlatable[*r++])
        return false;

/* String is matched. Ensure token delimiter follows, or this is end of line */
    if (q != string + len)         /* If not at end */
    {
      if (!is_delim(*q, ndel))
        continue;
    }                              /* if (q != string + len) */
/* We have a match. Set return variables and leave... */
    *strtpos = p - string - 1;
    *endpos = *strtpos + srchlen - 1;
    return true;
  }                                /* for (i = len - first - srchlen + 1; ...) */
/*  Not found string if get here */
  return false;
}

/* ******************************** is_delim ******************************** */

static bool
is_delim(uint8_t d, uint8_t *ndel)
{
  uint8_t *q;

  if (isalnum(d))
    return false;                  /* Not a delimiter */
  for (q = ndel;;)
  {
/* Check token non-delimiter table */
    if (!*q)
      break;                       /* End of table */
    if (d == *q++)
      return false;                /* Matched non-delim */
  }                                /* for (q = ndel;;) */
  return true;
}                                  /* is_delim() d) */
