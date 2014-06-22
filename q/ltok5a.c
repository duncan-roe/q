/* L T O K 5 A
 *
 * Copyright (C) 1993 Duncan Roe & Associates P/L
 * Copyright (C) 2012,2014 Duncan Roe
 *
 * This routine searches for a string as a token. The search is case
 * independent iff CASDEP is 1.
 * XLATABLE is set up by XLASET if not in the required state.
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
 *  The function result is 1 if a match is found, else 0.
 */
#include <stdio.h>
#include "alledit.h"
#include "fmode.h"
/* */
unsigned char xlatable[256];
int tbstat;
/* */
#ifdef ANSI5
int
ltok5a(unsigned char *srchstr, int srchlen, unsigned char *string,
  int first, int len, int *strtpos, int *endpos, unsigned char *ndel)
#else
int
ltok5a(srchstr, srchlen, string, first, len, strtpos, endpos, ndel)
unsigned char *srchstr, *string, *ndel;
int srchlen, first, len, *strtpos, *endpos;
#endif
{
  unsigned char cfirst,            /* 1st char to look for, u/c if casind */
   *p, *q, *r,                     /* Scratch */
    d;                             /* Putative token delimiter */
  int i, k,                        /* Scratch */
    l = 0;                         /* Token chack for last char */
/* */
  if (first >= len)
  {
    printf("\aFirst position out of range: first=%d, len=%d (lsub5a)\r\n",
      first, len);
    return 0;
  }
  if (tbstat != CASDEP)
    xlateset();                    /* Get table right */
  cfirst = xlatable[srchstr[0]];   /* Get 1st char */
/*
 *  First off, look for match on 1st char
 */
  p = string + first;
  for (i = len - first - srchlen + 1; i > 0; i--) /* # possible 1st matches */
  {
    if (xlatable[*p++] != cfirst)  /* Xlate for case (in)sensitive */
      continue;                    /* J no match */
/*
 *  Match on 1st char. Ensure preceding character is tokdel, or this is
 *  1st char in string. Note tests done in descending ASCII order...
 */
    if (p == string + 1)
      goto p1002;                  /* J is 1st char */
    d = *(p - 2);                  /* Char before match */
  p1003:
    if (d >= 'a' && d <= 'z')
      goto p1001;                  /* J l/c alpha (not delim) */
    if (d >= 'A' && d <= 'Z')
      goto p1001;                  /* J u/c alpha (not delim) */
    if (d >= '0' && d <= '9')
      goto p1001;                  /* J numeric (not delim) */
    q = ndel;
    for (;;)                       /* Check token non-delimiter table */
    {
      if (!*q)
        break;                     /* J end of table */
      if (d == *q++)
        goto p1001;                /* J matched non-delim */
    }
    if (l)
      goto p1004;                  /* J was for LAST char delim */
/*
 * 1st-char-match preceded by delimiter or at start of range. Check the
 * rest...
 */
  p1002:
    q = p;                         /* 1st of the rest */
    r = srchstr + 1;               /* 2nd char in search string */
    for (k = srchlen - 1; k > 0; k--) /* J rest doesn't match */
      if (xlatable[*q++] != xlatable[*r++])
        goto p1001;
/*
 * We matched the string. Now ensure a token delimiter follows, or this
 * is end of line...
 */
    if (q != string + len)         /* If not at end */
    {
      d = *q;                      /* Putative delimiter */
      l = 1;                       /* Trying for LAST */
      goto p1003;
    }
/*
 * We have a match. Set return variables and leave...
 */
  p1004:
    *strtpos = p - string - 1;
    *endpos = *strtpos + srchlen - 1;
    return 1;
  p1001:l = 0;
  }
/*
 *  Not found string if get here
 */
  return 0;
}
