/* S E T M O D E
 *
 * Copyright (C) 1994,1995 Duncan Roe & Associates P/L
 * Copyright (C) 2003,2012,2014,2017-2020 Duncan Roe
 *
 * This routine manipulates the fmode bit settings */

/* Headers */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include "prototypes.h"
#include "macros.h"
#include "edmast.h"
#include "fmode.h"

/* Macros */

#define GIVE_UP goto bad_arg

/* Static prototypes */

static void show_current(fmode_t mode);

/* ******************************** setqmode ******************************** */

bool
setqmode()
{
  int octok = 1, first = 1;
  fmode_t u, result = fmode;
/*
 * Read an arg. If there are none, reset to default & display settings
 */
  for (;;)
  {
/* Read mnemonic or octal */
    if (scrdtk(1, (uint8_t *)ubuf, 12, oldcom))
    {
      perror("setqmode - scrdtk");
      fputs("\r", stderr);
    bad_arg:
      fprintf(stderr, "Bad argument: %s", ubuf);
      return false;
    }                              /* if(scrdtk(1,(uint8_t *)ubuf,12,oldcom)) */
    switch (oldcom->toktyp)
    {
      case nultok:
        if (!first)                /* Null token at start = reset */
          GIVE_UP;                 /* drop thru otherwise */
      case eoltok:
        if (first)
        {
          first = 0;               /* Not 1st time next time */
          if (curmac >= 0 || ysno5a("Reset modes to default [no]", A5DNO))
          {
            result = dfltmode;     /* Reset to defaults */
            if (curmac < 0)
              show_current(result); /* Display settings */
          }                        /* if (curmac >= 0 || ysno5a(... */
          continue;                /* Keep reading */
        }                          /* if (first) */
        else                       /* Eol, line not empty */
        {
          fmode = result;          /* End of command */
          return true;             /* Finished command */
        }                          /* if (first) else */
      case nortok:
        break;                     /* normal tokens dealt with below */
    }                              /* switch(oldcom->toktyp) */
    first = 0;                     /* Not 1st time next time */
/* We have a regular token. If it is signed, it must be a mnemonic... */
    if (oldcom->plusf || oldcom->minusf)
    {
      if (oldcom->toklen == 1)     /* Unary + or - */
      {
        show_current(result);
        continue;
      }                            /* if(oldcom->toklen==1) */
      octok = 0;                   /* No more octal args allowed */
      switch (ubuf[1])
      {
        case 'D':                  /* DOS */
          if (oldcom->toklen == 2)
            u = DOS_READ_BIT | DOS_WRITE_BIT;
          else
            switch (ubuf[2])
            {
              case 'R':
                u = DOS_READ_BIT;
                break;
              case 'W':
                u = DOS_WRITE_BIT;
                break;
              default:
                GIVE_UP;
            }                      /* else switch(ubuf[2]) */
          break;

/* fm +t forces fm -l */
        case 'T':                  /* Tab expand / compress */
          if (oldcom->plusf && (result & FM_PLUS_L_BIT))
          {
            fputs("Forcing fm -l\r\n", stdout);
            result &= ~FM_PLUS_L_BIT;
          }                  /* if(oldcom->plusf && (result & FM_PLUS_L_BIT)) */
          if (oldcom->toklen == 2)
            u = TAB_READ_BIT | TAB_WRITE_BIT;
          else
            switch (ubuf[2])
            {
              case 'R':
                u = TAB_READ_BIT;
                break;
              case 'W':
                u = TAB_WRITE_BIT;
                break;
              default:
                GIVE_UP;
            }                      /* else switch(ubuf[2]) */
          break;
        case 'S':                  /* Leave trailing Spaces */
          u = FM_PLUS_S_BIT;
          break;
        case '*':                  /* Show '^' as "^*" */
          u = FM_PLUS_STAR_BIT;
          break;
        case 'Q':                  /* Q in macro quits program */
          u = FM_PLUS_Q_BIT;
          break;
        case '#':                  /* Q $-+# moves argptr */
          u = FM_PLUS_HASH_BIT;
          break;
        case 'F':                  /* Fixed-length edit */
          u = FM_PLUS_F_BIT;
          break;
        case 'V':                  /* Verbose always */
          u = FM_PLUS_V_BIT;
          break;
        case 'M':                  /* Use mmap for file input */
          u = FM_PLUS_M_BIT;
          break;
        case 'R':                  /* Locate searches backwards */
          u = FM_PLUS_R_BIT;
          break;
        case 'E':                  /* Deferred read of mmapping files */
          u = FM_PLUS_E_BIT;
          break;
        case 'N':                  /* E-eNter mmap's also */
          u = FM_PLUS_N_BIT;
          break;

/* fm +l forces fm -t */
        case 'L':                  /* Expand/compress Leading tabs */
          u = FM_PLUS_L_BIT;
          if (oldcom->plusf && (result & (TAB_READ_BIT | TAB_WRITE_BIT)))
          {
            fputs("Forcing fm -t\r\n", stdout);
            result &= ~(TAB_READ_BIT | TAB_WRITE_BIT);
          } /* if(oldcom->plusf && (result & (TAB_READ_BIT | TAB_WRITE_BIT))) */
          break;

        case 'H':                  /* Half-screen scroll */
          u = FM_PLUS_H_BIT;
          break;

        case 'I':                  /* Show Interpreted ALU opcodes */
          u = FM_PLUS_I_BIT;
          break;

        case 'W':                  /* Warn when overwriting nonzero memory  */
          u = FM_PLUS_W_BIT;
          break;

        case 'A':                  /* ^G & ^NG match Any whitespace */
          u = FM_PLUS_A_BIT;
          break;

        case 'X':            /* eXclusive L & FL: find next line not matching */
          u = FM_PLUS_X_BIT;
          break;

        case 'Y':                  /* Store tab for single-space on boundary */
          u = FM_PLUS_Y_BIT;
          break;

        case 'G':                  /* L & Y do reGexp matching */
          u = FM_PLUS_G_BIT;
          break;

        case '8':                 /* Log macro & usefile as well as keybd i/p */
          u = FM_PLUS_8_BIT;
          break;

        case '9':                  /* Log '^' as '^*' */
          u = FM_PLUS_9_BIT;
          break;

        case '0':                  /* Log '^' as '^*' */
          u = FM_PLUS_0_BIT;
          break;

        default:
          printf("Warning - unrecognised %s ignored\r\n", ubuf);
          continue;
      }                            /* switch(ubuf[1]) */
      if (oldcom->minusf)
        result &= ~u;
      else
        result |= u;
      continue;
    }                              /* if(oldcom->plusf||oldcom->minusf) */
    else if (!(oldcom->octok))
      GIVE_UP;                     /* J not octal value */
    if (!octok)
    {
      fputs("Octal arg not allowed after symbolics", stdout);
      return false;
    }                              /* if(!octok) */
    result = oldcom->octval;
  }                                /* for(;;) */
}

/* ****************************** show_current ****************************** */

static void
show_current(fmode_t mode)
{
  int i;
  fmode_t mask;
  const char *const modestring = "\\\\\\\\s*q#fvmrenlhiwaxyg890";
  const char *p;

  printf("Current mode is %" PRIofmode ":-\r\n", mode);
  switch (i = mode & (DOS_READ_BIT | DOS_WRITE_BIT))
  {
    case 0:
    case (DOS_READ_BIT | DOS_WRITE_BIT):
      putchar(i ? '+' : '-');
      putchar('d');
      break;
    case DOS_READ_BIT:
      fputs("+dr, -dw", stdout);
      break;
    case DOS_WRITE_BIT:
      fputs("-dr, +dw", stdout);
      break;
  }                     /* switch (i = mode & (DOS_READ_BIT | DOS_WRITE_BIT)) */
  fputs(", ", stdout);
  switch (i = mode & (TAB_READ_BIT | TAB_WRITE_BIT))
  {
    case 0:
    case (TAB_READ_BIT | TAB_WRITE_BIT):
      putchar(i ? '+' : '-');
      putchar('t');
      break;
    case TAB_READ_BIT:
      fputs("+tr, -tw", stdout);
      break;
    case TAB_WRITE_BIT:
      fputs("-tr, +tw", stdout);
      break;
  }                     /* switch (i = mode & (TAB_READ_BIT | TAB_WRITE_BIT)) */
  for (i = strlen(modestring), p = modestring, mask = 1; i > 0;
    i--, p++, mask <<= 1)
  {
    if (*p == '\\')                /* Placeholder */
      continue;
    printf(", %c%c", mode & mask ? '+' : '-', *p);
  }
  fputs("\r\n", stdout);
  switch (mode & FN_CMD_BITS)
  {
    case 0:
      fputs("verbose", stdout);
      break;
    case FB_CMD_BIT:
      fputs("brief", stdout);
      break;
    case FN_CMD_BITS:
      fputs("none", stdout);
      break;
  }                                /* switch (mode & FN_CMD_BITS) */
  fputs(", ", stdout);
  if (!INDENT)
    fputs("not ", stdout);
  fputs("indenting", stdout);
  fputs(", case-", stdout);
  if (CASDEP)
    putchar('d');
  else
    fputs("ind", stdout);
  printf("ependent L&Y, FF o%s", NOWRAP ? "n" : "ff");
  printf(", ALU stores %s-position tabs", STORE_FILE_POS ? "file" : "cursor");
  fputs(".\r\n", stdout);
}                                  /* static void show_current(void) */
