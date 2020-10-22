/* N E W M A C
 *
 * Copyright (C) 1981,1999,2011, D. C. Roe
 * Copyright (C) 2012,2014,2019-2020 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * This routine stores a new SCREENEDIT macro definition as read by
 * the mainline
 */
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "prototypes.h"
#include "backtick.h"
#include "edmast.h"
#include "macros.h"
#include "fmode.h"
#include "tabs.h"
#include "c1in.h"
#include "alu.h"

/* Macros */
#define GIVE_UP return 0

/* Instantiate externals */
long ALU_memory[01000] = { 0 };
double FPU_memory[01000] = { 0 };

/* Static Variables */

static uint8_t alubuf[64];
static scrbuf5 aluscrbuf;

int
newmac()
{
  int mcchrs;                      /* Chars in macro def'n */
  uint8_t p[14];
/*
 * Q calls us when it sees the NEWMACRO command. We read the next
 * token off the command line - if it is "-" then return a -ve value
 * instructing Q to display / write to a file the macro  expansions.
 * Otherwise, validate the macro ID & read the expansion,  which may not
 * be null. Return 0 if any error occurs, having first o/p an
 * explanatory message. If all OK, return +ve.
 */
/* Get macro name or minus */
  if (scrdtk(1, p, 13, oldcom))
  {
    fprintf(stderr, "%s. reading macro name (scrdtk)", strerror(errno));
    GIVE_UP;
  }
/* A single-character macro name stands for itself (B defines the ^B
 * macro, for instance). A 2-character name must be valid octal, and
 * like the 1-character name is followed by a quoted macro definition.
 * 3 or more characters must be valid octal, but the macro definition is
 * read as raw text.
 * Any macro may be defined by any available method, but the pseudo
 * macros may not be defined. These occupy macros 0100 - 0177 on a
 * standard-Ascii system */
  if (oldcom->toklen == 1)
  {
    verb = p[0];                   /* Get the character typed */
    if (verb >= 'A')
    {
      if (verb <= '_')             /* In range A - _ */
        verb -= 0100;              /* Convert to a control */
      else
      {
        fprintf(stderr, "illegal macro name \"%c\"", (char)verb);
        GIVE_UP;
      }                            /* verb <= '_' else */
    }                              /* if (verb >= 'A') */
    else
/*
 * Look for non control char macro (63-macro enhancement) Anything
 * except an actual control is ok.
 * If we have a "-", return a -ve value so Q can list / type the
 * currently defined macros...
 */
    {
      if (verb == '-')
        return -1;                 /* R macro expansions wanted */
      if (verb == '@')
        verb = '-';                /* Fudge to define "-" macro */
      if (verb < SPACE)
      {                            /* An actual control - illegal */
        fprintf(stderr, "illegal macro name \"^%c\"", (char)(verb + 0100));
        GIVE_UP;
      }
    }                              /* if (verb >= 'A') else */
  }                                /* if (oldcom->toklen == 1) */
  else
  {
/* Look for N--. This is a request to type / list only the alu memory locns */
    if (oldcom->toklen == 2 && !strcmp((char *)p, "--"))
    {
      alu_macros_only = true;
      return -1;
    }                 /* if (oldcom->toklen == 2 && !strcmp((char *)p, "--")) */

    if (!(oldcom->octok))          /* Will pick up zero-length name */
    {
      fprintf(stderr, "macro name \"%s\" neither octal nor single-char", p);
      GIVE_UP;
    }                              /* J not octal after all */
    verb = oldcom->octval;
  }                                /* if (oldcom->toklen == 1) else */
/*
 * Get the macro definition.
 * This is quoted text unless macro name specified by octal >=3 digits
 */
  if (oldcom->toklen <= 2)
  {

/* Old-style macro def'n - macro text is quoted */
    if (scrdtk(2, (uint8_t *)ubuf, BUFMAX, oldcom))
    {
      fprintf(stderr, "%s. reading macro text (scrdtk)", strerror(errno));
      GIVE_UP;
    }                              /* Get text of macro expansion */
    mcchrs = oldcom->toklen;       /* Remember length of expansion */

/* Check no more tokens */
    scrdtk(1, NULL, 0, oldcom);
    if (oldcom->toktyp != eoltok)
    {
      fprintf(stderr, "Too many arguments for this command");
      GIVE_UP;
    }
  }                                /* if (oldcom->toklen <= 2) */
  else
  {
/* Check for being in pseudomacro region or out of range */
    if (
      (!verb || verb > TOPMAC || (verb >= FIRST_PSEUDO && verb <= LAST_PSEUDO))
      && (verb & 07000) != 07000 && verb != 04007 && (verb & 013000) != 013000
      && verb != 04010 && !(verb >= 04013 && verb <= 04015))
    {
      fprintf(stderr, "Macro %o is reserved or out of range", (int)verb);
      GIVE_UP;
    }
    if (scrdtk(4, (uint8_t *)ubuf, BUFMAX, oldcom))
    {
      fprintf(stderr, "%s. reading macro text (scrdtk)", strerror(errno));
      GIVE_UP;
    }                              /* R bad token */
    mcchrs = oldcom->toklen;       /* Remember length of expansion */
  }                                /* if (oldcom->toklen <= 2) else */
/*
 * Ensure non-null def'n
 */
  if (!mcchrs)
  {
    fprintf(stderr, "null macro specified");
    GIVE_UP;
  }

/* Setting an ALU or FPU memory location? */
  if ((verb & 07000) == 07000 || (verb & 013000) == 013000)
  {
    char *endptr;
    int idx = verb & 0777;
    long oldval = ALU_memory[idx];
    double oldfpval = FPU_memory[idx];
    char *strbuf = ubuf;
    unsigned long ulong_result;    /* Guard against ULONG_MAX on error */
    long long_result;              /* Guard against LONG_MAX on error */

/* Parse out token from supplied buffer to allow slash star comments */
    scrdtk(5, NULL, 0, &aluscrbuf);
    aluscrbuf.bchars = snprintf((char *)aluscrbuf.bdata, BUFMAX, "%s", ubuf);
    if (aluscrbuf.bchars > 0)
    {
      scrdtk(1, alubuf, sizeof alubuf - 1, &aluscrbuf);
      if (aluscrbuf.toktyp == nortok)
      {
        scrdtk(1, NULL, 0, &aluscrbuf);
        if (aluscrbuf.toktyp == eoltok)
          strbuf = (char *)alubuf;
      }                            /* if (aluscrbuf.toktyp == nortok) */
    }                              /* if (aluscrbuf.bchars >0) */

    errno = 0;
    if (verb < 010000)
    {
      long_result = strtol(strbuf, &endptr, 0);
      if (!errno)
        ALU_memory[idx] = long_result;
      else
      {
        if (errno == ERANGE)
        {
/* Attempt unsigned Hex or Octal entry. */
          uint8_t *q = (void *)strbuf;

          while (isspace(*q))
            q++;
          if (*q == '0')
          {
            errno = 0;
            ulong_result = strtoul(strbuf, &endptr, 0);
            if (errno)
            {
              fprintf(stderr, "%s. %s (strtoul)", strerror(errno), ubuf);
              GIVE_UP;
            }                      /* if (errno) */
            else
              *(unsigned long *)(&ALU_memory[idx]) = ulong_result;
          }                        /* if (*q == '0') */
        }                          /* if (errno == ERANGE) */
      }                            /* if (!errno) else */
      if (errno)
      {
        fprintf(stderr, "%s. %s (strtol)", strerror(errno), ubuf);
        GIVE_UP;
      }                            /* if (errno) */
      if (!*endptr || (oldcom->toklen == 2 &&
        toupper((uint8_t)ubuf[0]) == 'T' &&
        gettab(ubuf[1], false, &ALU_memory[idx], false)))
      {
        if (oldval && !BRIEF && WARN_NONZERO_MEMORY)
        {
          fputs("Warning - value was previously ", stdout);
          printf(Iformat, oldval);
          fputs("\r\n", stdout);
        }                     /* if (oldval && !BRIEF && WARN_NONZERO_MEMORY) */
        return 1;                  /* All chars parsed */
      }
      if (*endptr && !(oldcom->toklen == 2 && toupper((uint8_t)ubuf[0]) == 'T'))
        fprintf(stderr, "Illegal character '%c' in number \"%s\"", *endptr,
          ubuf);
      else
        fprintf(stderr, "Invalid number format");
      GIVE_UP;
    }                              /* if (verb < 010000) */
    else
    {
      FPU_memory[idx] = strtod(strbuf, &endptr);
      if (errno)
      {
        fprintf(stderr, "%s. %s (strtod)", strerror(errno), ubuf);
        GIVE_UP;
      }                            /* if (errno) */
      if (!*endptr)
      {
        if (oldfpval != 0.0 && !BRIEF && WARN_NONZERO_MEMORY)
        {
          fputs("Warning - value was previously ", stdout);
          printf(FPformat, oldfpval);
          fputs("\r\n", stdout);
        }
        return 1;                  /* OK */
      }                            /* if (!*endptr) */
      fprintf(stderr, "Illegal character '%c' in number \"%s\"", *endptr, ubuf);
      GIVE_UP;
    }                              /* if (verb < 010000) else */
  }                                /* if (verb & 07000 == 07000) */

/* Defining floating-point format? */
  if (verb == 04007)
  {
    if (mcchrs > sizeof FPformat - 1)
    {
      fprintf(stderr, "%s", "Format string too long");
      GIVE_UP;
    }                              /* if (mcchrs > sizeof FPformat -1) */
    strncpy(FPformat, ubuf, mcchrs);
    FPformat[mcchrs] = 0;
    return 1;
  }                                /* if (verb == 04007) */
/* Defining date format? */
  if (verb == 04010)
  {
    if (mcchrs > sizeof DTformat - 1)
    {
      fprintf(stderr, "%s", "Format string too long");
      GIVE_UP;
    }                              /* if (mcchrs > sizeof DTformat -1) */
    strncpy(DTformat, ubuf, mcchrs);
    DTformat[mcchrs] = 0;
    return 1;
  }                                /* if (verb == 04010) */
/* Defining integer format? */
  if (verb == 04013)
  {
    if (mcchrs > sizeof Iformat - 1)
    {
      fprintf(stderr, "%s", "Format string too long");
      GIVE_UP;
    }                              /* if (mcchrs > sizeof Iformat -1) */
    strncpy(Iformat, ubuf, mcchrs);
    Iformat[mcchrs] = 0;
    return 1;
  }                                /* if (verb == 04013) */
/* Command output substitution? */
  if (verb == 04014 || verb == 04015)
  {
    final5();
    qreg = cmd(ubuf, true);
    init5();
    if (qreg && verb == 04014)
    {
      fprintf(stderr, "%s", stderrbuf);
      GIVE_UP;
    }                              /* if (qreg && verb == 04014) */
    return 1;
  }                                /* if (verb == 04014 || verb == 04015) */

/* Advise user if an existing macro being overwritten */
  if (scmacs[verb] && (curmac < 0 || !BRIEF))
    fputs("Warning - overwriting old macro\r\n", stdout);
  return newmac2(false) ? 1 : 0;
}
