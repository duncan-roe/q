/* S E T M O D E
 *
 * Copyright (C) 1994,1995 Duncan Roe & Associates P/L
 * Copyright (C) 2003,2012,2014,2017-2019 Duncan Roe
 *
 * This routine manipulates the fmode bit settings */

/* Headers */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "prototypes.h"
#include "macros.h"
#include "edmast.h"
#include "fmode.h"

/* Macros */

#define GIVE_UP goto bad_arg

/* Static prototypes */

static void show_current(unsigned long mode);

/* ******************************** setqmode ******************************** */

bool
setqmode()
{
  int octok = 1, first = 1;
  unsigned long u, result = fmode;
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
          }                /* if(ysno5a("Reset modes to default [no]",A5DNO)) */
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
            u = 3;                 /* Read & write */
          else
            switch (ubuf[2])
            {
              case 'R':
                u = 1;
                break;
              case 'W':
                u = 2;
                break;
              default:
                GIVE_UP;
            }                      /* else switch(ubuf[2]) */
          break;

/* fm +t forces fm -l */
        case 'T':                  /* Tab expand / compress */
          if (oldcom->plusf && (result & 040000))
          {
            fputs("Forcing fm -l\r\n", stdout);
            result &= ~040000;
          }                        /* if(oldcom->plusf&&(result&040000)) */
          if (oldcom->toklen == 2)
            u = 014;               /* Read & write */
          else
            switch (ubuf[2])
            {
              case 'R':
                u = 04;
                break;
              case 'W':
                u = 010;
                break;
              default:
                GIVE_UP;
            }                      /* else switch(ubuf[2]) */
          break;
        case 'S':                  /* Leave trailing Spaces */
          u = 020;
          break;
        case '*':                  /* Show '^' as "^*" */
          u = 040;
          break;
        case 'Q':                  /* Q in macro quits program */
          u = 0100;
          break;
        case '#':                  /* Q $-+# moves argptr */
          u = 0200;
          break;
        case 'F':                  /* Fixed-length edit */
          u = 0400;
          break;
        case 'V':                  /* Verbose always */
          u = 01000;
          break;
        case 'M':                  /* Use mmap for file input */
          u = 02000;
          break;
        case 'R':                  /* Locate searches backwards */
          u = 04000;
          break;
        case 'E':                  /* Deferred read of mmapping files */
          u = 010000;
          break;
        case 'N':                  /* E-eNter mmap's also */
          u = 020000;
          break;

/* fm +l forces fm -t */
        case 'L':                  /* Expand/compress Leading tabs */
          u = 040000;
          if (oldcom->plusf && (result & 014))
          {
            fputs("Forcing fm -t\r\n", stdout);
            result &= ~014;
          }                        /* if(oldcom->plusf&&(result&014)) */
          break;

        case 'H':                  /* Half-screen scroll */
          u = 0100000;
          break;

        case 'I':                  /* Show Interpreted ALU opcodes */
          u = 0200000;
          break;

        case 'W':                  /* Warn when overwriting nonzero memory  */
          u = 0400000;
          break;

        case 'A':                  /* ^G & ^NG match Any whitespace */
          u = 01000000;
          break;

        case 'X':            /* eXclusive L & FL: find next line not matching */
          u = 02000000;
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
show_current(unsigned long mode)
{
  char c;
  int i;

  printf("Current mode is %lo:-\r\n", mode);
  switch (i = (int)mode & 03)
  {
    case 0:
    case 3:
      putchar(i ? '+' : '-');
      putchar('d');
      break;
    case 1:
      fputs("+dr, -dw", stdout);
      break;
    case 2:
      fputs("-dr, +dw", stdout);
      break;
  }                                /* switch((int)mode&03) */
  fputs(", ", stdout);
  switch (i = (int)mode & 014)
  {
    case 0:
    case 014:
      putchar(i ? '+' : '-');
      putchar('t');
      break;
    case 04:
      fputs("+tr, -tw", stdout);
      break;
    case 010:
      fputs("-tr, +tw", stdout);
      break;
  }                                /* switch((int)mode&014) */
  fputs(", ", stdout);
  c = mode & 020 ? '+' : '-';
  putchar(c);
  putchar('s');
  fputs(", ", stdout);
  c = mode & 040 ? '+' : '-';
  putchar(c);
  putchar('*');
  fputs(", ", stdout);
  c = mode & 0100 ? '+' : '-';
  putchar(c);
  putchar('q');
  fputs(", ", stdout);
  c = mode & 0200 ? '+' : '-';
  putchar(c);
  putchar('#');
  fputs(", ", stdout);
  c = mode & 0400 ? '+' : '-';
  putchar(c);
  putchar('f');
  fputs(", ", stdout);
  c = mode & 01000 ? '+' : '-';
  putchar(c);
  putchar('v');
  fputs(", ", stdout);
  c = mode & 02000 ? '+' : '-';
  putchar(c);
  putchar('m');
  fputs(", ", stdout);
  c = mode & 04000 ? '+' : '-';
  putchar(c);
  putchar('r');
  fputs(", ", stdout);
  c = mode & 010000 ? '+' : '-';
  putchar(c);
  putchar('e');
  fputs(", ", stdout);
  c = mode & 020000 ? '+' : '-';
  putchar(c);
  putchar('n');
  fputs(", ", stdout);
  c = mode & 040000 ? '+' : '-';
  putchar(c);
  putchar('l');
  fputs(", ", stdout);
  c = mode & 0100000 ? '+' : '-';
  putchar(c);
  putchar('h');
  fputs(", ", stdout);
  c = mode & 0200000 ? '+' : '-';
  putchar(c);
  putchar('i');
  fputs(", ", stdout);
  c = mode & 0400000 ? '+' : '-';
  putchar(c);
  putchar('w');
  fputs(", ", stdout);
  c = mode & 01000000 ? '+' : '-';
  putchar(c);
  putchar('a');
  fputs(", ", stdout);
  c = mode & 02000000 ? '+' : '-';
  putchar(c);
  putchar('x');
  fputs("\r\n", stdout);
  switch ((int)(mode >> 30 & 3))
  {
    case 0:
      fputs("verbose", stdout);
      break;
    case 1:
      fputs("brief", stdout);
      break;
    case 3:
      fputs("none", stdout);
      break;
  }                                /* switch((int)(mode>>30&3)) */
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
