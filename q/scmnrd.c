/* >%---- CODE_STARTS ./scmnrd.c */
/* S C M N R D
 *
 * Copyright (C) 1981 D. C. Roe
 * Copyright (C) 2012-2014,2016-2021 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 *
 * Reads a command line - insists on a 1-char verb
 * Leaves READTOKEN ready to read 1st param
 *
 * This function is (trivially) recursive:
 * it can call itself via rerdcm(),
 * but always returns immediately after doing so
 */

/* Headers */

/* >%---- KEEP2HERE ./scmnrd.c */
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "prototypes.h"
#include "edmast.h"
#include "macros.h"
#include "cmndcmmn.h"
#include "scrnedit.h"
#include "fmode.h"
#include "c1in.h"
/* >%---- CUT_HERE ./scmnrd.c */

/* Static prototypes */

static void massage_q_arg(void);
static void complain_bad_command(void);
static action do_the_massage(void);
static action set__want_disply(void);
static action display_maybe(void);
static void complain_cvmba(void);

/* Instantiate externals */

scrbuf5 cmthis;
bool cmsplt, cmover;

/* Static variables */

static char *cmtabl[52] = {        /* Table of commands in full */
  "APPEND",
  "BACKUP",
  "COPY",
  "DELETE",
  "ENTERFILE",
  "",
  "GOTO",
  "HELP",
  "INSERT",
  "JOIN",
  "KEYLOG",
  "LOCATE",
  "MODIFY",
  "NEWMACRO",
  "ONOFFINDENT",
  "PRINT",
  "QUIT",
  "REPOSITION",
  "SAVE",
  "TABSET",
  "USEFILE",
  "VIEW",
  "WRITEFILE",
  "XISTICS",
  "YCHANGEALL",
  "ZENDUSE",

/* Commands starting with F */
  "",                              /* FA */
  "BRIEF",                         /* FB */
  "CASEINDEPEND",                  /* FC */
  "DEVNULL",                       /* FD */
  "",                              /* FE */
  "FNOWRAP",                       /* FF */
  "",                              /* FG */
  "",                              /* FH */
  "IMMEDIATE_MACRO",               /* FI */
  "",                              /* FJ */
  "",                              /* FK */
  "LOCATE",                        /* FL */
  "MODE",                          /* FM */
  "NONE",                          /* FN */
  "ORGET",                         /* FO */
  "",                              /* FP */
  "QUIT",                          /* FQ */
  "REPROMPT",                      /* FR */
  "",                              /* FS */
  "TOKENCHAR",                     /* FT */
  "",                              /* FU */
  "VERBOSE",                       /* FV */
  "",                              /* FW */
  "XCHANGE",                       /* FX */
  "YCHANGEALL",                    /* FY */
  ""                               /* FZ */
};

static int idx;
static bool want_disply;
static int start_pos, split_pos;
static bool fanout;                /* For F commands */
static int wanted_arg;

void
scmnrd()
{
  int k;                           /* Scratch */
  int file_start;
  int savcurs;

/* Initialise former stack variables */
  want_disply = false;
  fanout = false;

/* Read line */
  while (true)
  {
    cmsplt = false;                /* No split command yet */
    if (USING_FILE)
    {
      putchar('>');
      putchar(' ');
      scurs = 2;
      cmover = false;              /* No line overflow yet */
    }
    lstvld = true;                 /* Previous command is alway valid */
    in_cmd = true;                 /* Prevent indenting */
    scrdit(newcom, oldcom, "> ", 2); /* Read line */
    in_cmd = false;
    if (USING_FILE)
    {
      screen[0] = '$';             /* So get prompt after 'Z' */
      if (cmover)                  /* If command overflowed */
      {
        printf("Command line over %d chars", BUFMAX);
        rerdcm();
        return;                    /* Reread command */
      }
    }                              /* if (USING_FILE) */
    if (verb == '[')               /* ESC = ^U + REREAD */
    {
      newcom->bcurs = 0;
      newcom->bchars = 0;
      continue;
    }                              /* if (verb == '[') */
    if (verb == 'T')               /* If we have a command split */
    {
      cmsplt = true;               /* We have a split */

/* Save in case massage exceeds total capacity */
      memcpy(&cmthis, oldcom,
        sizeof cmthis - sizeof cmthis.bdata + oldcom->bchars);
      newcom->bcurs = 0;           /* Leave cursor at strt new line */
    }                              /* if (verb == 'T') */
/*
 * Massage command, i.e. split off a numeric parameter hard against
 * the verb. If there are any single quotes around, forget it.
 * Some format checking can be done here: blank line (repeat input),
 * null verb (object), verb doesn't start with a letter (object).
 */
    scrdtk(5, NULL, 0, oldcom);    /* Init command buffer */

/* Read unmassaged verb */
    scrdtk(1, (uint8_t *)ubuf, BUFMAX, oldcom);
    if (oldcom->toktyp != nortok)  /* We have to display */
    {
      if (curmac < 0 || !BRIEF)
      {
/* If we got an "empty" line from a U-USE file, it has already been shown. */
/* "empty" includes slash-star comment lines */
        if (!(USING_FILE && oldcom->toktyp == eoltok))
          disply(oldcom, false);
      }
      if (oldcom->toktyp == eoltok) /* Empty line */
      {
        if (!cmsplt)               /* No valid data in line */
        {
          newcom->bchars = 0;
          newcom->bcurs = 0;
        }
/* Check if user is trying to ^C out from keybd input */
        if (!cntrlc || USING_FILE || curmac >= 0)
          continue;                /* J not to quit */
        oldcom->bdata[0] = '!';
        oldcom->bcurs = 1;
        oldcom->bchars = 1;
        verb = '!';
        cntrlc = false;            /* ^C noted */
        SWITCH_ACTION(display_maybe()); /* J back in - simulate ! */
      }                            /* if (oldcom->toktyp != nortok) */
      fputs("Null command verb not allowed", stdout);
      rerdcm();
      return;
    }
    verb = ubuf[0];
    if (verb == '#')
      verb = ASTRSK;
    if (verb == ASTRSK)
      SWITCH_ACTION(set__want_disply()); /* # comment - display */

/* Check out non-alpha's... */
    if (!(verb >= 'A' && verb <= 'Z')) /* Verb non-alpha */
    {
      if (verb != '!')             /* If not system command */
      {
        if (oldcom->toklen == 1 && verb == ASTRSK)
          SWITCH_ACTION(set__want_disply()); /* * comment - display */
        complain_cvmba();
        return;
      }
    }
/*
 * Determine if quotes by comparing length of verb returned with
 * length we simplistically calculate ourselves
 */
    if (oldcom->toklen == 1)
      SWITCH_ACTION(set__want_disply()); /* Can't massage 1-char verb */
    for (start_pos = 0;; start_pos++)
      if (oldcom->bdata[start_pos] != SPACE)
        break;                     /* Find start of command */
/*
 * SCRDTK leaves cursor just after token delimiter or at e.o.l.
 */
    k = oldcom->bdata[oldcom->bcurs - 1];
    if (k == SPACE || k == COMMA)
      oldcom->bcurs--;             /* If was after delimiter */

/* If lengths are unequal (implying quotes somewhere), don't attempt
 * massage. Furthermore, complain if token started '!'... */
    if (oldcom->bcurs - start_pos != oldcom->toklen)
    {
      if (verb == '!')
      {
        complain_cvmba();
        return;
      }                            /* if (verb == '!') */
      else
        SWITCH_ACTION(set__want_disply()); /* Don't attempt massage */
    }                          /* if(oldcom->bcurs-start_pos!=oldcom->toklen) */
/*
 * Massage '!' right now
 */
    if (verb == '!')
    {
      split_pos = 1;               /* Split off straight after ! */
      SWITCH_ACTION(do_the_massage());
    }                              /* if (verb == '!') */
    else
    {
/* Work along the token (which will have been converted to u/c) looking for a
 * non-alpha. If one found, calculate where in SCREENEDIT buffer it would be,
 * and insert a space there. Don't examine the 1st char */
      for (split_pos = 1; split_pos < oldcom->toklen; split_pos++)
        if (ubuf[split_pos] < 'A' || ubuf[split_pos] > 'Z')
          SWITCH_ACTION(do_the_massage()); /* Found non-alpha */
      SWITCH_ACTION(set__want_disply()); /* No massage to do */
    }                              /* if (verb == '!') else */
  }                                /* while(true) */

  if (verb == '!')                 /* Was system command */
  {
    if (want_disply)
      disply(oldcom, true);
    return;
  }                                /* if (verb == '!') */
  if (verb == 'F')
  {
/* Code to deal with commands starting F */
    if (oldcom->toklen == 1)       /* Not unique abb'n */
    {
      if (want_disply)
        disply(oldcom, true);      /* Display the command */
      fputs("Command abbreviation not unique", stdout);
      rerdcm();
      return;
    }                              /* if (oldcom->toklen == 1) */
    fanout = true;

    memmove(ubuf, ubuf + 1, --oldcom->toklen); /* Overlapping move */
    ubuf[oldcom->toklen] = '\0';
    idx = 'F' & 037;               /* In case not alpha verb */
    verb = ubuf[0];                /* Get app'n standard verb */
    if (verb < 'A' || verb > 'Z')
    {
      complain_bad_command();
      return;
    }                              /* if (verb < 'A' || verb > 'Z') */
    verb = verb + 040;             /* Make lower case */
    idx = (verb & 037) + 25;       /* Get array subscript */
  }                                /* if (verb == 'F') */
  if (strncmp(ubuf, cmtabl[idx], oldcom->toklen)) /* Not a abbr'n */
  {
    complain_bad_command();
    return;
  }
/* Check for disallowed command if Fixed-Length Mode */
  if (fmode & FM_PLUS_F_BIT && ((verb == 'A' && !binary) || verb == 'C' ||
    verb == 'D' || verb == 'E' || verb == 'I'))
  {
    if (want_disply)
      disply(oldcom, true);        /* Display the command */
    fputs("Command disallowed when FIXED LENGTH mode asserted", stdout);
    rerdcm();
    return;
  }
  savcurs = oldcom->bcurs;         /* Changed by disply, Q massage */
/* Q COMMAND MASSAGE */
  if (verb == 'Q' && !recursing_copy)
  {
    previous_argno = argno;
/* See if command has args */
    scrdtk(1, (uint8_t *)ubuf, BUFMAX, oldcom);
    if (oldcom->toktyp == nortok && optind < argc && ubuf[0] == '$' &&
      oldcom->toklen > 1 && oldcom->bdata[oldcom->tokbeg] == '$')
    {
/* First check for no more args */
      file_start = oldcom->tokbeg; /* Remember where filename starts */
      scrdtk(1, NULL, 0, oldcom);
      if (oldcom->toktyp == eoltok)
      {
        oldcom->bcurs = savcurs;
        oldcom->bdata[file_start] = SPACE; /* Remove leading '$' */
/* Read arg no */
        scrdtk(1, (uint8_t *)ubuf, BUFMAX, oldcom);
        oldcom->bdata[file_start] = '$'; /* Reinstate leading '$' */
        oldcom->tokbeg = file_start;
        if (oldcom->decok)         /* Ok decimal */
        {
/* See if relative or absolute. Zero is always relative */
          if (!oldcom->decval || oldcom->plusf || oldcom->minusf)
            wanted_arg = argno + oldcom->decval;
          else
            wanted_arg = oldcom->decval - 1; /* Wanted arg # */
          if (wanted_arg >= 0 && wanted_arg + optind < argc)
          {
            if (fmode & FM_PLUS_HASH_BIT)
              argno = wanted_arg;  /* If +# mode */
            oldcom->bchars = file_start - 1; /* Truncate command line */
            massage_q_arg();       /* Massage in argument */
          }             /* if (wanted_arg >= 0 && wanted_arg + optind < argc) */
        }                          /* if (oldcom->decok) */
      }                            /* if (oldcom->toktyp == eoltok) */
    }                              /* if (oldcom->toktyp == nortok && ...) */
    else if (argno >= 0 && oldcom->toktyp == eoltok &&
      (wanted_arg = ++argno) + optind < argc)
      massage_q_arg();
  }                                /* if (verb == 'Q') */
  if (want_disply)
    disply(oldcom, false);         /* Display the command */
  oldcom->bcurs = savcurs;
  return;
}

/* Static functions */

/* ****************************** display_maybe ***************************** */

static action
display_maybe(void)
{
/* Defer displaying the command owing to a possible Q massage... */
  scrdtk(5, NULL, 0, oldcom);      /* Reset command buffer */
  scrdtk(1, (uint8_t *)ubuf, BUFMAX, oldcom);
  if (oldcom->toklen > 12 && verb != ASTRSK) /* Verb too long */
  {
    if (want_disply)
      disply(oldcom, true);        /* Display the command */
    fputs("Command verb too long", stdout);
    rerdcm();
    return RETURN;
  }
  idx = (ubuf[0] & 037) - 1;       /* Get 1st char as a subscript */
  if (verb == ASTRSK)              /* Was just an * comment */
  {
    if (want_disply)
      disply(oldcom, true);
    if (!cmsplt)                   /* No valid data in line */
    {
      newcom->bchars = 0;
      newcom->bcurs = 0;
    }
    return CONTINUE;               /* Read next command */
  }
  return BREAK;                    /* Leave loop containing caller */
}                                  /* static action display_maybe(void) */

/* ****************************** massage_q_arg ***************************** */

static void
massage_q_arg(void)
{
  int alen = strlen(*(argv + optind + wanted_arg));

  oldcom->bdata[oldcom->bchars++] = SPACE; /* Append space */
  memcpy(&oldcom->bdata[oldcom->bchars], /* Don't want trlg NUL */
    *(argv + optind + wanted_arg), alen);
  oldcom->bchars += alen;          /* Append next arg */
}                                  /* static void massage_q_arg(void) */

/* ***************************** do_the_massage ***************************** */

static action
do_the_massage(void)
{
  oldcom->bcurs = start_pos + split_pos;
  insert = true;
  ordch(SPACE, oldcom);
  return set__want_disply();
}                                  /* static action do_the_massage(void) */

/* **************************** set__want_disply **************************** */

static action
set__want_disply(void)
{
  if (USING_FILE && curmac < 0)
    want_disply = false;           /* U-use & !macro */
  else
    want_disply = !BRIEF || curmac < 0;
  return display_maybe();
}                                  /* static action set__want_disply(void) */

/* ************************** complain_bad_command ************************** */

static void
complain_bad_command(void)
{
  if (want_disply)
    disply(oldcom, true);          /* Display the command */
  printf("%s%c ", fanout ? "F" : "", (char)(verb & 0137));
  if (cmtabl[idx][0])
    printf("is short for %s%s", fanout ? "F" : "", cmtabl[idx]);
  else
    fputs("is not a command", stdout);
  rerdcm();
}                                  /* static void complain_bad_command(void) */

/* ***************************** complain_cvmba ***************************** */

static void
complain_cvmba(void)
{
  newlin();
  fputs("Command verb must be alphabetic", stdout);
  rerdcm();
}                                  /* static void complain_cvmba(void) */
