/* Q
 *
 *
 * Copyright (C) 1981 D. C. Roe
 * Copyright (C) 2002,2007 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * This is the Master Q module
 *
 *
 */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <memory.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/mman.h>
#ifdef ANSI5
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#endif
#include "alledit.h"
#include "edmast.h"
#include "macros.h"
#include "termio5.hl"
#include "c1in.h"

/* Macros */

#define ERR1025(x) do {printf("%s", (x)); goto p1025;} while (0)
/*  */
typedef enum q_yesno
{
  Q_YES,
  Q_NO,
  Q_MAYBE,
  Q_UNREC
} q_yesno;                         /* typedef enum q_yesno */
/* */
long timlst;
unsigned char fxtabl[128];
int tbstat;

static char *help_dir;
static char *help_cmd;
/* */
/* ********************************** eolok ********************************* */

/* Check no extra params */

static bool
eolok(void)
{
  scrdtk(1, (unsigned char *)NULL, 0, oldcom);
  if (oldcom->toktyp == eoltok)    /* OK */
    return true;
  printf("%s", "Too many arguments for this command");
  return false;
}                                  /* static bool eolok(void) */

/* ******************************** yes_or_no ******************************* */

/* Parse rest of line for yes / no indication */

static q_yesno
get_answer(void)
{
  if (scrdtk(1, (unsigned char *)buf, 6, oldcom))
  {
    printf("%s. (scrdtk)", strerror(errno));
    return Q_UNREC;
  }                        /* if (scrdtk(1, (unsigned char *)buf, 6, oldcom)) */
  if (oldcom->toktyp == eoltok)
    return Q_MAYBE;
  if (oldcom->toktyp != nortok)    /* I.e. null token */
  {
    printf("%s", "Bad parameter for command");
    return Q_UNREC;
  }                                /* if (oldcom->toktyp != nortok) */
  if (!eolok())
    return Q_UNREC;
  switch (buf[0] & 0337)
  {
    case 'O':
      if ((buf[1] & 0337) == 'N')
        return Q_YES;
      if ((buf[1] & 0337) == 'F')
        return Q_NO;
      break;
    case 'Y':
    case 'T':
      return Q_YES;
    case 'F':
    case 'N':
      return Q_NO;
  }
  printf("%s", "Parameter not recognised");
  return Q_UNREC;
}                                  /* get_answer(void) */

/* ********************************** main ********************************** */
int
main(int xargc, char **xargv)
{
  struct stat statbuf;
  long timnow;
  struct tms tloc;
  char tmfile[PTHSIZ];             /* Name of .TM file */
  char tmtree[PTHSIZ];             /* Name of HELP file */
  char oldkey[3];                  /* 1st param to FX command */
  char xkey[2], newkey[3];         /* 2nd param to FX command */
  char *r;                         /* Scratch */
  char ndel[33];                   /* Table of non-delimiters FL & FY */
  int newl = 0;                    /* Label to go if Nl (MOD, INS, AP) */
  int numok;                     /* Label to go if # of lines OK & last param */
  int rtn = 0;                     /* Return from INS/MOD/APPE common code */
  int nofil;                       /* Label for no filename (BSEW) */
  int i, j, k = 0, l, m, n, dummy; /* Scratch - I most so */
  int nonum;                       /* Label if no # of lines */
  int rdwr = 0;                    /* Mode for file opens */
  int oldlen = 0;                  /* Length of OLDSTR */
  int newlen = 0;                  /* Length of NEWSTR */
  int ydiff = 0;                   /* OLDLEN-NEWLEN */
  int yposn;                       /* How far we are along a line (Y) */
  int locpos;                   /* Position in line of string found by LOCATE */
  int minlen = 0;                  /* Min line length (L&Y) */
  int first = 0;                   /* First pos to search (L&Y) */
  int last = 0;                    /* Last pos to search (L&Y) */ ;
  bool revrse;                     /* Locate backwards */
  int savtok;                      /* Saved token type */
  int colonline;                   /* Line number from <file>:<line> */
/* */
  long count = 0;                  /* Returned by GETNUM seq */
/* For those commands that take 2 #'s of lines */
  long count2 = 0;
  long i4, j4 = 0, k4 = 0;         /* Scratch */
  long savpos = 0;                 /* Remembered pointer during SlongB & Y */
  long revpos;                     /* Remembered pointer during backwards L */
  long wrtnum = 0;                 /* # of lines to write */
  long xcount = 0;                 /* For V-View */
/* */
  int tmode;                       /* Mode of file */
  int towner;                      /* Owner of file */
  int tgroup;                      /* Group of file */
  int tmask = 0;                   /* Current umask */
/* */
  char oldstr[Q_BUFSIZ], newstr[Q_BUFSIZ]; /* YCHANGEALL. !!AMENDED USAGE!! */
  unsigned char *p, *q;            /* Scratch */
  char *colonpos;                  /* Pos'n of ":" in q filename */
/* */
  bool splt;                       /* Last line ended ^T (for MODIFY) */
  bool bspar = false;              /* BACKUP/SAVE had a param */
  bool logtmp = false, lgtmp2 = false, lgtmp3 = false, lgtmp4; /* Scratch */
  bool repos = false;              /* We are R-REPOSITION (not C-COPY) */
  bool linmod;                     /* This line modified (Y) */
  bool tokens = false;             /* Token search yes/no */
  bool dontrc = false;             /* Don't do .qrc */
  bool errflg = false;             /* Illegal switch seen */
  short bootq = 0;                 /* Bootstrap q into 1st file */
  bool fullv = false;              /* Fulll VIEW wanted */
  scrbuf5 b1, b2, b3, b4;          /* 2 line & 2 command buffers */
  q_yesno answer;
/*
 * Initial Tasks
 */
  argc = xargc;                    /* Xfer invocation arg to common */
  argv = xargv;                    /* Xfer invocation arg to common */
  dfltmode = 012045;               /* +e +m +* +tr +dr */
/* Pick up any option arguments and set bootq if more args follow */
  while ((i = getopt(argc, argv, "bdemnt")) != -1)
    switch (i)
    {
      case 'n':
        dontrc = true;
        break;
      case 'b':
        binary = true;
        dfltmode |= 0400;          /* +f */
        break;
      case 'm':
        dfltmode ^= 02000;         /* m */
        break;
      case 'e':
        dfltmode ^= 010000;        /* e */
        break;
      case 'd':
        dfltmode ^= 1;             /* dr */
        break;
      case 't':
        dfltmode ^= 4;             /* tr */
        break;
      case '?':
        errflg = true;
    }                              /* switch(i) */
  if (errflg)
  {
    (void)fprintf(stderr, "Usage: q [-bdemnt] [+<n> file] [files]\n");
    return 1;
  }
  if (!(sh = getenv("SHELL")))
    sh = "/bin/sh";
  if (!(help_dir = getenv("Q_HELP_DIR")))
    help_dir = "/usr/local/lib/q";
  if (!(macro_dir = getenv("Q_MACRO_DIR")))
    macro_dir = help_dir;
  if (!(help_cmd = getenv("Q_HELP_CMD")))
    help_cmd = "more";
  fmode = dfltmode;                /* Assert defaults */
  if (optind < argc)
    bootq = 1;
  else
    argno = -1;                    /* No filenames */
  signal(SIGINT, quthan);
#ifdef SIGWINCH
  signal(SIGWINCH, winchhan);
#endif
  cntrlc = false;                  /* Not yet seen ^C */
  ndel[0] = '\0';                  /* No FT commands yet */
  for (i = 127; i >= 0; i--)
    fxtabl[i] = i;                 /* No FX commands yet */
  oldcom = &b1;
  newcom = &b2;
  curr = &b3;
  prev = &b4;
  init5();                         /* Set half duplex &c */
/*
 * Set up Screenedit buffers
 */
  oldcom->bcurs = 0;
  oldcom->bchars = 0;              /* Initialise OLDCOM only this once */
  prev->bchars = 0;
  prev->bcurs = 0;                 /* Initialise PREV only this once */
  oldcom->bmxch = BUFMAX;
  newcom->bmxch = BUFMAX;
  curr->bmxch = BUFMAX;
  prev->bmxch = BUFMAX;
/* */
  finitl();                        /* Initialise workfile system */
  sinitl();                        /* Initialise screen system */
  newlin();                        /* Screen displaying blanks */
  splt = false;                    /* Not splitting a line initially */
  mods = false;                    /* No mods to i/p file yet */
  pcnta[0] = 0;                    /* No default filename yet */
  locpos = 0;                      /* Stays at 1 except after a LOCATE */
  locerr = false;                  /* 1st error might not be LOCATE */
  noRereadIfMacro = false;
  forych = false;                  /* Start off VERBOSE */
  puts("Type H for help\r");
  if (size5)
    printf("Noted screen dimensions %hd x %hd\r\n", col5, row5);
  orig_stdout = -1;                /* Haven't dup'd stdout */
/*
 * Initially INDENT switched OFF
 */
  ndntch = 0;                      /* For when it's first switched on */
  tbstat = -1;                     /* Xlation table not set up */
  stdinstkptr = -1;                /* No U-use file */
/*
 * Must be last initial task: Use .qrc if it exists here or in $HOME
 */
  if (dontrc)
    goto p1004;                    /* Skip initial tasks if requested */

/* Forge a u-use: push current stdin */
  stdinstkptr = 0;
  do
  {
    stdinstack[stdinstkptr] = dup(0);
  }
  while (stdinstack[stdinstkptr] == -1 && errno == EINTR);
  if (stdinstack[stdinstkptr] == -1)
  {
    fprintf(stderr, "\r\n%s. (dup(0))\r\n", strerror(errno));
    refrsh(NULL);
    stdinstkptr--;
    goto p1004;                    /* Don't try to open .qrc */
  }                                /* if (stdinstack[stdinstkptr] == -1) */

  do
    i = close(0);
  while (i == -1 && errno == EINTR);

/* Try for .qrc or ~/.qrc */
  logtmp = true;                   /* retry on failure */
  strcpy(buf, ".qrc");
retry_qrc:do
    i = open(buf, O_RDONLY);
  while (i == -1 && errno == EINTR);
  if (i == -1)
  {
    if (logtmp)
    {
      logtmp = false;
      strcpy(buf, "~/.qrc");
      tildexpn(buf);
      goto retry_qrc;
    }                              /* if (logtmp) */
    pop_stdin();
  }                                /* if (i == -1) */
  else
  {
    if (i)
    {
      do
        j = dup2(i, 0);
      while (j == -1 && errno == EINTR);
      if (j == -1)
      {
        fprintf(stderr, "\r\n%s. (dup2(%d, 0))\r\n", strerror(errno), i);
        fprintf(stderr, "Serious problem - new stdin opened on funit %d\r\n",
          i);
        refrsh(NULL);
        pop_stdin();
        goto p1004;
      }                            /* if (j == -1) */
      else
      {
        do
          j = close(i);
        while (j == -1 && errno == EINTR);
      }                            /* if (j == -1) else */
    }                              /* if (i) */
    duplx5(true);                  /* Assert XOFF recognition */
    printf("> u %s\r\n", buf);     /* Simulate a command */
  }                                /* if (i == -1) else */
/*
 * End initial tasks, except possible bootstrap to 1st file arg
 *
 *
 * Main command reading loop
 */
p1004:
  if (bootq && !USING_FILE && curmac < 0)
  {

/* Q-quit into the first file on the command line. Pagers (e.g. less) may
 * precede this with +<line#> so deal with this too. Cause command to be
 * actioned & disoplayed by setting up oldcom, also set up "verb" since sccmd is
 * not being called */

    switch (bootq)                 /* 1;initial, 2:goto, 3:view */
    {

/* assume if the first arg starts "+" and there is at least 1 more arg then the
 * first arg is a line number */

      case 1:
        if (**(argv + optind) == '+' && strlen(*(argv + optind)) > 1 &&
          argc - optind >= 2)
        {
          optind++;                /* "hide" +# arg */
          bootq = 2;               /* Have a +# arg */
        }                          /* if(**(argv+optind)=='+'&&... */
        else
          bootq = 0;
        oldcom->bchars = strlen(*(argv + optind)) + 2;
        strncpy((char *)&oldcom->bdata[2], *(argv + optind),
          (size_t)oldcom->bchars);
        oldcom->bdata[0] = 'q';
        verb = 'Q';
        break;
      case 2:
        oldcom->bchars = strlen(*(argv + optind - 1)) + 1;
        strncpy((char *)&oldcom->bdata[2], *(argv + optind - 1) + 1,
          (size_t)oldcom->bchars);
        oldcom->bdata[0] = 'g';
      case2part2:
        bootq = 3;
        verb = 'G';
        break;
      case 4:
/* Don't rely on integer return from sprintf: BSD returns buf addr */
        sprintf((char *)oldcom->bdata, "g %d", colonline);
        oldcom->bchars = strlen((char *)oldcom->bdata);
        goto case2part2;
      case 3:
        bootq = 0;
        oldcom->bchars = 1;
        oldcom->bdata[0] = 'v';
        verb = 'V';
    }                              /* switch(bootq) */
    oldcom->bcurs = verb != 'V' ? 2 : 1;
    oldcom->bdata[1] = verb != 'V' ? SPACE : 0;
    printf("> %s\r\n", oldcom->bdata);
  }
  else
    sccmnd();                      /* Read a command; set VERB */
p1201:
  if (cntrlc)                      /* There has been a ^C or BRK */
  {
    cntrlc = false;                /* Reset flag */
    if (USING_FILE || curmac >= 0) /* If in macro, force an error */
    {
      (void)write(1, "Keyboard interrupt", 18);
      goto p1025;
    }
  }                                /* Else ignore the quit */
  revrse = false;                  /* Guarantee validity if true */
  switch (verb)
  {
    case 'A':
      goto p1005;
    case 'B':
      goto p1006;
    case 'C':
      goto p1007;
    case 'D':
      goto p1008;
    case 'E':
      goto p1009;
    case 'G':
      goto p1010;
    case 'H':
      goto p1011;
    case 'I':
      goto p1012;
    case 'J':
      goto p1013;
    case 'L':
      goto p1014;
    case 'M':
      goto p1015;
    case 'P':
      goto p1016;
    case 'Q':
      goto p1017;
    case 'q':
      goto p1017;
    case 'R':
      goto p1018;
    case 'S':
      goto p1006;                  /* Same as 'B' */
    case 'U':
      goto p1020;
    case 'V':
      goto p1021;
    case 'W':
      goto p1022;
    case 'X':
      goto p1023;
    case 'T':
      goto p1101;
    case 'Z':
      goto p1102;
    case 'O':
      goto p1401;
    case 'N':
      goto p1501;
    case 'K':
      goto p1525;
    case 'Y':
      goto p1607;
    case 'b':
      goto p1701;
    case 'v':
      goto p1702;
    case 'n':
      goto p1703;
    case 'o':
      goto p1707;
    case '!':
      goto p1801;
    case 'x':
      goto p1904;
    case 'l':
      goto p1014;                  /* Same as L */
    case 'y':
      goto p2002;
    case 't':
      goto p2003;
    case 'f':
      goto p2013;
    case 'c':
      goto p2016;
    case 'm':                      /* "FM"ode */
      if (setmode())
        goto p1004;
/* Get Yes / No (no default) */
      goto p1025;
    case 'i':                      /* "FI'mmediate macro */
      if (scrdtk(4, (unsigned char *)buf, BUFMAX, oldcom))
      {
        perror("SCRDTK of macro text");
        putchar('\r');
        printf("Unexpected error");
        goto p1025;
      }
/* LATER Decide which macro this will be. */
/* LATER Some nesting of FI macros is allowed, */
/* LATER just in cae anyone ever wants it. */
      verb = FIRST_INLINE_MACRO;
      if (newmac2(strlen(buf)) <= 0)
        goto p1025;
      curmac = verb;
      mcposn = 0;
      goto p1004;
    case 'd':                      /* "FD"evnull */
      switch (get_answer())
      {
        case Q_UNREC:
          goto p1025;
        case Q_MAYBE:
        case Q_NO:
          restore_stdout();
          break;
        case Q_YES:
          if (USING_FILE)
          {
            devnull_stdout();
            break;
          }                        /* if (USING_FILE) */
          else
          {
            printf("%s", "fd y is not available from the keyboard");
            goto p1025;
          }                        /* if (USING_FILE) else */
      }                            /* switch (get_answer()) */
      goto p1004;
  }
  (void)write(1, "unknown command", 15); /* Dropped out of switch */
p1025:
  if (bootq > 1)                   /* Bad cmd line arg after "+ something" */
  {
    bootq = 0;
    optind--;
  }                                /* if(bootq>1) */
  rerdcm();
  goto p1201;
/* ******************************************************************
 *                        Start line modifiers
 * ******************************************************************
 *
 * A - Append
 */
p1005:
  if (!eolok())
    goto p1025;                    /* Reread command */
  if (deferd)                      /* File not all read in yet */
    dfread(LONG_MAX, NULL);
  setptr(lintot + 1);              /* Ptr after last line in file */
/*
 * Code used by INSERT and APPEND
 */
p1034:modify = false;
  lstvld = false;                  /* Previous line not valid */
  newl = 1026;
p1026:
  curr->bchars = 0;
  curr->bcurs = 0;                 /* Set up new empty line */
/*
 * Code used by INSERT, APPEND, MODIFY
 */
  sprmpt(ptrpos);                  /* PROMPT = # of new line */
/*
 * See if ^C has been typed - get user out of INSERT/APPEND/MODIFY
 * if it has. (Particularly user 1, who can't input an ESC)
 */
p1027:if (cntrlc)
    goto p1901;                    /* J ^C keyed */
  scrdit(curr, prev, (char *)prmpt, pchrs, 0); /* Edit the line */
  lgtmp3 = curmac < 0 || !BRIEF;   /* Do a DISPLY if true */
  switch (verb)                    /* Check EOL type */
  {
    case 'J':
      goto p1029;
    case '[':
      goto p1301;
    case 'T':
      goto p1030;
  }
  (void)write(1, "Internal error - EOL char not recognised", 40);
  newlin();
  goto p1004;
p1029:rtn = newl;
p1033:lgtmp4 = (modify || splt);   /* We are not inserting */
  if (lgtmp4 && modlin)
    delete(0);                     /* Delete CHANGED existing line */
  splt = false;                    /* Not a split this time */
  if (lgtmp3)
    disply(prev, 0);               /* Display final line */
  if ((!lgtmp4) || modlin)
    inslin(prev);                  /* Insert changed or new line */
  goto asg2rtn;                    /* ^M & ^T part here */
p1030:
  rtn = 1032;
  goto p1033;                      /* Display & update file */
p1032:splt = true;                 /* Force a delete next time */
  inslin(curr);                    /* In case ESC next time */
  sprmpt(ptrpos - 1);
  goto p1027;                      /* New line for all 3 (A,I,M) */

/* P1301 - ESC. If we were changing an existing line, display the original */

p1301:
  if (modify || splt)
  {
    if (lgtmp3)                    /* Display req'd */
    {
      setptr(ptrpos - 1);
      rdlin(prev, 0);
      disply(prev, 0);
    }                              /* if(lgtmp3) */
    splt = false;
  }                                /* if(modify||splt) */
  if (revrse)                      /* "L"ocate backwards */
    setptr(revpos);
  goto p1004;
/*
 * I - Insert
 */
p1012:
  if (!getlin(1, 1))
    goto p1025;                    /* J line # u/s */
  setptr(oldcom->decval);          /* Get ready to insert */
  if (eolok())                     /* No extra params */
    goto p1034;
  goto p1025;                      /* Re-read command */
/*
 * M - MODIFY
 */
p1015:modify = true;
  if (!getlin(0, 0))
    goto p1713;                    /* J bad line # */
  j4 = (oldcom->decval);           /* 1st line to be altered */
  lstvld = false;                  /* Previous line not valid */
  numok = 1036;
p1074:                             /* Get opt # of lines & check eof */
  if (getnum(0))                   /* Format of optional # of lines OK */
  {
    count = oldcom->decval;
    if (oldcom->toktyp == eoltok || eolok()) /* EOL already or next */
      goto asg2numok;
  }                                /* if(getnum(0)) */
  goto p1025;                      /* Re-read command */
p1713:locerr = true;               /* An scmac can detect this error */
/* Report err unless brief macro */
  if (curmac < 0 || !BRIEF)
    (void)write(1, ermess, errlen);
  goto p1025;                      /* Reread command */
p1036:
  newl = 1037;                  /* Eventually come back here after any splits */
  if (verb == 'M')                 /* Was M-modify */
    setptr(j4);                    /* Position on 1st line to alter */
  for (i = count; i > 0; i--)
  {
    if (!rdlin(curr, 0))           /* Get lin to mod / EOF */
    {
      printf("E - O - F\r\n");
      goto p1004;
    }                              /* if(!rdlin(curr,0)) */
    curr->bcurs = locpos;          /* In case just come from LOCATE */
    locpos = 0;                    /* In case just come from LOCATE */
    sprmpt(ptrpos - 1);            /* Set up prompt lin # just read */
    goto p1027;
  p1037:;                          /* Nl typed (possibly after ^T(s)) */
  }

/* If doing a reverse, locate move pointer back to where locate was, ready for
 * the next one. Also has to be done after Ec) */

  if (revrse)
    setptr(revpos);

  goto p1004;                      /* Finished this MODIFY */
/* ******************************************************************
 * End line modifiers
 * ******************************************************************
 *
 *
 *
 *
 * ******************************************************************
 * Start file handlers
 * ******************************************************************
 *
 * B - Save file with a .BU backup copy
 * S - Save file
 */
p1006:
  nofil = 10407;
  rtn = 10403;
  savpos = ptrpos;                 /* So we can leave pos'n same at end */
  setptr((long)1);                 /* Pos'n 1st line */
  if (deferd)
    dfread(LONG_MAX, NULL);        /* Ensure all file in */
  wrtnum = lintot;                 /* Write all lines */
/*
 * This line for S-SAVE, B-BACKUP, and W-WRITE
 */
p1072:
  rdwr = O_WRONLY + O_CREAT;       /* Don't truncate yet in case mmap'd */
/*
 * Code used by all commands
 */
p1075:
  if (scrdtk(2, (unsigned char *)buf, PTHMAX, oldcom)) /* Read a f/n */
  {
    perror("backup/save - scrdtk");
    putchar('\r');
  p1043:
    (void)write(1, "Error in filename", 17);
    goto p1025;
  }
  if (oldcom->toktyp == eoltok)
    goto asg2nofil;                /* J that was EOL */
  if (oldcom->toktyp != nortok)
    goto p1043;                    /* J not normal token */
  tildexpn(buf);                   /* Do tilde expansion */
  if (verb == 'W')
  {
    if (!getlin(1, 0))
      goto p1025;                  /* J line # u/s */
    setptr(oldcom->decval);        /* Get ready to write */
    numok = 1073;
    goto p1074;
  }
  if (verb == 'U')
  {
    if (eolok())
      goto p1510;
    goto p1025;
  }

/* For S B & Q, if the file exists then use its mode from now on. Don't complain
 * here if it doesn't exist. To check whether the file is a symlink, we need to
 * call readlink to find its real name. The only real error here is a symlink
 * loop */

  if (verb != 'E')
  {
  colontrunc:                     /* Jump to here after truncating buf at ":" */
    errno = 0;                     /* Ensure valid */
    if (!stat(buf, &statbuf))
    {
      tmode = statbuf.st_mode;
      tgroup = statbuf.st_gid;
      towner = statbuf.st_uid;
    }                              /* if (!stat(buf, &statbuf)) */
    else
    {
      if (!tmask)
      {
        tmask = umask(0);          /* Get current umask */
        umask(tmask);              /* Reinstate umask */
      }                            /* if (!tmask) */
      tmode = ~tmask & 0666;       /* Assume no execute on new file */
      tgroup = towner = 0;
    }                              /* if (!stat(buf, &statbuf)) else */
    if (!lstat(buf, &statbuf) && S_ISLNK(statbuf.st_mode) && errno != ELOOP)
      for (;;)
      {
        if (0 < (i = readlink(buf, tmfile, (size_t)PTHSIZ)))
        {                          /* S, B or Q on a symlink */
          tmfile[i] = 0;           /* No trlg NUL from readlink */
          if (tmfile[0] == '/' || tmfile[0] == '~')
            strcpy(buf, tmfile);
          else
          {
            p = (unsigned char *)strrchr(buf, '/'); /* Find last '/' if any */
            if (!p)
              p = (unsigned char *)buf - 1; /* Filename at buf start */
            *(p + 1) = '\0';       /* Throw away filename */
            strcat(buf, tmfile);   /* Append linked name */
          }                        /* if(tmfile[0]=='/'||tmfile[0]=='~') else */
          printf("Symbolic link resolves to %s", buf);
          newlin();
/* See if symlink points to another symlink... */
          if (lstat(buf, &statbuf))
            break;                 /* B link to a new file */
          if (!S_ISLNK(statbuf.st_mode))
            break;                 /* B now not on a symlink */
        }                          /* if(0<(i=readlink(buf,tmfile,... */
        else
        {
          printf("%s. %s (readlink)", strerror(errno), buf);
          goto p1025;              /* Bad readlink */
        }                          /* if(0<(i=readlink(buf,tmfile,... else */
      }                            /* if(!lstat(buf,&statbuf)&&S_ISLNK(... */
    if (!(errno || S_ISREG(statbuf.st_mode)))
    {
      printf("Not a regular file. %s", buf);
      goto p1025;
    }                              /* if(!S_ISREG(statbuf.st_mode)) */
  }                                /* if(verb!='E' */
  if (eolok())
    goto p1044;
  goto p1025;                      /* Re-read command */
p1073:                             /* W-write continuing */
  wrtnum = count;                  /* Get back here if ok # of lines */
p1044:
  bspar = true;                    /* We have a parameter (if B or S) */
/*
 * If B-BACKUP, back up supplied param anyway
 */
  if (verb == 'B')
    goto p10445;                   /* Join S&B with no params */
  lgtmp3 = false;                  /* Q-QUIT into existing file */
p1708:
  if ((funit = open(buf, rdwr, tmode)) == -1)
  {
/*
 * If Q-QUIT, file may not exist as user wishes to create a new one. Or file may
 * not exist because it's of the form <filename>:<line number>
 */
    if (errno == ENOENT && verb == 'Q' && !lgtmp3)
    {
      if (!bootq)
      {
        if ((colonpos = strchr(buf, ':')) &&
          sscanf(colonpos + 1, "%d", &colonline) == 1)
        {
          bootq = 4;               /* line # in colonline */
          *colonpos = '\0';        /* Truncate filename */
          goto colontrunc;         /* Try with truncated buf */
        }                          /* if((colonpos=strchr(buf,':'))&&... */
      }                            /* if(!bootq) */
      else if (bootq == 4)         /* Just tried truncating at ":" */
        *colonpos = ':';           /* Undo truncation */
      bootq = 0;
      if (ysno5a("Do you want to create a new file (y,n,Cr [n])", A5DNO))
      {
        lgtmp3 = true;             /* Q-QUIT into new file */
        rdwr = O_WRONLY + O_CREAT + O_EXCL; /* File should *not* exist */
        goto p1708;                /* So create file */
      }  /* if(ysno5a("Do you want to create a new file (y,n,Cr [n])",A5DNO)) */
    }                              /* if(errno==ENOENT&&verb=='Q'&&!lgtmp3) */
    printf("%s. %s (open)", strerror(errno), buf);
    goto p1025;                    /* Bad open */
  }                                /* if((funit=open(buf,rdwr,tmode))==-1) */
  if (lgtmp3)                      /* Have just created file for Q-QUIT */
  {
    mods = false;                  /* No mods to new file yet */
    (void)close(funit);
  }                                /* if(lgtmp3) */
  else if (rdwr != O_RDONLY)       /* Need to write lines */
  {
  p1066:
    code = 1;                      /* Set to 0 on good writfl */
    if (fstat(funit, &statbuf))
      printf("%s. funit %d (fstat)", strerror(errno), funit);
    else if (ismapd(statbuf.st_ino))
      printf("%s is mmap'd", buf);
    else if (ftruncate(funit, 0))
      printf("%s. funit %d (ftruncate)", strerror(errno), funit);
    else
      writfl(wrtnum);              /* Write lines to o/p file */
    if (code != 0)                 /* Some kind of failure above */
      goto p1062;
  }                                /* else if(rdwr!=O_RDONLY) */
  goto asg2rtn;                    /* Finished, except S & B */
/*
 * B-BACKUP & S-SAVE only once again
 */
p10403:mods = false;               /* OK to Q-QUIT now */
  setptr(savpos);                  /* Repos'n file as before */
  if (bspar)
    strcpy(pcnta, buf);            /* We had a param. Set as dflt */
  else if (verb != 'B')
  {
    printf("%s\r\n", pcnta);       /* Remind user what file he's editing */
    if (unlink(tmfile) == -1)
      printf("%s. %s (unlink)\r\n", strerror(errno), tmfile);
  }

/* ---------------------------------------------------------------------- */
/* Attempt to restore as many attributes of the original file as possible */
/* current file attributes are in statbuf                                 */
/* ---------------------------------------------------------------------- */

  if (towner)                      /* If there *was* an original file */
  {
    code = 0;
    if (tgroup != statbuf.st_gid)
    {
      if (chown(pcnta, -1, tgroup) == -1)
      {
        code = 1;
        printf("Warning - original group not restored\r\n");
      }                            /* if (chown(pcnta, -1, tgroup) == -1) */
    }                              /* if (tgroup != statbuf.st_gid) */
    if (towner != statbuf.st_uid)
    {
/* Don't try to change user if group failed, but do warn */
      if (code || chown(pcnta, towner, -1) == -1)
      {
        code = 1;
        printf("Warning - original owner not restored\r\n");
      }                        /* if (code || chown(pcnta, towner, -1) == -1) */
    }                              /* if (towner != statbuf.st_gid) */
/* If there were no problems above, set any extra original mode bits */
    if (tmode != statbuf.st_mode && (code || chmod(pcnta, tmode) == -1))
      printf("Warning - original mode not restored\r\n");
  }                                /* if (towner) */
  goto p1004;                      /* Next command */
/*
 * Deal with the case where S or B had no param. Get joined by
 * B-BACKUP later anyway
 */
p10407:
  if (!pcnta[0])                   /* We have no default f/n */
  {
    printf("%s", "filename must be specified");
    goto p1025;                    /* Correct command */
  }
  bspar = false;                   /* Don't have a param */
/*
 * Duplicate f/n in buf so B-BACKUP can merge in
 */
  strcpy(buf, pcnta);
/*
 * P10445 - B-Backup joins here
 */
p10445:;
/*
 * S-SAVE with no f/n / B-BACKUP continuing. Use TMFILE for name of
 * backup file
 */
  strcpy(tmfile, buf);
  if (verb == 'B')
    strcat(tmfile, ".bu");         /* \add approp. */
  else
    strcat(tmfile, ".tm");         /* /postfix */

/* ---------------------------------------------------- */
/* We used to rely on link failing if the file existed. */
/* But then, link() always fails in a DOS file system.  */
/* So now we stat() the file to see if it exists,       */
/* then use rename() if it doesn't.                     */
/* (rename() would overwrite an old file)               */
/* ---------------------------------------------------- */

  while (!stat(tmfile, &statbuf))
  {
    if (verb == 'B' &&
      !ysno5a("Do you want to delete the old backup file", A5NDEF))
      ERR1025("Need another filename to take backup");
    if (unlink(tmfile))
    {
      printf("%s. %s (unlink)", strerror(errno), tmfile);
      goto p1062;
    }                              /* if(unlink(tmfile)) */
    if (verb == 'B')
      printf("Previous backup file deleted:- ");
  }                                /* if(!stat(tmfile,&statbuf)) */
  if (rename(buf, tmfile))         /* If rename fails */
  {
/* OK nofile if B with param */
    if (bspar && (errno == ENOENT))
    {
      puts("New file - no backup taken\r");
      goto p1065;
    }                              /* if(bspar&&(errno==ENOENT)) */
    printf("%s. %s to %s (rename)", strerror(errno), buf, tmfile);
  p1062:
    close(funit);                  /* In case anything left open */
    goto p1025;                    /* Get corrected command */
  }                                /* if (rename(buf, tmfile)) */
/*
 * File renamed - now open new file of same type as original
 */
  rdwr = O_WRONLY + O_CREAT + O_EXCL; /* Must be new file */
/* */
  if (verb == 'B')
    printf("Backup file is %s\r\n", tmfile); /* Report backup f/n */
p1065:
  if ((funit = open(buf, rdwr, tmode)) == -1)
  {
    printf("%s. %s (open)", strerror(errno), buf);
    goto p1025;                    /* No need to close since open failed */
  }
  goto p1066;                      /* Now write file */
/*
 * W - WRITEFILE
 */
p1022:
  nofil = 1069;
  rtn = 1004;                      /* Next command when finished */
  goto p1072;
/*
 * E - Enter
 */
p1009:
  rtn = 10755;
  nofil = 1069;                    /* Must have a filename */
  rdwr = O_RDONLY;                 /* Open file for reading */
  goto p1075;                      /* Off we go */
p10755:                            /* Q <filename> joins here */
  savpos = ptrpos;                 /* Return here with file open. read it... */

/* Use mmap if requested */

  if (fmode & 02000 && (verb != 'E' || fmode & 020000))
  {
    if (fstat(funit, &statbuf))
    {
      printf("%s. funit %d (fstat)", strerror(errno), funit);
      goto p1062;                  /* Bad fstat */
    }                              /* if(fstat(funit,&statbuf)) */
    if (statbuf.st_size)
    {
      if ((p = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE
#ifdef MAP_DENYWRITE
        | MAP_DENYWRITE      /* Doesn't appear to work (need MAP_EXECUTABLE?) */
#endif
        , funit, (off_t) 0)) == (void *)-1)
      {
        printf("%s. funit %d (mmap)", strerror(errno), funit);
        goto p1062;
      }                            /* if(!(p=mmap(NULL,statbuf.st_size... */
      mapfil(statbuf.st_ino, statbuf.st_size, p);
    }                              /* if(statbuf.st_size) */
    else
      printf("0 lines read.\r\n");
  }                                /* if(fmode&02000&&... */

  else
    readfl();
  for (;;)
  {
    if (!(close(funit)))           /* Success */
      break;
    if (errno == EINTR)            /* Interrupted system call */
      continue;
    perror("close");
    putchar('\r');
    break;
  }                                /* for(;;) */
  if (verb == 'Q')
    mods = false;                  /* No mods yet if that was Q-QUIT */
  setptr(savpos);                  /*  ...retaining old file pos'n... */
  goto p1004;                      /* ... and finish (!) */
/*
 * Q - Quit
 *
 * We accept a filename - starts user off editing a fresh file
 */
p1017:
  if (mods &&
    !ysno5a("file modified since last b-backup/s-save, ok to quit (y,n,Cr [n])",
    A5DNO))
    goto p1004;                    /* J user changed his mind */
  Tcl_DumpActiveMemory("t5mem");
  nofil = 1132;                    /* Old-style Q - no f/n */
  rdwr = O_RDONLY;
  rtn = 1521;
  goto p1075;                      /* Open next file if one given */
/*
 * If in a macro, only action solitary Q if mode says so.
 * Otherwise, convert to ^NU...
 */
p1132:
  if (curmac >= 0 && !(fmode & 0100) && verb == 'Q')
  {
    macdef(64, (unsigned char *)"", 0, 1); /* Macro is ^NU only */
    curmac = 64;
    mcposn = 0;
    goto p1004;
  }
  final5();                        /* Reset terminal */
  return 0;
/*
 * P1521 - Q with a filename continuing
 */
p1521:
  finitl();                        /* Erases old file but keeps segments */
  verb = 'Q';                      /* In case was 'q' */
/*
 * Set up new default for S&B
 */
  (void)strcpy(pcnta, buf);        /* Filename & length now remembered */
  if (lgtmp3)                      /* Q-QUIT into new file */
    goto p1004;                    /* Read next command */
  goto p10755;                     /* Get the file... and off we go! */
/*
 * ******************************************************************
 * End file handlers (except HELP & USE)
 * ******************************************************************
 *
 * D - DELETE
 */
p1008:
  if (!getlin(1, 0))
    goto p1025;                    /* J bad line # */
  k4 = oldcom->decval + 1;         /* Pos here for each delete */
  numok = 1076;
  goto p1074;                      /* Get optional # of lines in COUNT */
p1076:
  clrfgt();                        /* In case lines from last D */
  for (i4 = count; i4 > 0; i4--)
  {

/* Next line checks if there are deferred lines then try to get one. We need to
 * re-check lintot: "deferd" will get cleared in the process if the last line
 * was unterminated */

    if (k4 == lintot + 2 && !(deferd && (dfread(1, NULL), k4 != lintot + 2)))
      goto p1078;
    setptr(k4);                    /* Pos 1 past 1st line to go */
    delete(0);                     /* Knock off line. Use normal ptr */
  }
  goto p1004;                      /* Finished if get here */
p1078:
  rtn = 1079;
p1112:
  i4 = count - i4 + 1;             /* This many *not* deleted */
q1112:
  printf("%s of file reached:- ", revrse ? "start" : "end");
p1120:
  printf("%ld lines ", i4 - 1);
  goto asg2rtn;
p1079:
  puts("deleted\r");
  goto p1004;                      /* End DELETE */
/*
 * G - GOTO
 */
p1010:
  if (getlin(1, 1))                /* Good line # */
  {
    k4 = oldcom->decval;   /* LATER - see if eolok() preserves oldcom->decval */
    if (eolok())
    {
      setptr(k4);
      goto p1004;                  /* Finished GOTO */
    }                              /* if(eolok()) */
  }                                /* if(getlin(1,1)) */
  goto p1025;
/*
 * U - USE
 */
p1020:
  nofil = 1069;
  goto p1075;                      /* Get the file name */
p1510:
  duplx5(true);                    /* Assert XOFF recognition */
  tildexpn(buf);                   /* Do tilde expansion */

/* DEVNULL setting is same as parent */
  devnullstack[stdinstkptr + 1] =
    stdinstkptr < 0 ? 0 : devnullstack[stdinstkptr];

/* Save current stdin */
  stdinstkptr++;
  do
    stdinstack[stdinstkptr] = dup(0);
  while (stdinstack[stdinstkptr] == -1 && errno == EINTR);
  if (stdinstack[stdinstkptr] == -1)
  {
    stdinstkptr--;
    printf("%s. (dup(0))", strerror(errno));
    goto p1025;
  }                                /* if (stdinstack[stdinstkptr] == -1) */

/* Close funit 0 */
  do
    i = close(0);
  while (i == -1 && errno == EINTR);

/* Open new input source */
  do
    i = open(buf, O_RDONLY);
  while (i == -1 && errno == EINTR);
  if (i == -1)
  {
    pop_stdin();
    printf("%s. %s (open)", strerror(errno), buf);
    goto p1025;
  }                                /* if (i == -1) */

/* Verify new input opened on funit 0. Try to rectify if not */
  if (i)
  {
    do
      j = dup2(i, 0);
    while (j == -1 && errno == EINTR);
    if (j == -1)
    {
      printf("%s.(dup2(%d, 0))", strerror(errno), i);
      do
        j = close(i);
      while (j == -1 && errno == EINTR);
      pop_stdin();
      goto p1025;
    }                              /* if (j == -1) */
    do
      j = close(i);
    while (j == -1 && errno == EINTR);
  }                                /* if (i) */

  buf5len = 0;                     /* Flush any input left over */
  goto p1004;
/*
 * H - HELP
 */
p1011:
/*
 * We have some extra work here, because HELP doesn't actually take
 * a filename, and doesn't want one.
 */
  if (scrdtk(2, (unsigned char *)tmfile, 17, oldcom))
  {
  p11042:
    printf("%s. (scrdtk)", strerror(errno));
    goto p1025;
  }
  k = oldcom->toklen;              /* Get length of HELP topic */
  if (k == 0)
  {                                /* If no topic given */
    k = 1;
    tmfile[0] = '#';               /* Dummy topic of "#" */
    tmfile[1] = '\0';
  }
  if (!eolok())
    goto p1025;
  for (i = k - 1; i >= 0; i--)
  {
    if (tmfile[i] >= 'A' && tmfile[i] <= 'Z')
      tmfile[i] += 040;
  }
  sprintf(tmtree, "%s/%s", help_dir, tmfile);
  if (stat(tmtree, &statbuf))
  {
/* Output a potted message if he "typed h for help" */
    if (tmfile[0] == '#' && tmfile[1] == 0)
    {
      printf("\r\n");
      printf("Sorry - I can't find my HELP files.\r\n");
      printf("If you have them installed somewhere,\r\n"
        "please put that path in your shell environment"
        " with the name Q_HELP_DIR.\r\n\n");
      goto p1004;
    }
    printf("%s. %s (HELP)", strerror(errno), tmtree);
    goto p1025;
  }
  sprintf(buf, "%s %s", help_cmd, tmtree);
  semifinl();                      /* For some pagers */
  if (system(buf) < 0)
  {
    printf("%s. %s (system)", strerror(errno), buf);
    init5();
    goto p1025;
  }
  init5();
  goto p1004;                      /* Finished */
/*
 * X - Set terminal characteristics
 */
p1023:
  if (!eolok())
    goto p1025;
  puts("Switching off screenedit mode\r");
  xistcs();                        /* So set them */
  puts("Re-enabling screenedit\r");
  goto p1004;                      /* Finished X */
/*
 * P - Print
 */
p1016:
  numok = 1092;
  lstlin = ptrpos;                 /* -TO strt curr lin */
  goto p1074;                      /* Get optional # of lines to COUNT */
p1092:
  rtn = 1093;
p1104:
  for (i = count; i > 0; i--)
  {
    if (!rdlin(curr, 0))           /* EOF. Normal ptr */
    {
      puts(" E - O - F\r");
      break;
    }
    sprmpt(ptrpos - 1);
    pdsply(curr, prmpt, pchrs);
    if (cntrlc || kbd5())          /* User wants out */
    {
      newlin();
      puts(" *** keyboard interrupt *** \r");
      cntrlc = false;
      break;
    }
  }
  goto asg2rtn;                    /* All req'd lines printed */
p1093:goto p1004;                  /* Finished P */
/*
 * V - View
 *
 *
 * Print n lines before current line, then current line, then n lines
 * after. N may be specified as zero - show current line only.
 * If no n, assume enough lines to completely fill the screen.
 * Afterwards, move pointer back to posn of entry.
 */
p1021:
  numok = 10965;
  nonum = 1097;
  xcount = 0;                      /* Only show requested # */
  lstlin = -1;                     /* Not allowed -TO */
  logtmp = true;                   /* Accept 0 lines */
  fullv = false;                   /* Assume not just "V" */
p1107:
  if (!getnum(logtmp))
    goto p1025;                    /* J format err on # of lines */
  count = oldcom->decval;
  if (oldcom->toktyp == eoltok)
    goto asg2nonum;                /* J no # given */
/*
 * !CAUTION! Tricky coding. LOGTMP is really for whether 0 is allowed
 * but it is also used here, in the reverse sense, to see whether extra
 * parameters are allowed or not. I.E. if true then 0 is allowed
 * but extra parameters aren't; false is the other way round.
 */
  savtok = oldcom->toktyp;         /* eolok overwrites */
  if (!logtmp || eolok())          /* More allowed or eol now */
  {
    if (savtok == nultok)          /* No # given */
      goto asg2nonum;
    else
      goto asg2numok;
  }                                /* if(!logtmp||eolok()) */
  goto p1025;
/* Print enough lines to fill the screen... */
p1097:
  fullv = true;                    /* Try very hard to fill screen */
  count = (row5 / 2) - 1;          /* No count given so assume 1/2 screen */
  xcount = row5 & 1;               /* Extra line if odd screen length */
p10965:
  if (!lintot && !(deferd && (dfread(1, NULL), lintot))) /* Empty file */
  {
    puts("THE FILE IS EMPTY\r");
    goto p1004;                    /* End V */
  }
  k4 = ptrpos;                     /* Remember pos'n */
  j4 = k4 - count;
  if (j4 < 1)
    j4 = 1;                        /* J4=1st line to list */
  setptr(j4);
  j4 = k4 + count + xcount;        /* Extra line if odd length */
  if (j4 > lintot && deferd)
    dfread(j4 - lintot, NULL);     /* Not -1, want that extra line */
  if (j4 > lintot + 1)
    j4 = lintot + 1;               /* J4=last line to list */
  count = j4 - ptrpos + 1;         /* Set up # lines to print */
/* Want to fill screen but not enough lines yet */
  if (fullv && count < row5 - 1)
  {
    if (ptrpos == 1)               /* Viewing from file start */
    {
      if (deferd && lintot < row5)
        dfread(row5, NULL);        /* Ensure lintot is adequate */
      count = lintot + 1 >= row5 - 1 ? row5 - 1 : lintot + 1;
    }                              /* if(ptrpos==1) */
    else                           /* Viewing to file end */
    {
      if (deferd)
        dfread(row5, NULL);        /* Ensure lintot is adequate */
      j4 = lintot + 3 - row5;
      if (j4 < 1)
        j4 = 1;
      setptr(j4);
      count = lintot + 3 - j4;
    }                              /* if(ptrpos==1) else */
  }
  rtn = 1103;
  goto p1104;                      /* Print them */
p1103:
  setptr(k4);                      /* Reposition */
  goto p1004;                      /* Finished */
/*
 * L - Locate
 */
p1014:
  tokens = verb == 'l';            /* Whether FL */
  if ((revrse = (fmode & 04000) != 0))
    revpos = ptrpos;
  lgtmp2 = !BRIEF || curmac < 0;   /* Display error messages if true */
  if (revrse ? ptrpos <= 1 : ptrpos > lintot && !(deferd &&
    (dfread(1, NULL), ptrpos <= lintot)))
  {
    if (lgtmp2)
      printf("At %s of file already - no lines to search\r\n",
        revrse ? "start" : "end");
    goto p1004;                    /* Next command */
  }                                /* if(revrse?ptrpos<=1:ptrpos>lintot&&... */

/* Get the string to locate. The string is read to the ermess array, as buf will
 * get overwritten and we aren't going to use getlin, so ermess is spare. */

  if (scrdtk(2, (unsigned char *)ermess, BUFMAX, oldcom))
    goto p11042;                   /* J bad RDTK */
  if (oldcom->toktyp == eoltok)
    goto p11043;
  if (!(l = oldcom->toklen))       /* String is length zero */
  {
  p11043:
    write(1, "Null string to locate", 21);
    goto p1025;                    /* Reread command */
  }                                /* if(!(l=oldcom->toklen)) */
  numok = 1105;
  nonum = 1106;
  lstlin = ptrpos;                 /* -TO rel currnt line */
  logtmp = false;                  /* Don't accept 0 lines */
  goto p1107;                      /* Get number lines to search */
p1106:                             /* No num, so search to [se]of */
  if (revrse)
    count = ptrpos - 1;
  else if (deferd)
    count = LONG_MAX;
  else
    count = lintot - ptrpos + 1;
p1105:count2 = count;              /* We have another COUNT to get */
  lstlin = -1;                     /* Not allowed -TO */
  numok = 1514;
  nonum = 1514;
  goto p1107;                      /* Get # lines to mod on location */
p1514:
  rtn = 1715;                      /* Shared 1st/last code */
p1722:
  numok = 1716;
  nonum = 1716;                    /* 1st pos 0 default */
  goto p17165;                     /* Get 1st pos to search */
p1716:
  first = oldcom->decval - 1;      /* Columns start at 0 */
  numok = 1717;
  nonum = 1718;                    /* Last max lin len default */
  goto p17165;                     /* Get last pos'n */
p1718:
  last = BUFMAX - 1;
  goto p1719;
p1717:
  last = oldcom->decval - 1;       /* Last start position */
  if (last < first)                /* Impossible combination of columns */
  {
    printf("Last pos'n < first\r\n");
    goto p1025;
  }                                /* if(last<first) */
  last = last + l;                 /* Add search length to get wanted length */
p1719:
  minlen = first + l;              /* Get minimum line length to search */
  if (eolok())
    goto asg2rtn;
  goto p1025;
p1715:
  savpos = ptrpos;                 /* Remember pos in case no match */

/* Start of search */

  for (i4 = count2; i4 > 0; i4--)
  {
    if (cntrlc)                    /* User abort */
    {
      count = count2;              /* Fit in with Y's error printing */
      goto p1622;
    }                              /* if(cntrlc) */
    if (revrse)
    {
      if (revpos <= 1)             /* Sof (< shouldn't happen) */
        goto s1112;
      setptr(--revpos);            /* Read previous line */
      rdlin(curr, 0);              /* "can't" hit eof */
    }                              /* if(revrse) */
    else if (!rdlin(curr, 0))      /* If eof */
    {
    s1112:
      if (!lgtmp2 || count2 == LONG_MAX) /* No message wanted */
        break;                     /* for(i4=count2;i4>0;i4--) */
      rtn = 1111;
    r1112:
      i4 = count2 - i4 + 1;        /* This many *not* searched */
      goto q1112;
    p1111:
      puts("searched\r");
      break;                       /* for(i4=count2;i4>0;i4--) */
    }                              /* if(!rdlin(curr,0)) */
    m = curr->bchars;
    if (m < minlen)
      continue;                    /* Skip search if too short */
    if (m > last)
      m = last;                    /* Get length to search */
    if (tokens ? ltok5a((unsigned char *)ermess, l, curr->bdata, first, m,
      &locpos, &dummy, (unsigned char *)ndel) : lsub5a((unsigned char *)ermess,
      l, curr->bdata, first, m, &locpos, &dummy))
    {                              /* Line located */
      if (revrse)
        setptr(revpos);
      else
      p1110:                       /* Joined here by "J"oin */
        setptr(ptrpos - 1);
      lstvld = false;              /* Previous line not valid */
      modify = true;               /* For "M"odify code */
      goto p1036;                  /* End L & J - carry on as M */
    }                              /* if((tokens?ltok5a((... */
  }                                /* for(i4=count2;i4>0;i4--) */

/* Didn't locate it if we get here */

  setptr(savpos);                  /* Move pointer back */
  locpos = 0;                      /* zeroised by lstr5a */
  if (lgtmp2)
    (void)write(1, "Specified string not found", 26);
p1810:locerr = true;               /* Picked up by RERDCM */
p1811:
/* Reset screen cursor */
  (void)scrdtk(5, (unsigned char *)NULL, 0, oldcom);
/* Move past command & 1st param */
  (void)scrdtk(1, (unsigned char *)NULL, 0, oldcom);
  (void)scrdtk(1, (unsigned char *)NULL, 0, oldcom);
  goto p1025;
/*
 * P17165 - Get 1st & last posn's for L & Y
 */
p17165:
  if (!getnum(0))
    goto p1025;
  if (oldcom->toktyp != nortok)
    goto asg2nonum;                /* J no number given */
  goto asg2numok;
/*
 * J - Join
 */
p1013:
  if (!getlin(1, 0))
    goto p1025;                    /* J bad line # */
  setptr(oldcom->decval);          /* Pos'n on line to be joined onto */
  numok = 1114;
  nonum = 1114;
  logtmp = false;                  /* 0 not allowed - not eol yet */
  goto p1107;                      /* Get opt. # lines to join in COUNT */
p1114:count2 = count;              /* Another # to get */
  lstlin = -1;                     /* Not allowed -TO */
  numok = 1515;
  goto p1074;                      /* Get # lines to mod after */
p1515:
/* At eof */
  if (ptrpos >= lintot && !(deferd && (dfread(1, NULL), ptrpos < lintot)))
  {
    if (curmac < 0 || !BRIEF)      /* Message wanted */
      puts("Can't join anything - no lines follow\r");
    goto p1004;                    /* Next command */
  }                                /* if(ptrpos>=lintot&&... */
  rdlin(prev, 0);                  /* 1st line */
  delete(0);                       /* Del lin before normal ptr */
  for (i4 = count2; i4 > 0; i4--)
  {
    if (!rdlin(curr, 0))           /* If eof */
    {
      rtn = 1118;
      goto r1112;
    p1118:
      puts("joined\r");
      break;
    }
    j = curr->bchars + prev->bchars;
    if (j > prev->bmxch)
      goto p1117;                  /* J would bust line */
    r = (char *)&prev->bdata[prev->bchars]; /* Appending posn */
    prev->bchars = j;              /* New length */
    delete(0);                     /* Delete line just read */
/* Append line just read */
    memcpy(r, (char *)curr->bdata, (size_t)curr->bchars);
  }
  inslin(prev);                    /* Put composed line back */
  goto p1110;                      /* Join M-MODIFY eventually */
p1117:
  setptr(ptrpos - 1);              /* P1117 - bust line */
  (void)write(1, "joining next line would exceed line size :- ", 44);
  rtn = 1118;
  goto p1120;                      /* End JOIN */
/*
 * R - Re position
 *
 *
 * Most of the code is common to C-COPY
 */
p1018:
  rtn = 1121;
  repos = true;
p1129:
  if (!getlin(1, 0))               /* Bad source. C joins here */
  {
    (void)write(1, " in source line", 15);
    goto p1025;
  }
  k4 = oldcom->decval;             /* Remember source */
  if (!getlin(1, 1))               /* Bad dest'n */
  {
    (void)write(1, " in dest'n line", 15);
    goto p1025;
  }
  j4 = oldcom->decval;             /* Remember dest'n */
  lstlin = k4;                     /* -TO refers from source line */
  if (j4 == k4)                    /* Error if equal */
  {
    (void)write(1, "Source and destination line #'s must be different", 49);
    goto p1025;
  }
  goto asg2rtn;                    /* End 1st common part */
p1121:
  if (k4 == j4 - 1)
  {
    (void)write(1, "moving a line to before the next line is a no-op", 48);
    goto p1025;
  }
  rtn = 1125;                      /* Only used if hit eof */
p1131:                             /* C joins us here */
  numok = 1126;
  goto p1074;                      /* Get opt # of lines & check eof */
p1126:setptr(j4);                  /* Set main ptr at dest'n */
/*
 * Note:- When setting both pointers, always set AUX second
 */
  setaux(k4);                      /* Set AUX ptr at source */
  for (i4 = count; i4 > 0; i4--)
  {
    if (!rdlin(prev, 1))
      goto p1112;                  /* Read AUX, j eof to mess code */
    if (repos)
      delete(1);                   /* For reposition only, delete line read */
    inslin(prev);
  }
  goto p1004;                      /* Finished C or R */
p1125:
  puts("repositioned\r");          /* Eof on repos. most of mess done */
  goto p1004;                      /* End R */
/*
 * C-COPY
 */
p1007:repos = false;               /* C-COPY, not R-REPOS */
  rtn = 1128;
  goto p1129;                      /* Phase 1 - get source & dest'n #'s */
p1128:
  rtn = 1130;                      /* Come back if hit eof */
  goto p1131;                      /* Get 3rd param & do copy */
p1130:
  puts("copied\r");                /* Eof. Most of message o/p */
  goto p1004;                      /* End C */
/*
 * T - Tabset
 */
/* Enter SCREENEDIT subsystem to complete command */
p1101:if (tabset(oldcom))
    goto p1004;
  goto p1025;                      /* Allow user to correct any errors */
/*
 * Z - Return from a Use file
 */
p1102:
  if (!eolok() || !pop_stdin())
    goto p1025;
  goto p1004;
/*
 * O - Switch On INDENT
 */
p1401:
  logtmp = INDENT != 0;
  rtn = 1407;
p2015:
  answer = get_answer();
  switch (answer)
  {
    case Q_YES:
      logtmp = true;
      break;
    case Q_NO:
      logtmp = false;
      break;
    case Q_MAYBE:
      logtmp = !logtmp;
      break;
    case Q_UNREC:
      goto p1025;
  }                                /* switch (answer) */
  goto asg2rtn;
p1407:
  if (logtmp)
    fmode |= 04000000000;
  else
    fmode &= ~04000000000;
  goto p1004;
/*
 * P2013 - FFortran. Use the O-indent code...
 */
p2013:
  rtn = 2014;
  logtmp = FTNMOD != 0;
  goto p2015;
p2014:
  if (logtmp)
    fmode |= 01000000000;
  else
    fmode &= ~01000000000;
  goto p1004;
/*
 * P2016 - FCaseind. Use the O-indent code...
 */
p2016:
  rtn = 2017;
  logtmp = !CASDEP;
  goto p2015;
p2017:
  if (logtmp)
    fmode |= 02000000000;
  else
    fmode &= ~02000000000;
  goto p1004;
/*
 * N - NEWMACRO
 */
p1501:
  i = newmac();
  if (i < 0)                       /* Token was "-" */
  {
    scrdtk(2, (unsigned char *)buf, BUFMAX, oldcom);
    if (oldcom->toktyp == eoltok)
      typmac();
    else
    {
      if (!eolok())
        goto p1025;
/* Need to preserve stdout for this */
      if (orig_stdout == -1)
      {
        do
          orig_stdout = dup(1);
        while (orig_stdout == -1 && errno == EINTR);
      }                            /* if (orig_stdout == -1) */
      if (orig_stdout == -1)
      {
        fprintf(stderr, "\r\n%s. (dup(1))\r\n", strerror(errno));
        refrsh(NULL);
        goto p1025;
      }                            /* if (orig_stdout == -1) */
      else
      {
        do
          i = close(1);
        while (i == -1 && errno == EINTR);
        do
          i = open(buf, O_WRONLY + O_CREAT, 0666);
        while (i == -1 && errno == EINTR);
        if (i == 1)
        {
          lstmac();
          i = 0;                   /* Success */
        }                          /* if (i == 1) */
        else
          i = errno;
        restore_stdout();
        if (i)
          printf("%s. %s (freopen)", strerror(i), buf);
        i = !i;                    /* Required below */
      }                            /* if (orig_stdout == -1) else */
    }                              /* if (oldcom->toktyp == eoltok) else */
  }                                /* if (i < 0) */
  if (i)
    goto p1004;                    /* No error */
  goto p1025;
/*
 * K - pop a shell prompt
 */
p1525:
  if (!eolok())
    goto p1025;
  if (!USING_FILE)
    puts("Exit from shell to restart\r");
  semifinl();
  i = system(sh);
  init5();
  if (i < 0)
    printf("%s. (system(\"%s\"))", strerror(errno), sh);
  if (!USING_FILE)
    puts("Re-entering Q\r");
  goto p1004;
/*
 * FX - Exchange the functions of 2 keyboard keys
 */
p1904:
  if (scrdtk(2, (unsigned char *)oldkey, 3, oldcom))
    goto p11042;
  if (oldcom->toktyp != eoltok)
    goto p1905;                    /* J line wasn't empty */
/*
 * FX with no params - Re-initialise the table, subject to
 *                     confirmation
 */
  if (!ysno5a("Re-initialise keyboard table to default - ok", A5NDEF))
    goto p1004;                    /* Finish if changed his mind */
  for (i = 127; i >= 0; i--)
    fxtabl[i] = i;                 /* No FX commands yet */
  goto p1004;                      /* Finished after reset */
/*
 * Validate parameter just read ...
 */
p1905:
  rtn = 1909;
  xkey[0] = oldkey[0];
  xkey[1] = oldkey[1];
p1915:
  i = oldcom->toklen;
  if (i == 0)
    goto p11043;                   /* Null param not allowed */
  if (i == 2)
    goto p1908;                    /* ^<char> allowed */
  k = xkey[0];                     /* Convert to subscript */
  if (k < 128)
    goto p1916;                    /* Continue */
  (void)write(1, "parity-high \"keys\" not allowed", 30);
  goto p1025;
p1908:
  if (xkey[0] == CARAT)
    goto p1910;                    /* J starts "^" (legal) */
  (void)write(1, "may only have single char or ^ char", 35);
  goto p1025;
p1910:
  k = xkey[1];                     /* Isolate putative control */
  if (k >= 0100 && k <= 0137)
    goto p1911;                    /* J a real control char */
  if (k == ASTRSK)
    goto p1912;                    /* ^* (=^) */
  if (k == QM)
    goto p1913;                    /* ^? (=rubout) */
  (void)write(1, "Illegal control character representation", 40);
  goto p1025;
p1911:
  k = k - 0100;                    /* Control char to subscript */
  goto p1916;
p1912:
  k = CARAT;                       /* "^" as subscript */
  goto p1916;
p1913:
  k = 127;                         /* Rubout is the last character */
p1916:
  if (k > 31 && k != 127)
    puts("Warning: non-control char as argument\r");
  goto asg2rtn;
p1909:
  oldkey[0] = k;                   /* Now a subscript */
  if (scrdtk(2, (unsigned char *)newkey, 3, oldcom))
    goto p11042;
  if (oldcom->toktyp == eoltok)
    goto p11043;                   /* Must have another parameter */
  xkey[0] = newkey[0];
  xkey[1] = newkey[1];
  rtn = 1914;
  goto p1915;                      /* Validate parameter */
p1914:
  newkey[0] = k;                   /* Now a subscript */
  if (!eolok())
    goto p1025;
  i = fxtabl[(int)oldkey[0]];
  fxtabl[(int)oldkey[0]] = fxtabl[(int)newkey[0]];
  fxtabl[(int)newkey[0]] = i;
  goto p1004;
/*
 * Y - Change all occurrences of 1 string to another
 *
 *
 * P1607 - Get string to look for
 */
p2002:
  tokens = true;                   /* FY */
  goto p2005;
p1607:
  tokens = false;                  /* Not FY */
p2005:
  if (scrdtk(2, (unsigned char *)oldstr, BUFMAX, oldcom))
    goto p11042;
  if (oldcom->toktyp == eoltok)
    goto p11043;
  oldlen = oldcom->toklen;         /* Length of string */
  if (oldlen == 0)
    goto p11043;                   /* J null string (error) */
/*
 * Get string with which to replace it
 */
  if (scrdtk(2, (unsigned char *)newstr, BUFMAX, oldcom))
    goto p11042;
  newlen = oldcom->toklen;
  ydiff = newlen - oldlen;
/* strings must be equal length if Fixed-Length mode */
  if (ydiff && fmode & 0400)
  {
    printf("Replace string must be same length in FIXED LENGTH mode");
    goto p1025;                    /* Report error */
  }
  if (oldcom->toktyp == eoltok)
    goto p1608;                    /* J no more params */
/* */
  if (getlin(0, 0))
    goto p1609;                    /* J definitely ok 1st line # */
  if (oldcom->toktyp != nortok)
    goto p1608;                    /* J ok after all */
  (void)write(1, ermess, errlen);
  goto p1025;
p1608:j4 = 1;                      /* Start looking at line 1 */
  if (oldcom->toktyp == eoltok)
    goto p1610;                    /* Join no # lines code if eol now */
  goto p16105;
p1609:
  j4 = oldcom->decval;
p16105:if (!getnum(0))
    goto p1025;                    /* Get # lines, 0 not allowed */
  count = oldcom->decval;
  if (oldcom->toktyp != nortok)    /* No number given */
  p1610:
    count = deferd ? LONG_MAX : lintot + 1 - j4; /* Process to eof */
  rtn = 1612;
  lstlin = -1;                     /* -TO not allowed for column pos'ns */
  l = oldlen;                      /* Req'd by code for L-LOCATE */
  goto p1722;                      /* Look for 1st & last pos'ns in line */

/* Error messages */

p1622:
  cntrlc = false;                  /* ^C noticed */
  printf("Command abandoned :-");
p16221:
  printf(" %ld lines ", count - i4);
p1616:
  (void)write(1, "scanned", 7);
  newlin();
p1712:setptr(savpos);              /* Restore file pos'n */
  goto p1004;                      /* Leave Y */
p16175:
  savpos = ptrpos - 1;             /* Point to too big line */
  printf("Next line would exceed max size:-");
  goto p16221;
p1620:
  if (curmac < 0 || !BRIEF)
    (void)write(1, "specified string not found", 26);
  setptr(savpos);                  /* Restore file pos'n */
  goto p1810;

p1612:
  if (!lintot && !(deferd && (dfread(1, NULL), lintot))) /* Empty file */
  {
    (void)write(1, "Empty file - can't changeall any lines", 38);
    goto p1025;
  }                                /* if(!lintot&&... */

/* We act on BRIEF or NONE if in a macro without question. Otherwise, BRIEF or
 * NONE is queried, and we reset to VERBOSE if we don't get confirmation */

  if (curmac >= 0)
    goto p1710;                    /* J in a macro */
  if (!BRIEF)
    goto p1710;                    /* J VERBOSE already */
  if (ysno5a("Use brief/none in this command (y,n,Cr [n])", A5DNO))
    goto p1710;
  puts("Reverting to verbose\r");
  fmode &= 07777777777;
p1710:savpos = ptrpos;             /* Remember so we can get back */
  setptr(j4);                      /* First line to look at */
  lgtmp2 = newlen != 0;
  lgtmp3 = false;                  /* No lines changed yet */
/*
 * Main loop on specified lines
 */
  for (i4 = count; i4 > 0; i4--)
  {
    if (cntrlc)                    /* User has interrupted */
      goto p1622;
    if (!rdlin(curr, 0))           /* Eof on main pointer */
    {
      if (count == LONG_MAX)       /* Was going to deferred eof */
        break;                     /* for(i4=count;i4>0;i4--) */
      rtn = 1616;
      goto p1112;                  /* Print most of end of file message */
    }
/*
 * Initial tasks for each line
 */
    yposn = first;                 /* Search from 1st position spec'd */
    linmod = false;                /* No match this line yet */
    k = curr->bchars;              /* Remembers line length */
    if (k < minlen)
      continue;                    /* J line shorter than minimum */
    n = 0;
    if (k > last)
      n = k - last;                /* N=# at end not to search */
/* */
  p1619:if (tokens)
      goto p2011;                  /* J FY */
/* J no more occurrences this line */
    if (!lsub5a((unsigned char *)oldstr, oldlen, curr->bdata, yposn, k - n, &l,
      &m))
      goto p1617;
    goto p2012;
  p2011:
/* J no more occurrences this line */
    if (!ltok5a((unsigned char *)oldstr, oldlen, curr->bdata, yposn, k - n, &l,
      &m, (unsigned char *)ndel))
      goto p1617;
  p2012:
    if (k + ydiff > curr->bmxch)
      goto p16175;                 /* J would bust line */
    if (m == k - 1)
      goto p1618;                  /* J no chars after string */
    if (ydiff > 0)
/* Create a gap of the right length. Overlapping r/h move */
    {
      p = &curr->bdata[k - 1];     /* End char to pick up */
      q = &curr->bdata[k - 1 + ydiff]; /* End char to set down */
      for (i = k - m - 1; i > 0; i--)
        *q-- = *p--;
    }
    else if (ydiff < 0)
/* Close up by the right length. Overlapping l/h move */
    {
      p = &curr->bdata[m + 1];     /* Start char to pick up */
      q = &curr->bdata[m + 1 + ydiff]; /* Start char to set down */
      for (i = k - m - 1; i > 0; i--)
        *q++ = *p++;
    }
  p1618:
/* Move in new string if not null */
    if (lgtmp2)
      memcpy((char *)&curr->bdata[l], (char *)newstr, (size_t)newlen);
    k = k + ydiff;                 /* Get new line length */
    linmod = true;                 /* This line has been modified */
    yposn = m + 1 + ydiff;         /* Resume search after new string */
/* Seek more occurrences if room */
    if (k - n - yposn >= oldlen)
      goto p1619;
  p1617:
    if (!linmod)
      continue;                    /* J no mods to this line */
    linmod = false;
    lgtmp3 = true;                 /* A line changed now */
    curr->bchars = k;
    if (!NONE)                     /* Some display may be req'd */
    {
/*
 * SPEEDUP - If BRIEF, only display every 1/5th sec
 */
      if (BRIEF)                   /* only display if time to (or 1st) */
      {
        if ((timnow = times(&tloc)) == -1)
        {
          perror("times");
          putchar('\r');
          goto p1025;
        }
        if (timnow - timlst < 20)
          goto p1711;              /* J not yet time to display */
        timlst = timnow;           /* Displaying */
        forych = true;             /* Tell PDSPLY short display */
      }                            /* if(BRIEF) */
      sprmpt(ptrpos - 1);          /* set up line # */
      pdsply(curr, prmpt, pchrs);  /* Display the modified line */
    }                              /* if(!NONE) */
  p1711:
    delete(0);                     /* Remove old version of line */
    inslin(curr);                  /* Insert new version */
  }                                /* for(i4=count;i4>0;i4--) */
  if (!lgtmp3)
    goto p1620;                    /* J no lines changed */
  goto p1712;                      /* End Y */
/*
 * FB - BRIEF
 */
p1701:
  if (!eolok())
    goto p1025;
  if (fmode & 01000)
  {
  q1704:
    printf("f%c ignored (mode +v)\r\n", verb);
    goto p1004;
  }
  fmode |= 010000000000;
  fmode &= 017777777777;
  goto p1004;                      /* Finished */
/*
 * FV - VERBOSE
 */
p1702:
  if (!eolok())
    goto p1025;
  fmode &= 07777777777;
  goto p1004;                      /* Finished */
/*
 * FN - NONE
 */
p1703:
  if (!eolok())
    goto p1025;
  if (fmode & 01000)
    goto q1704;
#ifdef ANSI5
  fmode |= 030000000000u;
#else
  fmode |= 030000000000;
#endif
  goto p1004;                      /* Finished */
/*
 * FO - FORGET
 */
p1707:
  if (!eolok())
    goto p1025;
  forget();                        /* In fact implemented by workfile */
  goto p1004;
/*
 * FT - TOKENCHAR
 */
p2003:
  if (strlen(ndel) < 32)
    goto p2007;                    /* J table not full */
  (void)write(1, "no room for further entries", 27);
  goto p1025;
p2007:
/* Get character to add */
  if (scrdtk(1, (unsigned char *)buf, 40, oldcom))
    goto p11042;
  if (oldcom->toktyp != eoltok)
    goto p2008;                    /* J not EOL */
  (void)write(1, "command requires a parameter", 28);
  goto p1025;
p2008:
  if (oldcom->toklen == 1)
    goto p2009;                    /* J 1-char param (good) */
  (void)write(1, "parameter must be single character", 34);
  goto p1025;
p2009:
  if (!eolok())
    goto p1025;
  strcat(ndel, buf);
  goto p1004;
/*
 * FP or ! - DO SHELL COMMAND
 */
p1801:
  if (do_cmd())
  {
    (void)write(1, "bad luck", 8);
    noRereadIfMacro = true;
    goto p1811;                    /* Error has been reported */
  }
  if (!cntrlc)
    goto p1004;                    /* J no ^C in command */
  newlin();
p1901:
  puts("Quit,\r");
  if (!USING_FILE && curmac < 0)
    cntrlc = false;                /* Forget ^C now unless macro */
  goto p1004;
asg2rtn:switch (rtn)
  {
    case 1037:
      goto p1037;
    case 1026:
      goto p1026;
    case 1616:
      goto p1616;
    case 1612:
      goto p1612;
    case 1715:
      goto p1715;
    case 1914:
      goto p1914;
    case 1909:
      goto p1909;
    case 2017:
      goto p2017;
    case 2014:
      goto p2014;
    case 1407:
      goto p1407;
    case 1521:
      goto p1521;
    case 1130:
      goto p1130;
    case 1128:
      goto p1128;
    case 1125:
      goto p1125;
    case 1121:
      goto p1121;
    case 1118:
      goto p1118;
    case 1111:
      goto p1111;
    case 1103:
      goto p1103;
    case 1093:
      goto p1093;
    case 1079:
      goto p1079;
    case 10755:
      goto p10755;
    case 1004:
      goto p1004;
    case 10403:
      goto p10403;
    case 1032:
      goto p1032;
    default:
      printf("Assigned Goto failure, rtn = %d\r\n", rtn);
      return 1;
  }
asg2numok:switch (numok)
  {
    case 1126:
      goto p1126;
    case 1717:
      goto p1717;
    case 1515:
      goto p1515;
    case 1114:
      goto p1114;
    case 1716:
      goto p1716;
    case 1514:
      goto p1514;
    case 1105:
      goto p1105;
    case 10965:
      goto p10965;
    case 1092:
      goto p1092;
    case 1076:
      goto p1076;
    case 1073:
      goto p1073;
    case 1036:
      goto p1036;
    default:
      printf("Assigned Goto failure, numok = %d\r\n", numok);
      return 1;
  }
asg2nofil:switch (nofil)
  {
    case 1132:
      goto p1132;
    case 1069:
      ERR1025("filename must be specified");
    case 10407:
      goto p10407;
    default:
      printf("Assigned Goto failure, nofil = %d\r\n", nofil);
      return 1;
  }
asg2nonum:switch (nonum)
  {
    case 1114:
      goto p1114;
    case 1718:
      goto p1718;
    case 1716:
      goto p1716;
    case 1514:
      goto p1514;
    case 1106:
      goto p1106;
    case 1097:
      goto p1097;
    default:
      printf("Assigned Goto failure, nonum = %d\r\n", nonum);
      return 1;
  }
}
