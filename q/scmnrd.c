/* S C M N R D
 *
 * Copyright (C) 1981 D. C. Roe
 * Copyright (C) 2012-2014,2016-2019 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 *
 * Reads a command line - insists on a 1-char verb
 * Leaves READTOKEN ready to read 1st param
 */
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
  "",
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
/*
 * Commands starting with F
 */
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
  "",                              /* FR */
  "",                              /* FS */
  "TOKENCHAR",                     /* FT */
  "",                              /* FU */
  "VERBOSE",                       /* FV */
  "",                              /* FW */
  "XCHANGE",                       /* FX */
  "YCHANGEALL",                    /* FY */
  ""                               /* FZ */
};

void
scmnrd()
{
  long i, n, k, l;                 /* Scratch */
  bool want_disply = false;
  bool fanout = false;             /* For F commands */
/*
 * Read line
 */
  while (true)
  {
    cmsplt = false;                /* No split command yet */
    zmode = fmode;                 /* So n4000 can return proper value */
    zmode_valid = true;
    fmode &= 033777777777U;        /* Turn off indent */
    if (USING_FILE)
    {
      putchar('>');
      putchar(' ');
      scurs = 2;
      cmover = false;              /* No line overflow yet */
    }
    lstvld = true;                 /* Previous command is alway valid */
    scrdit(newcom, oldcom, "> ", 2, true); /* Read line */
    fmode = zmode;                 /* Reinstate indent mode, if on */
    zmode_valid = false;
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
      memcpy((char *)&cmthis, (char *)oldcom, sizeof(scrbuf5));
      newcom->bcurs = 0;           /* Leave cursor at strt new line */
    }                              /* if (verb == 'T') */
/*
 * Massage command, i.e. split off a numeric parameter hard against
 * the verb. If there are any single quotes around, forget it.
 * Some format checking can be done here: blank line (repeat input),
 * null verb (object), verb doesn't start with a letter (object).
 */
    scrdtk(5, 0, 0, oldcom);       /* Init command buffer */

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
        goto p1003;                /* J back in - simulate ! */
      }                            /* if (oldcom->toktyp != nortok) */
      printf("Null command verb not allowed");
      rerdcm();
      return;
    }
    verb = ubuf[0];
    if (verb == '#')
      verb = ASTRSK;
    if (verb == ASTRSK)
      goto p1109;                  /* J # comment - display */

/* Check out non-alpha's... */
    if (!(verb >= 'A' && verb <= 'Z')) /* Verb non-alpha */
    {
      if (verb != '!')             /* If not system command */
      {
        if (oldcom->toklen == 1 && verb == ASTRSK)
          goto p1109;              /* J * comment - display */
      p1106:
        newlin();
        printf("Command verb must be alphabetic");
        rerdcm();
        return;
      }
    }
/*
 * Determine if quotes by comparing length of verb returned with
 * length we simplistically calculate ourselves
 */
    if (oldcom->toklen == 1)
      goto p1109;                  /* Can't massage 1-char verb */
    for (i = 0;; i++)
      if (oldcom->bdata[i] != SPACE)
        break;                     /* Find start of command */
/*
 * SCRDTK leaves cursor just after token delimiter or at e.o.l.
 */
    k = oldcom->bdata[oldcom->bcurs - 1];
    if (k == SPACE || k == COMMA)
      oldcom->bcurs--;             /* If was after delimiter */

/* If lengths are unequal (implying quotes somewhere), don't attempt
 * massage. Furthermore, complain if token started '!'... */
    if (oldcom->bcurs - i != oldcom->toklen)
    {
      if (verb == '!')
        goto p1106;                /* Complain if was '!' */
      else
        goto p1109;                /* Don't attempt massage */
    }                              /* if(oldcom->bcurs-i!=oldcom->toklen) */
/*
 * Massage '!' right now
 */
    if (verb == '!')
      n = 1;                       /* Split off straight after ! */
/*
 * Work along the token (which will have been converted to u/c)
 * looking for a non-alpha. If one found, calculate where in
 * SCREENEDIT buffer it would be, and insert a space there.
 */
    else
    {
      for (n = 1; n < oldcom->toklen; n++) /* Don't examine 1st char */
      {
        l = ubuf[n];
        if (l < 'A')
          goto p1111;              /* J found non-alpha */
        if (l > 'Z')
          goto p1111;              /* J found non-alpha */
      }
      goto p1109;                  /* No mass. to do if drop thro' */
    }
/*
 * p1111 - DO THE MASSAGE...
 */
  p1111:
    oldcom->bcurs = i + n;
    insert = true;
    ordch(SPACE, oldcom);
  p1109:
    if (USING_FILE && curmac < 0)
      want_disply = false;         /* U-use & !macro */
    else
      want_disply = !BRIEF || curmac < 0;

/* Defer displaying the command owing to a possible Q massage... */
  p1003:
    (void)scrdtk(5, 0, 0, oldcom); /* Reset command buffer */
    scrdtk(1, (uint8_t *)ubuf, BUFMAX, oldcom);
    if (oldcom->toklen > 12 && verb != ASTRSK) /* Verb too long */
    {
      if (want_disply)
        disply(oldcom, true);      /* Display the command */
      printf("Command verb too long");
      rerdcm();
      return;
    }
    i = (ubuf[0] & 037) - 1;       /* Get 1st char as a subscript */
    if (verb == ASTRSK)            /* Was just an * comment */
    {
      if (want_disply)
        disply(oldcom, true);
      if (!cmsplt)                 /* No valid data in line */
      {
        newcom->bchars = 0;
        newcom->bcurs = 0;
      }
      continue;                    /* Read next command */
    }
    break;                         /* Only repeat loop via continue */
  }                                /* while(true) */

  if (verb == '!')                 /* Was system command */
  {
    if (want_disply)
      disply(oldcom, true);
    return;
  }
  if (verb == 'F')
  {
/*
 * Code to deal with commands starting F
 */
    if (oldcom->toklen == 1)       /* Not unique abb'n */
    {
      if (want_disply)
        disply(oldcom, true);      /* Display the command */
      printf("Command abbreviation not unique");
      rerdcm();
      return;
    }
    fanout = true;

    memmove(ubuf, ubuf + 1, --oldcom->toklen); /* Overlapping move */
    ubuf[oldcom->toklen] = '\0';
    i = 5;                         /* In case not alpha VERB */
    verb = ubuf[0];                /* Get app'n standard verb */
    if (verb < 'A' || verb > 'Z')
      goto p1205;                  /* J out of range */
    verb = verb + 040;             /* Make lower case */
    i = (verb & 037) + 25;         /* Get array subscript */
  }
  if (strncmp(ubuf, cmtabl[i], oldcom->toklen)) /* Not a abbr'n */
  {
  p1205:
    if (want_disply)
      disply(oldcom, true);        /* Display the command */
    printf("%s%c ", fanout ? "F" : "", (char)(verb & 0137));
    if (cmtabl[i][0])
      printf("is short for %s%s", fanout ? "F" : "", cmtabl[i]);
    else
      printf("is not a command");
    rerdcm();
    return;
  }
/* Check for disallowed command if Fixed-Length Mode */
  if (fmode & 0400 && ((verb == 'A' && !binary) || verb == 'C' || verb == 'D' ||
    verb == 'E' || verb == 'I'))
  {
    if (want_disply)
      disply(oldcom, true);        /* Display the command */
    printf("Command disallowed when FIXED LENGTH mode asserted");
    rerdcm();
    return;
  }
  i = oldcom->bcurs;               /* Changed by disply, Q massage */
/* Q COMMAND MASSAGE */
  if (verb == 'Q')
  {
/* See if cmd has args */
    (void)scrdtk(1, (uint8_t *)ubuf, BUFMAX, oldcom);
/* No quotes */
    if (oldcom->toktyp == nortok && optind < argc && ubuf[0] == '$' &&
      oldcom->toklen > 1 && oldcom->bdata[oldcom->tokbeg] == '$')
    {
/* First check for no more args */
      k = oldcom->tokbeg;          /* Remember where filename starts */
      (void)scrdtk(1, 0, 0, oldcom);
      if (oldcom->toktyp == eoltok)
      {
        oldcom->bcurs = i;
        oldcom->bdata[k] = SPACE;  /* Remove leading '$' */
/* Read arg no */
        (void)scrdtk(1, (uint8_t *)ubuf, BUFMAX, oldcom);
        oldcom->bdata[k] = '$';    /* Reinstate leading '$' */
        oldcom->tokbeg = k;
        if (oldcom->decok)         /* Ok decimal */
        {
/* See if relative or absolute. Zero is always relative */
          if (!oldcom->decval || oldcom->plusf || oldcom->minusf)
            l = argno + oldcom->decval;
          else
            l = oldcom->decval - 1; /* Wanted arg # */
          if (l >= 0 && l + optind < argc)
          {
            if (fmode & 0200)
              argno = l;           /* If +# mode */
            oldcom->bchars = k - 1; /* Truncate command line */
            goto q1001;            /* Massage in argument */
          }
        }
      }
    }
    else if (argno >= 0 && oldcom->toktyp == eoltok &&
      (l = ++argno) + optind < argc)
    {                              /* No cmd args & more file args */
    q1001:
      oldcom->bdata[oldcom->bchars++] = SPACE; /* Append a SPACE */
      k = strlen(*(argv + optind + l));
      strncpy((char *)&oldcom->bdata[oldcom->bchars], *(argv + optind + l), k);
      oldcom->bchars += k;         /* Append next arg */
    }
  }                                /* if (verb == 'Q') */
  if (want_disply)
    disply(oldcom, false);         /* Display the command */
  oldcom->bcurs = i;
  return;
}
