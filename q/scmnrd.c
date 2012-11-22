/* S C M N R D
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012, Duncan Roe
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
#include "alledit.h"
#include "edmast.h"
#include "macros.h"
#include "cmndcmmn.h"
#include "scrnedit.h"
#include "termio5.hl"
#include "c1in.h"
/* */
void
scmnrd()
{
  long i, n, k, l;                 /* Scratch */
  bool logtmp = false;             /* Remembers setting of INDENT */
  bool fanout;                     /* For F commands */
/*
 */
  static char *cmtabl[52] = {      /* Table of commands in full */
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
    "KILLFORNOW",
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
    "",                            /* FA */
    "BRIEF",                       /* FB */
    "CASEINDEPEND",                /* FC */
    "DEVNULL",                     /* FD */
    "",                            /* FE */
    "FORTRAN",                     /* FF */
    "",                            /* FG */
    "",                            /* FH */
    "IMMEDIATE_MACRO",             /* FI */
    "",                            /* FJ */
    "",                            /* FK */
    "LOCATE",                      /* FL */
    "MODE",                        /* FM */
    "NONE",                        /* FN */
    "ORGET",                       /* FO */
    "",                            /* FP */
    "QUIT",                        /* FQ */
    "",                            /* FR */
    "",                            /* FS */
    "TOKENCHAR",                   /* FT */
    "",                            /* FU */
    "VERBOSE",                     /* FV */
    "",                            /* FW */
    "XCHANGE",                     /* FX */
    "YCHANGEALL",                  /* FY */
    ""                             /* FZ */
  };
/*
 * Read line
 */
  fanout = false;
p1002:
  cmsplt = false;                  /* No split command yet */
  zmode = fmode;                   /* So n4000 can return proper value */
#ifdef ANSI5
  fmode &= 033777777777U;          /* Turn off indent */
#else
  fmode &= 033777777777;           /* Turn off indent */
#endif
  if (USING_FILE)                      /* Input from a file */
  {
    putchar('>');
    putchar(' ');
    scurs = 2;
    cmover = false;                /* No line overflow yet */
  }
  lstvld = true;                   /* Previous command is alway valid */
  scrdit(newcom, oldcom, "> ", 2, 1); /* Read line */
  fmode = zmode;                   /* Reinstate indent mode, if on */
  if (USING_FILE)
  {
    screen[0] = '$';               /* So get prompt after 'Z' */
    if (cmover)                    /* If command overflowed */
    {
      printf("Command line over %d chars", BUFMAX);
      goto p1101;                  /* Reread command */
    }
  }
  if (verb == '[')                 /* ESC = ^U + REREAD */
  {
    newcom->bcurs = 0;
    newcom->bchars = 0;
    goto p1002;
  }
  if (verb == 'T')                 /* If we have a command split */
  {
    cmsplt = true;                 /* We have a split */
/* Save in case massage busts total capacity */
    memcpy((char *)&cmthis, (char *)oldcom, sizeof(scrbuf5));
    newcom->bcurs = 0;             /* Leave cursor at strt new line */
  }
/*
 * Massage command, i.e. split off a numeric parameter hard against
 * the verb. If there are any single quotes around, forget it.
 * Some format checking can be done here: blank line (repeat input),
 * null verb (object), verb doesn't start with a letter (object).
 */
  (void)scrdtk(5, 0, 0, oldcom);   /* Init command buffer */
/* Read unmassaged verb */
  (void)scrdtk(1, (unsigned char *)buf, BUFMAX, oldcom);
  if (oldcom->toktyp != nortok)    /* We have to display */
  {
    if (curmac < 0 || !BRIEF)
      disply(oldcom, 0);
    if (oldcom->toktyp == eoltok)  /* Empty line */
    {
      if (!cmsplt)                 /* No valid data in line */
      {
        newcom->bchars = 0;
        newcom->bcurs = 0;
      }
/* Check if user is trying to ^C out from keybd input */
      if (!cntrlc || USING_FILE || curmac >= 0)
        goto p1002;                /* J not to quit */
      oldcom->bdata[0] = 'K';
      oldcom->bcurs = 1;
      oldcom->bchars = 1;
      verb = 'K';
      cntrlc = false;              /* ^C noted */
      goto p1003;                  /* J back in - simulate K */
    }
    printf("Null command verb not allowed");
    goto p1101;
  }
  verb = buf[0];
/* Check out non-alpha's... */
  if (!(verb >= 'A' && verb <= 'Z')) /* Verb non-alpha */
  {
    if (verb != '!')               /* If not system command */
    {
      if (oldcom->toklen == 1 && verb == '*')
        goto p1109;                /* J * comment - display */
    p1106:
      newlin();
      printf("Command verb must be alphabetic");
      goto p1101;
    }
  }
/*
 * Determine if quotes by comparing length of verb returned with
 * length we simplistically calculate ourselves
 */
  if (oldcom->toklen == 1)
    goto p1109;                    /* Can't massage 1-char verb */
  for (i = 0;; i++)
    if (oldcom->bdata[i] != SPACE)
      break;                       /* Find start cmmand */
/*
 * SCRDTK leaves cursor just after token delimiter or at e.o.l.
 */
  k = oldcom->bdata[oldcom->bcurs - 1];
  if (k == SPACE || k == COMMA)
    oldcom->bcurs--;               /* If was after delimiter */
/* If lengths are unequal (implying quotes somewhere), don't attempt
 * massage. Furthermore, complain if token started '!'... */
  if (oldcom->bcurs - i != oldcom->toklen)
  {
    if (verb == '!')
      goto p1106;                  /* Complain if was '!' */
    else
      goto p1109;                  /* Don't attempt massage */
  }                                /* if(oldcom->bcurs-i!=oldcom->toklen) */
/*
 * Massage '!' right now
 */
  if (verb == '!')
    n = 1;                         /* Split off straight after ! */
/*
 * Work along the token (which will have been converted to u/c)
 * looking for a non-alpha. If one found, calculate where in
 * SCREENEDIT buffer it would be, and insert a space there.
 */
  else
  {
    for (n = 1; n < oldcom->toklen; n++) /* Don't examine 1st char */
    {
      l = buf[n];
      if (l < 'A')
        goto p1111;                /* J found non-alpha */
      if (l > 'Z')
        goto p1111;                /* J found non-alpha */
    }
    goto p1109;                    /* No mass. to do if drop thro' */
  }
/*
 * P1111 - DO THE MASSAGE...
 */
p1111:
  oldcom->bcurs = i + n;
  insert = true;
  ordch(SPACE, oldcom);
p1109:
  if (USING_FILE && curmac < 0)
    logtmp = false;                /* Skip display if COMI & !macro */
  else
    logtmp = !BRIEF || curmac < 0; /* Whether to display command */
/* Defer displaying the command owing to a possible Q massage... */
p1003:
  (void)scrdtk(5, 0, 0, oldcom);   /* Reset command buffer */
  scrdtk(1, (unsigned char *)buf, BUFMAX, oldcom);
  if (oldcom->toklen > 12)         /* Verb too long */
  {
    if (logtmp)
      disply(oldcom, 1);           /* Display the command */
    printf("Command verb too long");
    goto p1101;
  }
  i = (buf[0] & 037) - 1;          /* Get 1st char as a subscript */
  if (verb == '*')                 /* Was just an * comment */
  {
    if (logtmp)
      disply(oldcom, 1);
    if (!cmsplt)                   /* No valid data in line */
    {
      newcom->bchars = 0;
      newcom->bcurs = 0;
    }
    goto p1002;                    /* Read next command */
  }
  if (verb == '!')                 /* Was system command */
  {
    if (logtmp)
      disply(oldcom, 1);
    return;
  }
  if (verb == 'F')
  {
/*
 * Code to deal with commands starting F
 */
    if (oldcom->toklen == 1)       /* Not unique abb'n */
    {
      if (logtmp)
        disply(oldcom, 1);         /* Display the command */
      printf("Command abbreviation not unique");
      goto p1101;
    }
    fanout = true;
/* Not sure if MEMCPY does overlapping moves in either direction, so */
    for (i = 1; i < oldcom->toklen; i++)
      buf[i - 1] = buf[i];
    oldcom->toklen--;              /* Now have removed F & shortened */
    buf[oldcom->toklen] = '\0';
    i = 5;                         /* In case not alpha VERB */
    verb = buf[0];                 /* Get app'n standard verb */
    if (verb < 'A' || verb > 'Z')
      goto p1205;                  /* J out of range */
    verb = verb + 040;             /* Make lower case */
    i = (verb & 037) + 25;         /* Get array subscript */
  }
  if (strncmp(buf, cmtabl[i],
#ifdef ANSI5
    (size_t)
#endif
    oldcom->toklen))               /* Not a abbr'n */
  {
  p1205:
    if (logtmp)
      disply(oldcom, 1);           /* Display the command */
    if (fanout)
      putchar('F');
    putchar(verb & 0137);
    putchar(SPACE);
    if (!cmtabl[i][0])
      printf("is not a command");
    else
    {
      printf("is short for ");
      if (fanout)
        putchar('F');
      printf(cmtabl[i]);
    }
/*
 * Reread command
 */
  p1101:
    rerdcm();
    return;
  }
/* Check for disallowed command if Fixed-Length Mode */
  if (fmode & 0400 && ((verb == 'A' && !binary) || verb == 'C' || verb == 'D' ||
    verb == 'E' || verb == 'I'))
  {
    if (logtmp)
      disply(oldcom, 1);           /* Display the command */
    printf("Command disallowed when FIXED LENGTH mode asserted");
    goto p1101;
  }
  i = oldcom->bcurs;               /* Changed by disply, Q massage */
/* Q COMMAND MASSAGE */
  if (verb == 'Q')
  {
/* See if cmd has args */
    (void)scrdtk(1, (unsigned char *)buf, BUFMAX, oldcom);
/* No quotes */
    if (oldcom->toktyp == nortok && optind < argc && buf[0] == '$' &&
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
        (void)scrdtk(1, (unsigned char *)buf, BUFMAX, oldcom);
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
      strncpy((char *)&oldcom->bdata[oldcom->bchars], *(argv + optind + l),
#ifdef ANSI5
        (size_t)
#endif
        k);
      oldcom->bchars += k;         /* Append next arg */
    }
  }
  if (logtmp)
    disply(oldcom, 0);             /* Display the command */
  oldcom->bcurs = i;
  return;
}
