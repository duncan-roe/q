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

/* Macros */

#define REREAD_CMD goto msg_read_command

char *xistics_end_sequence = "\025x\n";
void
xistcs()
{
  scrbuf5 cmdbuf;
  uint8_t buf[5], *p = NULL;
  int rtn;                         /* Return from subroutines */
  int result = 0;                  /* Returned value */
  int octnum = 0;                  /* Returned value */
  int j, k = 0;                    /* Scratch */
  char *msg;

  end_seq = xistics_end_sequence;
/*
 * On entry, we are back to normal duplex. Don't read commands via
 * Screenedit system, as this is suspect until characteristics sorted
 * out.
 */
ok_command:
  msg = "Ok";
msg_read_command:
  if (strcmp(msg, "Ok"))
  {
    fprintf(stderr, "%s\r\n", msg);
    msg = "ER";
  }                                /* if (strcmp(msg, "Ok")) */
  printf("%s> ", msg);
  if (!cl5get((char *)cmdbuf.bdata, BUFMAX, true, true))
  {
    cmdbuf.bdata[0] = 'x';
    cmdbuf.bdata[1] = 0;
    puts("x");
  }                                /* if (!cl5get(...)) */
  cmdbuf.bchars = strlen((char *)cmdbuf.bdata);
  (void)scrdtk(5, 0, 0, &cmdbuf);
  (void)scrdtk(1, buf, 2, &cmdbuf);
  verb = buf[0];
  if (cmdbuf.toktyp != nortok || cmdbuf.toklen != 1)
  {
/*
 * Bad verb
 */
    msg = "Must have single-char command";
    REREAD_CMD;
  }
  switch (verb)
  {
    case 'A':
      goto p1001;
    case 'B':
      goto p1002;
    case 'D':
      goto p1003;
    case 'R':
      goto p1004;
    case 'X':
      goto p1006;
    case 'T':
      goto p1401;
    case 'W':
      rtn = 1301;
      goto p10216;                 /* Check eol now */
    p1301:
      vt100 = false;
      goto ok_command;
  }
  putchar(verb);                   /* Error if drop thro' GOTO */
  msg = " is not a recognised characteristic";
  REREAD_CMD;
/* T - Tab spacing in file */
p1401:
  rtn = 1402;
  goto p1025;
p1402:
  tabsiz = result ? result : 8;
  goto ok_command;
/*
 * A - End - of - line
 */
p1001:
  rtn = 1008;
/*
 * P1025 - Get a decimal number. 0 if EOL. Result in result.
 */
p1025:
  if (scrdtk(1, buf, 5, &cmdbuf))
    fprintf(stderr, "%s. decno (scrdtk)\r\n", strerror(errno));
  result = 0;
  if (cmdbuf.toktyp == eoltok)
    goto asg2rtn;                  /* Finish if EOL */
  if (cmdbuf.toktyp != nortok)
  {
    msg = "Null decno illegal";
    REREAD_CMD;
  }                                /* if (cmdbuf.toktyp != nortok) */
  if (!cmdbuf.decok)
  {
    msg = "Bad decno";
    REREAD_CMD;
  }                                /* if (!cmdbuf.decok) */
  result = cmdbuf.decval;          /* All checks OK: set result */
  goto asg2rtn;
/*
 * P1008 - ^A continuing
 */
p1008:if (result != 0)
    goto p1011;
  if (cmdbuf.toktyp == eoltok)
    goto p1018;
  rtn = 1018;
  goto p10216;                     /* Check EOL now */
p1018:cacnt = result;
  goto ok_command;
p1011:
  p = cachrs;                      /* Where characters go */
  j = result;                      /* Result includes backspaces */
  if (j > PRMAX)
    j = PRMAX;
  rtn = 1013;
  for (k = j; k > 0; k--)
  {
/*
 * P1034 - Get an octal character to OCTNUM. Single characters stand for
 *         themselves, so octal numbers must have at least 2 digits...
 */
  p1034:
    if (scrdtk(1, buf, 4, &cmdbuf))
    {
      fprintf(stderr, "%s. octno (scrdtk)\r\n", strerror(errno));
    p1101:
      msg = "Bad octal #";
      REREAD_CMD;
    }
    if (cmdbuf.toktyp == eoltok)
      goto asg2rtn;                /* J EOL */
    if (cmdbuf.toktyp ^= nortok)
      goto p1011;                  /* J not poss OK octal */
    if (cmdbuf.toklen == 1)
    {
      octnum = buf[0];
      goto asg2rtn;                /* Return single-char */
    }
    if (!cmdbuf.octok)
      goto p1101;                  /* J not octal after all */
    octnum = cmdbuf.octval;
  p1203:
    if (octnum < 0200)
      goto asg2rtn;                /* J a char */
    fprintf(stderr, "%*s", cmdbuf.toklen, buf);
    msg = " not octal for any char";
    REREAD_CMD;
/*
 * P1013 - ^A Continuing
 */
  p1013:
    if (cmdbuf.toktyp == eoltok)
      goto p1018;                  /* J out no octal char (EOL) */
    *p++ = octnum;
  }                                /* End of loop on special chrs */
  rtn = 1018;
/*
 * P10216- E.O.L. check
 */
p10216:
  (void)scrdtk(1, 0, 0, &cmdbuf);
  if (cmdbuf.toktyp == eoltok)
    goto asg2rtn;                  /* J EOL (OK) */
  msg = "Spurious params - command not done";
  REREAD_CMD;
/*
 * B - Backspace
 */
p1002:
  (void)scrdtk(1, buf, 4, &cmdbuf);
  if (cmdbuf.toktyp != eoltok)
    goto p1019;                    /* J not EOL */
  bspace = !bspace;
  goto ok_command;                 /* No params => invert BSPACE */
p1019:
  if (cmdbuf.toktyp != nortok)
  {
    msg = "Bad param";
    REREAD_CMD;
  }                                /* if (cmdbuf.toktyp != nortok) */
/*
 * If we have been given an octal character, set this as the
 * backspace character from now on, and set BSPACE to 1...
 */
  verb = buf[0];
  result = cmdbuf.octok;           /* In case we got an OCTNUM */
  octnum = cmdbuf.octval;          /* In case we got an OCTNUM */
/*
 * Now do E.O.L. check
 */
  rtn = 10213;
  goto p10216;
p10213:
  if (result)
    goto p1201;                    /* J we were given OCTNUM */
  switch (verb)
  {
    case 'Y':
      goto p1022;
    case 'T':
      goto p1022;
    case 'N':
      goto p1023;
    case 'F':
      goto p1023;
  }
  msg = "param not recognised";
  REREAD_CMD;
/*
 * P1201 - We have a octal param to B. Check in limits (i.e a character)
 */
p1201:
  rtn = 1202;
  goto p1203;
p1202:
  backsp = octnum;
p1022:bspace = true;
  goto ok_command;
p1023:bspace = false;
  goto ok_command;
/*
 * D - Set per-character delay or typeout if no param
 */
p1003:
  puts("Delay is not currently working\r");
  goto ok_command;
/*
 * R - C/R sequence
 */
/* WARNING - THIS IS IGNORED BY THE REST OF Q. LATER, WE SHOULD PROBABLY
 * TIDY THIS UP... */
p1004:
  rtn = 1028;
  goto p1025;                      /* Get # of chrs */
p1028:if (result != 0)
    goto p1029;
  if (cmdbuf.toktyp == eoltok)
    goto p1030;                    /* J EOL */
  rtn = 1030;
  goto p10216;                     /* Check nothing further */
p1030:rtcnt = result;
  goto ok_command;
p1029:
  if (result >= 2)
  {
    msg = "C/r sequence 1 chr max";
    REREAD_CMD;
  }                                /* if (result >= 2) */
  rtn = 1033;
  if (result == 1)
  {
    goto p1034;                    /* Get an octal character */
  p1033:
    if (cmdbuf.toktyp == eoltok)
      goto p1035;                  /* J EOL (illegal) */
    rtchrs = octnum;               /* Remember the character */
  }
/*
 * We have the chars. But there must not be any more...
 */
  rtn = 1030;
  goto p10216;
p1035:
  msg = "# of chrs supplied & spec'd disagree";
  REREAD_CMD;
/*
 * X - Back to main editor
 */
p1006:
  rtn = 1037;
  goto p10216;                     /* Check no params */
p1037:
  end_seq = normal_end_sequence;
  if (simulate_q)
    simulate_q_idx = 0;
  return;
asg2rtn:switch (rtn)
  {
    case 1037:
      goto p1037;
    case 1033:
      goto p1033;
    case 1030:
      goto p1030;
    case 1301:
      goto p1301;
    case 1028:
      goto p1028;
    case 1202:
      goto p1202;
    case 10213:
      goto p10213;
    case 1013:
      goto p1013;
    case 1018:
      goto p1018;
    case 1008:
      goto p1008;
    case 1402:
      goto p1402;
    default:
      fprintf(stderr, "Assigned Goto failure, rtn = %d\r\n", rtn);
  }
}
