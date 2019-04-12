/* S C R D I T */
/*
 * Copyright (C) 1981 D. C. Roe
 * Copyright (C) 2012-2014,2017-2019 Duncan Roe
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
#include "prototypes.h"
#include "macros.h"
#include "cmndcmmn.h"
#include "scrnedit.h"
#include "fmode.h"
#include "tabs.h"
#include "c1in.h"
#include "alu.h"
#include "backtick.h"

/* Macros */

#define GETNEXTCHR goto getnextchr
#define BOL_OR_EOL goto bol_or_eol
#define ERR_IF_MAC goto err_if_mac
#define SOUNDALARM goto soundalarm
#define TABOORANGE goto taboorange
#define SKIP2MACCH goto skip2macch
#define NORMALCHAR goto normalchar
#define RANOFF_END goto ranoff_end
#define CHECK_HAS_MACCH(x) \
  do { if (curmac > 0 && mcposn > scmacs[curmac]->maclen - x) RANOFF_END; } \
  while(0)
#define GET_FOLLOWING_CHAR goto get_following_char
#define TEST_TAB goto test_tab
#define CHECK_MACRO_END goto check_macro_end
#define LEAVE_SCRDIT goto leave_scrdit

/* Externals that are not in any header */

uint8_t fxtabl[128];
clock_t timlst;

/* Initialise External Variables */

scrbuf5 *last_Curr = NULL;

/* *************************** print_failed_opcode ************************** */

static void
print_failed_opcode(uint16_t thisch)
{
  char *opcd = opcode_defs[alu_table_index[thisch - FIRST_ALU_OP]].name;

  fprintf(stderr, "\r\nFailing opcode: ");
  while (*opcd)
    putc(toupper(*(uint8_t *)opcd++), stdout);
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
  uint8_t lastch;                  /* Dump for char we nullify */
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
  uint8_t *p, *q;                  /* Scratch */
  char *c;                         /* Scratch */
  char *err = NULL;                /* Point to error text */
  char tbuf[256];                  /* Scratch */
  int i, j, k, h = 0, m = 0;       /* Scratch variables */
  int gotoch = -1;                 /* Char of last ^G */
  long i4;                         /* Scratch */
  long olen;                       /* Original line length */
  struct tms tloc;                 /* Junk from TIMES */
  clock_t timnow;                  /* Time from TIMES */
  uint16_t thisch;                 /* Character being processed */
  uint8_t *indent_string = NULL;
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
  bool contp = false, cntrlw = false, nseen = false, glast = false;
  bool gpseu = false, gposn;
  bool gwrit, gcurs, gtest, gpast;
  bool gmacr, fornj = false;
  bool gwthr = false;              /* WheTHeR macro exists or length */
/*
 * LOCAL LOGICAL VARIABLES:
 * ========================
 *
 * cntrlw - true if ^W seen
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
 * gmacr  - This pseudo macro is actually ^NM (not ^NG)
 * gwthr  - This pseudo macro is actually ^NW (not ^NG)
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
  insert = modlin = endlin = false;
  pchars = pchrs;
  mxchrs = Curr->bchars;
  last_Curr = Curr;
  if (pchars)
    strcpy((char *)prompt, (char *)prmpt); /* Move prompt to common area */
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
      if (lstvld)
        indent_string = Prev->bdata;
      else
        indent_string = sindnt();  /* Sets ndntch & forces lstvld to be true */
      if (ndntch > 0)
      {
        for (i = ndntch, p = indent_string; i > 0; i--)
          ordch(*p++, Curr);       /* Pad out with spaces */
        modlin = true;             /* Line has been changed */
      }                            /* if (ndntch > 0) */
    }                              /* if !(Curr->bchars) */
    else
    {
      ndntch = 0;
      for (j = Curr->bchars; j > 0; j--)
      {
        if (!isspace(Curr->bdata[ndntch]))
          break;
        ndntch = ndntch + 1;
      }
      if (Curr->bcurs < ndntch)
      {
/* Leave the cursor if came here from [F]L */
        if (verb != 'L' && verb != 'l')
          Curr->bcurs = ndntch;
      }                            /* if (Curr->bcurs < ndntch) */
    }                              /* if !(Curr->bchars) else */
  }                                /* if (INDENT) */
  else
    ndntch = 0;                    /* Not indenting, so no indent chars */

  mctrst = false;
getnextchr:
  if (curmac < 0)
    refrsh(Curr);                  /* Display prompt etc */
  if (curmac >= 0)
  {
/* Clear expanding if that was last char from macro */
    if (mcposn >= scmacs[curmac]->maclen)
    {
      notmac(false);
      GETNEXTCHR;
    }                              /* if (mcposn >= scmacs[curmac]->maclen) */
    thisch = scmacs[curmac]->data[mcposn++];
  }                                /* if (curmac >= 0) */
  else
  {
    bool eof_encountered;

    thisch = c1in5(&eof_encountered); /* Read 1 char */
    if (eof_encountered)
    {                              /* c1in5 will have returned '\r' */
      if (in_cmd && Curr->bchars == 0 && USING_FILE)
      {
        Curr->bdata[0] = 'z';
        Curr->bchars = 1;
      }                            /* if (in_cmd && ...) */
      else
        pop_stdin();
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
  {
    if (cntrlw)
    {
      thisch |= 0200;              /* Set parity following ^W */
      cntrlw = false;
    }                              /* if (cntrlw) */
    if (nseen)
      goto p1905;                  /* Treat as macro char if ^N Prev */
    NORMALCHAR;                    /* Treat as ordinary character */
  }                                /* if (contp) */
  if (nseen)
    goto p1502;                    /* J expecting macro name */
  if (glast)                       /* In middle of ^G */
  {
    glast = false;
    if (gpseu)
      goto p1601;                  /* J on if actually for pseudomac */
  ctl_uparrow_joins:
/* Search the rest of the line AFTER the current character */
    i = Curr->bchars - Curr->bcurs - 1; /* # chars to search */
    p = &Curr->bdata[Curr->bcurs]; /* Point to cursor char */
    for (; i > 0; i--)
    {
      p++;
      if (*p == thisch || (thisch == SPACE && isspace(*p) && MATCH_ANY_WHSP))
        break;
    }                              /* for (; i > 0; i--) */
    if (i >= 0)
      Curr->bcurs = Curr->bchars - i; /* Guard against -ve i */
    gotoch = thisch;               /* Remember char for ^^ */
    GETNEXTCHR;                    /* Finish ^G */
  }                                /* if (glast) */
  if (thisch == DEL)               /* Delete char before cursor */
  {
    if (!Curr->bcurs)
      BOL_OR_EOL;                  /* Error if at line start */

/* If at indent point,decrement INDENT */
    if (INDENT && Curr->bcurs == ndntch)
      ndntch--;

/* Move cursor back 1 and become ^D */
    Curr->bcurs--;
    thisch = 4;
  }                                /* if (thisch == 0177) */
  if (thisch >= SPACE)
    NORMALCHAR;                    /* J not a control */
  verb = thisch + 0100;            /* 'J' for ^J, &c. */
  switch (thisch)
  {
    case 0:                        /* NUL - (ignored) */
      GETNEXTCHR;

    case 1:                        /* ^A - Again/Append */
/*
 * If more info in previous line than current, append previous excess
 * to current, then in either case move cursor to follow last char
 */
      i = ndntch;                  /* In case we SINDNT */
      if (!lstvld)
        sindnt();                  /* Get previous line valid */
      ndntch = i;
      if (Curr->bchars < Prev->bchars)
      {
/*
 * We have to move chars from the previous line
 */
        modlin = true;             /* Line has been changed */
        memcpy((char *)&Curr->bdata[Curr->bchars],
          (char *)&Prev->bdata[Curr->bchars],
          (size_t)(Prev->bchars - Curr->bchars));
        Curr->bchars = Prev->bchars; /* Extend length of line */
        if (mxchrs < Curr->bchars)
          mxchrs = Curr->bchars;
      }                            /* if (Curr->bchars < Prev->bchars) */
      Curr->bcurs = Curr->bchars;  /* Put cursor at end */
      GETNEXTCHR;                  /* Finish ^A */

    case 2:                        /* ^B - Word back */
/*
 * Go back to the start of this word or back to the start of the
 * previous word if at the start of this one. Words are delimited by any
 * white space. Error if no backwards movement possible.
 */
      i = INDENT ? ndntch : 0;     /* S.O.L. if no indenting */
      k = Curr->bcurs - i;         /* k - running available movement */
/* Note k can be -ve, after ^NR to move before the indent point */
      if (k <= 0)
        BOL_OR_EOL;                /* J start of line (error) */
/* Check whether at word start. If so, back up to a non-space (or line
 * start)... */
      p = &Curr->bdata[Curr->bcurs];
/* Reduce movement available, B none left; B found non-space */
      if (isspace(*(p - 1)))
        for (;;)
          if (!--k || !isspace(*--p))
            break;
/* We may now be back at line start. If not, move cursor back until the
 * preceding character is space. This may now be the case already, so
 * check the value of the preceding character first... */
      if (k)
        for (;;)
          if (isspace(*--p) || !--k)
            break;
      Curr->bcurs = i + k;         /* Start of line + room */
      GETNEXTCHR;                  /* Finish ^B */

    case 4:                        /* ^D - Right hand rubout */
      if (Curr->bcurs == Curr->bchars)
      {
        err = "^D at EOL";
        ERR_IF_MAC;
      }
      k = Curr->bcurs++;           /* Get past char to erase */
      modlin = true;               /* Line has been changed */
      j = Curr->bchars - 1;        /* How many chars there will be */
      if (j != k)
        memmove(Curr->bdata + k, Curr->bdata + k + 1, j - k);
      Curr->bcurs = k;             /* Step cursor back */
      Curr->bchars = j;            /* Reduce # of chars */
      GETNEXTCHR;

    case 5:                        /* ^E - Enter/Leave insert mode */
      insert = !insert;
      GETNEXTCHR;                  /* End insert */

    case 6:                        /* ^F - Forward to start next word */
      k = Curr->bchars - Curr->bcurs; /* k - how far we can move forward */
/* If on non-space, move forward to space, unless at EOL now... */
      p = &Curr->bdata[Curr->bcurs];
/* Ensure we're pointing at a space (or we were). Always reduces k,
 * unless at EOL to start with... */
      if (k < 0)
      {
        fprintf(stderr, "Internal error: k=%d\r\n", k);
        notmac(true);
        SOUNDALARM;
      }
      if (k)
        for (;;)
          if (!--k || isspace(*p++))
            break;
/* Reduce k further if not now pointing at a non-space... */
      if (k)
        for (;;)
          if (!isspace(*p++) || !--k)
            break;
      Curr->bcurs = Curr->bchars - k;
      GETNEXTCHR;

    case 7:                        /* ^G - Goto next character */
      glast = true;
      gpseu = false;               /* Not in pseudo macro */
      GETNEXTCHR;                  /* Will be back shortly */

    case 8:                        /* ^H - Home */
      Curr->bcurs = INDENT ? ndntch : 0; /* Reset cursor */
      GETNEXTCHR;

    case 9:                        /* ^I - Insert a tab */
/*
 * Insert enough spaces to get to next tab posn if there is one, otherwise
 * insert 1 space. Once inserted, there is no remembrance that this was a tab -
 * spaces may be individually deleted etc.
 */
      modlin = true;               /* Line has been changed */
      for (i = 0; i < tabcnt; i++) /* Find 1st tab past where we are */
        if (Curr->bcurs < tabs[i].value && tabs[i].tabtyp == CHRPOS)
          break;                   /* Tab found */
      if (i == tabcnt)             /* Force 1 space if drop thro' */
        j = 1;                     /* Insert 1 space */
      else
        j = tabs[i].value - Curr->bcurs; /* # spaces */
/*
 * See how many spaces we actually have room for
 */
      i = Curr->bchars;
      h = Curr->bmxch - i;         /* # there is room for */
      if (h > 0)                   /* Some room */
      {
        p = &Curr->bdata[i];       /* 1 past last char to pick up */
        m = i - Curr->bcurs;       /* # chars to move up */
        if (h > j)
          h = j;                   /* # spaces to insert */
        i = i + h;                 /* New length */
        q = &Curr->bdata[i];       /* 1 past last char to set down */
/* Do the right-hand overlapping move */
        for (; m > 0; m--)
          *--q = *--p;
/* Move in spaces */
        p = &Curr->bdata[Curr->bcurs];
        for (m = h; m > 0; m--)
          *p++ = SPACE;
/*
 * Adjust buffer and other variables ...
 */
        if (INDENT && Curr->bcurs == ndntch) /* Indenting and at indent point */
          ndntch = ndntch + h;
        Curr->bcurs = Curr->bcurs + h; /* New cursor */
        Curr->bchars = i;
        if (mxchrs < i)
          mxchrs = i;
        if (j == h)                /* Tab inserted successfully */
          GETNEXTCHR;
      }                            /* if (h > 0) */
      err = "Line full";
      ERR_IF_MAC;

    case 13:                       /* ^M - End of line */
      verb = 'J';                  /* Treat as Nl: drop thru to ^J */

    case 10:                       /* ^J - End of line: drop thru to ^T */
      k = Curr->bchars;

    case 20:                       /* ^T - Split line */
      if (verb == 'T')             /* If not a drop-thru */
      {
        k = Curr->bcurs;
        modlin = modlin || (k < Curr->bchars); /* modlin unless ^T at EOL */
      }                            /* if (verb == 'T') */

/* Disallow length change in FIXED LENGTH mode, when editing the file */
      if (Curr->bchars != olen && !in_cmd && fmode & 0400)
      {
        fprintf(stderr, "\r\nLine length must not change in FIXED LENGTH mode");
        visbel();
        newlin();
        GETNEXTCHR;
      }

/* Copy 1st part (^T) or all (^J) of line to previous buffer */
      memcpy((char *)Prev->bdata, (char *)Curr->bdata, (size_t)k);
      Prev->bchars = k;
      Prev->bdata[k] = '\0';       /* Null terminate */
      Prev->bcurs = 0;
      lstvld = true;               /* Last line now valid */

      if (verb == 'J')
        LEAVE_SCRDIT;              /* Finish if was n/l else drop thru to ^L */

    case 12:                       /* ^L - Left hand kill */
/*
 * If indenting and at the indent point kill the indent, otherwise
 * kill back to the indent point only.
 * ^T obeys this code with some modification.
 */

/* Move down the rest of the line then return immediately if ^T.
 * Caller [q] must check verb for what to do vis the workfile */
      k = Curr->bcurs;
      if (k)
      {
        p = &Curr->bdata[k];       /* 1st char to be moved down */
        if (verb == 'L')
          modlin = true;           /* ^T makes its own decision */
        j = 0;                     /* Eventual cursor pos if not indenting */
        if (INDENT)
        {
          if (verb == 'L' && k == ndntch)
            ndntch = 0;            /* Kill indent if at indenting point */
          k = k - ndntch;
          j = ndntch;
        }                          /* if (INDENT) */
        h = Curr->bchars - k;      /* Set new line length */
        Curr->bchars = h;
        Curr->bcurs = j;
        q = &Curr->bdata[j];       /* Dest'n 1st movedown char */
        if (h != j)
        {
/* Do overlapping left hand move */
          for (m = h - j; m > 0; m--)
            *q++ = *p++;
        }                          /* if (h != j) */
      }                            /* if (k) */
      if (verb == 'T')
        LEAVE_SCRDIT;              /* Return if end of ^T */
      GETNEXTCHR;                  /* Finish ^L */

    case 11:                       /* ^K - Kill */
      Curr->bchars = Curr->bcurs;
      modlin = true;               /* Line has been changed */
      GETNEXTCHR;                  /* Finished ^K */

    case 14:                       /* ^N - expaNd macro */
      nseen = true;
      GETNEXTCHR;

    case 15:                       /* ^O - cOmment modify */
      k = Curr->bcurs;             /* Cursor pos'n */
      p = Curr->bdata + k;         /* 1st char to check */
      i = Curr->bchars - k - 1;    /* Chars to check for "/" */
      for (; i > 0; i--, k++, p++)
        if (*p == '/' && *(p + 1) == '*')
          break;                   /* Found start of comment */
      if (i > 0)
      {
        k++;                       /* Point to '*' */
        do
        {
          k++;
          if (k >= Curr->bchars)
            break;
        }
        while (Curr->bdata[k] == SPACE);
        if (k < Curr->bchars)
        {
          Curr->bcurs = k;         /* Cursor to 1st comment char */
          GETNEXTCHR;              /* Finish ^O */
        }                          /* if (k < Curr->bchars) */
      }                            /* if (i > 0) */
      Curr->bcurs = Curr->bchars;  /* Set cursor to E.O.L. */
      GETNEXTCHR;

    case 3:                        /* drop thru */

    case 16:                       /* ^P (or ^C) - next char not special */
      contp = true;
      GETNEXTCHR;

    case 19:                       /* drop thru: ^S - Lower -> upper */

    case 17:                       /* ^Q - Rest of line upper -> lower */
    {
      int (*cvt) (int);

      if (verb == 'S')
        cvt = toupper;
      else
        cvt = tolower;
      k = Curr->bchars;
      j = Curr->bcurs;
      p = &Curr->bdata[j];
      k = k - j;                   /* # to do */
      modlin = k > 0;              /* Line changed if any chars to do */
      for (; k > 0; k--, p++)
        *p = cvt(*p);
      GETNEXTCHR;                  /* Finished ^Q */
    }

    case 18:                       /* ^R - Reveal all of line */
/*
 * Fudge in case recovering from accidental ESC - if no chars typed
 * reveal max possible BUF chars. BUT,,, don't reveal trailing null
 * characters, which have never been used...
 */
      i = INDENT ? ndntch : 0;
      if (mxchrs == i)
      {
        for (k = Curr->bmxch - 1; k >= i; k--)
          if (Curr->bdata[k])
            break;
        mxchrs = k + 1;
      }
      if (Curr->bchars != mxchrs)
        modlin = true;             /* If characters rescued */
      Curr->bchars = mxchrs;
      Curr->bcurs = mxchrs;
      GETNEXTCHR;                  /* Finish ^R */

    case 21:                       /* ^U - Cancel */
      i = INDENT ? ndntch : 0;
      if (Curr->bchars != i)
        modlin = true;             /* Cancel empty line not a mod */
      Curr->bchars = Curr->bcurs = i;
      GETNEXTCHR;                  /* Finish ^U */

    case 22:                      /* re-display line to recoVer from whatever */
      newlin();
      GETNEXTCHR;                  /* Finish ^V */

    case 23:                       /* ^W - Next char With parity bit on */
      cntrlw = true;
      contp = true;
      GETNEXTCHR;

    case 24:                       /* ^X - Move cursor 1 forward */
/* Append space if at EOL */
      if (Curr->bcurs != Curr->bchars)
        Curr->bcurs++;             /* Move cursor fwd */
      else
      {
        ordch(SPACE, Curr);
        modlin = true;             /* Line has been changed */
      }                            /* If (curr->bcurs != curr->bchars) else */
      GETNEXTCHR;                  /* Finish ^X */

    case 25:                       /* ^Y - Cursor back 1 */
      k = Curr->bcurs;             /* Not allowed into indent area however */
      i = INDENT ? ndntch : 0;
      if (k == i)                  /* At start or indent already */
        BOL_OR_EOL;
      Curr->bcurs--;
      GETNEXTCHR;                  /* Finish ^Y */

    case 26:                       /* ^Z - Go to end current line */
      Curr->bcurs = Curr->bchars;
      GETNEXTCHR;                  /* Finish ^Z */

    case 27:                       /* ESC - Abandon */
      LEAVE_SCRDIT;                /* Simply exit */

    case 30:                       /* ^^ - repeat ^G */
      if (gotoch == -1)            /* No Prev ^G */
      {
        err = "^^ no previous ^G";
        ERR_IF_MAC;
      }
      thisch = gotoch;
      goto ctl_uparrow_joins;      /* Jump into the middle of ^G */
  }                                /* switch (thisch) */
/*
 * Non-special character
 */
normalchar:
  ordch(thisch, Curr);             /* Insert or replace as appropriate */
  modlin = true;                   /* Line has been changed */
  contp = false;
  GETNEXTCHR;                      /* Finish */
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
  fornj = gpseu = glast = false;
  visbel();                        /* O/p BEL since error */
  mctrst = false;                  /* User no longer trusted */
  GETNEXTCHR;                      /* End rubout */
taboorange:
  if (curmac >= 0)
  {
    fprintf(stderr, "\r\nTab ID %c or value in that tab out of range. ",
      thisch);
    notmac(true);
  }
  SOUNDALARM;
p1502:
/*
 * We have a putative macro char. May be ^P or ^W,
 * in which case we want the character following...
 */
  if (thisch == '\20' || thisch == '\3') /* ^P or (if using alt INTR) ^C */
  {
    contp = true;
    GETNEXTCHR;
  }
  if (thisch == '\27')
  {
    cntrlw = contp = true;
    GETNEXTCHR;
  }                                /* if (thisch == '\27') */
p1905:
/* We have a macro char unless it's DEL or ESC from keybd */
  contp = false;
  if (thisch == 0377)              /* User typed DEL after ^W */
    GETNEXTCHR;                    /* Undo ^W after ^N */
  nseen = false;
  if (thisch == DEL)               /* User typed DEL after ^N */
    GETNEXTCHR;                    /* Undo ^N */

/* Recognise ESC  if not in a macro */
  if (thisch == ESC && curmac < 0)
  {
    verb = '[';                    /* For benefit of mainline */
    LEAVE_SCRDIT;                  /* Same as normal ESC */
  }                                /* if (thisch == ESC && curmac < 0) */

  if (thisch <= LAST_PSEUDO && thisch >= FIRST_PSEUDO) /* BRKPT startg macro */
    goto p1503;                    /* J a pseudo or rubout */

/* Look for SIGINT seen if chaining.
 * We can fully deal with it unless in U-USE,
 * but we must abandon macro chaining */
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
        i = snprintf(tbuf, sizeof tbuf, "%lo", zmode_valid ? zmode : fmode);
        macdef(FIRST_PSEUDO, (uint8_t *)tbuf, i, true);
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

        j = snprintf(tbuf, sizeof tbuf, "%d", i);
        macdef(FIRST_PSEUDO, (uint8_t *)tbuf, j, true);
        break;

      case 04002:                  /* Return curent edit file */
/*
 * If there is no current file, ensure macro will not return. This since
 * the null argument might not be noticed in a macro, and the commnad's
 * next argument (if any) taken as a filename (numbers being legal
 * filenames)
 */
        i = strlen(pcnta);
        macdef(FIRST_PSEUDO, (uint8_t *)pcnta, i, i != 0);
        break;

      case 04003:                  /* HELP dir (for macros usually) */
        macdef(FIRST_PSEUDO, (uint8_t *)macro_dir, strlen(macro_dir), true);
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

        i = snprintf(tbuf, sizeof tbuf, "%d", j);
        macdef(FIRST_PSEUDO, (uint8_t *)tbuf, i, true);
        break;

      case 04005:                  /* Return screen width */
        i = snprintf(tbuf, sizeof tbuf, "%u", col5);
        macdef(FIRST_PSEUDO, (uint8_t *)tbuf, i, true);
        break;

      case 04006:                  /* Return screen height */
        i = snprintf(tbuf, sizeof tbuf, "%u", row5);
        macdef(FIRST_PSEUDO, (uint8_t *)tbuf, i, true);
        break;

      case 04007:                  /* Return floating point format */
        i = snprintf(tbuf, sizeof tbuf, "%s", FPformat);
        macdef(FIRST_PSEUDO, (uint8_t *)tbuf, i, true);
        break;

      case 04010:                  /* Return date format */
        i = snprintf(tbuf, sizeof tbuf, "%s", DTformat);
        macdef(FIRST_PSEUDO, (uint8_t *)tbuf, i, true);
        break;

      case 04011:                  /* Return date */
      {
        time_t t = time(NULL);
        i = strftime(tbuf, sizeof tbuf, DTformat, localtime(&t));
      }
        macdef(FIRST_PSEUDO, (uint8_t *)tbuf, i, true);
        break;

      case 04012:                  /* Return UTC date */
      {
        time_t t = time(NULL);
        i = strftime(tbuf, sizeof tbuf, DTformat, gmtime(&t));
      }
        macdef(FIRST_PSEUDO, (uint8_t *)tbuf, i, true);
        break;

      case 04013:                  /* Return integer format */
        i = snprintf(tbuf, sizeof tbuf, "%s", Iformat);
        macdef(FIRST_PSEUDO, (uint8_t *)tbuf, i, true);
        break;

      case 04014:                  /* Return stdout from last backtick */
/* Drop thru */
      case 04015:                  /* Return stderr from last backtick */
        curmac = thisch == 04014 ? STDOUT_MACRO_IDX : STDERR_MACRO_IDX;
        mcposn = 0;
        GETNEXTCHR;

      default:
        found = false;
        break;
    }                              /* switch (thisch) */
/*
 * Deal with ALU memory and tab access pseudos
 */
    j = thisch & 017000;
    if (j == 07000)
    {
      i = snprintf(tbuf, sizeof tbuf, Iformat, ALU_memory[thisch & 0777]);
      macdef(FIRST_PSEUDO, (uint8_t *)tbuf, i, true);
      found = true;
    }                              /* if (j == 07000) */
    else if (j == 05000)
    {
      if (get_effective_address(thisch & 0777) &&
        push_register(ALU_memory[effaddr]))
        GETNEXTCHR;
      SOUNDALARM;
    }                              /* else if (j == 05000) */
    else if (j == 06000)
    {
      if (get_effective_address(thisch & 0777) &&
        pop_register(&ALU_memory[effaddr]))
        GETNEXTCHR;
      SOUNDALARM;
    }                              /* else if (j == 06000) */
    else if (j == 013000)
    {
      i = snprintf(tbuf, sizeof tbuf, FPformat, FPU_memory[thisch & 0777]);
      macdef(FIRST_PSEUDO, (uint8_t *)tbuf, i, true);
      found = true;
    }                              /* if (j == 013000) */
    else if (j == 011000)
    {
      if (get_effective_address(thisch & 0777) &&
        push_fp_register(FPU_memory[effaddr]))
        GETNEXTCHR;
      SOUNDALARM;
    }                              /* else if (j == 011000) */
    else if (j == 012000)
    {
      if (get_effective_address(thisch & 0777) &&
        pop_fp_register(&FPU_memory[effaddr]))
        GETNEXTCHR;
      SOUNDALARM;
    }                              /* else if (j == 012000) */
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
          tabs[tabidx].tabtyp = STORE_FILE_POS ? LINENUM : CHRPOS;
      }
      else
        success = push_register(tabs[tabidx].value);
      if (success)
        GETNEXTCHR;
      SOUNDALARM;
    }                         /* else if (ch >= FIRST_ALU_OP + num_ops && ... */

    if (found)
    {
      curmac = FIRST_PSEUDO;
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
          if (mcposn > scmacs[curmac]->maclen)
          {
            print_failed_opcode(thisch);
            err = "ALU skip off macro end";
            ERR_IF_MAC;
          }                        /* if (mcposn > scmacs[curmac]->maclen) */
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
        fprintf(stderr, "\r\nCalling undefined macro ^<%03o>. ", thisch);
        notmac(true);
      }
      SOUNDALARM;
    }
    mcposn = 0;                    /* Got the macro */

/* If curmac was an immediate macro, that immediate macro is now available */
/* for re-use, *unless* it is in the macro stack (so ^NU can return to it) */
  if (curmac >= FIRST_IMMEDIATE_MACRO && curmac <= LAST_IMMEDIATE_MACRO)
  {
    bool found = false;

    for (i = mcnxfr - 1; i >= 0; i--)
      if (mcstck[i].mcprev == curmac)
      {
        found = true;
        break;
      }                            /* if (scmacs[i].mcprev == curmac) */
    if (!found)
      immnxfr = curmac;
  }                                /* if (curmac >= FIRST_IMMEDIATE_MACRO ... */
    curmac = thisch;
  }                                /* if (!thisch && curmac > 0) else */
  GETNEXTCHR;
p1503:
/*
 * Look for a pseudo macro. Some are only legal if in a macro
 */
  verb = toupper(thisch);
  if (curmac < 0)
  {
/* We are not in a macro. Check whether this pseudo is allowed from the
 * keyboard... */
    for (c = "EORFNMW"; *c; c++)
      if (*c == verb)
        break;
    if (*c == 0)
      SOUNDALARM;                  /* This pseudo not allowed */
  }                                /* if (curmac < 0) */
/* Clear all possibly relevant booleans here, rather than higgelty-piggelty
 * all over the place */
  gposn = gwrit = gcurs = gtest = gpast = gmacr = gwthr = false;

  switch (thisch & 037)
  {                                /* Try for a pseudo u/c or h/c */
    case 1:                        /* ^NA - Obey if at EOL */
      CHECK_HAS_MACCH(2);
      if (Curr->bcurs == Curr->bchars)
        GETNEXTCHR;                /* J at EOL */
    skip2macch:
      CHECK_HAS_MACCH(2);
      mcposn += 2;                 /* Skip 2 */
      GETNEXTCHR;

    case 2:                        /* ^NB - obey if Before spec'd tab */
      gcurs = true;
      TEST_TAB;

    case 3:                        /* ^NC - obey if in Command processor */
      CHECK_HAS_MACCH(2);
      if (!in_cmd)
        SKIP2MACCH;
      GETNEXTCHR;

    case 4:
      goto p7604;

    case 5:                        /* ^NE - reset insErt mode (^E) */
      insert = false;
      if (curmac < 0)
        refrsh(Curr);              /* Close gap */
      GETNEXTCHR;

    case 6:                        /* ^NF - set spec'd tab to File position */
      gwrit = true;
      CHECK_MACRO_END;

    case 7:                        /* ^NG - obey if Got next ch */
      CHECK_HAS_MACCH(3);          /* Need another char + something to obey */
    get_following_char:
      glast = gpseu = true;        /* So we will come back */
      GETNEXTCHR;

    case 9:             /* ^NI - Increment link (to make macro a conditional) */
/* Error if < 2 chars left in macro, because it can't then do a ^NU */
      if (mcposn > scmacs[curmac]->maclen - 2)
      {
      ranoff_end:
        err = "^NA, ^NB, ^NC &c. too near macro end";
        ERR_IF_MAC;
      }
      if (mcnxfr == MCDTUM)
        GETNEXTCHR;                /* No-op if stack empty */
      h = curmac;                  /* Save current macro BRKPT ^NI invoked */
      m = mcposn;                  /* Save current macro position */
      goto p1706;                  /* Do a dummy UP */

    case 10:                       /* ^NJ long (signed) Jump */
      CHECK_HAS_MACCH(1);          /* Must be 1 more char in macro */
      fornj = true;                /* Indicate ^N^J */
      GET_FOLLOWING_CHAR;          /* Get jump length */

    case 12:                       /* ^NL - Long skip */
      mcposn += 2;                 /* Skip 2 ... */
      SKIP2MACCH;                  /* ... skip 2 more */

    case 13:                       /* ^NM - make Macro from current line */
      if (curmac > 0)
        CHECK_HAS_MACCH(1);        /* Must be 1 more char in macro */
      gmacr = true;                /* We are defining a macro */
      GET_FOLLOWING_CHAR;          /* Get macro ID */

    case 14:                       /* ^NN - positioN file to spec'd tab */
      CHECK_MACRO_END;

    case 15:                       /* ^NO set spec'd tab to cursOr position */
      gwrit = gcurs = true;
      CHECK_MACRO_END;

    case 16:                       /* ^NP - obey if Past spec'd tab */
      gpast = gcurs = true;
      TEST_TAB;

    case 18:                       /* ^NR - move cursoR to spec'd tab */
      gcurs = true;
      CHECK_MACRO_END;

    case 19:                 /* ^NS - unconditional Skip (reverse obey sense) */
      SKIP2MACCH;

    case 20:         /* ^NT - Trust user to change file pointer during modify */
      mctrst = true;
      GETNEXTCHR;

    case 21:
      goto p7625;

    case 23:              /* ^NW Whether macro exists or length (pushes to R) */
      CHECK_HAS_MACCH(1);
      gwthr = true;
      GET_FOLLOWING_CHAR;          /* Get macro ID */

    case 24:                       /* ^NX - eXit from macro */
      notmac(false);
      GETNEXTCHR;

    case 27:                       /* ^N[ - Obey if file before spec'd tab */
      TEST_TAB;                    /* !past !curs */

    case 29:                       /* ^N] - Obey if file past spec'd tab */
      gpast = true;
      TEST_TAB;
  }                                /* switch (thisch & 037) */
/* ^NH is not a pseudo */
/* ^NK is not a pseudo */
/* ^NQ is not a pseudo */
/* ^NV is not a pseudo */
/* ^NY is not a pseudo */
/* ^NZ is not a pseudo */
/* ^N\ is not a pseudo and never will be (it gets used to signal error) */
  if (curmac >= 0)
  {
    fprintf(stderr, "\r\nCalling undefined pseudo-macro \"%c\". ", thisch);
    notmac(true);
  }
  SOUNDALARM;

/* ^NB, ^NP, ^N[ & ^N] test if cursor / file position before/after tab */
test_tab:
  CHECK_HAS_MACCH(3);              /* Err if not tabid + 2 chars */
  gtest = true;                    /* We are a position tester */

/* ^NO,^NR,^NF,^NN - Remember or move to cursor or file position */
check_macro_end:
  CHECK_HAS_MACCH(1);             /* Error if was last character in the macro */
  gposn = true;                    /* Distinguish from ^G */
  GET_FOLLOWING_CHAR;              /* Get tab # */
/*
 * ^NU - Up from a macro s/r
 */
p7625:
/* Treat as exit if stack empty */
  if (mcnxfr == MCDTUM)
  {
    notmac(false);
    GETNEXTCHR;
  }                                /* if (mcnxfr == MCDTUM) */
/*
 * P1706 - I joins here
 */
p1706:
  mcnxfr--;                        /* Previous stack entry */
/*
 * Look for stack corruption
 */
  i = mcstck[mcnxfr].mcprev;       /* Macro # */
  j = mcstck[mcnxfr].mcposn;       /* Macro position */
  if (i < 0 || i > TOPMAC || !scmacs[i] || j > scmacs[i]->maclen || j < 0)
  {
    fprintf(stderr, "\r\nReturn macro ^<%o>out of range or empty. ", i);
    notmac(true);
    SOUNDALARM;
  }

/* If leaving an immediate macro, this entry is now free */
  if (curmac >= FIRST_IMMEDIATE_MACRO && curmac <= LAST_IMMEDIATE_MACRO)
    immnxfr = curmac;

  curmac = i;                      /* BRKPT resuming macro */
  mcposn = j;                      /* Accept the popped values */

/* If the frame we just freed was created by U-use, we must reinstate it. */
/* Only z-enduse (possibly implied by u-usefile EOF) can free this frame. */
/* Arrange that ^NI becomes a no-op, otherwise revert to the command source */
/* (which can't be a macro). */
  if (mcstck[mcnxfr].u_use)
  {
    mcnxfr++;
    if (verb == 'I')
    {
/* Restore macro that issued ^NI */
      curmac = h;
      mcposn = m;
    }
    else
      curmac = -1;
  }                                /* if (mcstck[mcnxfr+1].u_use) */
  else if (verb == 'I')
    goto p7604;
  GETNEXTCHR;
/*
 * ^ND - go Down a level ( a macro as a subroutine)
 */
p7604:
  CHECK_HAS_MACCH(2);              /* Error if < 2 chars left in macro */
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
    curmac = h;
    mcposn = m;
  }
  GETNEXTCHR;
/*
 * P1601 - Come here with ^NG char
 */
p1601:
  gpseu = false;
  if (fornj)
  {
    fornj = false;
    mcposn += thisch;              /* Do the jump */
    if (mcposn >= 0 && mcposn < scmacs[curmac]->maclen)
      GETNEXTCHR;
    err = "^NJ off macro end or start";
    ERR_IF_MAC;
  }
  if (gposn)
  {                                /* Tab code in thisch. 1->tab 1 ... */
    if (thisch == '-')
      i = 79;
    else
      i = thisch - '1';            /* Get TABS array subscript */
    if (i < 0 || i >= NUM_TABS)
      TABOORANGE;                  /* This jumps: gtest may be set */
    if (gtest)
    {
      err = NULL;
      if (tabs[i].tabtyp == UNDEFINED)
        err = "^NB &c. testing undefined tab";
      else
      {
        if (gcurs)
        {
          if (tabs[i].tabtyp == LINENUM)
            err = "^NB/^NP testing filepos tab";
        }                          /* if (gcurs) */
        else
        {
          if (tabs[i].tabtyp == CHRPOS)
            err = "^N[/^N] testing chrpos tab";
        }                          /* if (gcurs) else */
      }                            /* if (tabs[i].tabtyp == UNDEFINED) else */
      if (err)
        ERR_IF_MAC;
      if (gpast)
      {
        if (!gcurs)
        {
          if (ptrpos > tabs[i].value)
            GETNEXTCHR;
          SKIP2MACCH;
        }                          /* if (!gcurs) */
        if (Curr->bcurs > tabs[i].value)
          GETNEXTCHR;
        SKIP2MACCH;
      }                            /* if (gpast) */
      if (!gcurs)                  /* if (gpast) else */
      {
        if (ptrpos < tabs[i].value)
          GETNEXTCHR;
        SKIP2MACCH;
      }                            /* if (!gcurs) */
      if (Curr->bcurs < tabs[i].value)
        GETNEXTCHR;
      SKIP2MACCH;
    }                              /* if (gtest) */
    if (gwrit)                     /* Remember file or cursor position */
    {
      if (!gcurs)
      {
        tabs[i].value = ptrpos;
        tabs[i].tabtyp = LINENUM;
        GETNEXTCHR;
      }                            /* if (!gcurs) */
      tabs[i].value = Curr->bcurs; /* ^NO */
      tabs[i].tabtyp = CHRPOS;
      GETNEXTCHR;
    }                              /* if (gwrit) */
    i4 = tabs[i].value;
    if (i4 < 0)
      TABOORANGE;                  /* J certainly too low */
    if (!gcurs)
    {                              /* ^NN */
/* Trusted to change file pos'n while modifying? */
      if (!in_cmd && !mctrst)
        BOL_OR_EOL;
/* EOF? */
      if (i4 > lintot + 1 && !(deferd &&
        (dfread(i4 - lintot, NULL), i4 <= lintot + 1)))
        BOL_OR_EOL;
      setptr(i4);                  /* Position the file */
      GETNEXTCHR;                  /* Finish ^NN */
    }                              /* if (!gcurs) */
    if (i4 > Curr->bchars)
      TABOORANGE;                  /* J trying to pos'n off end of line */
    Curr->bcurs = i4;              /* Set cursor position */
    GETNEXTCHR;                    /* End ^NR - may want to REFRSH */
  }                                /* if (gposn) */

  if (gmacr)
  {
    if (Curr->bchars == 0)
      BOL_OR_EOL;                  /* Trying to define null macro */
    if ((thisch < 0200 && thisch > 077) ||
      (thisch > TOPMAC && thisch < 07000) ||
      (thisch > 07777 && thisch < 013000) || thisch > 13777)
    {
/* Implement ^NM for some definable active pseudos */

/* 4014 & 4015 (backtick) */
      if (thisch == 04014 || thisch == 04015)
      {
        Curr->bdata[Curr->bchars] = 0; /* NUL-terminate target */
        final5();
        qreg = cmd((char *)Curr->bdata, true);
        init5();
        if (qreg && thisch == 04014)
        {
          err = (char *)stderrbuf;
          ERR_IF_MAC;
        }                          /* if (qreg && thisch == 04014) */
        GETNEXTCHR;
      }                            /* if (thisch == 04014 || thisch == 04015) */

/* Error if we get here */
      err = "";
      fprintf(stderr, "\r\n^NM cannot define macro %o", thisch);
      newlin();
      ERR_IF_MAC;
    }                              /* if ((thisch < 0200 && ... )) */

/* Define the macro. Report if some problem... */
    if (macdef((int)thisch, Curr->bdata, (int)Curr->bchars, true))
      GETNEXTCHR;
    if (curmac >= 0)
      notmac(true);
    SOUNDALARM;                    /* Report failure */
  }                                /* if (gmacr) */

  if (gwthr)
  {
    qreg = -1;                     /* Default response: no such macro */
    if (thisch < 0)
      ;                            /* Can't be a macro */
    else if (thisch == 0 && curmac > 0)
      qreg = 0;                    /* ^N^@ legal in a macro */

/* Regular pseodomacros. Force thisch to upper case */
    else if (thisch >= FIRST_PSEUDO && thisch <= LAST_PSEUDO)
    {
      thisch = toupper(thisch);

/* Test for pseudo that is always legal (i.e.from keyboard) */
      for (i = 6; i >= 0; i--)     /* There are 7 of them */
      {
        if (thisch == "EORFNMW"[i])
        {
          qreg = 0;
          break;
        }                          /* if (thisch == "EORFNMW"[i]) */
      }                            /* for (i = 6; i >= 0; i--) */
/* Test for pseudos that are legal in macros */
      if (qreg && curmac > 0)
      {
        for (i = 14; i >= 0; i--)  /* 15 of them (from "h pm") */
        {
          if (thisch == "ABP[]CDUGSLJITX"[i])
          {
            qreg = 0;
            break;
          }                        /* if (thisch == "ABP[]CDUGSLJITX"[i]) */
        }                          /* for (i = 14; i >= 0; i--) */
      }                            /* if (qreg ...) */
    }            /* else if (thisch >= FIRST_PSEUDO && thisch <= LAST_PSEUDO) */

/* Test for a regular macro */
    else if (thisch <= TOPMAC)
    {
      if (scmacs[thisch])
        qreg = scmacs[thisch]->maclen;
    }                              /* else if (thisch <= TOPMAC) */

/* Test for an active pseudo: range from "h pm". */
/* Return lengths for some of them */
    else if (thisch >= 04000 && thisch <= 04015)
    {
      qreg = 0;
      switch (thisch)
      {
        case 04000:                /* Mode */
          qreg = snprintf(NULL, 0, "%lo", zmode_valid ? zmode : fmode);
          break;

        case 04002:                /* Curent edit file */
          qreg = strlen(pcnta);
          break;

        case 04003:                /* HELP dir (for macros usually) */
          qreg = strlen(macro_dir);
          break;

        case 04007:                /* Floating point format */
          qreg = snprintf(NULL, 0, "%s", FPformat);
          break;

        case 04010:                /* Date format */
          qreg = snprintf(NULL, 0, "%s", DTformat);
          break;

        case 04011:                /* Date */
        {
          time_t t = time(NULL);
          qreg = strftime(tbuf, sizeof tbuf, DTformat, localtime(&t));
        }
          break;

        case 04012:                /* UTC date */
        {
          time_t t = time(NULL);
          qreg = strftime(tbuf, sizeof tbuf, DTformat, gmtime(&t));
        }
          break;

        case 04013:                /* Integer format */
          qreg = snprintf(NULL, 0, "%s", Iformat);
          break;

        case 04014:                /* Backtick stdout */
          qreg = scmacs[STDOUT_MACRO_IDX]->maclen - 2; /* Strip ^NU */
          break;

        case 04015:                /* Backtick stderr */
          qreg = scmacs[STDERR_MACRO_IDX]->maclen - 2; /* Strip ^NU */
          break;

      }                            /* switch (thisch) */
    }                         /* else if (thisch >= 04000 && thisch <= 04015) */

/* ALU opcode? */
    else if (thisch >= FIRST_ALU_OP &&
      thisch < FIRST_ALU_OP + num_ops + NUM_TABS * 2)
      qreg = 0;

/* Other ALU pseudos */
    j = thisch & 017000;
    if (j == 07000)
      qreg = snprintf(NULL, 0, Iformat, ALU_memory[thisch & 0777]);
    else if (j == 05000 || j == 06000 || j == 011000 || j == 012000)
      qreg = 0;
    else if (j == 013000)
      qreg = snprintf(NULL, 0, FPformat, FPU_memory[thisch & 0777]);

    GETNEXTCHR;
  }                                /* if (gwthr) */

/* Must be ^NG if we get here */
  if (Curr->bcurs == Curr->bchars)
    SKIP2MACCH;                    /* Skip if at eol */
  if (Curr->bdata[Curr->bcurs] != thisch &&
    !(thisch == SPACE && isspace(Curr->bdata[Curr->bcurs]) && MATCH_ANY_WHSP))
    SKIP2MACCH;                    /* Skip if mismatch */
  GETNEXTCHR;
/*
 * leave_scrdit - Exit sequence. Reinstate XON if req'd...
 */
leave_scrdit:
  if (!(USING_FILE || nodup))
    duplx5(true);                  /* enable XOFF */
  return;
}
