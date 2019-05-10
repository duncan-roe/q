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
#define REREAD_CMD_S(x) do {msg = x; REREAD_CMD;} while (0)
#define READ_NEXT_COMMAND goto ok_command

char *xistics_end_sequence = "\025x\n";
void
xistcs()
{
  scrbuf5 cmdbuf;
  uint8_t buf[5], *p = NULL;
  int result = 0;                  /* Returned value */
  int octnum = 0;                  /* Returned value */
  int j, k = 0;                    /* Scratch */
  char *msg;

/* INTERNAL FUNCTIONS */

/* ******************************* get_result ******************************* */

  bool get_result(void)
  {
    if (scrdtk(1, buf, 5, &cmdbuf))
    {
      fprintf(stderr, "%s. decno (scrdtk)\r\n", strerror(errno));
      msg = "Error - see above";
      return false;
    }                              /* if (scrdtk(1, buf, 5, &cmdbuf)) */
    if (cmdbuf.toktyp == eoltok)
    {
      result = 0;
      return true;                 /* Finish if EOL */
    }                              /* if (cmdbuf.toktyp == eoltok) */
    if (cmdbuf.toktyp != nortok)
    {
      msg = "Null decno illegal";
      return false;
    }                              /* if (cmdbuf.toktyp != nortok) */
    if (!cmdbuf.decok)
    {
      msg = "Bad decno";
      return false;
    }                              /* if (!cmdbuf.decok) */
    result = cmdbuf.decval;        /* All checks OK: set result */
    return true;
  }                                /* bool get_result(void) */

/* ******************************* good_octnum ****************************** */

  bool good_octnum(void)
  {
    if (octnum < 0200)
      return true;                 /* J a char */
    fprintf(stderr, "%*s", cmdbuf.toklen, buf);
    msg = " not octal for any char";
    return false;
  }                                /* bool good_octnum(void) */

/* ********************************** eolok ********************************* */

  bool eolok(void)
  {
    (void)scrdtk(1, 0, 0, &cmdbuf);
    if (cmdbuf.toktyp == eoltok)
      return true;                 /* J EOL (OK) */
    msg = "Spurious params - command not done";
    return false;
  }                                /* bool eolok(void) */

/* END INTERNAL FUNCTIONS */

  end_seq = xistics_end_sequence;
/*
 * On entry, we are back to normal duplex. Don't read commands via
 * Screenedit system, as this is suspect until characteristics are sorted out
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
    REREAD_CMD_S("Must have single-char command");
  switch (verb)
  {
    case 'A':                      /* End - of - line */
      if (!get_result())
        REREAD_CMD;
      if (result == 0)
      {
        if (!eolok())
          REREAD_CMD;              /* Check EOL now */
      }                            /* if (result == 0) */
      else
      {
        p = cachrs;                /* Where characters go */
        j = result;                /* Result includes backspaces */
        if (j > PRMAX)
          j = PRMAX;
        for (k = j; k > 0; k--)
        {
/* Get an octal character to OCTNUM. Single characters stand for themselves,
 * so octal numbers must have at least 2 digits... */
          if (scrdtk(1, buf, 4, &cmdbuf))
          {
            fprintf(stderr, "%s. octno (scrdtk)\r\n", strerror(errno));
            REREAD_CMD_S("Error - see above");
          }
          if (cmdbuf.toktyp != eoltok)
          {
            if (cmdbuf.toktyp ^= nortok)
              REREAD_CMD_S("Null value not allowed");
            if (cmdbuf.toklen == 1)
              octnum = buf[0];
            else
            {
              if (!cmdbuf.octok)
                REREAD_CMD_S("Bad octal #"); /* J not octal after all */
              octnum = cmdbuf.octval;
              if (!good_octnum())
                REREAD_CMD;
            }                      /* if (cmdbuf.toklen == 1) else */
          }                        /* if (cmdbuf.toktyp != eoltok) */
          if (cmdbuf.toktyp == eoltok)
            break;                 /* J out no octal char (EOL) */
          *p++ = octnum;
        }                          /* for (k = j; k > 0; k--) */
        if (!eolok())
          REREAD_CMD;
      }                            /* if (result == 0) else */
      cacnt = result;
      READ_NEXT_COMMAND;

    case 'B':                      /* Backspace */
      (void)scrdtk(1, buf, 4, &cmdbuf);
      if (cmdbuf.toktyp == eoltok)
        bspace = !bspace;
      else
      {
        if (cmdbuf.toktyp != nortok)
        {
          msg = "Bad param";
          REREAD_CMD;
        }                          /* if (cmdbuf.toktyp != nortok) */
/*
 * If we have been given an octal character, set this as the
 * backspace character from now on, and set bspace true ...
 */
        verb = buf[0];
        result = cmdbuf.octok;     /* In case we got an OCTNUM */
        octnum = cmdbuf.octval;    /* In case we got an OCTNUM */
        if (!eolok())
          REREAD_CMD;
        if (result)
        {
          if (!good_octnum())
            REREAD_CMD;
          backsp = octnum;
          bspace = true;
        }                          /* if (result) */
        else
        {
          switch (verb)
          {
            case 'Y':
              bspace = true;
              break;
            case 'T':
              bspace = true;
              break;
            case 'N':
              bspace = false;
              break;
            case 'F':
              bspace = false;
              break;
            default:
              msg = "param not recognised";
              REREAD_CMD;
          }                        /* switch (verb) */
        }                          /* if (result) else */
      }                            /* if (cmdbuf.toktyp == eoltok) else */
      READ_NEXT_COMMAND;

    case 'D':                      /* Set or display per-character delay */
      puts("Delay is not currently working\r");
      READ_NEXT_COMMAND;

    case 'X':                      /* Back to main editor */
      if (!eolok())
        REREAD_CMD;                /* Check no params */
      end_seq = normal_end_sequence;
      if (simulate_q)
        simulate_q_idx = 0;
      return;

    case 'T':                      /* Tab spacing in file */
      if (!get_result())
        REREAD_CMD;
      if (!eolok())
        REREAD_CMD;
      tabsiz = result ? result : 8;
      READ_NEXT_COMMAND;

    case 'W':
      if (!eolok())
        REREAD_CMD;
      vt100 = false;
      READ_NEXT_COMMAND;

    default:
      fprintf(stderr, "%c", verb);
      msg = " is not a recognised characteristic";
      REREAD_CMD;
  }
}                                  /* main() */
