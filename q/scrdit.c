/* S C R D I T */
/*
 * Copyright (C) 1981 D. C. Roe
 * Copyright (C) 2012-2014 Duncan Roe
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
#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/times.h>
#include "alledit.h"
#include "macros.h"
#include "cmndcmmn.h"
#include "scrnedit.h"
#include "fmode.h"
#include "tabs.h"
#include "c1in.h"
#include "alu.h"

/* Macros */

#define GETNEXTCHR goto getnextchr
#define BOL_OR_EOL goto bol_or_eol
#define RAWNEXTCHR goto rawnextchr
#define ERR_IF_MAC goto err_if_mac
#define SOUNDALARM goto soundalarm
#define TABOORANGE goto taboorange
#define SKIP2MACCH goto skip2macch
#define NORMALCHAR goto normalchar
#define RANOFF_END goto ranoff_end

/* Externals that are not in any header */

unsigned char fxtabl[128];
long timlst;

/* Static Variables */

static scrbuf5 *last_Curr = NULL;

/* *************************** print_failed_opcode ************************** */

static void
print_failed_opcode(short thisch)
{
  char *opcd = opcode_defs[alu_table_index[thisch - FIRST_ALU_OP]].name;

  fprintf(stderr, "\r\nFailing opcode: ");
  while (*opcd)
    putc(toupper(*(unsigned char *)opcd++), stdout);
}                                  /* print_failed_opcode) */

/* ************************** get_effective_address ************************* */

static bool
get_effective_address(int addr)
{
  effaddr = addr;
  if (index_next)
  {
    index_next = false;
    effaddr += xreg;
    if ((effaddr & 0777) != effaddr)
    {
      fprintf(stderr, "\r\nIndexing fault at 0%o. ", effaddr);
      dump_registers(true);
      notmac(true);
      return false;
    }                              /* if ((effaddr & 0777) != effaddr) */
  }                                /* if (index_next) */
  return true;
}                                  /* get_effective_address() */

/* ****************************** push_register ***************************** */

static bool
push_register(long val)
{
  if (++rsidx >= stack_size)
  {
    rsidx--;
    fprintf(stderr, "%s",
      "\r\nRegister stack overflow.\r\nFailing opcode: PSH\r\n");
    dump_registers(true);
    notmac(true);
    return false;
  }                                /* if (++rsidx >= stack_size) */
  rs[rsidx] = val;
  return true;
}                                  /* push_register() */

/* **************************** push_fp_register **************************** */

static bool
push_fp_register(double val)
{
  if (++fsidx >= stack_size)
  {
    fsidx--;
    fprintf(stderr, "%s",
      "\r\nFP register stack overflow.\r\nFailing opcode: PSHF\r\n");
    dump_registers(true);
    notmac(true);
    return false;
  }                                /* if (++fsidx >= stack_size) */
  fs[fsidx] = val;
  return true;
}                                  /* push_fp_register() */

/* ****************************** pop_register ****************************** */

static bool
pop_register(long *val)
{
  if (rsidx < 0)
  {
    fprintf(stderr, "%s",
      "\r\nRegister stack underflow.\r\nFailing opcode: POP\r\n");
    dump_registers(true);
    notmac(true);
    return false;
  }                                /* if (rsidx < 0) */
  *val = rs[rsidx--];
  return true;
}                                  /* pop_register() */

/* ***************************** pop_fp_register **************************** */

static bool
pop_fp_register(double *val)
{
  if (fsidx < 0)
  {
    fprintf(stderr, "%s",
      "\r\nFP register stack underflow.\r\nFailing opcode: POPF\r\n");
    dump_registers(true);
    notmac(true);
    return false;
  }                                /* if (fsidx < 0) */
  *val = fs[fsidx--];
  return true;
}                                  /* pop_fp_register() */

/* ********************************* get_inp ******************************** */

bool
get_inp(double *fval, long *val, long *len, char **err)
{
  char *endptr;                    /* 1st char after number */
  unsigned char lastch;            /* Dump for char we nullify */
  int i;

/* Skip whitespace (including tabs) */
  for (i = last_Curr->bcurs; i < last_Curr->bchars; i++)
    if (!isspace(last_Curr->bdata[i]))
      break;
  if (i == last_Curr->bchars)
  {
    *err = "No number before eol";
    return false;
  }                                /* if (i == last_Curr->bchars) */
  if (!(isdigit(last_Curr->bdata[i]) || last_Curr->bdata[i] == '+' ||
    last_Curr->bdata[i] == '-' || (fval && last_Curr->bdata[i] == '.')))
  {
    *err = "Next item on line is not a number";
    return false;
  }                                /* if (!(isdigit(last_Curr->bdata[i] ... */
  last_Curr->bcurs = i;

/* Force null termination so strtol() will stop at end */
  lastch = last_Curr->bdata[last_Curr->bchars];
  last_Curr->bdata[last_Curr->bchars] = 0;

/* Get the value (cast is to conform with strtol prototype) */
  errno = 0;
  if (val)
    *val = strtol((char *)last_Curr->bdata + i, &endptr, 0);
  else
    *fval = strtod((char *)last_Curr->bdata + i, &endptr);

/* Reinstate zeroed char */
  last_Curr->bdata[last_Curr->bchars] = lastch;

/* Check for overflow. Only check errno (previously zeroed) */
/* since LONG_MAX or LONG_MIN might be returned legitimately */
  if (errno)
  {
    *err = strerror(errno);
    return false;
  }                                /* if (errno) */

/* All OK. Return length and finish */
 *len = endptr - (char *)last_Curr->bdata - i;
  return true;
  return true;
}                                  /* get_inp() */

/* ********************************* scrdit ********************************* */

void
scrdit(scrbuf5 *Curr, scrbuf5 *Prev, char *prmpt, int pchrs, bool in_cmd)
{
  unsigned char *p, *q;            /* Scratch */
  char *c;                         /* Scratch */
  char *err = NULL;                /* Point to error text */
  char tbuf[256];                  /* Scratch */
  int i, j, k, l = 0, m = 0;       /* Scratch variables */
  int gotoch;                      /* Char of last ^G */
  long i4;                         /* Scratch */
  long olen;                       /* Original line length */
  struct tms tloc;                 /* Junk from TIMES */
  long timnow;                     /* Time from TIMES */
  short thisch;                    /* Character being processed */
/*
 * PARAMETERS:-
 * ============
 *
 * Curr  - Current buffer to modify \ these are scrnedit buffers
 * Prev  - Previous line modified   / as defined above
 * prmpt - Prompt characters (if any) (string)
 * pchrs - # of prompt chars (zero implies none and PRMPT not a valid
 *                                  addr)
 * in_cmd - => in command mode
 */
  bool contp, nseen, glast, gpseu = false, gposn = false;
  bool gwrit = false, gcurs = false, gtest = false, gpast = false;
  bool gdiff = false, gmacr = false, fornj;
/*
 * LOCAL LOGICAL VARIABLES:
 * ========================
 *
 * contp  - true if last char ^P (nxt ch not special)
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

/* Curr->bchars counts the trailing null but Curr->bmxch does not */
  if ((olen = Curr->bchars) > Curr->bmxch + 1)
  {
    err = "Bad char cnt Curr line";
  p1001:
    fprintf(stderr, "%s\r\n", err);
    err = NULL;
    verb = 'J';                    /* Let user... */
    return;                        /* ... salvage his edit. */
  }

/* J bad char cnt this line */
  if (Curr->bcurs < 0)             /* Bad cursor this line */
  {
    err = "Curr curs<1";
    goto p1001;
  }
  if (pchrs > PRMAX)               /* Prompt too big */
  {
    err = "Prompt>15 chars";
    goto p1001;
  }

/* J cursor off end this line */
  if (Curr->bcurs > Curr->bchars)
  {
    err = "Curr curs off end of info";
    goto p1001;
  }
/*
 * Switch off XOFF if input from tty
 */
  if (!USING_FILE)
  {
    if (curmac < 0)
    {
      duplx5(false);               /* No XOFF recognition */
      nodup = false;
    }                              /* if (curmac < 0) */
    else
      nodup = true;                /* So we don't do one on the way out */
  }                                /* if (!USING_FILE) */
/*
 * Initialise common variables &c
 */
  endlin = false;
  modlin = false;
  insert = false;
  pchars = pchrs;
  mxchrs = Curr->bchars;
  nseen = false;
  glast = false;
  fornj = false;
  last_Curr = Curr;
  if (pchars == 0)
    goto p1005;                    /* J no prompt to move in */
/*
 * Get a null-terminated prompt buffer first
 */
  strcpy((char *)prompt, (char *)prmpt);
/* Move prompt to common area */
p1005:contp = false;
  cntrlw = false;
  gotoch = -1;                     /* No ^G yet */
/*
 * Normally we do no REFRSH if in an scmac, but if BRIEF is on
 * we here refresh the prompt only (if editing a line)
 *  Speedup: so we don't bank up on TTY o/p (& CPU!), only display
 * new line # if .GT. 1/5 sec since last time ...
 */
  if (curmac >= 0 && !in_cmd && BRIEF && !NONE && pchars)
  {
    timnow = times(&tloc);
/* Assume a clock tick rate of 100 - not critical. We don't cater for
 * wraparound, currently... */
    if (timnow - timlst >= 20)
    {
      timlst = timnow;             /* Displaying */
      sdsply();                    /* Display the line number */
    }                              /* if (timnow - timlst >= 20) */
  }                /* if (curmac >= 0 && !in_cmd && BRIEF && !NONE && pchars) */
/*
 * If this is a new line and INDENT is on, pad out with appropriate
 * number of spaces. If not a new line and INDENT on, get new
 * indent position.
 */
  if (INDENT)
  {
    if (!Curr->bchars)
    {
      if (!lstvld)
        sindnt();                  /* Get the indent */
      if (ndntch > 0)
      {
        for (i = ndntch; i > 0; i--)
          ordch(SPACE, Curr);      /* Pad out with spaces */
        modlin = true;             /* Line has been changed */
      }                            /* if (ndntch > 0) */
    }                              /* if !(Curr->bchars) */
    else
    {
      ndntch = 0;
      for (j = Curr->bchars; j > 0; j--)
      {
        if (Curr->bdata[ndntch] != SPACE)
          break;
        ndntch = ndntch + 1;
      }
      if (Curr->bcurs < ndntch)
        Curr->bcurs = ndntch;
    }                              /* if !(Curr->bchars) else */
  }                                /* if (INDENT) */
  else
    ndntch = 0;                    /* Not indenting, so no indent chars */

  mctrst = false;
rawnextchr:
  if (curmac < 0)
    refrsh(Curr);                  /* Display prompt etc */
  if (curmac >= 0)
  {
/* Clear expanding if that was last char from macro */
    if (mcposn >= scmacs[curmac]->mcsize)
    {
      notmac(false);
      GETNEXTCHR;
    }                              /* if (mcposn >= scmacs[curmac]->mcsize) */
    thisch = scmacs[curmac]->data[mcposn];
    mcposn = mcposn + 1;
  }                                /* if (curmac >= 0) */
  else
  {
    bool eof_encountered;

    thisch = c1in5(&eof_encountered); /* Read 1 char */
    if (eof_encountered)
    {
      if (in_cmd && Curr->bchars == 0 && USING_FILE)
      {
        Curr->bdata[0] = 'z';
        Curr->bchars = 1;
      }                            /* if (in_cmd && ...) */
      else
        thisch = c1in5(NULL);
    }                              /* if (eof_encountered) */
/*
 * FX command implementation - replace char read by one from the
 * table unless we think we are cominputting or by some strange
 * chance the char is parity-high.
 */
    if (!USING_FILE && !(thisch & 0200))
      thisch = fxtabl[thisch];
  }                                /* if (curmac >= 0) else */
  if (contp)
    goto p1802;                    /* J ^P last char */
  if (nseen)
    goto p1502;                    /* J expecting macro name */
  if (glast)
    goto p1518;                    /* J in middle of ^G */
  if (thisch == 0177)
    goto p10065;                   /* J it was a rubout */
  if (thisch & 0200)
    NORMALCHAR;                    /* J parity-high */
  if (thisch >= SPACE)
    NORMALCHAR;                    /* J not a control */
  verb = thisch + 0100;            /* 'J' for ^J, &c. */
  switch (thisch)
  {
    case 0:
      GETNEXTCHR;
    case 1:
      goto p7701;
    case 2:
      goto p7702;
    case 4:
      goto p7704;
    case 5:
      goto p7705;
    case 6:
      goto p7706;
    case 7:
      goto p7707;

    case 8:
/* ^H - Home */
      Curr->bcurs = INDENT ? ndntch : 0; /* Reset cursor */
      GETNEXTCHR;

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

    case 3:                        /* drop thru */
    case 16:
/* ^P (or ^C) - next char not special */
      contp = true;
      GETNEXTCHR;

    case 17:
      goto p7721;
    case 18:
      goto p7722;
    case 19:
      goto p7723;
    case 20:
      goto p7724;

    case 21:
/* ^U - Cancel */
      i = INDENT ? ndntch : 0;
      if (Curr->bchars != i)
        modlin = true;             /* Cancel empty line not a mod */
      Curr->bchars = Curr->bcurs = i;
      GETNEXTCHR;                  /* Finish ^U */

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
  NORMALCHAR;                      /* Not special control */
p1802:
  if (cntrlw)
    thisch |= 0200;                /* Set parity following ^W */
  cntrlw = false;
  if (nseen)
    goto p1905;                    /* Treat as macro char if ^N Prev */
  NORMALCHAR;                      /* Treat as ordinary character */
/*
 * DEL - Deletes char before cursor
 */
p10065:
  k = Curr->bcurs - 1;
  if (k < 0)
    BOL_OR_EOL;                    /* J at start lin (error) */
  modlin = true;                   /* Line has been changed */
/* If at indent point,decrement INDENT */
  if (INDENT && k + 1 == ndntch)
    ndntch--;
  j = Curr->bchars - 1;            /* How many chars there will be */
  if (j != k)
  {
    q = &Curr->bdata[k];           /* Where chars to move to */
    p = q + 1;                     /* Where chars to come from */
    for (i = j - k; i > 0; i--)
      *q++ = *p++;                 /* Move chars back */
  }                                /* if (j != k) */
  Curr->bcurs = k;                 /* Step cursor back */
  Curr->bchars = j;                /* Reduce # of chars */
getnextchr:
/* Get another character if one available */
  if (curmac < 0 && !USING_FILE && !kbd5())
    refrsh(Curr);
  RAWNEXTCHR;
bol_or_eol:
  err = "Ran off beginning or end of line";
err_if_mac:
  if (curmac >= 0)
  {
    if (*err)
      fprintf(stderr, "\r\n%s. ", err);
    err = NULL;
    notmac(true);
  }
soundalarm:
  putchar('\a');                   /* O/p BEL since error */
  mctrst = false;                  /* User no longer trusted */
  GETNEXTCHR;                      /* End rubout */
taboorange:
  if (curmac >= 0)
  {
    printf("\r\nTab ID %c or value in that tab out of range. ", thisch);
    notmac(true);
  }
  SOUNDALARM;
/*
 * Non-special character
 */
normalchar:
  ordch(thisch, Curr);             /* Insert or replace as appropriate */
  modlin = true;                   /* Line has been changed */
  contp = false;
  GETNEXTCHR;                      /* Finish */
/*
 * ^A - Again/Append
 * If more info in previous line than current, append previous excess
 * to current, then in either case move cursor to follow last char
 */
p7701:i = ndntch;                  /* In case we  SINDNT */
  if (!lstvld)
    sindnt();                      /* Get previous line valid */
  ndntch = i;
  if (Curr->bchars >= Prev->bchars)
    goto p1029;
/* J append only */
/*
 * We have to move chars from the previous line
 */
  modlin = true;                   /* Line has been changed */
  memcpy((char *)&Curr->bdata[Curr->bchars],
    (char *)&Prev->bdata[Curr->bchars], (size_t)(Prev->bchars - Curr->bchars));
  Curr->bchars = Prev->bchars;     /* Extend length of line */
  if (mxchrs < Curr->bchars)
    mxchrs = Curr->bchars;
/* For ^R */
p1029:
  Curr->bcurs = Curr->bchars;      /* Put cursor at end */
  GETNEXTCHR;                      /* Finish ^A */
/*
 * ^B - Go back to the start of this word or back to the start of the
 * previous word if at the start of this one. Words are delimited by
 * spaces only. Error if no backwards movement possible.
 * N.B. unlike D.G., we stop on the first character of the word.
 */
p7702:i = 0;                       /* S.O.L. if no indenting */
  if (INDENT)
    i = ndntch;
  k = Curr->bcurs - i;             /* K - running available movement */
/* Note k can be -ve, if he used ^NR to move before the indent point */
  if (k <= 0)
    BOL_OR_EOL;                    /* J start of line (error) */
/* Check whether at word start. If so, back up to a non-space (or line
 * start)... */
  p = &Curr->bdata[Curr->bcurs];
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
  Curr->bcurs = i + k;             /* Start of line + room */
  GETNEXTCHR;                      /* Finish ^B */
/*
 * ^E - Enter/Leave insert mode
 */
p7705:insert = !insert;
  GETNEXTCHR;                      /* End insert */
/*
 * ^F - Forward to start next word. If at e.o.l., no-op;
 * if on last word, stop at e.o.l.
 */
p7706:
  k = Curr->bchars - Curr->bcurs;  /* K - how far we can move forward */
/* If on non-space, move forward to space, unless at EOL now... */
  p = &Curr->bdata[Curr->bcurs];
/* Ensure we're pointing at a space (or we were). Always reduces K,
 * unless at EOL to start with... */
  if (k < 0)
  {
    fprintf(stderr, "Internal error: k=%d\r\n", k);
    notmac(true);
    SOUNDALARM;
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
  Curr->bcurs = Curr->bchars - k;
  GETNEXTCHR;                      /* Refresh or read */
/*
 * ^I - Insert a tab. We insert enough spaces to get to next tab
 * posn. if there is one, otherwise insert 1 space. once inserted,
 * there is no remembrance that this was a tab - spaces may be
 * individually deleted etc.
 */
p7711:
  modlin = true;                   /* Line has been changed */
  if (tabcnt == 0)
    goto p1038;                    /* If no tabs, force 1 space */
/*
 * Find 1st tab past where we are
 */
  for (i = 0; i < tabcnt; i++)
    if (Curr->bcurs < tabs[i].value && tabs[i].tabtyp == CHRPOS)
      goto p1040;                  /* J found 1 */
/* Force 1 space if drop thro' */
p1038:j = 1;                       /* Insert 1 space */
  goto p1042;
p1040:
  j = tabs[i].value - Curr->bcurs; /* # spaces */
/*
 * See how many spaces we actually have room for
 */
p1042:
  i = Curr->bchars;
  l = Curr->bmxch - i;             /* # there is room for */
  if (l == 0)                      /* no room at all */
  {
  q1023:
    err = "Line full";
    ERR_IF_MAC;
  }
  p = &Curr->bdata[i];             /* 1 past last char to pick up */
  m = i - Curr->bcurs;             /* # chars to move up */
  if (l > j)
    l = j;                         /* # spaces to insert */
  i = i + l;                       /* New length */
  q = &Curr->bdata[i];             /* 1 past last char to set down */
/* Do the right-hand overlapping move */
  for (; m > 0; m--)
    *--q = *--p;
/* Move in spaces */
  p = &Curr->bdata[Curr->bcurs];
  for (m = l; m > 0; m--)
    *p++ = SPACE;
/*
 * Adjust buffer and other variables ...
 */
  if (!INDENT)
    goto p2001;                    /* J no indent to look after */
  if (Curr->bcurs == ndntch)
    ndntch = ndntch + l;           /* Up indent if were at strt */
p2001:
  Curr->bcurs = Curr->bcurs + l;   /* New cursor */
  Curr->bchars = i;
  if (mxchrs < i)
    mxchrs = i;
  if (j != l)
    goto q1023;                    /* Bell if wasn't room for whole tab */
  GETNEXTCHR;                      /* Refresh or read */
/*
 * ^K - Kill
 */
p7713:
  Curr->bchars = Curr->bcurs;
  modlin = true;                   /* Line has been changed */
  GETNEXTCHR;                      /* Finished ^K */
/*
 * ^L - Left hand kill
 * If indenting and at the indent point kill the indent, otherwise
 * kill back to the indent point only.
 ^T obeys this code with some modification.
 */
p7714:
  k = Curr->bcurs;
  if (k)
  {
    p = &Curr->bdata[k];           /* 1st char to be moved down */
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
    l = Curr->bchars - k;          /* Set new line length */
    Curr->bchars = l;
    Curr->bcurs = j;
    q = &Curr->bdata[j];           /* Dest'n 1st movedown char */
    if (l != j)
    {
/* Do overlapping left hand move */
      for (m = l - j; m > 0; m--)
        *q++ = *p++;
    }                              /* if (l != j) */
  }                                /* if (k) */
  if (verb == 'T')
    goto p1201;                    /* Return if end of ^T */
  GETNEXTCHR;                      /* Finish ^L */
/*
 * ^T - Split line. Return 1st part of line in previous buffer then
 * move down the rest, then return. Calling routine must check
 * verb of course.
 */
p7724:
/*
 * Move across l/h info
 */
  k = Curr->bcurs;
/* J attempt to change file length in Fixed-Length mode */
  if (Curr->bchars != olen && !in_cmd && fmode & 0400)
    goto p2101;
/* If at eol, old line not modified here */
  modlin = modlin || (k < Curr->bchars);
p1048:
  if (k)                           /* If some chars to move */
  {
    memcpy((char *)Prev->bdata, (char *)Curr->bdata, (size_t)k);
    lstvld = true;                 /* Last line now valid */
  }                                /* if (k) */
  Prev->bchars = k;
  Prev->bdata[k] = '\0';           /* Null terminate */
  Prev->bcurs = 0;
  if (verb == 'J')
    goto p1201;                    /* Finish if was n/l */
  goto p7714;                      /* Move down r/h chars & finish */
/*
 * ^X - Move cursor 1 forward. If at end, force a space
 */
p7730:
  k = Curr->bcurs;
  if (k != Curr->bchars)
  {
    Curr->bcurs++;                 /* Move cursor fwd */
    GETNEXTCHR;                    /* Refresh etc */
  }                                /* if (k != Curr->bchars) */
  ordch(SPACE, Curr);
  modlin = true;                   /* Line has been changed */
  GETNEXTCHR;                      /* Finish ^X */
/*
 * ^Y - Cursor back 1
 * Not allowed into indent area however
 */
p7731:
  k = Curr->bcurs;
  i = 0;
  if (INDENT)
    i = ndntch;
  if (k == i)
    BOL_OR_EOL;                    /* J strt alrdy */
  Curr->bcurs--;
  GETNEXTCHR;                      /* Finish ^Y */
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
  if ((k = Curr->bchars) != olen && !in_cmd && fmode & 0400)
  {
  p2101:
    printf("\007\r\nLine length must not change in FIXED LENGTH mode");
    newlin();
    GETNEXTCHR;
  }
  goto p1048;                      /* Move chars to Prev */
/*
 * ^Z - Go to end current line but ignore previous 1
 */
p7732:
  Curr->bcurs = Curr->bchars;
  GETNEXTCHR;                      /* Finish ^Z */
/*
 * ^Q - Rest of line upper -> lower
 */
p7721:
  l = 1204;
p1207:
  k = Curr->bchars;
  j = Curr->bcurs;
  p = &Curr->bdata[j];
  k = k - j;                       /* # to do */
  if (k == 0)
    GETNEXTCHR;                    /* J out at end of line */
  modlin = true;                   /* Line has been changed */
  goto asg2l;                      /* Do the conversion */
p1204:
  for (; k > 0; k--)
  {
    if (*p >= 'A' && *p <= 'Z')
      *p += 040;
    p++;
  }
  GETNEXTCHR;                      /* Finished ^Q */
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
  GETNEXTCHR;                      /* Finished ^S */
/*
 * ^D - Right hand rubout
 */
p7704:
  if (Curr->bcurs == Curr->bchars)
  {
    err = "^D at EOL";
    ERR_IF_MAC;
  }
/* BEL if at EOL */
  Curr->bcurs++;                   /* Get past char to erase */
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
    for (k = Curr->bmxch - 1; k >= i; k--)
      if (Curr->bdata[k])
        break;
    mxchrs = k + 1;
  }
  if (Curr->bchars != mxchrs)
    modlin = true;                 /* If characters rescued */
  Curr->bchars = mxchrs;
  Curr->bcurs = mxchrs;
  GETNEXTCHR;                      /* Finish ^R */
/*
 * ^O - cOmment modify
 */
p7717:
  k = Curr->bcurs;                 /* Cursor pos'n */
  j = Curr->bchars;                /* Saves array accesses */
  if (k == j)
    GETNEXTCHR;                    /* J if at E.O.L. already */
  p = &Curr->bdata[k];             /* 1st char to check */
  i = j - k - 1;                   /* Chars to check for "/" */
  for (; i > 0; i--)
  {
    if (*p == '/' && *(p + 1) == '*')
      goto p1409;                  /* J found start of comment */
    k++;
    p++;
  }
p1407:
  Curr->bcurs = j;                 /* Set cursor to E.O.L. */
  GETNEXTCHR;                      /* End ^O here */
p1409:k = k + 1;
p14091:
  k++;
  if (k >= j)
    goto p1407;
  if (Curr->bdata[k] == SPACE)
    goto p14091;                   /* J not 1st sig char */
  Curr->bcurs = k;                 /* Cursor to 1st comment char */
  GETNEXTCHR;                      /* Finish ^O */
/*
 * ^V - recoVer from a broadcast etc.
 */
p7726:
  newlin();
  GETNEXTCHR;                      /* Finish ^V */
/*
 * ^G - Goto next character
 */
p7707:glast = true;
  gpseu = false;                   /* Not in pseudo macro */
  GETNEXTCHR;                      /* Will be back shortly */
/*
 * P1518 - We are back continuing ^G but now we have the char to look
 * for.
 */
p1518:glast = false;
  if (gpseu)
    goto p1601;                    /* J on if actually for pseudomac */
p1804:
/* Search the rest of the line AFTER the current character */
  i = Curr->bchars - Curr->bcurs - 1; /* # chars to search */
  p = &Curr->bdata[Curr->bcurs];   /* Point to cursor char */
  for (; i > 0; i--)
    if (*++p == thisch)
      break;
  if (i >= 0)
    Curr->bcurs = Curr->bchars - i; /* Guard against -ve i */
  gotoch = thisch;                 /* Remember char for ^^ */
  GETNEXTCHR;                      /* Finish ^G */
/*
 * ^^ - repeat ^G
 */
p7736:
  if (gotoch == -1)                /* No Prev ^G */
  {
    err = "^^ no previous ^G";
    ERR_IF_MAC;
  }
  thisch = gotoch;
  goto p1804;                      /* Join ^G */
/*
 * ^N - expaNd macro
 */
p7716:nseen = true;
  RAWNEXTCHR;
/*
 * P1502 - We have the macro char. May be a real macro or a pseudo
 * if we already in a macro. May also be DEL if user has changed
 * his mind. May be ^P or ^W, in which case we want the character
 * following...
 */
p1502:
  if (thisch == '\3' || thisch == '\20') /* ^C or ^P */
  {
    contp = true;
    GETNEXTCHR;
  }
  if (thisch == '\27')
    goto p7727;                    /* J is ^W */
p1905:
  nseen = false;
  contp = false;
  if (thisch == (unsigned char)'\377')
    RAWNEXTCHR;                    /* High parity rubout */
/* Recognise ESC  if not in a macro */
  if (thisch == ESC && curmac < 0)
  {
    verb = '[';                    /* For benefit of mainline */
    goto p7733;                    /* Join normal ESC */
  }                                /* if (thisch == ESC && curmac < 0) */
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
    ERR_IF_MAC;
  }
/* Check for out of range or active pseudo macro */
  if (thisch > TOPMAC)
  {
    bool found = true;
    switch (thisch)
    {
      case 04000:                  /* Return mode */
        sprintf(tbuf, "%lo", zmode);
        macdef(64, (unsigned char *)tbuf, (int)strlen(tbuf), true);
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

/* Look for half-screen requested */
        if ((fmode & 00000100000) && i > row5 / 2)
        {
          if (i > row5)
            i -= row5 / 2 + 1;
          else
            i = row5 / 2 - 1;
        }

        sprintf(tbuf, "%d", i);
        macdef(64, (unsigned char *)tbuf, (int)strlen(tbuf), true);
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
        macdef(64, (unsigned char *)macro_dir, strlen(macro_dir), true);
        break;

      case 04004:                  /* Like 4001 but for going backwards */
        j = row5;                  /* Usual amount to move + 1 */
        if (ptrpos < j)            /* On 1st screen */
          j = ptrpos;
        else if (lintot - ptrpos < j / 2 - !(row5 & 1))
        {
          j += j / 2 - lintot + ptrpos;

/* Need to go back 1 less because "E-O-F" is displaying */
          j--;

/* Need to go back 1 less again if even # lines on screen */
          j -= !(row5 & 1);
        }
        j--;
/* Sanity check - don't try to move before start of file */
        if (j >= ptrpos)
          j = ptrpos - 1;

/* Look for half-screen requested */
        if ((fmode & 00000100000) && j > row5 / 2)
        {

/* Reduce j by an extra 1 because empirically that moves the */
/* current line to the top of the screen. */
          if (j > row5)
            j -= row5 / 2 + 1;
          else
            j = row5 / 2 - 1;
        }

        sprintf(tbuf, "%d", j);
        macdef(64, (unsigned char *)tbuf, (int)strlen(tbuf), true);
        break;

      case 04005:                  /* Return screen width */
        sprintf(tbuf, "%u", col5);
        macdef(64, (unsigned char *)tbuf, (int)strlen(tbuf), true);
        break;

      case 04006:                  /* Return screen height */
        sprintf(tbuf, "%u", row5);
        macdef(64, (unsigned char *)tbuf, (int)strlen(tbuf), true);
        break;

      case 04007:                  /* Return floating point format */
        sprintf(tbuf, "%s", FPformat);
        macdef(64, (unsigned char *)tbuf, (int)strlen(tbuf), true);
        break;

      case 04010:                  /* Return date format */
        sprintf(tbuf, "%s", DTformat);
        macdef(64, (unsigned char *)tbuf, (int)strlen(tbuf), true);
        break;

      case 04011:                  /* Return date */
      {
        time_t t = time(NULL);
        strftime(tbuf, sizeof tbuf, DTformat, localtime(&t));
      }
        macdef(64, (unsigned char *)tbuf, (int)strlen(tbuf), true);
        break;

      default:
        found = false;
        break;
    }                              /* switch (thisch) */
/*
 * Deal with ALU memory and tab access pseudos
 */
    if ((thisch & 017000) == 07000)
    {
      sprintf(tbuf, "%ld", ALU_memory[thisch & 0777]);
      macdef(64, (unsigned char *)tbuf, (int)strlen(tbuf), true);
      found = true;
    }                              /* if ((thisch & 017000) == 07000) */
    else if ((thisch & 017000) == 05000)
    {
      if (get_effective_address(thisch & 0777) &&
        push_register(ALU_memory[effaddr]))
        GETNEXTCHR;
      SOUNDALARM;
    }                              /* else if ((thisch & 017000) == 05000) */
    else if ((thisch & 017000) == 06000)
    {
      if (get_effective_address(thisch & 0777) &&
        pop_register(&ALU_memory[effaddr]))
        GETNEXTCHR;
      SOUNDALARM;
    }                              /* else if ((thisch & 017000) == 06000) */
    else if ((thisch & 017000) == 013000)
    {
      sprintf(tbuf, FPformat, FPU_memory[thisch & 0777]);
      macdef(64, (unsigned char *)tbuf, (int)strlen(tbuf), true);
      found = true;
    }                              /* if ((thisch & 017000) == 013000) */
    else if ((thisch & 017000) == 011000)
    {
      if (get_effective_address(thisch & 0777) &&
        push_fp_register(FPU_memory[effaddr]))
        GETNEXTCHR;
      SOUNDALARM;
    }                              /* else if ((thisch & 017000) == 011000) */
    else if ((thisch & 017000) == 012000)
    {
      if (get_effective_address(thisch & 0777) &&
        pop_fp_register(&FPU_memory[effaddr]))
        GETNEXTCHR;
      SOUNDALARM;
    }                              /* else if ((thisch & 017000) == 012000) */
    else if (thisch >= FIRST_ALU_OP + num_ops &&
      thisch < FIRST_ALU_OP + num_ops + NUM_TABS * 2)
    {
      bool is_pop = false;
      int tabidx = thisch - FIRST_ALU_OP - num_ops;
      bool success;

      if (tabidx >= NUM_TABS)
      {
        tabidx -= NUM_TABS;
        is_pop = true;
      }                            /* if (tabidx >= NUM_TABS) */
      if (is_pop)
      {
        if ((success = pop_register(&tabs[tabidx].value)))
          tabs[tabidx].tabtyp = store_file_pos ? LINENUM : CHRPOS;
      }
      else
        success = push_register(tabs[tabidx].value);
      if (success)
        GETNEXTCHR;
      SOUNDALARM;
    }                         /* else if (ch >= FIRST_ALU_OP + num_ops && ... */

    if (found)
    {
      curmac = 64;
      mcposn = 0;
      GETNEXTCHR;
    }                              /* if (found) */
    if (thisch >= FIRST_ALU_OP && thisch < FIRST_ALU_OP + num_ops)
    {
      if (exec_alu_opcode(thisch, &err))
      {
        if (alu_skip)
        {
          alu_skip = false;
          mcposn = mcposn + 2;     /* Skip 2 */
          if (mcposn > scmacs[curmac]->mcsize)
          {
            print_failed_opcode(thisch);
            err = "ALU skip off macro end";
            ERR_IF_MAC;
          }                        /* if (mcposn > scmacs[curmac]->mcsize) */
        }                          /* if (alu_skip) */
        GETNEXTCHR;
      }                            /* if (exec_alu_opcode(thisch)) */
      else
      {
        print_failed_opcode(thisch);
        fprintf(stderr, "\r\nRegister dump:-\r\n");
        dump_registers(false);
        ERR_IF_MAC;                /* It always will be in a macro */
      }                            /* if (exec_alu_opcode(thisch)) else */
    }       /* if (thisch >= FIRST_ALU_OP && thisch < FIRST_ALU_OP + num_ops) */
  }                                /* if (thisch > TOPMAC) */

/* ^N^<000> restarts current macro */
  if (!thisch && curmac > 0)
    mcposn = 0;                    /* Leave curmac as-is */
  else
  {
/* Signal error if null or bad macro */
    if (thisch > TOPMAC || !scmacs[thisch])
    {
      if (curmac >= 0)
      {
        printf("\r\nCalling undefined macro ^<%03o>. ", thisch);
        notmac(true);
      }
      SOUNDALARM;
    }
    mcposn = 0;                    /* Got the macro */
    curmac = thisch;
  }                                /* if (!thisch && curmac > 0) else */
  RAWNEXTCHR;
p1503:
  if (thisch == 0177)
    RAWNEXTCHR;                    /* J rubout */
/*
 * Look for a pseudo macro. Some are only legal if in a macro
 */
  verb = thisch;
  if (verb >= 'a' && verb <= 'z')
    verb &= 0337;                  /* Upper case letter */
  if (curmac < 0)
  {
/* We are not in a macro. Check whether this pseudo is allowed from the
 * keyboard... */
    for (c = "EORFNMY"; *c; c++)
      if (*c == verb)
        break;
    if (*c == 0)
      SOUNDALARM;                  /* This pseudo not allowed */
  }                                /* if (curmac < 0) */
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
    notmac(true);
  }
  SOUNDALARM;
/*
 * ^NI - Increment link (to make macro a conditional)
 */
p7611:
/* Error if < 2 chars left in macro, because it can't then do a ^NU */
  if (mcposn > scmacs[curmac]->mcsize - 2)
  {
  ranoff_end:
    err = "^NI, ^NB &c. too near macro end";
    ERR_IF_MAC;
  }
  if (mcnxfr == MCDTUM)
    RAWNEXTCHR;                    /* No-op if stack empty */
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
    ERR_IF_MAC;
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
  if (thisch == '-')
    i = 79;
  else
    i = thisch - '1';              /* Get TABS array subscript */
  if (i < 0 || i >= NUM_TABS)
    TABOORANGE;
  if (gtest)
    goto p1718;                    /* J test pos'n */
  if (gwrit)
    goto p1712;                    /* J remember current pos'n */
  i4 = tabs[i].value;
  if (i4 < 0)
    TABOORANGE;                    /* J certainly too low */
  if (!gcurs)
    goto p1713;                    /* J file positioner */
  if (i4 > BUFMAX)
    TABOORANGE;                    /* J too big for a cursor value */
  i = i4;
  if (i > Curr->bchars)
    TABOORANGE;                    /* J trying to pos'n off end of line */
  Curr->bcurs = i;                 /* Set cursor position */
  GETNEXTCHR;                      /* End ^NR - may want to REFRSH */
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
    RANOFF_END;
  goto p1716;                      /* Get tab # */
p1718:
  if (tabs[i].tabtyp == UNDEFINED)
  {
    err = "^NB &c. testing undefined tab";
    ERR_IF_MAC;
  }
  if (gpast)
    goto p1719;                    /* J on if not a B */
  if (!gcurs)
    goto p1814;                    /* J actually ^N[ */
  if (tabs[i].tabtyp == LINENUM)
  {
  p17181:
    err = "^NB/^NP testing filepos tab";
    ERR_IF_MAC;
  }
  if (Curr->bcurs < tabs[i].value)
    RAWNEXTCHR;
  SKIP2MACCH;
p1814:
  if (tabs[i].tabtyp == CHRPOS)
  {
  p1841:
    err = "^N[/^N] testing chrpos tab";
    ERR_IF_MAC;
  }
  if (ptrpos < tabs[i].value)
    RAWNEXTCHR;
  SKIP2MACCH;
/*
 * ^NP - obey if Past spec'd tab
 */
p7620:gpast = true;                /* We are in fact P */
  goto p1717;
p1719:if (!gcurs)
    goto p1815;                    /* J actually ^N] */
  if (tabs[i].tabtyp == LINENUM)
    goto p17181;
  if (Curr->bcurs > tabs[i].value)
    RAWNEXTCHR;
  SKIP2MACCH;
p1815:
  if (tabs[i].tabtyp == CHRPOS)
    goto p1841;
  if (ptrpos > tabs[i].value)
    RAWNEXTCHR;
  SKIP2MACCH;
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
  if (!in_cmd && !mctrst) /* Not trusted to change file pos'n while modifying */
    BOL_OR_EOL;
/* Eof */
  if (i4 > lintot + 1 && !(deferd &&
    (dfread(i4 - lintot, NULL), i4 <= lintot + 1)))
    BOL_OR_EOL;
  setptr(i4);                      /* Position the file */
  RAWNEXTCHR;                      /* Finish ^NN */
/*
 * P1712 - Remember file or cursor position
 */
p1712:if (gcurs)
    goto p1714;                    /* J is cursor pos'n */
  if (gdiff)
    goto p1809;                    /* ^NY splits off here */
  tabs[i].value = ptrpos;
  tabs[i].tabtyp = LINENUM;
  RAWNEXTCHR;
p1714:
  tabs[i].value = Curr->bcurs;     /* ^NO */
  tabs[i].tabtyp = CHRPOS;
  RAWNEXTCHR;
/*
 * P1809 - ^NY continuing
 */
p1809:
  if (i > 77)
    TABOORANGE;                    /* J above max for a result */
  if (tabs[i].tabtyp == UNDEFINED || tabs[i].tabtyp != tabs[i + 1].tabtyp ||
    tabs[i].value >= tabs[i + 1].value)
  {
    err = "^NY tab type / value error";
    ERR_IF_MAC;
  }
  tabs[i + 2].value = tabs[i + 1].value - tabs[i].value; /* Set result */
  tabs[i + 2].tabtyp = tabs[i].tabtyp;
/*
 * Slightly awful hack - user percieves cursor tabs as 1-based but they
 * are stored zero-based. To maintain the deception, we must subtract 1
 * from the result if dealing with cursor tabs...
 */
  if (tabs[i].tabtyp == CHRPOS)
    tabs[i + 2].value--;
  RAWNEXTCHR;
/*
 * ^NA - Obey if at EOL
 */
p7601:
  if (mcposn > scmacs[curmac]->mcsize - 2)
    RANOFF_END;
/* J could skip off e.o. mac */
  if (Curr->bcurs == Curr->bchars)
    RAWNEXTCHR;
/* J at EOL */
skip2macch:
  mcposn = mcposn + 2;             /* Skip 2 */
  if (mcposn > scmacs[curmac]->mcsize)
    RANOFF_END;                    /* J skipped off e.o. mac */
  RAWNEXTCHR;
/*
 * ^NC - Obey if in command processor
 */
p7603:
  if (mcposn > scmacs[curmac]->mcsize - 2)
    BOL_OR_EOL;
/* J could skip off e.o. mac */
  if (!in_cmd)
    SKIP2MACCH;
  RAWNEXTCHR;
/*
 * ^NX - eXit from macro
 */
p7630:
  notmac(false);
  GETNEXTCHR;
/*
 * ^NT - Trust user if he wants to change file pointer during modify
 */
p7624:mctrst = true;
  RAWNEXTCHR;
/*
 * ^NS - Unconditional skip (reverse obey sense)
 */
p7623:
  SKIP2MACCH;
/*
 * ^NE - Reset insErt mode (^E) to OFF
 */
p7605:insert = false;
  if (curmac < 0)
    refrsh(Curr);                  /* Close gap */
  RAWNEXTCHR;
/*
 * ^NU - Up from a macro s/r
 */
p7625:
/* Treat as exit if stack empty */
  if (mcnxfr == MCDTUM)
    goto p7630;
/*
 * Look for stack corruption
 */
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
    notmac(true);
    SOUNDALARM;
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

/* If leaving an immediate macro, this entry is now free */
  if (curmac >= FIRST_IMMEDIATE_MACRO && curmac <= LAST_IMMEDIATE_MACRO)
    immnxfr = curmac;

  curmac = i;
  mcposn = j;                      /* Accept the popped values */

/* If the frame we just freed was created by U-use, we must reinstate it. */
/* Only z-enduse can free this frame. */
/* Arrange that ^NI becomes a no-op, otherwise revert to the command source */
/* (which can't be a macro). */
  if (mcstck[mcnxfr].u_use)
  {
    mcnxfr++;
    if (verb == 'I')
    {
/* Restore macro that issued ^NI */
      curmac = l;
      mcposn = m;
    }
    else
      curmac = -1;
  }                                /* if (mcstck[mcnxfr+1].u_use) */
  else if (verb == 'I')
    goto p7604;
  RAWNEXTCHR;
/*
 * ^ND - go Down a level ( a macro as a subroutine)
 */
p7604:
/* Error if < 2 chars left in macro */
  if (mcposn > scmacs[curmac]->mcsize - 2)
    BOL_OR_EOL;
  if (MCLMIT == mcnxfr)
  {
    err = "Macro stack depth limit exceeded";
    ERR_IF_MAC;
  }
  mcstck[mcnxfr].mcprev = curmac;
/* Return addr after following macro */
  mcstck[mcnxfr].mcposn = mcposn + 2;
  mcstck[mcnxfr].u_use = false;
  mcnxfr++;                        /* Up stack pointer */
  if (verb == 'I')
  {
/* Restore next macro in stack */
    curmac = l;
    mcposn = m;
  }
  RAWNEXTCHR;
/*
 * ^NJ long (signed) jump
 */
p7612:
/* Must be 1 more char in macro */
  if (mcposn >= scmacs[curmac]->mcsize)
    RANOFF_END;
  fornj = true;                    /* Indicate ^N^J */
  goto p1902;                      /* Get jump length */
/*
 * ^NL - Long skip
 */
p7614:mcposn = mcposn + 2;         /* Skip 2 ... */
  SKIP2MACCH;                      /* ... skip 2 more */
/*
 * ^NM - Make Macro from current line
 */
p7615:
  if (curmac < 0)
    goto p1908;                    /* J keybd pseudo */
/* Must be 1 more char in macro */
  if (scmacs[curmac]->mcsize == mcposn)
    BOL_OR_EOL;
p1908:gmacr = true;                /* We are defining a macro */
  goto p1902;                      /* Get macro ID */
/*
 * ^NG - obey if Got next ch
 */
p7607:
/* J not another char + something to obey */
  if (scmacs[curmac]->mcsize - mcposn < 3)
    RANOFF_END;
  gmacr = false;                   /* We are not defining a macro */
p1902:gposn = false;               /* We are not a file or curs posn */
p1707:glast = true;                /* So we will come back */
  gpseu = true;                    /* So we will come back */
  RAWNEXTCHR;
/*
 * P1601 - Come here with ^NG char
 */
p1601:
  if (fornj)
  {
    fornj = false;
    mcposn += thisch;              /* Do the jump */
    if (mcposn >= 0 && mcposn < scmacs[curmac]->mcsize)
      RAWNEXTCHR;
    err = "^NJ off macro end or start";
    ERR_IF_MAC;
  }
  if (gposn)
    goto p1708;                    /* J in fact for a positioner */
  if (gmacr)
    goto p1903;                    /* J in fact defining a macro */
  if (Curr->bcurs == Curr->bchars)
    SKIP2MACCH;
/* Skip if at eol */
  if (Curr->bdata[Curr->bcurs] != thisch)
    SKIP2MACCH;
/* Skip if mismatch */
  RAWNEXTCHR;
/*
 * P1903 - Come here with macro to define. All characters are legal
 *         except the pseudomacros...
 */
p1903:
  if ((thisch < 0200 && thisch > 077) ||
    (thisch > TOPMAC && thisch < 07000) || thisch > 07777)
  {
    err = "";
    fprintf(stderr, "\r\n^NM cannot define macro %o", thisch);
    newlin();
    ERR_IF_MAC;
  }                                /* if ((thisch < 0200 && ... )) */
  if (Curr->bchars == 0)
    BOL_OR_EOL;                    /* Trying to define null macro */
/* Define the macro. Report if some problem... */
  Curr->bdata[Curr->bchars] = 0;   /* macdef prefers terminated strings */
  if (macdef((int)thisch, Curr->bdata, (int)Curr->bchars, true))
    RAWNEXTCHR;
  if (curmac >= 0)
    notmac(true);
  SOUNDALARM;                      /* Report failure */
/*
 * ^W - Next char not special but Without parity
 */
p7727:cntrlw = true;
  contp = true;
  GETNEXTCHR;
/*
 * P1201 - Exit sequence. Reinstate XON if req'd...
 */
p1201:
  if (!(USING_FILE || nodup))
    duplx5(true);                  /* enable XOFF */
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
      return;                      /* ... salvage his edit. */
  }
}
