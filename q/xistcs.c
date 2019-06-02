/* X I S T C S
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2017,2019 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 *
 * Set Terminal Characteristics:
 * A - Set ^A sequence and/or availability
 * B - Set backspace availability or character
 * D - Set or typeout per-character delay
 * R - Set C/R sequence and/or availability
 * X - Exit to main editor
 * T - Set number of spaces between tabstops in file
 * W - Turn vt100 mode back off again
 */
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include "prototypes.h"
#include "scrnedit.h"
#include "c1in.h"
#include "tabsiz.h"

/* Static prototypes */

static bool eolok(void);
static bool good_octnum(void);
static bool get_result(void);

/* Static variables */

static int octnum;                 /* Returned value */
static scrbuf5 cmdbuf;
static uint8_t buf[5];
static const char *msg;
static int result;                 /* Returned value */
static const char *const xistics_end_sequence = "\025x\n";

void
xistcs()
{
  uint8_t *p = NULL;
  int j, k = 0;                    /* Scratch */
  bool ok = true;

/* On entry, we are back to normal duplex. Don't read commands via
 * Screenedit system, as this is suspect until characteristics are sorted out */

/* [Re-]initialise static variables */
  result = 0;
  octnum = 0;

  end_seq = xistics_end_sequence;

  for (;;)
  {
    if (ok)
      msg = "Ok";
    else
    {
      fprintf(stderr, "%s\r\n", msg);
      msg = "ER";
      ok = true;
    }                              /* if (ok) else */
    printf("%s> ", msg);
    if (!cl5get((char *)cmdbuf.bdata, BUFMAX, true, true))
    {
      cmdbuf.bdata[0] = 'x';
      cmdbuf.bdata[1] = 0;
      puts("x");
    }                              /* if (!cl5get(...)) */
    cmdbuf.bchars = strlen((char *)cmdbuf.bdata);
    (void)scrdtk(5, 0, 0, &cmdbuf);
    (void)scrdtk(1, buf, 2, &cmdbuf);
    verb = buf[0];
    if (cmdbuf.toktyp != nortok || cmdbuf.toklen != 1)
    {
      msg = "Must have single-char command";
      ok = false;
      continue;
    }
    switch (verb)
    {
      case 'A':                    /* End - of - line */
        if (!get_result())
        {
          ok = false;
          continue;
        }
        if (result == 0)
        {
          if (!eolok())
          {
            ok = false;
            continue;
          }
        }                          /* if (result == 0) */
        else
        {
          p = cachrs;              /* Where characters go */
          j = result;              /* Result includes backspaces */
          if (j > PRMAX)
            j = PRMAX;
          for (k = j; k > 0; k--)
          {
/* Get an octal character to OCTNUM. Single characters stand for themselves,
 * so octal numbers must have at least 2 digits... */
            if (scrdtk(1, buf, 4, &cmdbuf))
            {
              fprintf(stderr, "%s. octno (scrdtk)\r\n", strerror(errno));
              {
                msg = "Error - see above";
                ok = false;
                break;
              }
            }
            if (cmdbuf.toktyp != eoltok)
            {
              if (cmdbuf.toktyp ^= nortok)
              {
                msg = "Null value not allowed";
                ok = false;
                break;
              }
              if (cmdbuf.toklen == 1)
                octnum = buf[0];
              else
              {
                if (!cmdbuf.octok)
                {
                  msg = "Bad octal #";
                  ok = false;
                  break;
                }
                octnum = cmdbuf.octval;
                if (!good_octnum())
                {
                  ok = false;
                  break;
                }
              }                    /* if (cmdbuf.toklen == 1) else */
            }                      /* if (cmdbuf.toktyp != eoltok) */
            if (cmdbuf.toktyp == eoltok)
              break;               /* J out no octal char (EOL) */
            *p++ = octnum;
          }                        /* for (k = j; k > 0; k--) */
          if (!ok || !eolok())
          {
            ok = false;
            continue;
          }
        }                          /* if (result == 0) else */
        cacnt = result;
        ok = true;
        continue;

      case 'B':                    /* Backspace */

/* It turns out that no code outside this module checks bspace any more */
/* so silently ignore attempts to change it. */
/* (bspace was a boolean saying whether this terminal can backspace) */

        (void)scrdtk(1, buf, 4, &cmdbuf);
        if (cmdbuf.toktyp != eoltok)
        {
          if (cmdbuf.toktyp != nortok)
          {
            msg = "Bad param";
            ok = false;
            continue;
          }                        /* if (cmdbuf.toktyp != nortok) */

/* If we have been given an octal character, set this as the
 * backspace character from now on */
          verb = buf[0];
          result = cmdbuf.octok;   /* In case we got an OCTNUM */
          octnum = cmdbuf.octval;  /* In case we got an OCTNUM */
          if (!eolok())
          {
            ok = false;
            continue;
          }
          if (result)
          {
            if (!good_octnum())
            {
              ok = false;
              continue;
            }
            backsp = octnum;
          }                        /* if (result) */
          else
          {
            switch (verb)
            {
              case 'Y':            /* Drop thru */
              case 'T':            /* Drop thru */
              case 'N':            /* Drop thru */
              case 'F':
                break;
              default:
                msg = "param not recognised";
                ok = false;
                continue;
            }                      /* switch (verb) */
          }                        /* if (result) else */
        }                          /* if (cmdbuf.toktyp != eoltok) */
        ok = true;
        continue;

      case 'D':                    /* Set or display per-character delay */
        puts("Delay is not currently working\r");
        ok = true;
        continue;

      case 'X':                    /* Back to main editor */
        if (!eolok())
        {
          ok = false;
          continue;
        }
        end_seq = normal_end_sequence;
        if (simulate_q)
          simulate_q_idx = 0;
        return;

      case 'T':                    /* Tab spacing in file */
        if (!get_result() || !eolok())
        {
          ok = false;
          continue;
        }
        tabsiz = result ? result : 8;
        ok = true;
        continue;

      case 'W':
        if (!eolok())
        {
          ok = false;
          continue;
        }
        vt100 = false;
        ok = true;
        continue;

      default:
        fprintf(stderr, "%c", verb);
        msg = " is not a recognised characteristic";
        ok = false;
        continue;
    }                              /* switch (verb) */
  }                                /* for(;;) */
}                                  /* main() */

/* ******************************* get_result ******************************* */

static bool
get_result(void)
{
  if (scrdtk(1, buf, 5, &cmdbuf))
  {
    fprintf(stderr, "%s. decno (scrdtk)\r\n", strerror(errno));
    msg = "Error - see above";
    return false;
  }                                /* if (scrdtk(1, buf, 5, &cmdbuf)) */
  if (cmdbuf.toktyp == eoltok)
  {
    result = 0;
    return true;                   /* Finish if EOL */
  }                                /* if (cmdbuf.toktyp == eoltok) */
  if (cmdbuf.toktyp != nortok)
  {
    msg = "Null decno illegal";
    return false;
  }                                /* if (cmdbuf.toktyp != nortok) */
  if (!cmdbuf.decok)
  {
    msg = "Bad decno";
    return false;
  }                                /* if (!cmdbuf.decok) */
  result = cmdbuf.decval;          /* All checks OK: set result */
  return true;
}                                  /* bool get_result(void) */

/* ******************************* good_octnum ****************************** */

static bool
good_octnum(void)
{
  if (octnum < 0200)
    return true;                   /* J a char */
  fprintf(stderr, "%*s", cmdbuf.toklen, buf);
  msg = " not octal for any char";
  return false;
}                                  /* bool good_octnum(void) */

/* ********************************** eolok ********************************* */

static bool
eolok(void)
{
  (void)scrdtk(1, 0, 0, &cmdbuf);
  if (cmdbuf.toktyp == eoltok)
    return true;                   /* J EOL (OK) */
  msg = "Spurious params - command not done";
  return false;
}                                  /* bool eolok(void) */
