/* S C R D I T */
/*
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012, Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * This is the top-level screenedit subroutine.
 *
 * On entry, half duplex must be on and the contents of the array
 * "screen" must be valid (i.e. we know what's displaying).
 * (newlin() can be used to initialise screen)
 *
 */
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <sys/types.h>
#include <sys/times.h>
#include "alledit.h"
#include "macros.h"
#include "cmndcmmn.h"
#include "scrnedit.h"
#include "termio5.hl"
#include "c1in.h"
/* */
unsigned char fxtabl[128];
long timlst;
/* */
void
scrdit(curr, prev, prmpt, pchrs, cmmand)
int cmmand;
scrbuf5 *curr, *prev;
char *prmpt;
int pchrs;
{
  unsigned char *p, *q             /* Scratch */
   ;
  char *c                          /* Scratch */
   , *err = NULL                   /* Point to error text */
    , tbuf[20]                     /* Scratch */
    ;
  int i, j, k, l = 0, m = 0,       /* Scratch variables */
    rtn,                           /* Used by local subroutines */
    gotoch                         /* Char of last ^G */
    ;
  long i4                          /* Scratch */
   , olen                          /* Original line length */
   ;
  struct tms tloc;                 /* Junk from TIMES */
  long timnow;                     /* Time from TIMES */
  short thisch;                    /* Character being processed */
/*
 * PARAMETERS:-
 * ============
 *
 * curr  - Current buffer to modify \ these are scrnedit buffers
 * prev  - Previous line modified   / as defined above
 * prmpt - Prompt characters (if any) (string)
 * pchrs - # of prompt chars (zero implies none and PRMPT not a valid
 *                                  addr)
 * cmmand - nonzero => in command mode
 */
  bool contc, nseen, glast, gpseu = false, gposn = false;
  bool gwrit = false, gcurs = false, gtest = false, gpast = false;
  bool gdiff = false, gmacr = false, fornj;
/*
 * LOCAL LOGICAL VARIABLES:
 * ========================
 *
 * contc  - true if last char ^C (nxt ch not special)
 * instmp - Holds actual value of INSERT whilst tabbing.
 * nseen  - If ^N last char so expect macro name
 * glast  - Last char input was ^G
 * gpseu  - GLAST actually set for pseudo macro ^NG
 * gposn  - Next char got by ^G mechanism is for a positioning pseudo
 * gwrit  - This pseudo macro remembers something
 * gcurs  - This pseudo macro deals with screen cursor
 * gtest  - This pseudo macro tests screen cursor position
 * gpast  - This pseudo macro tests screen cursor position past tab
 * gdiff  - This pseudo macro is actually ^NY (not ^NF)
 * gmacr  - This pseudo macro is actually ^NM (not ^NG)
 *
 *   Validate cursor &c
 */
  if ((olen = curr->bchars) > curr->bmxch)
  {
    err = "Bad char cnt curr line\r";
  p1001:
    puts(err);
    verb = 'J';                    /* Let user... */
    goto p1522;                    /* ... salvage his edit. */
  }
/* J bad char cnt this line */
  if (curr->bcurs < 0)             /* Bad cursor this line */
  {
    err = "Curr curs<1\r";
    goto p1001;
  }
  if (pchrs > PRMAX)               /* Prompt too big */
  {
    err = "Prompt>10 chars\r";
    goto p1001;
  }
/* J cursor off end this lin */
  if (curr->bcurs > curr->bchars)
  {
    err = "Curr curs off end of info\r";
    goto p1001;
  }
/*
 * Switch off XOFF if on unless COMINPutting
 */
  if (USING_FILE)
    goto p1521;
  if (curmac >= 0)
    goto p1816;                    /* May want to ^S macro */
  duplx5(false);                   /* No XOFF recognition */
  nodup = false;
  goto p1521;
p1816:nodup = true;                /* So we don't do one on the way out */
/*
 * Initialise common variables &c
 */
p1521:endlin = false;
  modlin = false;
  insert = false;
  pchars = pchrs;
  mxchrs = curr->bchars;
  nseen = false;
  glast = false;
  fornj = false;
  if (pchars == 0)
    goto p1005;                    /* J no prompt to move in */
/*
 * Get a null-terminated prompt buffer first
 */
  strcpy((char *)prompt, (char *)prmpt);
/* Move prompt to common area */
p1005:contc = false;
  cntrlw = false;
  gotoch = -1;                     /* No ^G yet */
/*
 * Normally we do no REFRSH if in an scmac, but if BRIEF is on
 * we here refresh the prompt only (if editing a line)
 *  Speedup: so we don't bank up on TTY o/p (& CPU!), only display
 * new line # if .GT. 1/5 sec since last time ...
 */
  if (curmac >= 0 && !cmmand && BRIEF && !NONE && pchars)
  {
    if ((timnow = times(&tloc)) == -1)
    {
      perror("times");
      putchar('\r');
      goto p1816;
    }
/* Assume a clock tick rate of 100 - not critical. We don't cater for
 * wraparound, currently... */
    if (timnow - timlst >= 20)
    {
      timlst = timnow;             /* Displaying */
      sdsply();                    /* Display the line number */
    }                              /* if (timnow - timlst >= 20) */
  }                /* if (curmac >= 0 && !cmmand && BRIEF && !NONE && pchars) */
/*
 * If this is a new line and INDENT is on, pad out with appropriate
 * number of spaces. If not a new line and INDENT on, get new
 * indent position.
 */
  if (INDENT)
  {
    if (!curr->bchars)
    {
      if (!lstvld)
        sindnt();                  /* Get the indent */
      if (ndntch == 0)
        goto p1027;                /* No indenting anyway */
      for (i = ndntch; i > 0; i--)
        ordch(SPACE, curr);        /* Pad out with spaces */
      modlin = true;               /* Line has been changed */
      goto p1027;                  /* Finish new line */
    }                              /* if !(curr->bchars) */
    ndntch = 0;
    for (j = curr->bchars; j > 0; j--)
    {
      if (curr->bdata[ndntch] != SPACE)
        break;
      ndntch = ndntch + 1;
    }
/* Don't alter cursor if positioned by LOCATE */
    if (curr->bcurs < ndntch)
      curr->bcurs = ndntch;
  }                                /* if (INDENT) */
  else
    ndntch = 0;                    /* Not indenting, so no indent chars */
/* */
p1027:
  mctrst = false;
  if (curmac < 0)
    refrsh(curr);                  /* Display prompt etc */
p1026:
  if (curmac >= 0)
  {
/* Clear expanding if that was last char from macro */
    if (mcposn >= scmacs[curmac]->mcsize)
      goto p1515;
    thisch = scmacs[curmac]->data[mcposn];
    mcposn = mcposn + 1;
  }                                /* if (curmac >= 0) */
  else
  {
    thisch = c1in5();              /* Read 1 char */
/*
 * FX command implementation - replace char read by one from the
 * table unless we think we are cominputting or by some strange
 * chance the char is parity-high.
 */
    if (!USING_FILE && !(thisch & 0200))
      thisch = fxtabl[thisch];
  }                                /* if (curmac >= 0) else */
  if (contc)
    goto p1802;                    /* J ^C last char */
  if (nseen)
    goto p1502;                    /* J expecting macro name */
  if (glast)
    goto p1518;                    /* J in middle of ^G */
  if (thisch == 0177)
    goto p10065;                   /* J it was a rubout */
  if (thisch & 0200)
    goto p1007;                    /* J parity-high */
  if (thisch >= SPACE)
    goto p1007;                    /* J not a control */
  verb = thisch + 0100;            /* 'J' for ^J, &c. */
  switch (thisch)
  {
    case 0:
      goto p7700;
    case 1:
      goto p7701;
    case 2:
      goto p7702;
    case 3:
      goto p7703;
    case 4:
      goto p7704;
    case 5:
      goto p7705;
    case 6:
      goto p7706;
    case 7:
      goto p7707;
    case 8:
      goto p7710;
    case 9:
      goto p7711;
    case 10:
      goto p7712;
    case 11:
      goto p7713;
    case 12:
      goto p7714;
    case 13:
      goto p7712;                  /* Treat Cr as Nl */
    case 14:
      goto p7716;
    case 15:
      goto p7717;
    case 16:
      goto p7703;                  /* ^P - next char not special */
    case 17:
      goto p7721;
    case 18:
      goto p7722;
    case 19:
      goto p7723;
    case 20:
      goto p7724;
    case 21:
      goto p7725;
    case 22:
      goto p7726;
    case 23:
      goto p7727;
    case 24:
      goto p7730;
    case 25:
      goto p7731;
    case 26:
      goto p7732;
    case 27:
      goto p7733;
    case 30:
      goto p7736;
  }
  goto p1007;                      /* Not special control */
p1802:
  if (cntrlw)
    thisch |= 0200;                /* Set parity following ^W */
  cntrlw = false;
  if (nseen)
    goto p1905;                    /* Treat as macro char if ^N prev */
  goto p1007;                      /* Treat as ordinary character */
/*
 * RUBOUT - Deletes char before cursor
 */
p10065:
  k = curr->bcurs - 1;
  if (k < 0)
    goto p1023;                    /* J at start lin (error) */
  modlin = true;                   /* Line has been changed */
/* If at indent point,decrement INDENT */
  if (INDENT && k + 1 == ndntch)
    ndntch--;
  j = curr->bchars - 1;            /* How many chars there will be */
  if (j != k)
  {
    q = &curr->bdata[k];           /* Where chars to move to */
    p = q + 1;                     /* Where chars to come from */
    for (i = j - k; i > 0; i--)
      *q++ = *p++;                 /* Move chars back */
  }                                /* if (j != k) */
  curr->bcurs = k;                 /* Step cursor back */
  curr->bchars = j;                /* Reduce # of chars */
p7700:
/* Get another character if one available */
  if (curmac >= 0 || USING_FILE || kbd5())
    goto p1026;
  goto p1027;                      /* Refresh */
p1023:
  err = "Ran off beginning or end of line";
p10231:
  if (curmac >= 0)
  {
    if (*err)
      printf("\r\n%s. ", err);
    notmac(1);
  }
p10232:
  putchar('\a');                   /* O/p BEL since error */
  mctrst = false;                  /* User no longer trusted */
  goto p7700;                      /* End rubout */
w1023:
  if (curmac >= 0)
  {
    printf("\r\nTab ID %c or value in that tab out of range. ", thisch);
    notmac(1);
  }
  goto p10231;
/*
 * Non-special character
 */
p1007:ordch(thisch, curr);         /* Insert or replace as appropriate */
  modlin = true;                   /* Line has been changed */
  contc = false;
  goto p7700;                      /* Finish */
/*
 * ^A - Again/Append
 * If more info in previous line than current, append previous excess
 * to current, then in either case move cursor to follow last char
 */
p7701:i = ndntch;                  /* In case we  SINDNT */
  if (!lstvld)
    sindnt();                      /* Get previous line valid */
  ndntch = i;
  if (curr->bchars >= prev->bchars)
    goto p1029;
/* J append only */
/*
 * We have to move chars from the previous line
 */
  modlin = true;                   /* Line has been changed */
  memcpy((char *)&curr->bdata[curr->bchars],
    (char *)&prev->bdata[curr->bchars], (size_t)(prev->bchars - curr->bchars));
  curr->bchars = prev->bchars;     /* Extend length of line */
  if (mxchrs < curr->bchars)
    mxchrs = curr->bchars;
/* For ^R */
p1029:
  curr->bcurs = curr->bchars;      /* Put cursor at end */
  goto p7700;                      /* Finish ^A */
/*
 * ^B - Go back to the start of this word or back to the start of the
 * previous word if at the start of this one. Words are delimited by
 * spaces only. Error if no backwards movement possible.
 * N.B. unlike D.G., we stop on the first character of the word.
 */
p7702:i = 0;                       /* S.O.L. if no indenting */
  if (INDENT)
    i = ndntch;
  k = curr->bcurs - i;             /* K - running available movement */
/* Note k can be -ve, if he used ^NR to move before the indent point */
  if (k <= 0)
    goto p1023;                    /* J start of line (error) */
/* Check whether at word start. If so, back up to a non-space (or line
 * start)... */
  p = &curr->bdata[curr->bcurs];
/* Reduce movement available, B none left; B found non-space */
  if (*(p - 1) == SPACE)
    for (;;)
      if (!--k || *--p != SPACE)
        break;
/* We may now be back at line start. If not, move cursor back until the
 * preceding character is space. This may now be the case already, so
 * check the value of the preceding character first... */
  if (k)
    for (;;)
      if (*--p == SPACE || !--k)
        break;
  curr->bcurs = i + k;             /* Start of line + room */
  goto p7700;                      /* Finish ^B */
/*
 * ^C - nextch not special
 */
p7703:contc = true;
  goto p7700;                      /* Finish ^C */
/*
 * ^E - Enter/Leave insert mode
 */
p7705:insert = !insert;
  goto p7700;                      /* End insert */
/*
 * ^F - Forward to start next word. If at e.o.l., no-op;
 * if on last word, stop at e.o.l.
 */
p7706:
  k = curr->bchars - curr->bcurs;  /* K - how far we can move forward */
/* If on non-space, move forward to space, unless at EOL now... */
  p = &curr->bdata[curr->bcurs];
/* Ensure we're pointing at a space (or we were). Always reduces K,
 * unless at EOL to start with... */
  if (k < 0)
  {
    printf("Internal error: k=%d\r", k);
    notmac(1);
    goto p10232;
  }
  if (k)
    for (;;)
      if (!--k || *p++ == SPACE)
        break;
/* Reduce K further if not now pointing at a non-space... */
  if (k)
    for (;;)
      if (*p++ != SPACE || !--k)
        break;
  curr->bcurs = curr->bchars - k;
  goto p7700;                      /* Refresh or read */
/*
 * ^H - Home
 */
p7710:
  curr->bcurs = 0;                 /* Reset cursor */
  if (INDENT)
    curr->bcurs = ndntch;          /* Set to S.O.Data if indenting */
  goto p7700;                      /* Finish ^H */
/*
 * ^I - Insert a tab. We insert enough spaces to get to next tab
 * posn. if there is one, otherwise insert 1 space. once inserted,
 * there is no remembrance that this was a tab - spaces may be
 * individually deleted etc.
 */
p7711:
  k = curr->bcurs;                 /* Useful to know */
  modlin = true;                   /* Line has been changed */
  if (tabcnt == 0)
    goto p1038;                    /* If no tabs, force 1 space */
/*
 * Find 1st tab past where we are
 */
  for (i = 0; i < tabcnt; i++)
    if (k < tabs[i].value && tabs[i].tabtyp == chrpos)
      goto p1040;                  /* J found 1 */
/* Force 1 space if drop thro' */
p1038:j = 1;                       /* Insert 1 space */
  goto p1042;
p1040:
  j = tabs[i].value - k;           /* # spaces */
/*
 * See how many spaces we actually have room for
 */
p1042:
  i = curr->bchars;
  l = curr->bmxch - i;             /* # there is room for */
  if (l == 0)                      /* no room at all */
  {
  q1023:
    err = "Line full";
    goto p10231;
  }
  p = &curr->bdata[i];             /* 1 past last char to pick up */
  m = i - k;                       /* # chars to move up */
  if (l > j)
    l = j;                         /* # spaces to insert */
  i = i + l;                       /* New length */
  q = &curr->bdata[i];             /* 1 past last char to set down */
/* Do the right-hand overlapping move */
  for (; m > 0; m--)
    *--q = *--p;
/* Move in spaces */
  p = &curr->bdata[k];
  for (m = l; m > 0; m--)
    *p++ = SPACE;
/*
 * Adjust buffer and other variables ...
 */
  if (!INDENT)
    goto p2001;                    /* J no indent to look after */
  if (k == ndntch)
    ndntch = ndntch + l;           /* Up indent if were at strt */
p2001:
  curr->bcurs = k + l;             /* New cursor */
  curr->bchars = i;
  if (mxchrs < i)
    mxchrs = i;
  if (j != l)
    goto q1023;                    /* Bell if wasn't room for whole tab */
  goto p7700;                      /* Refresh or read */
/*
 * ^K - Kill
 */
p7713:
  curr->bchars = curr->bcurs;
  modlin = true;                   /* Line has been changed */
  goto p7700;                      /* Finished ^K */
/*
 * ^L - Left hand kill
 * If indenting and at the indent point kill the indent, otherwise
 * kill back to the indent point only.
 ^T obeys this code with some modification.
 */
p7714:
  k = curr->bcurs;
  if (k)
  {
    p = &curr->bdata[k];           /* 1st char to be moved down */
    if (verb == 'L')
      modlin = true;               /* ^T makes its own decision */
    j = 0;                         /* Eventual cursor pos if not */
/* indenting */
    if (INDENT)
    {
      if (verb == 'L' && k == ndntch)
        ndntch = 0;                /* Kill indent if at indenting point */
      k = k - ndntch;
      j = ndntch;
    }                              /* if (INDENT) */
    l = curr->bchars - k;          /* Set new line length */
    curr->bchars = l;
    curr->bcurs = j;
    q = &curr->bdata[j];           /* Dest'n 1st movedown char */
    if (l != j)
    {
/* Do overlapping left hand move */
      for (m = l - j; m > 0; m--)
        *q++ = *p++;
    }                              /* if (l != j) */
  }                                /* if (k) */
  if (verb == 'T')
    goto p1201;                    /* Return if end of ^T */
  goto p7700;                      /* Finish ^L */
/*
 * ^T - Split line. Return 1st part of line in previous buffer then
 * move down the rest, then return. Calling routine must check
 * verb of course.
 */
p7724:
/*
 * Move across l/h info
 */
  k = curr->bcurs;
/* J attempt to change file length in Fixed-Length mode */
  if (curr->bchars != olen && !cmmand && fmode & 0400)
    goto p2101;
/* If at eol, old line not modified here */
  modlin = modlin || (k < curr->bchars);
p1048:
  if (k)                           /* If some chars to move */
  {
    memcpy((char *)prev->bdata, (char *)curr->bdata, (size_t)k);
    lstvld = true;                 /* Last line now valid */
  }                                /* if (k) */
  prev->bchars = k;
  prev->bdata[k] = '\0';           /* Null terminate */
  prev->bcurs = 0;
  if (verb == 'J')
    goto p1201;                    /* Finish if was n/l */
  goto p7714;                      /* Move down r/h chars & finish */
/*
 * ^U - Cancel
 */
p7725:
  i = INDENT ? ndntch : 0;
  if (curr->bchars != i)
    modlin = true;                 /* Cancel empty line not a mod */
  curr->bchars = i;
  curr->bcurs = i;
  goto p7700;                      /* Finish ^U */
/*
 * ^X - Move cursor 1 forward. If at end, force a space
 */
p7730:
  k = curr->bcurs;
  if (k != curr->bchars)
  {
    curr->bcurs++;                 /* Move cursor fwd */
    goto p7700;                    /* Refresh etc */
  }                                /* if (k != curr->bchars) */
  ordch(SPACE, curr);
  modlin = true;                   /* Line has been changed */
  goto p7700;                      /* Finish ^X */
/*
 * ^Y - Cursor back 1
 * Not allowed into indent area however
 */
p7731:
  k = curr->bcurs;
  i = 0;
  if (INDENT)
    i = ndntch;
  if (k == i)
    goto p1023;                    /* J strt alrdy */
  curr->bcurs--;
  goto p7700;                      /* Finish ^Y */
/*
 * ESC - Abandon
 */
p7733:goto p1201;                  /* Simply exit */
/*
 * End of line
 */
p7712:
  verb = 'J';                      /* In case it was 'M' */
/* Disallow length change in FIXED LENGTH mode, when editing the file */
  if ((k = curr->bchars) != olen && !cmmand && fmode & 0400)
  {
  p2101:
    printf("\007\r\nLine length must not change in FIXED LENGTH mode");
    newlin();
    goto p7700;
  }
  goto p1048;                      /* Move chars to prev */
/*
 * ^Z - Go to end current line but ignore previous 1
 */
p7732:
  curr->bcurs = curr->bchars;
  goto p7700;                      /* Finish ^Z */
/*
 * ^Q - Rest of line upper -> lower
 */
p7721:
  l = 1204;
p1207:
  k = curr->bchars;
  j = curr->bcurs;
  p = &curr->bdata[j];
  k = k - j;                       /* # to do */
  if (k == 0)
    goto p7700;                    /* J out at end of line */
  modlin = true;                   /* Line has been changed */
  goto asg2l;                      /* Do the conversion */
p1204:
  for (; k > 0; k--)
  {
    if (*p >= 'A' && *p <= 'Z')
      *p += 040;
    p++;
  }
  goto p7700;                      /* Finished ^Q */
/*
 * ^S - Lower -> upper
 */
p7723:
  l = 1206;
  goto p1207;
p1206:                             /* Convert lower -> upper */
  for (; k > 0; k--)
  {
    if (*p >= 'a' && *p <= 'z')
      *p -= 040;
    p++;
  }
  goto p7700;                      /* Finished ^S */
/*
 * ^D - Right hand rubout
 */
p7704:
  if (curr->bcurs == curr->bchars)
  {
    err = "^D at EOL";
    goto p10231;
  }
/* BEL if at EOL */
  curr->bcurs++;                   /* Get past char to erase */
  goto p10065;                     /* Do an ordinary rubout */
/*
 * ^R - Reveal all of line
 *
 * Fudge in case recovering from accidental ESC - if no chars typed
 * reveal max possible BUF chars. BUT,,, don't reveal trailing null
 * characters, which have never been used...
 */
p7722:
  i = INDENT ? ndntch : 0;
  if (mxchrs == i)
  {
    for (k = curr->bmxch - 1; k >= i; k--)
      if (curr->bdata[k])
        break;
    mxchrs = k + 1;
  }
  if (curr->bchars != mxchrs)
    modlin = true;                 /* If characters rescued */
  curr->bchars = mxchrs;
  curr->bcurs = mxchrs;
  goto p7700;                      /* Finish ^R */
/*
 * ^O - cOmment modify
 */
p7717:
  k = curr->bcurs;                 /* Cursor pos'n */
  j = curr->bchars;                /* Saves array accesses */
  if (k == j)
    goto p7700;                    /* J if at E.O.L. already */
  p = &curr->bdata[k];             /* 1st char to check */
  i = j - k - 1;                   /* Chars to check for "/" */
  for (; i > 0; i--)
  {
    if (*p == '/' && *(p + 1) == '*')
      goto p1409;                  /* J found start of comment */
    k++;
    p++;
  }
p1407:
  curr->bcurs = j;                 /* Set cursor to E.O.L. */
  goto p7700;                      /* End ^O here */
p1409:k = k + 1;
p14091:
  k++;
  if (k >= j)
    goto p1407;
  if (curr->bdata[k] == SPACE)
    goto p14091;                   /* J not 1st sig char */
  curr->bcurs = k;                 /* Cursor to 1st comment char */
  goto p7700;                      /* Finish ^O */
/*
 * ^V - recoVer from a broadcast etc.
 */
p7726:
  newlin();
  goto p7700;                      /* Finish ^V */
/*
 * ^G - Goto next character
 */
p7707:glast = true;
  gpseu = false;                   /* Not in pseudo macro */
  goto p7700;                      /* Will be back shortly */
/*
 * P1518 - We are back continuing ^G but now we have the char to look
 * for.
 */
p1518:glast = false;
  if (gpseu)
    goto p1601;                    /* J on if actually for pseudomac */
p1804:
/* Search the rest of the line AFTER the current character */
  i = curr->bchars - curr->bcurs - 1; /* # chars to search */
  p = &curr->bdata[curr->bcurs];   /* Point to cursor char */
  for (; i > 0; i--)
    if (*++p == thisch)
      break;
  if (i >= 0)
    curr->bcurs = curr->bchars - i; /* Guard against -ve i */
  gotoch = thisch;                 /* Remember char for ^^ */
  goto p7700;                      /* Finish ^G */
/*
 * ^^ - repeat ^G
 */
p7736:
  if (gotoch == -1)                /* No prev ^G */
  {
    err = "^^ no previous ^G";
    goto p10231;
  }
  thisch = gotoch;
  goto p1804;                      /* Join ^G */
/*
 * ^N - expaNd macro
 */
p7716:nseen = true;
  goto p1026;
/*
 * P1502 - We have the macro char. May be a real macro or a pseudo
 * if we already in a macro. May also be RUBOUT if user has changed
 * his mind. May be ^P or ^W, in which case we want the character
 * following...
 */
p1502:
  if (thisch == '\3' || thisch == '\20')
    goto p7703;                    /* J is ^C or ^P */
  if (thisch == '\27')
    goto p7727;                    /* J is ^W */
p1905:nseen = false;
  contc = false;
  if (thisch == (unsigned char)'\377')
    goto p1026;                    /* High parity rubout */
/* Recognise ESC  if not in a macro */
  if (thisch == ESC && curmac < 0)
    goto p1505;
/* ESC legal from another macro */
  if (thisch <= LAST_PSEUDO && thisch >= FIRST_PSEUDO)
    goto p1503;                    /* J a pseudo probably */
/* Look for a keyboard interrupt if chaining. We can fully deal with it
 * unless in U-USE, but we must abandon macro chaining */
  if (curmac >= 0 && cntrlc)       /* Keyboard interrupt */
  {
    if (!USING_FILE)
      cntrlc = false;
    err = "Interrupt";
    goto p10231;
  }
/* Check for out of range or active pseudo macro */
  if (thisch > TOPMAC)
  {
    switch (thisch)
    {
      case 04000:                  /* Return mode */
        sprintf(tbuf, "%lo", zmode);
        macdef(64, (unsigned char *)tbuf, (int)strlen(tbuf), 1);
        break;

      case 04001:                  /* Return screen height - 1 */

/* Actually this is a bit fancier then just returning the screen height. We
 * return that, unless we are so close to the end of the file that G+<that
 * amount> would fail. In this case, we return only the maximum # lines that can
 * successfully be G+'d. Also, if nearer to the file start than half way down
 * the screen, we return extra so that ^N^V will not re-display lines */

        j = row5;                  /* Usual amount to move + 1 */
        if (ptrpos < row5 / 2)     /* In 1st 1/2 of 1st screen */
          j += row5 / 2 - ptrpos;  /* Move up to this - 1 */
        if (deferd && lintot - ptrpos < j) /* Some wanted lines not in yet */
          dfread(j, NULL);         /* Ensure lintot is adequate */
        i = lintot - ptrpos + 1;
        if (i > --j)
          i = j;

        // Look for half-screen requested
        if ((fmode & 00000100000) && i > row5 / 2)
        {
          if (i > row5)
            i -= row5 / 2 + 1;
          else
            i = row5 / 2 - 1;
        }

        sprintf(tbuf, "%d", i);
        macdef(64, (unsigned char *)tbuf, (int)strlen(tbuf), 1);
        break;

      case 04002:                  /* Return curent edit file */
/*
 * If there is no current file, ensure macro will not return. This since
 * the null argument might not be noticed in a macro, and the commnad's
 * next argument (if any) taken as a filename (numbers being legal
 * filenames)
 */
        i = strlen(pcnta);
        macdef(64, (unsigned char *)pcnta, i, i != 0);
        break;

      case 04003:                  /* HELP dir (for macros usually) */
        macdef(64, (unsigned char *)macro_dir, strlen(macro_dir), 1);
        break;

      case 04004:                  /* Like 4001 but for going backwards */
        j = row5;                  /* Usual amount to move + 1 */
        if (ptrpos < j)            /* On 1st screen */
          j = ptrpos;
        else if (lintot - ptrpos < j / 2 - !(row5 & 1))
        {
          j += j / 2 - lintot + ptrpos;

          // Need to go back 1 less because "E-O-F" is displaying
          j--;

          // Need to go back 1 less again if even # lines on screen
          j -= !(row5 & 1);
        }
        j--;
        // Sanity check - don't try to move before start of file
        if (j >= ptrpos)
          j = ptrpos - 1;

        // Look for half-screen requested
        if ((fmode & 00000100000) && j > row5 / 2)
        {

          // Reduce j by an extra 1 because empirically that moves the
          // current line to the top of the screen.
          if (j > row5)
            j -= row5 / 2 + 1;
          else
            j = row5 / 2 - 1;
        }

        snprintf(tbuf, sizeof tbuf, "%d", j);
        macdef(64, (unsigned char *)tbuf, (int)strlen(tbuf), 1);
        break;

      default:
        goto p2102;
    }                              /* switch (thisch) */
    curmac = 64;
    mcposn = 0;
    goto p7700;
  }                                /* if (thisch > TOPMAC) */
/* Signal error if null macro */
p2102:
  if (thisch > TOPMAC || !scmacs[thisch])
  {
    if (curmac >= 0)
    {
      printf("\r\nCalling undefined macro ^<%o>. ", thisch);
      notmac(1);
    }
    goto p10232;
  }
  mcposn = 0;                      /* Got the macro */
  goto p1613;
p1505:
  verb = '[';                      /* For benefit of mainline */
  goto p7733;                      /* Join normal ESC */
p1613:
  curmac = thisch;
  goto p1026;
p1503:
  if (thisch == 0177)
    goto p1026;                    /* J rubout */
/*
 * Look for a pseudo macro. Some are only legal if in a macro
 */
  verb = thisch;
  if (verb >= 'a' && verb <= 'z')
    verb &= 0337;                  /* Upper case letter */
  if (curmac >= 0)
    goto p1907;                    /* J in a macro */
/* We are not in a macro. Check whether this pseudo is allowed from the
 * keyboard... */
  for (c = "EORFNMY"; *c; c++)
    if (*c == verb)
      goto p1907;
  goto p10232;                     /* Definitely not in macro */
p1907:
  switch (thisch & 037)
  {                                /* Try for a pseudo u/c or l/c */
    case 1:
      goto p7601;
    case 2:
      goto p7602;
    case 3:
      goto p7603;
    case 4:
      goto p7604;
    case 5:
      goto p7605;
    case 6:
      goto p7606;
    case 7:
      goto p7607;
    case 9:
      goto p7611;
    case 10:
      goto p7612;
    case 12:
      goto p7614;
    case 13:
      goto p7615;
    case 14:
      goto p7616;
    case 15:
      goto p7617;
    case 16:
      goto p7620;
    case 18:
      goto p7622;
    case 19:
      goto p7623;
    case 20:
      goto p7624;
    case 21:
      goto p7625;
    case 24:
      goto p7630;
    case 25:
      goto p7631;
    case 27:
      goto p7633;
    case 29:
      goto p7635;
  }
/* ^NH is not a pseudo */
/* ^NK is not a pseudo */
/* ^NQ is not a pseudo */
/* ^NV is not a pseudo */
/* ^NW is not a pseudo */
/* ^NZ is not a pseudo */
/* ^N\ is not a pseudo */
  if (curmac >= 0)
  {
    printf("\r\nCalling undefined pseudo-macro \"%c\". ", thisch);
    notmac(1);
  }
  goto p10232;
/*
 * ^NI - Increment link (to make macro a conditional)
 */
p7611:
/* Error if < 2 chars left in macro, because it can't then do a ^NU */
  if (mcposn > scmacs[curmac]->mcsize - 2)
  {
  p76111:
    err = "^NI, ^NB &c. too near macro end";
    goto p10231;
  }
  if (mcnxfr == MCDTUM)
    goto p1026;                    /* No-op if stack empty */
  rtn = 7604;                      /* Force the UP code to do a DOWN */
  l = curmac;                      /* Save current macro */
  m = mcposn;                      /* Save current macro position */
  goto p1706;                      /* Do a dummy UP */
/*
 * ^NO,^NR,^NF,^NN - Remember or move to cursor or file position
 * ^NY - Get difference between 2 tabs to a third tab
 */
p7617:gwrit = true;                /* (^NO) We are writing to a tab */
p1709:gcurs = true;                /* Distinguish from file posn */
p1710:gtest = false;               /* Not B or P */
  if (curmac < 0)
    goto p1716;                    /* J keybd pseudo */
/* Error if was last character in the macro */
  if (mcposn >= scmacs[curmac]->mcsize)
  {
    err = "^NO &c. too near macro end";
    goto p10231;
  }
p1716:gposn = true;                /* Distinguish from ^G */
  goto p1707;                      /* Get tab # */
p7622:gwrit = false;               /* (^NR) move cursor, don't set tab */
  goto p1709;                      /* Join ^NO */
p7606:gdiff = false;               /* (^NF) set tab to file position */
p1808:gwrit = true;                /* (^NY joins here) */
p1711:gcurs = false;               /* We are file, not cursor */
  goto p1710;                      /* Join ^NO */
p7616:gwrit = false;               /* (^NN) Position file, don't write tab */
  goto p1711;                      /* Join ^NF */
p7631:gdiff = true;                /* (^NY) we are YDIFF */
  goto p1808;                      /* Join ^NF */
/*
 * P1708 - Come back here with THISCH set equal to the tab code.
 *        1->tab 1 ... D->tab 20
 */
p1708:
  i = thisch - '1';                /* Get TABS array subscript */
  if (i < 0 || i > 80)
    goto w1023;
  if (gtest)
    goto p1718;                    /* J test pos'n */
  if (gwrit)
    goto p1712;                    /* J remember current pos'n */
  i4 = tabs[i].value;
  if (i4 < 0)
    goto w1023;                    /* J certainly too low */
  if (!gcurs)
    goto p1713;                    /* J file positioner */
  if (i4 > BUFMAX)
    goto w1023;                    /* J too big for a cursor value */
  i = i4;
  if (i > curr->bchars)
    goto w1023;                    /* J trying to pos'n off end of line */
  curr->bcurs = i;                 /* Set cursor position */
  goto p7700;                      /* End ^NR - may want to REFRSH */
/*
 * ^NB - Obey if Before spec'd tab
 */
p7602:
  gpast = false;                   /* We are B, not P. */
p1717:
  gcurs = true;                    /* We are testing cursor position */
p1813:
  gtest = true;                    /* We are a position tester */
/* Err if not tabid + 2 chars */
  if (mcposn > scmacs[curmac]->mcsize - 3)
    goto p76111;
  goto p1716;                      /* Get tab # */
p1718:
  if (tabs[i].tabtyp == undefined)
  {
    err = "^NB &c. testing undefined tab";
    goto p10231;
  }
  if (gpast)
    goto p1719;                    /* J on if not a B */
  if (!gcurs)
    goto p1814;                    /* J actually ^N[ */
  if (tabs[i].tabtyp == linenum)
  {
  p17181:
    err = "^NB/^NP testing filepos tab";
    goto p10231;
  }
  if (curr->bcurs < tabs[i].value)
    goto p1026;
  goto p1511;
p1814:
  if (tabs[i].tabtyp == chrpos)
  {
  p1841:
    err = "^N[/^N] testing chrpos tab";
    goto p10231;
  }
  if (ptrpos < tabs[i].value)
    goto p1026;
  goto p1511;
/*
 * ^NP - obey if Past spec'd tab
 */
p7620:gpast = true;                /* We are in fact P */
  goto p1717;
p1719:if (!gcurs)
    goto p1815;                    /* J actually ^N] */
  if (tabs[i].tabtyp == linenum)
    goto p17181;
  if (curr->bcurs > tabs[i].value)
    goto p1026;
  goto p1511;
p1815:
  if (tabs[i].tabtyp == chrpos)
    goto p1841;
  if (ptrpos > tabs[i].value)
    goto p1026;
  goto p1511;
/*
 * ^N[ - Obey if file pos'n before specified tab
 */
p7633:gpast = false;               /* We are [, not ] */
p1812:gcurs = false;               /* We are testing file pos'n */
  goto p1813;                      /* Join cursor testers */
/*
 * ^N] - Obey if file pos'n past specified tab
 */
p7635:gpast = true;                /* We are ], not [ */
  goto p1812;                      /* Join ^N[ */
/*
 * P1713 - ^NN continuing
 */
p1713:
  if (!cmmand && !mctrst) /* Not trusted to change file pos'n while modifying */
    goto p1023;
/* Eof */
  if (i4 > lintot + 1 && !(deferd &&
    (dfread(i4 - lintot, NULL), i4 <= lintot + 1)))
    goto p1023;
  setptr(i4);                      /* Position the file */
  goto p1026;                      /* Finish ^NN */
/*
 * P1712 - Remember file or cursor position
 */
p1712:if (gcurs)
    goto p1714;                    /* J is cursor pos'n */
  if (gdiff)
    goto p1809;                    /* ^NY splits off here */
  tabs[i].value = ptrpos;
  tabs[i].tabtyp = linenum;
  goto p1026;
p1714:
  tabs[i].value = curr->bcurs;     /* ^NO */
  tabs[i].tabtyp = chrpos;
  goto p1026;
/*
 * P1809 - ^NY continuing
 */
p1809:
  if (i > 77)
    goto w1023;                    /* J above max for a result */
  if (tabs[i].tabtyp == undefined || tabs[i].tabtyp != tabs[i + 1].tabtyp ||
    tabs[i].value >= tabs[i + 1].value)
  {
    err = "^NY tab type / value error";
    goto p10231;
  }
  tabs[i + 2].value = tabs[i + 1].value - tabs[i].value; /* Set result */
  tabs[i + 2].tabtyp = tabs[i].tabtyp;
/*
 * Slightly awful hack - user percieves cursor tabs as 1-based but they
 * are stored zero-based. To maintain the deception, we must subtract 1
 * from the result if dealing with cursor tabs...
 */
  if (tabs[i].tabtyp == chrpos)
    tabs[i + 2].value--;
  goto p1026;
/*
 * ^NA - Obey if at EOL
 */
p7601:
  if (mcposn > scmacs[curmac]->mcsize - 2)
    goto p76111;
/* J could skip off e.o. mac */
  if (curr->bcurs == curr->bchars)
    goto p1026;
/* J at EOL */
p1511:mcposn = mcposn + 2;         /* Skip 2 */
  if (mcposn > scmacs[curmac]->mcsize)
    goto p76111;
/* J skipped off e.o. mac */
  goto p1026;
/*
 * ^NC - Obey if in command processor
 */
p7603:
  if (mcposn > scmacs[curmac]->mcsize - 2)
    goto p1023;
/* J could skip off e.o. mac */
  if (!cmmand)
    goto p1511;
  goto p1026;
/*
 * ^NX - eXit from macro
 */
p7630:
p1515:
  notmac(0);
  goto p7700;
/*
 * ^NT - Trust user if he wants to change file pointer during modify
 */
p7624:mctrst = true;
  goto p1026;
/*
 * ^NS - Unconditional skip (reverse obey sense)
 */
p7623:goto p1511;
/*
 * ^NE - Reset insErt mode (^E) to OFF
 */
p7605:insert = false;
  if (curmac < 0)
    refrsh(curr);                  /* Close gap */
  goto p1026;
/*
 * ^NU - Up from a macro s/r
 */
p7625:if (mcnxfr == MCDTUM)
    goto p7630;
/* Treat as exit if stack empty */
/*
 * Look for stack corruption
 */
  rtn = 1026;
/*
 * P1706 - I joins here
 */
p1706:
  mcnxfr--;                        /* Previous stack entry */
  i = mcstck[mcnxfr].mcprev;       /* Macro # */
  if (i < 0)
  {
  p17061:
    printf("\r\nReturn macro ^<%o>out of range or empty. ", i);
    notmac(1);
    goto p10232;
  }
  if (i > TOPMAC)
    goto p17061;                   /* Illegal macro # */
  if (!scmacs[i])
    goto p17061;                   /* J undefined macro */
  j = mcstck[mcnxfr].mcposn;       /* Macro position */
  if (j > scmacs[i]->mcsize)
    goto p17061;                   /* J pos'n now off end of macro */
  if (j < 0)
    goto p17061;                   /* J -ve pos'n */
  curmac = i;
  mcposn = j;                      /* Accept the popped values */
  goto asg2rtn;
/*
 * ^ND - go Down a level ( a macro as a subroutine)
 */
p7604:
/* Error if < 2 chars left in macro */
  if (mcposn > scmacs[curmac]->mcsize - 2)
    goto p1023;
  if (MCLMIT == mcnxfr)
  {
    err = "Macro stack depth limit exceeded";
    goto p10231;
  }
  mcstck[mcnxfr].mcprev = curmac;
/* Return addr after following macro */
  mcstck[mcnxfr].mcposn = mcposn + 2;
  mcnxfr++;                        /* Up stack pointer */
  if (verb != 'I')
    goto p1026;                    /* Return now unless incrementing */
  curmac = l;
  mcposn = m;
  goto p1026;
/*
 * ^NJ long (signed) jump
 */
p7612:
/* Must be 1 more char in macro */
  if (mcposn >= scmacs[curmac]->mcsize)
    goto p76111;
  fornj = true;                    /* Indicate ^N^J */
  goto p1902;                      /* Get jump length */
/*
 * ^NL - Long skip
 */
p7614:mcposn = mcposn + 2;         /* Skip 2 ... */
  goto p1511;                      /* ... skip 2 more */
/*
 * ^NM - Make Macro from current line
 */
p7615:
  if (curmac < 0)
    goto p1908;                    /* J keybd pseudo */
/* Must be 1 more char in macro */
  if (scmacs[curmac]->mcsize == mcposn)
    goto p1023;
p1908:gmacr = true;                /* We are defining a macro */
  goto p1902;                      /* Get macro ID */
/*
 * ^NG - obey if Got next ch
 */
p7607:
/* J not another char + something to obey */
  if (scmacs[curmac]->mcsize - mcposn < 3)
    goto p76111;
  gmacr = false;                   /* We are not defining a macro */
p1902:gposn = false;               /* We are not a file or curs posn */
p1707:glast = true;                /* So we will come back */
  gpseu = true;                    /* So we will come back */
  goto p1026;
/*
 * P1601 - Come here with ^NG char
 */
p1601:
  if (fornj)
  {
    fornj = false;
    mcposn += thisch;              /* Do the jump */
    if (mcposn >= 0 && mcposn < scmacs[curmac]->mcsize)
      goto p1026;
    err = "^NJ off macro end or start";
    goto p10231;
  }
  if (gposn)
    goto p1708;                    /* J in fact for a positioner */
  if (gmacr)
    goto p1903;                    /* J in fact defining a macro */
  if (curr->bcurs == curr->bchars)
    goto p1511;
/* Skip if at eol */
  if (curr->bdata[curr->bcurs] != thisch)
    goto p1511;
/* Skip if mismatch */
  goto p1026;
/*
 * P1903 - Come here with macro to define. All characters are legal
 *         except the pseudomacros...
 */
p1903:
  if ((thisch < 0200 && thisch > 077) || thisch > TOPMAC)
    goto w1023;
  if (curr->bchars == 0)
    goto p1023;                    /* Trying to define null macro */
/* Define the macro. Report if some problem... */
  if (macdef((int)thisch, curr->bdata, (int)curr->bchars, 1))
    goto p1026;
  if (curmac >= 0)
    notmac(1);
  goto p10232;                     /* Report failure */
/*
 * ^W - Next char not special but Without parity
 */
p7727:cntrlw = true;
  contc = true;
  goto p7700;
/*
 * P1201 - Exit sequence. Reinstate XON if req'd...
 */
p1201:if (USING_FILE)
    goto p1522;
  if (nodup)
    goto p1522;                    /* J we didn't do one on the way in */
  duplx5(true);                    /* enable XOFF */
p1522:
  return;
asg2l:switch (l)
  {
    case 1206:
      goto p1206;
    case 1204:
      goto p1204;
    default:
      printf("Assigned Goto failure, l = %d\r\n", l);
      verb = 'J';                  /* Let user... */
      goto p1522;                  /* ... salvage his edit. */
  }
asg2rtn:switch (rtn)
  {
    case 1026:
      goto p1026;
    case 7604:
      goto p7604;
    default:
      printf("Assigned Goto failure, rtn = %d\r\n", rtn);
      verb = 'J';                  /* Let user... */
      goto p1522;                  /* ... salvage his edit. */
  }
}
