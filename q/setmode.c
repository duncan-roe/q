/* S E T M O D E
 *
 * Copyright (C) 1994,1995 Duncan Roe & Associates P/L
 * Copyright (C) 2003,2012,2014,2017-2019 Duncan Roe
 *
 * This routine manipulates the fmode bit settings
 */
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "prototypes.h"
#include "edmast.h"
#include "fmode.h"
bool
setmode()
{
  int octok = 1, first = 1;
  unsigned long u, result = fmode;
  char c;
  int i;
/*
 * Read an arg. If there are none, reset to default & display settings
 */
  for (;;)
  {
/* Read mnemonic or octal */
    if (scrdtk(1, (uint8_t *)ubuf, 12, oldcom))
    {
      perror("setmode - scrdtk");
      putchar('\r');
    bad_arg:
      printf("Bad argument: %s", ubuf);
      return false;
    }                              /* if(scrdtk(1,(uint8_t *)ubuf,12,oldcom)) */
    switch (oldcom->toktyp)
    {
      case nultok:
        if (!first)
          goto bad_arg;            /* Null token at start = reset */
      case eoltok:
        if (first)
        {
          first = 0;               /* Not 1st time next time */
          if (ysno5a("Reset modes to default [no]", A5DNO))
          {
            result = dfltmode;     /* Reset to defaults */
            fmode = result;        /* So indent &c display right */
            goto show_current;     /* Display settings */
          }                /* if(ysno5a("Reset modes to default [no]",A5DNO)) */
          continue;                /* Keep reading */
        }                          /* if(first) */
        else                       /* Eol, line not empty */
        {
          fmode = result;          /* End of command */
          return true;             /* Finished command */
        }                          /* if(first) else */
      case nortok:
        break;                     /* normal tokens dealt with below */
    }                              /* switch(oldcom->toktyp) */
    first = 0;                     /* Not 1st time next time */
/* We have a regular token. If it is signed, it must be a mnemonic... */
    if (oldcom->plusf || oldcom->minusf)
    {
      if (oldcom->toklen == 1)     /* Unary + or - */
      {
      show_current:
        printf("Current mode is %lo:-\r\n", result);
        switch (i = (int)result & 03)
        {
          case 0:
          case 3:
            putchar(i ? '+' : '-');
            putchar('d');
            break;
          case 1:
            (void)write(1, "+dr, -dw", 8);
            break;
          case 2:
            (void)write(1, "-dr, +dw", 8);
            break;
        }                          /* switch((int)result&03) */
        (void)write(1, ", ", 2);
        switch (i = (int)result & 014)
        {
          case 0:
          case 014:
            putchar(i ? '+' : '-');
            putchar('t');
            break;
          case 04:
            (void)write(1, "+tr, -tw", 8);
            break;
          case 010:
            (void)write(1, "-tr, +tw", 8);
            break;
        }                          /* switch((int)result&014) */
        (void)write(1, ", ", 2);
        c = result & 020 ? '+' : '-';
        putchar(c);
        putchar('s');
        (void)write(1, ", ", 2);
        c = result & 040 ? '+' : '-';
        putchar(c);
        putchar('*');
        (void)write(1, ", ", 2);
        c = result & 0100 ? '+' : '-';
        putchar(c);
        putchar('q');
        (void)write(1, ", ", 2);
        c = result & 0200 ? '+' : '-';
        putchar(c);
        putchar('#');
        (void)write(1, ", ", 2);
        c = result & 0400 ? '+' : '-';
        putchar(c);
        putchar('f');
        (void)write(1, ", ", 2);
        c = result & 01000 ? '+' : '-';
        putchar(c);
        putchar('v');
        (void)write(1, ", ", 2);
        c = result & 02000 ? '+' : '-';
        putchar(c);
        putchar('m');
        (void)write(1, ", ", 2);
        c = result & 04000 ? '+' : '-';
        putchar(c);
        putchar('r');
        (void)write(1, ", ", 2);
        c = result & 010000 ? '+' : '-';
        putchar(c);
        putchar('e');
        (void)write(1, ", ", 2);
        c = result & 020000 ? '+' : '-';
        putchar(c);
        putchar('n');
        (void)write(1, ", ", 2);
        c = result & 040000 ? '+' : '-';
        putchar(c);
        putchar('l');
        (void)write(1, ", ", 2);
        c = result & 0100000 ? '+' : '-';
        putchar(c);
        putchar('h');
        (void)write(1, ", ", 2);
        c = result & 0200000 ? '+' : '-';
        putchar(c);
        putchar('i');
        (void)write(1, ", ", 2);
        c = result & 0400000 ? '+' : '-';
        putchar(c);
        putchar('w');
        (void)write(1, ", ", 2);
        c = result & 01000000 ? '+' : '-';
        putchar(c);
        putchar('a');
        (void)write(1, "\r\n", 2);
        switch ((int)(result >> 30 & 3))
        {
          case 0:
            (void)write(1, "verbose", 7);
            break;
          case 1:
            (void)write(1, "brief", 5);
            break;
          case 3:
            (void)write(1, "none", 4);
            break;
        }                          /* switch((int)(result>>30&3)) */
        (void)write(1, ", ", 2);
        if (!INDENT)
          (void)write(1, "not ", 4);
        (void)write(1, "indenting", 9);
        (void)write(1, ", case-", 7);
        if (CASDEP)
          putchar('d');
        else
          (void)write(1, "ind", 3);
        printf("ependent L&Y, FF o%s", NOWRAP ? "n" : "ff");
        printf(", ALU stores %s-position tabs",
          STORE_FILE_POS ? "file" : "cursor");
        (void)write(1, ".\r\n", 3);
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
                goto bad_arg;
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
                goto bad_arg;
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
      goto bad_arg;                /* J not octal value */
    if (!octok)
    {
      fputs("Octal arg not allowed after symbolics", stdout);
      return false;
    }                              /* if(!octok) */
    result = oldcom->octval;
  }                                /* for(;;) */
}
