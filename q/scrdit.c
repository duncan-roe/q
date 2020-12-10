/* S C R D I T */
/*
 * Copyright (C) 1981 D. C. Roe
 * Copyright (C) 2012-2014,2017-2020 Duncan Roe
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
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/times.h>
#include "prototypes.h"
#include "backtick.h"
#include "cmndcmmn.h"
#include "scrnedit.h"
#include "macros.h"
#include "fmode.h"
#include "tabs.h"
#include "c1in.h"
#include "alu.h"

/* Macros */

#define GETNEXTCHR continue
#define BOL_OR_EOL {err = "Ran off beginning or end of line"; ERR_IF_MAC;}
#define ERR_IF_MAC \
  {if (curmac >= 0){if (err) fprintf(stderr, "\r\n%s. ", err); err = NULL; \
  notmac(ERROR);}SOUNDALARM;}
#define SOUNDALARM \
  {mctrst = fornj = gpseu = glast = false; visbel(); GETNEXTCHR;}
#define TABOORANGE {if (curmac >= 0){fprintf(stderr, \
  "\r\nTab ID %c or value in that tab out of range. ", thisch); notmac(ERROR);}\
  SOUNDALARM;}
#define SKIP2MACCH {CHECK_HAS_MACCH(2); mcposn += 2; GETNEXTCHR;}
#define NORMALCHAR \
  {ordch(thisch, Curr); modlin = true; contp = false; GETNEXTCHR;}
#define RANOFF_END {err = "^NA, ^NB, ^NC &c. too near macro end"; ERR_IF_MAC;}
#define CHECK_HAS_MACCH(x) \
  {if (curmac >= 0 && mcposn > scmacs[curmac]->maclen - x) RANOFF_END;}
#define GET_FOLLOWING_CHAR {glast = gpseu = true; GETNEXTCHR;}
/* ^NB, ^NP, ^N[ & ^N] test if cursor / file position before/after tab */
#define TEST_TAB {\
  CHECK_HAS_MACCH(3);              /* Err if not tabid + 2 chars */ \
  gtest = true;                    /* We are a position tester */ \
  CHECK_MACRO_END;}
/* ^NO,^NR,^NF,^NN - Remember or move to cursor or file position */
#define CHECK_MACRO_END {CHECK_HAS_MACCH(1); \
  gposn = true;                   /* Distinguish from ^G */ GET_FOLLOWING_CHAR;}
#define LEAVE_SCRDIT {if (!(USING_FILE || nodup)) duplx5(true); return;}
#define ERR(x) {fputs(x "\r\n", stderr); verb = 'J'; return;}
#define LOG_BREAK do {if (log_fd && (fmode & FM_PLUS_8_BIT) && \
  (fmode & FM_PLUS_0_BIT)) fputc('\n', log_fd);} while (0)

/* Instantiate externals */

scrbuf5 *last_Curr = NULL;
double fbrief_interval = 0.1;
double timlst = 0;
uint8_t screen[SCRMAX], reqd[SCRMAX], prompt[PRSIZ], crsbuf[SCRMAX], backsp;
uint8_t cachrs[PRSIZ];
int cursr, scurs, pchars, crscnt, cacnt, tabcnt, cdone, partno, mxchrs;
bool insert, rfrsh, endlin;

/* Static prototypes */

static action macro_or_del_or_esc(scrbuf5 *Curr, bool in_cmd);
static action process_other(void);
static action process_pseudo(scrbuf5 *Curr, bool in_cmd);
static action process_pseudo_arg(scrbuf5 *Curr, bool in_cmd);
static action ctl_n_u_ctl_n_i_common(void);
static action ctl_n_d_ctl_n_i_common(void);
static action ctl_uparrow_ctl_g_common(scrbuf5 *Curr);
static bool pop_fp_register(double *val);
static bool pop_register(long *val);
static bool push_fp_register(double val);
static bool push_register(long val);
static bool get_effective_address(int addr);
static void print_failed_opcode(void);

/* Static variables (used by some static functions) */

static uint16_t thisch;            /* Character being processed */
static int i;                      /* Loops &c. */
static uint8_t *p;                 /* Loops &c. */
static int gotoch;                 /* Char of last ^G */
static char *err;                  /* Point to error text */
static bool fornj;                 /* Next char is offset for ^NJ (long jump) */
static bool gpseu;                 /* GLAST actually set for pseudo ^NG */
static bool glast;                 /* Last char input was ^G */
static int h, j, m;                /* Scratch variables */
static bool gposn;                 /* Next char is for a positioning pseudo */
static bool gtest;                /* This pseudo tests screen cursor position */
static bool gwrit;                 /* This pseudo remembers something */
static bool gcurs;                 /* This pseudo deals with screen cursor */
static bool gpast;       /* This pseudo tests screen cursor position past tab */
static bool gmacr;                 /* This pseudo is actually ^NM (not ^NG) */

/* WheTHeR macro exists or length (is ^NW (not ^NG)) */
static bool gwthr = false;

static char tbuf[256];             /* Scratch */
static bool contp;               /* true if last char ^P (nxt ch not special) */
static bool nseen;                 /* ^N last char so expect macro name */

/* ********************************* scrdit ********************************* */

void
scrdit(scrbuf5 *Curr, scrbuf5 *Prev, char *prmpt, int pchrs, bool in_cmd)
{
/* ARGUMENTS:-
 * ===========
 *
 * Curr  - Current buffer to modify \ these are scrnedit buffers
 * Prev  - Previous line modified   / as defined above
 * prmpt - Prompt characters (if any) (string)
 * pchrs - # of prompt chars (zero implies none and PRMPT not a valid
 *                                  addr)
 * in_cmd - => in command mode */

  uint8_t *q;                      /* Scratch */
  int k;                           /* Scratch */
  double t;                        /* Scratch */
  long olen;                       /* Original line length */
  uint8_t *indent_string = NULL;
  bool cntrlw = false;             /* true if ^W seen */

/* [Re-]initialise static variables */
  gotoch = -1;
  err = NULL;
  fornj = false;
  gpseu = false;
  glast = false;
  h = 0;
  m = 0;
  contp = false;
  nseen = false;

/* Validate cursor &c.
 * Curr->bchars counts the trailing null but Curr->bmxch does not */
  if ((olen = Curr->bchars) > Curr->bmxch + 1)
    ERR("Bad char cnt Curr line");
  if (Curr->bcurs < 0)             /* Bad cursor this line */
    ERR("Curr curs<1");
  if (pchrs > PRMAX)               /* Prompt too big */
    ERR("Prompt>15 chars");
  if (Curr->bcurs > Curr->bchars)
    ERR("Curr curs off end of info");

/* Switch off XOFF if input from tty */
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

/* Initialise some external variables */
  insert = modlin = endlin = false;
  pchars = pchrs;
  mxchrs = Curr->bchars;
  last_Curr = Curr;
  if (pchars)
    strcpy((char *)prompt, (char *)prmpt);

/* Normally we do no REFRSH if in an scmac, but if BRIEF is on
 * we here refresh the prompt only (if editing a line)
 *  Speedup: so we don't bank up on TTY o/p (& CPU!), only display
 * new line # every fbrief_interval seconds ... */
  if (curmac >= 0 && !in_cmd && BRIEF && !NONE && pchars)
  {
    if ((t = time_now()) - timlst >= fbrief_interval)
    {
      timlst = t;                  /* Displaying */
      sdsply();                    /* Display the line number */
    }                    /* if ((t = time_now()) - timlst >= fbrief_interval) */
  }                /* if (curmac >= 0 && !in_cmd && BRIEF && !NONE && pchars) */

/* If this is a new line and INDENT is on, pad out with appropriate
 * number of spaces. If not a new line and INDENT on, get new
 * indent position. */
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
  for (;;)
  {
    if (curmac < 0)
    {
      bool eof_encountered;

      refrsh(Curr);                /* Display prompt etc */
      thisch = c1in5(&eof_encountered); /* Read 1 char */
      LOG(thisch);
      if (eof_encountered)
      {                            /* c1in5 will have returned '\r' */
        if (in_cmd && Curr->bchars == 0 && USING_FILE)
        {
          Curr->bdata[0] = 'z';
          Curr->bchars = 1;
        }                          /* if (in_cmd && ...) */
        else
          pop_stdin();
      }                            /* if (eof_encountered) */

/* FX command implementation - replace char read by one from the
 * table unless we think we are cominputting or by some strange
 * chance the char is parity-high. */
      if (!USING_FILE && !(thisch & 0200))
        thisch = fxtabl[thisch];
    }                              /* if (curmac < 0) */
    else
    {
/* Clear expanding if that was last char from macro */
      if (mcposn >= scmacs[curmac]->maclen)
      {
        notmac(NORMAL);
        GETNEXTCHR;
      }                            /* if (mcposn >= scmacs[curmac]->maclen) */
      thisch = scmacs[curmac]->data[mcposn++];
      LOG(thisch);
    }                              /* if (curmac < 0) else */
    if (contp)
    {
      if (cntrlw)
      {
        thisch |= 0200;            /* Set parity following ^W */
        cntrlw = false;
      }                            /* if (cntrlw) */
      if (nseen)
      {
        if (macro_or_del_or_esc(Curr, in_cmd) == RETURN)
          LEAVE_SCRDIT;
        GETNEXTCHR;
      }                            /* if (nseen) */
      NORMALCHAR;                  /* Treat as ordinary character */
    }                              /* if (contp) */
    if (nseen)
    {
/* We have a putative macro char. May be ^P or ^W,
 * in which case we want the character following... */
      if (thisch == '\20' || thisch == '\3') /* ^P or (if using alt INTR) ^C */
      {
        contp = true;
        GETNEXTCHR;
      }
      if (thisch == '\27')
      {
        cntrlw = contp = true;
        GETNEXTCHR;
      }                            /* if (thisch == '\27') */

      macro_or_del_or_esc(Curr, in_cmd);
      GETNEXTCHR;
    }                              /* if (nseen) */
    if (glast)                     /* In middle of ^G */
    {
      glast = false;
      if (gpseu)
        process_pseudo_arg(Curr, in_cmd); /* Char is for pseudomac */
      else
        ctl_uparrow_ctl_g_common(Curr);
      GETNEXTCHR;                  /* Finish ^G  / ^^ */
    }                              /* if (glast) */
    if (thisch == DEL)             /* Delete char before cursor */
    {
      if (!Curr->bcurs)
        BOL_OR_EOL;                /* Error if at line start */

/* If at indent point,decrement INDENT */
      if (INDENT && Curr->bcurs == ndntch)
        ndntch--;

/* Move cursor back 1 and become ^D */
      Curr->bcurs--;
      thisch = 4;
    }                              /* if (thisch == 0177) */
    if (thisch >= SPACE)
      NORMALCHAR;                  /* J not a control */
    verb = thisch + 0100;          /* 'J' for ^J, &c. */
    switch (verb)
    {
      case '@':                    /* NUL - (ignored) */
        GETNEXTCHR;

      case 'A':                    /* Again/Append */

/* If more info in previous line than current, append previous excess
 * to current, then in either case move cursor to follow last char */
        i = ndntch;                /* In case we SINDNT */
        if (!lstvld)
          sindnt();                /* Get previous line valid */
        ndntch = i;
        if (Curr->bchars < Prev->bchars)
        {

/* We have to move chars from the previous line */
          modlin = true;           /* Line has been changed */
          memcpy(&Curr->bdata[Curr->bchars], &Prev->bdata[Curr->bchars],
            Prev->bchars - Curr->bchars);
          Curr->bchars = Prev->bchars; /* Extend length of line */
          if (mxchrs < Curr->bchars)
            mxchrs = Curr->bchars;
        }                          /* if (Curr->bchars < Prev->bchars) */
        Curr->bcurs = Curr->bchars; /* Put cursor at end */
        GETNEXTCHR;                /* Finish ^A */

      case 'B':                    /* word Back */

/* Go back to the start of this word or back to the start of the
 * previous word if at the start of this one. Words are delimited by any
 * white space. Error if no backwards movement possible. */
        i = INDENT ? ndntch : 0;   /* S.O.L. if no indenting */
        k = Curr->bcurs - i;       /* k - running available movement */
/* Note k can be -ve, after ^NR to move before the indent point */
        if (k <= 0)
          BOL_OR_EOL;              /* J start of line (error) */
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
        Curr->bcurs = i + k;       /* Start of line + room */
        GETNEXTCHR;                /* Finish ^B */

      case 'D':                    /* Delete char under cursor */
        if (Curr->bcurs == Curr->bchars)
        {
          err = "^D at EOL";
          ERR_IF_MAC;
        }
        k = Curr->bcurs++;         /* Get past char to erase */
        modlin = true;             /* Line has been changed */
        j = Curr->bchars - 1;      /* How many chars there will be */
        if (j != k)
          memmove(Curr->bdata + k, Curr->bdata + k + 1, j - k);
        Curr->bcurs = k;           /* Step cursor back */
        Curr->bchars = j;          /* Reduce # of chars */
        GETNEXTCHR;

      case 'E':                    /* enter/leave insErt mode */
        insert = !insert;
        GETNEXTCHR;                /* End insert */

      case 'F':                    /* Forward to start next word */
        k = Curr->bchars - Curr->bcurs; /* k - how far we can move forward */
/* If on non-space, move forward to space, unless at EOL now... */
        p = &Curr->bdata[Curr->bcurs];
/* Ensure we're pointing at a space (or we were). Always reduces k,
 * unless at EOL to start with... */
        if (k < 0)
        {
          fprintf(stderr, "Internal error: k=%d\r\n", k);
          notmac(ERROR);
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

      case 'G':                    /* Goto next character */
        glast = true;
        gpseu = false;             /* Not in pseudo macro */
        GETNEXTCHR;                /* Will be back shortly */

      case 'H':                    /* Home */
        Curr->bcurs = INDENT ? ndntch : 0; /* Reset cursor */
        GETNEXTCHR;

      case 'I':                    /* (tab key) Insert a tab */

/* Insert enough spaces to get to next tab posn if there is one, otherwise
 * insert 1 space. Once inserted, there is no remembrance that this was a tab -
 * spaces may be individually deleted etc. */
        modlin = true;             /* Line has been changed */
        for (i = 0; i < tabcnt; i++) /* Find 1st tab past where we are */
          if (Curr->bcurs < tabs[i].value && tabs[i].tabtyp == CHRPOS)
            break;                 /* Tab found */
        if (i == tabcnt)           /* Force 1 space if drop thro' */
          j = 1;                   /* Insert 1 space */
        else
          j = tabs[i].value - Curr->bcurs; /* # spaces */

/* See how many spaces we actually have room for */
        i = Curr->bchars;
        h = Curr->bmxch - i;       /* # there is room for */
        if (h > 0)                 /* Some room */
        {
          p = &Curr->bdata[i];     /* 1 past last char to pick up */
          m = i - Curr->bcurs;     /* # chars to move up */
          if (h > j)
            h = j;                 /* # spaces to insert */
          i = i + h;               /* New length */
          q = &Curr->bdata[i];     /* 1 past last char to set down */
/* Do the right-hand overlapping move */
          for (; m > 0; m--)
            *--q = *--p;
/* Move in spaces */
          p = &Curr->bdata[Curr->bcurs];
          for (m = h; m > 0; m--)
            *p++ = SPACE;

/* Adjust buffer and other variables ... */
          if (INDENT && Curr->bcurs == ndntch) /* Indenting & at indent point */
            ndntch = ndntch + h;
          Curr->bcurs = Curr->bcurs + h; /* New cursor */
          Curr->bchars = i;
          if (mxchrs < i)
            mxchrs = i;
          if (j == h)              /* Tab inserted successfully */
            GETNEXTCHR;
        }                          /* if (h > 0) */
        err = "Line full";
        ERR_IF_MAC;

      case 'M':                    /* end of line */
        verb = 'J';                /* Treat as Nl: drop thru to ^J */

      case 'J':                    /* end of line: drop thru to ^T */

      case 'T':                    /* spliT line */
        k = Curr->bchars;         /* Belongs after case 'J' but gcc complains */
        if (verb == 'T')           /* Not a drop-thru */
        {
          k = Curr->bcurs;
          modlin = modlin || (k < Curr->bchars); /* modlin unless ^T at EOL */
        }                          /* if (verb == 'T') */

/* Disallow length change in FIXED LENGTH mode, when editing the file */
        if (Curr->bchars != olen && !in_cmd && fmode & FM_PLUS_F_BIT)
        {
          fprintf(stderr,
            "\r\nLine length must not change in FIXED LENGTH mode");
          visbel();
          newlin();
          GETNEXTCHR;
        }

/* Copy 1st part (^T) or all (^J) of line to previous buffer */
        memcpy(Prev->bdata, Curr->bdata, k);
        Prev->bchars = k;
        Prev->bdata[k] = '\0';     /* Null terminate */
        Prev->bcurs = 0;
        lstvld = true;             /* Last line now valid */

        if (verb == 'J')
          LEAVE_SCRDIT;            /* Finish if was n/l else drop thru to ^L */

      case 'L':                    /* Left hand kill */

/* If indenting and at the indent point kill the indent, otherwise
 * kill back to the indent point only.
 * ^T obeys this code with some modification. */

/* Move down the rest of the line then return immediately if ^T.
 * Caller [q] must check verb for what to do vis the workfile */
        k = Curr->bcurs;
        if (k)
        {
          p = &Curr->bdata[k];     /* 1st char to be moved down */
          if (verb == 'L')
            modlin = true;         /* ^T makes its own decision */
          j = 0;                   /* Eventual cursor pos if not indenting */
          if (INDENT)
          {
            if (verb == 'L' && k == ndntch)
              ndntch = 0;          /* Kill indent if at indenting point */
            k = k - ndntch;
            j = ndntch;
          }                        /* if (INDENT) */
          h = Curr->bchars - k;    /* Set new line length */
          Curr->bchars = h;
          Curr->bcurs = j;
          q = &Curr->bdata[j];     /* Dest'n 1st movedown char */
          if (h != j)
          {
/* Do overlapping left hand move */
            for (m = h - j; m > 0; m--)
              *q++ = *p++;
          }                        /* if (h != j) */
        }                          /* if (k) */
        if (verb == 'T')
          LEAVE_SCRDIT;            /* Return if end of ^T */
        GETNEXTCHR;                /* Finish ^L */

      case 'K':                    /* Kill */
        Curr->bchars = Curr->bcurs;
        modlin = true;             /* Line has been changed */
        GETNEXTCHR;                /* Finished ^K */

      case 'N':                    /* expaNd macro */
        nseen = true;
        GETNEXTCHR;

      case 'O':                    /* cOmment modify */
        k = Curr->bcurs;           /* Cursor pos'n */
        p = Curr->bdata + k;       /* 1st char to check */
        i = Curr->bchars - k - 1;  /* Chars to check for "/" */
        for (; i > 0; i--, k++, p++)
          if (*p == '/' && *(p + 1) == '*')
            break;                 /* Found start of comment */
        if (i > 0)
        {
          k++;                     /* Point to '*' */
          do
          {
            k++;
            if (k >= Curr->bchars)
              break;
          }
          while (Curr->bdata[k] == SPACE);
          if (k < Curr->bchars)
          {
            Curr->bcurs = k;       /* Cursor to 1st comment char */
            GETNEXTCHR;            /* Finish ^O */
          }                        /* if (k < Curr->bchars) */
        }                          /* if (i > 0) */
        Curr->bcurs = Curr->bchars; /* Set cursor to E.O.L. */
        GETNEXTCHR;

      case 'C':                    /* drop thru */

      case 'P':                    /* next char not sPecial */
        contp = true;
        GETNEXTCHR;

      case 'S':                    /* drop thru: ^S - Lower -> upper */

      case 'Q':                    /* rest of line upper -> lower */
      {
        int (*cvt) (int);

        if (verb == 'S')
          cvt = toupper;
        else
          cvt = tolower;
        k = Curr->bchars;
        j = Curr->bcurs;
        p = &Curr->bdata[j];
        k = k - j;                 /* # to do */
        modlin = k > 0;            /* Line changed if any chars to do */
        for (; k > 0; k--, p++)
          *p = cvt(*p);
        GETNEXTCHR;                /* Finished ^Q */
      }

      case 'R':                    /* Reveal all of line */

/* Fudge in case recovering from accidental ESC - if no chars typed
 * reveal max possible BUF chars. BUT,,, don't reveal trailing null
 * characters, which have never been used... */
        i = INDENT ? ndntch : 0;
        if (mxchrs == i)
        {
          for (k = Curr->bmxch - 1; k >= i; k--)
            if (Curr->bdata[k])
              break;
          mxchrs = k + 1;
        }
        if (Curr->bchars != mxchrs)
          modlin = true;           /* If characters rescued */
        Curr->bchars = mxchrs;
        Curr->bcurs = mxchrs;
        GETNEXTCHR;                /* Finish ^R */

      case 'U':                    /* Undo (cancel) */
        i = INDENT ? ndntch : 0;
        if (Curr->bchars != i)
          modlin = true;           /* Cancel empty line not a mod */
        Curr->bchars = Curr->bcurs = i;
        GETNEXTCHR;                /* Finish ^U */

      case 'V':                   /* re-display line to recoVer from whatever */
        newlin();
        GETNEXTCHR;                /* Finish ^V */

      case 'W':                    /* next char With parity bit on */
        cntrlw = true;
        contp = true;
        GETNEXTCHR;

      case 'X':                    /* move cursor 1 forward */
/* Append space if at EOL */
        if (Curr->bcurs != Curr->bchars)
          Curr->bcurs++;           /* Move cursor fwd */
        else
        {
          ordch(SPACE, Curr);
          modlin = true;           /* Line has been changed */
        }                          /* If (curr->bcurs != curr->bchars) else */
        GETNEXTCHR;                /* Finish ^X */

      case 'Y':                    /* cursor back 1 */
        k = Curr->bcurs;           /* Not allowed into indent area however */
        i = INDENT ? ndntch : 0;
        if (k == i)                /* At start or indent already */
          BOL_OR_EOL;
        Curr->bcurs--;
        GETNEXTCHR;                /* Finish ^Y */

      case 'Z':                    /* go to end current line */
        Curr->bcurs = Curr->bchars;
        GETNEXTCHR;                /* Finish ^Z */

      case '[':                    /* ESC - Abandon */
        LEAVE_SCRDIT;              /* Simply exit */

      case '^':                    /* repeat ^G */
        if (gotoch == -1)          /* No Prev ^G */
        {
          err = "^^ no previous ^G";
          ERR_IF_MAC;
        }
        thisch = gotoch;
        ctl_uparrow_ctl_g_common(Curr);
        GETNEXTCHR;
    }                              /* switch (thisch) */
    NORMALCHAR;
  }                                /* for (;;) */
}                                  /* scrdit() */

/* *************************** print_failed_opcode ************************** */

static void
print_failed_opcode(void)
{
  char *opcd = opcode_defs[alu_table_index[thisch - FIRST_ALU_OP]].name;

  fprintf(stderr, "\r\nFailing opcode: ");
  while (*opcd)
    putc(toupper(*(uint8_t *)opcd++), stdout);
}                                  /* print_failed_opcode() */

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
      notmac(ERROR);
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
    notmac(ERROR);
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
    notmac(ERROR);
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
    notmac(ERROR);
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
    notmac(ERROR);
    return false;
  }                                /* if (fsidx < 0) */
  *val = fs[fsidx--];
  return true;
}                                  /* pop_fp_register() */

/* ************************************************************************** */
/* ************************************************************************** */
/*                                                                            */
/*          RE-DEFINE MACROS USED BY FOLLOWING STATIC FUNCTIONS               */
/*                                                                            */
/* ************************************************************************** */
/* ************************************************************************** */

#undef GETNEXTCHR
#define GETNEXTCHR return CONTINUE
#undef LEAVE_SCRDIT
#define LEAVE_SCRDIT return RETURN;

/* ************************ ctl_uparrow_ctl_g_common ************************ */

static action
ctl_uparrow_ctl_g_common(scrbuf5 *Curr)
{
/* Search the rest of the line AFTER the current character */
  i = Curr->bchars - Curr->bcurs - 1; /* # chars to search */
  p = &Curr->bdata[Curr->bcurs];   /* Point to cursor char */
  for (; i > 0; i--)
  {
    p++;
    if (*p == thisch || (thisch == SPACE && isspace(*p) && MATCH_ANY_WHSP))
      break;
  }                                /* for (; i > 0; i--) */
  if (i >= 0)
    Curr->bcurs = Curr->bchars - i; /* Guard against -ve i */
  gotoch = thisch;                 /* Remember char for ^^ */
  GETNEXTCHR;                      /* Finish ^G */
}                                  /* ctl_uparrow_ctl_g_common() */

/* ************************* ctl_n_d_ctl_n_i_common ************************* */

static action
ctl_n_d_ctl_n_i_common(void)
{
  CHECK_HAS_MACCH(2);              /* Error if < 2 chars left in macro */
  if (mcnxfr == MCLMIT)
  {
    err = "Macro stack depth limit exceeded";
    ERR_IF_MAC;
  }
  mcstck[mcnxfr].mcprev = curmac;

/* Return addr after following macro */
  mcstck[mcnxfr].mcposn = mcposn + 2;
  mcstck[mcnxfr].u_use = false;
  mcnxfr++;                        /* Up stack pointer */
  return BREAK;                    /* ^NI has more to do */
}                               /* static action ctl_n_d_ctl_n_i_common(void) */

/* ************************* ctl_n_u_ctl_n_i_common ************************* */

static action
ctl_n_u_ctl_n_i_common(void)
{
  mcnxfr--;                        /* Previous stack entry */

/* Look for stack corruption */
  i = mcstck[mcnxfr].mcprev;       /* Macro # */
  j = mcstck[mcnxfr].mcposn;       /* Macro position */
  if (i < 0 || i > TOPMAC || !scmacs[i] || j > scmacs[i]->maclen || j < 0)
  {
    fprintf(stderr, "\r\nReturn macro ^<%o>out of range or empty. ", i);
    notmac(ERROR);
    SOUNDALARM;
  }

/* If leaving an immediate macro, this entry is now free */
  if (curmac >= FIRST_IMMEDIATE_MACRO && curmac <= LAST_IMMEDIATE_MACRO)
    immnxfr = curmac;

  if (verb != 'I' && (curmac < FIRST_PSEUDO || curmac > LAST_PSEUDO))
    LOG_BREAK;
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
  {
    if (ctl_n_d_ctl_n_i_common() == CONTINUE)
      GETNEXTCHR;

/* Restore next macro in stack */
    curmac = h;
    mcposn = m;
  }                                /* else if (verb == 'I') */
  GETNEXTCHR;
}                                  /* ctl_n_u_ctl_n_i_common(void) */

/* *************************** process_pseudo_arg *************************** */

static action
process_pseudo_arg(scrbuf5 *Curr, bool in_cmd)
{
  long i4;                         /* Scratch */

  gpseu = false;
  if (fornj)
  {
    fornj = false;
    mcposn += (int16_t)thisch;    /* Do the (signed!) jump BRKPT jumping back */
    if (cntrlc)                    /* Guard against looping backwards jump */
    {
      if (!USING_FILE)
        cntrlc = false;
      err = "Interrupt";
      ERR_IF_MAC;
    }                              /* if (cntrlc) */
    if ((int16_t)thisch < 0)
      LOG_BREAK;
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
      notmac(ERROR);
    SOUNDALARM;                    /* Report failure */
  }                                /* if (gmacr) */

  if (gwthr)
  {
    qreg = -1;                     /* Default response: no such macro */
    if (thisch < 0)
      ;                            /* Can't be a macro */
    else if (thisch == 0 && curmac >= 0)
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
      if (qreg && curmac >= 0)
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
          qreg = snprintf(NULL, 0, "%" PRIofmode, FMODE);
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
}                                  /* process_pseudo_arg() */

/* ***************************** process_pseudo ***************************** */

static action
process_pseudo(scrbuf5 *Curr, bool in_cmd)
{
  char *c;                         /* Scratch */

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
  {                                /* Try for a pseudo u/c or l/c */
    case 1:                        /* ^NA - Obey if at EOL */
      CHECK_HAS_MACCH(2);
      if (Curr->bcurs == Curr->bchars)
        GETNEXTCHR;                /* J at EOL */
      SKIP2MACCH;

    case 2:                        /* ^NB - obey if Before spec'd tab */
      gcurs = true;
      TEST_TAB;

    case 3:                        /* ^NC - obey if in Command processor */
      CHECK_HAS_MACCH(2);
      if (!in_cmd)
        SKIP2MACCH;
      GETNEXTCHR;

    case 4:                    /* ^ND - go Down a level (macro as subroutine) */
      ctl_n_d_ctl_n_i_common();
      GETNEXTCHR;

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
      GET_FOLLOWING_CHAR;

    case 9:             /* ^NI - Increment link (to make macro a conditional) */
/* Error if < 2 chars left in macro, because it can't then do a ^NU */
      if (mcposn > scmacs[curmac]->maclen - 2)
        RANOFF_END;
      if (mcnxfr == mcdtum)
        GETNEXTCHR;                /* No-op if stack empty */
      h = curmac;                  /* Save current macro BRKPT ^NI invoked */
      m = mcposn;                  /* Save current macro position */
      ctl_n_u_ctl_n_i_common();    /* Do a dummy UP */
      GETNEXTCHR;

    case 10:                       /* ^NJ long (signed) Jump */
      CHECK_HAS_MACCH(1);          /* Must be 1 more char in macro */
      fornj = true;                /* Indicate ^N^J */
      GET_FOLLOWING_CHAR;          /* Get jump length */

    case 12:                       /* ^NL - Long skip */
      mcposn += 2;                 /* Skip 2 ... */
      SKIP2MACCH;                  /* ... skip 2 more */

    case 13:                       /* ^NM - make Macro from current line */
      if (curmac >= 0)
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

    case 21:                       /* ^NU - Up from a macro s/r */
      if (mcnxfr == mcdtum)
        notmac(NORMAL);            /* Treat as exit if stack empty */
      else
        ctl_n_u_ctl_n_i_common();
      GETNEXTCHR;

    case 23:              /* ^NW Whether macro exists or length (pushes to R) */
      CHECK_HAS_MACCH(1);
      gwthr = true;
      GET_FOLLOWING_CHAR;          /* Get macro ID */

    case 24:                       /* ^NX - eXit from macro */
      notmac(NORMAL);
      GETNEXTCHR;

    case 27:                       /* ^N[ - Obey if file before spec'd tab */
      TEST_TAB;                    /* !past !curs */

    case 29:                       /* ^N] - Obey if file past spec'd tab */
      gpast = true;
      TEST_TAB;

    default:
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
        notmac(ERROR);
      }
      SOUNDALARM;
  }                                /* switch (thisch & 037) */
}                                  /* process_pseudo() */

/* ****************************** process_other ***************************** */

static action
process_other(void)
{
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
        i = snprintf(tbuf, sizeof tbuf, "%" PRIofmode, FMODE);
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
        if ((fmode & FM_PLUS_H_BIT) && i > row5 / 2)
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

/* If there is no current file, ensure macro will not return. This since
 * the null argument might not be noticed in a macro, and the commnad's
 * next argument (if any) taken as a filename (numbers being legal
 * filenames) */
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
        if ((fmode & FM_PLUS_H_BIT) && j > row5 / 2)
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

/* Deal with ALU memory and tab access pseudos */
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
      }                            /* if (is_pop) */
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
            print_failed_opcode();
            err = "ALU skip off macro end";
            ERR_IF_MAC;
          }                        /* if (mcposn > scmacs[curmac]->maclen) */
        }                          /* if (alu_skip) */
        GETNEXTCHR;
      }                            /* if (exec_alu_opcode(thisch)) */
      else
      {
        print_failed_opcode();
        fprintf(stderr, "\r\nRegister dump:-\r\n");
        dump_registers(false);
        ERR_IF_MAC;                /* It always will be in a macro */
      }                            /* if (exec_alu_opcode(thisch)) else */
    }       /* if (thisch >= FIRST_ALU_OP && thisch < FIRST_ALU_OP + num_ops) */
  }                                /* if (thisch > TOPMAC) */

  LOG_BREAK;

/* ^N^<000> restarts current macro */
  if (!thisch && curmac >= 0)      /* BRKPT starting macro */
    mcposn = 0;                    /* Leave curmac as-is */
  else
  {
/* Signal error if null or bad macro */
    if (thisch > TOPMAC || !scmacs[thisch])
    {
      if (curmac >= 0)
      {
        fprintf(stderr, "\r\nCalling undefined macro ^<%03o>. ", thisch);
        notmac(ERROR);
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
        }                          /* if (scmacs[i].mcprev == curmac) */
      if (!found)
        immnxfr = curmac;
    }                              /* if (curmac >= FIRST_IMMEDIATE_MACRO ... */
    curmac = thisch;
  }                                /* if (!thisch && curmac >= 0) else */
  GETNEXTCHR;
}                                  /* static action process_other(void) */

/* *************************** macro_or_del_or_esc ************************** */

static action
macro_or_del_or_esc(scrbuf5 *Curr, bool in_cmd)
{
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

  if (thisch <= LAST_PSEUDO && thisch >= FIRST_PSEUDO)
    process_pseudo(Curr, in_cmd);
  else
    process_other();
  GETNEXTCHR;
}                                  /* macro_or_del_or_esc() */
