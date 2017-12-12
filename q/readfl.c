/* R D F I L E
 * Copyright (C) 1993,1998 Duncan Roe & Associates P/L
 * Copyright (C) 2005,2012-2014,2017  Duncan Roe
 *
 * This routine reads in a file, inserting it in the Workfile.
 *
 * Tab expansion takes cognizance of Bs and Cr characters.
 * Equivalent funtionality is not provided when writing: use "fm -tw".
 * The feature is intended for use when editing man pages, so this should not be
 * a problem.
 */
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include "prototypes.h"
#include "edmast.h"
#include "macros.h"
#include "fmode.h"
#include "c1in.h"
#include "tabsiz.h"

/* Initialise External Variables */

int tabsiz = 8;                    /* How often tabstops are */

static long dcount;                /* # deferred lines read */
static unsigned long dfmode;       /* Mode at deferred map time */
static unsigned char *defend;      /* Deferred file end */
static unsigned char *defpos;      /* Deferred file current address */
static unsigned char *dfaddr;      /* Deferred file start address */
static long count;                 /* # of lines read */
static unsigned char prwfbf[Q_BUFSIZ]; /* I/o buffer */
static unsigned char *bufpos;      /* Where next char comes from */
static unsigned char *prvpos;      /* Where last record started */
static unsigned char *bufend;      /* Current file end addr */
static int linpos;         /* Notional 0-based line pos'n (for tab expansion) */
static unsigned char thisch, prevch;
static unsigned long bytes;        /* Unprocessed file bytes in memory */
static int pbchars;                /* local copy of xxprev->bchars for speed */
static bool fileio;                /* Whether i/o from file */
static bool lngwrn;        /* Whether to store xxprev, warn of long lines &c. */
static scrbuf5 *xxprev;            /* Buffer where line is built up */
static unsigned long xxmode;       /* Copy of fmode, as is or was */
static short state;                /* Cr &c state */
/*
 * STATE VARIABLE TABLE:-
 * 0 - nothing special
 * 1 - Seen Cr when reading in DOS mode
 * 2 - Storing spaces until next tab stop
 *
 * STATE TRANSITIONS:-
 * 0->1  Saw Cr
 * 0->2  Saw Ht
 * 1->0  Saw Nl
 * Store Cr then 1->0  Saw other than Nl
 * 2 -> 0 Finished storing spaces
 */

/* ******************************* endLin ****************************** */

static void
endLin(void)
{
  count++;                         /* Up # of lines read */
  xxprev->bchars = pbchars;        /* Set = local copy */
  if (fileio)
    inslin(xxprev);                /* Insert line read */
  else
  {
    insmem(prvpos, bufend);        /* Store reference to this line */
    prvpos = bufpos;               /* Next line start prvpos */
  }                                /* if(fileio) else */
}                                  /* static void endLin(void) */

/* ******************************* getblk ****************************** */

static int
getblk(void)                       /* eof=getblk() */
{
  for (;;)                         /* Loop until good read */
  {
    if (!fileio || !(bytes = read(funit, prwfbf, Q_BUFSIZ)))
    {                              /* End of file reached */
      if (state == 1)              /* Holding on to a Cr */
        xxprev->bdata[pbchars++] = '\r'; /* Store the Cr */
      if (pbchars && lngwrn)
      {
        if (!binary)
          printf("Warning - last line unterminated\r\n");
        endLin();                  /* Store this line */
      }                            /* if(pbchars) */
      return 1;                    /* Indicate eof */
    }                             /* if(!(bytes=read(funit,prwfbf,Q_BUFSIZ))) */
    if (bytes > 0)
      break;                       /* J no error */
    if (errno == EINTR)
      continue;                    /* reread on interrupt */
    perror("Reading file");
    putchar('\r');
    return 1;                      /* Simulate eof */
  }                                /* for(;;) */
  bufpos = prwfbf;                 /* Where chars come from */
  return 0;                        /* Still going */
}                                  /* static int getblk(void) */

/* ******************************* prclin ****************************** */

static int
prclin(void)                       /* Build up line; return 1 for eof */
{
  unsigned char *pbptr;            /* -> xxprev->bdata[bchars] for speed */
  int inindent = 040000;           /* Inside indenting w/s, mask matches mode */
/*  */
  for (pbchars = linpos = 0, pbptr = xxprev->bdata; pbchars < BUFMAX; linpos++)
  {
    if (state != 0 && state != 1)
    {                              /* Else assume state 2 */
      if (!(linpos % tabsiz))      /* Reached tabstop (multiple of 8) */
        state = 0;                 /* Fall through */
      else
        goto endswitch;            /* switch(state) case 2 */
    }                              /* if(state!=0&&state!=1) */
    if (!bytes && getblk())        /* Eof, getblk writes data */
      return 1;
    thisch = *bufpos++;
    bytes--;

/* When reading in binary, try to guess good places to break "lines"
 * (which are arbitrary anyway) unless requested not to (for w/f to use
 * the least amount of memory). (LATER)
 * We also try not to break strings of printable characters. */

    if (binary)
    {
      if (thisch == '\n' || (thisch == 0 && pbchars > BLKCAP / 2) ||
        (pbchars >= BLKCAP - 2 && (prevch < ' ' || prevch > '\176')))
      {
        *pbptr++ = prevch = thisch;
        pbchars++;                 /* Store thisch */
        return 0;                  /* Finish this line */
      }                            /* if(thisch=='\n'||(thisch==0&&... */
    }                              /* if(binary) */
    else
/* For Ascii files, now process significant characters: Newline, Return & Tab */

    {
      if (inindent && thisch != '\t' && thisch != ' ')
        inindent = 0;
      if (thisch < ' ' || state)   /* One of the following tests might pass */
      {
        if (thisch == '\r')
        {

/* If doing a DOS read and already holding on to a Cr, store it and stay in
 * state 1. Else, move to state 1 and store nothing */

          if (xxmode & 1 && state != 1) /* DOS read, not holding Cr */
          {
            state = 1;             /* Holding a Cr */
            continue;              /* for(pbchars=linpos... (don't store Cr) */
          }                        /* if(xxmode&1) */
          linpos = -1;             /* Back at line start */
          goto endswitch;          /* switch(state) cases 0 & 1 */
        }                          /* if(thisch=='\r') */
        if (thisch == '\n')
        {
          if (state == 1)          /* Holding Cr */
            state = 0;             /* Discard Cr */
          if (state != 0)          /* Unexpected */
            printf("\r\n\aUnexpected state %d at Nl\r\n", state);
          return 0;                /* Have line */
        }                          /* if(thisch=='\n') */
        else if (state == 1)       /* Holding on to Cr 2B stored now */
        {
          bytes++, bufpos--;       /* Put back char just read */
          state = 0;
          thisch = '\r';
          linpos = -1;             /* Back at line start */
        }                          /* else if(state==1) */

/* Only check for tab character after return has been checked for */

        if (thisch == '\t' || thisch == '\b')
        {
/* +tr || +l in ldg w/s */
          if ((xxmode & 4) || (xxmode & 040000 & inindent))
          {
            if (thisch == '\t')
            {
              thisch = ' ';        /* Store spaces */
              state = 2;           /* Until on next 8-char boundary */
            }                      /* if(thisch=='\t' */
            else if (linpos > 0)   /* thisch must be '\b' */
              linpos -= 2;
          }                        /* if(xxmode&4) */
        }                          /* if(thisch=='\t'||thisch=='\b') */
      }                            /* if(thisch<' '||state) */
    }                              /* if(binary) else */
  endswitch:
    *pbptr++ = prevch = thisch;
    pbchars++;
  }      /* for(pbchars=linpos=0,pbptr=xxprev->bdata;pbchars<BUFMAX;linpos++) */
/*
 * Dropped out of loop: we have a full line. If the file is binary, this
 * is the norm, just write away the chars read.
 * Otherwise warn user, drain chars until EOL...
 */
  state = 0;                       /* No longer holding on to Cr */
  if (!binary && lngwrn)
  {
/* If the very next ch is Nl, we haven't actually misssed anything, */
/* so defer outputting a warning here. */
    bool tried_next_ch = false;

    for (;;)
    {
      if (!bytes && getblk())
      {
        if (!tried_next_ch)
          puts("Long line found!\r");
        return 1;
      }                            /* if (!bytes && getblk()) */
      thisch = *bufpos++;
      bytes--;
      if (thisch == '\n')          /* Eol */
        break;
      if (!tried_next_ch)
      {
        tried_next_ch = true;
        puts("Long line found!\r");
      }                            /* if (!tried_next_ch) */
    }                              /* for(;;) */
  }                                /* if(!binary&&lngwrn) */
  return 0;
}                                  /* static int prclin(void) */

/* ******************************* ctlcak ****************************** */

/* Called when an interrupt (i.e. ^C) has been noticed.
 * If in a macro or U-use file,
 * abandon w/out asking a question, and do not clear the ^C flag.
 * Else ask */

static bool
ctlcak(void)
{
  bool doctlc = curmac >= 0 || USING_FILE; /* Don't query after ^C seen */
/*  */
  printf("%ld lines read so far...\r\n", count);
  cntrlc = doctlc;                 /* ^C ack'd if question 2b asked */
  return doctlc || ysno5a("Do you wish to abandon inputting this file", A5NDEF);
}                                  /* static int ctlcak(void) */

/* ******************************* readfl ****************************** */

void
readfl()
{
  lngwrn = true;                   /* Not memrec */
  fileio = true;                   /* Read from file */
  count = 0;                       /* No lines read yet */
  state = 0;                       /* Normal state initially */
  xxmode = fmode;                  /* Use current mode */
  xxprev = prev;                   /* prev contains last line on exit */
  xxprev->bcurs = 0;               /* Cursor at line start */
  bytes = 0;                       /* Nothing read from file yet */
  for (;;)                         /* Loop on lines */
  {
    if (cntrlc && ctlcak())      /* user typed a ^C & confirmed (or macro &c) */
      break;
    if (prclin())                  /* Eof */
      break;
    endLin();
  }                                /* for(;;) */
  printf("%ld lines read.\r\n", count);
}

/* ******************************* mapfil ****************************** */

#ifdef ANSI5
void
mapfil(ino_t inode, off_t size, unsigned char *addr)
#else
void
mapfil(inode, size, addr)
unsigned long inode;
long size;
unsigned char *addr
#endif
{
  newmap(inode, size, addr);      /* Tell the Workfile system about this file */

/* If we can defer reading the file, do so */

  if (ptrpos == lintot + 1 && fmode & 010000 && !deferd)
  {
    dcount = 0;                    /* No lines read in from file */
    dfmode = fmode;                /* Remember mode at map time */
    defend = addr + size;          /* Remember file size */
    defpos = dfaddr = addr;        /* Remember file start & current addresses */
    deferd = true;                 /* Tell the world */
    printf("%lld bytes mapped.\r\n", (long long int)size); /* Tell the user */
    return;
  }                                /* if(ptrpos==lintot+1&&fmode&010000&&... */

  lngwrn = true;                   /* Not memrec */
  fileio = false;                  /* File already mmap'd */
  count = 0;                       /* No lines read yet */
  state = 0;                       /* Normal state initially */
  xxmode = fmode;                  /* Use current mode */
  xxprev = prev;                   /* prev contains last line on exit */
  xxprev->bcurs = 0;               /* Cursor at column 1 on exit */
  bytes = size;                    /* Entire file in memory */
  prvpos = bufpos = addr;          /* Data starts here */
  bufend = addr + bytes;           /* Data ends here */
  for (;;)                         /* Loop on lines */
  {
    if (cntrlc && ctlcak())      /* user typed a ^C & confirmed (or macro &c) */
      break;
    if (prclin())                  /* Eof */
      break;
    endLin();
  }                                /* for(;;) */
  printf("%ld lines read.\r\n", count);
}                                  /* void mapfil(ino_t inode,off_t size) */

/* ******************************* memrec ****************************** */

#ifdef ANSI5
void
memrec(unsigned char *start, unsigned char *end, unsigned long mode, scrbuf5 *s)
#else
void
memrec(start, end, mode, s)
unsigned char *start;
unsigned char *end;
unsigned long mode;
scrbuf5 *s;
#endif
{
  lngwrn = false;                  /* This *is* memrec */
  fileio = false;                  /* File already mmap'd */
  state = 0;                       /* Normal state initially */
  bytes = end - start;             /* Remaining length of file in memory */
  bufend = end;                    /* Where data ends */
  bufpos = start;                  /* Line start addr */
  xxprev = s;                      /* Assemble line in a4 */
  xxmode = mode;                   /* Use supplied mode */
  prclin();                        /* Read line, don't care if eof */
  s->bchars = pbchars;             /* Store how long line is */
}                                  /* void memrec(unsigned char*start,... */

/* ******************************* dfread ****************************** */

/* Attempt to read up to num lines from the current deferred file. If we reach
 * eof, clear the deferred flag. Special values of num:-
 *
 * 1 - Do not check here for ^C (probably L or Y calling: they check)
 * ***SPECIAL NOTE: for now, we *never* check for ^C. It is too dangerous,
 *                  consider: If we were reading lines for "S"ave and the user
 *                            interrupted, the Save of the partial file would
 *                            proceed, with the original being *deleted*.
 *                  Perhaps one day we can do something about this
 * LONG_MAX - Report total # lines read */

#ifdef ANSI5
void
dfread(long num, scrbuf5 *s)
#else
void
dfread(num)
long num;
scrbuf5 *s;
#endif
{
  long savpos = ptrpos;            /* Workfile pos'n req'd on exit */
  long l;                          /* Scratch */
  scrbuf5 s5;                      /* For when a2 is NULL */
/*  */
  setptr(lintot + 1);              /* Move to eof */
  lngwrn = true;                   /* Not memrec */
  fileio = false;                  /* File already mmap'd */
  count = dcount;                  /* Lines read in from file */
  state = 0;                       /* Normal state initially */
  xxmode = dfmode;                 /* Set mode at map time */
  if (s)
    xxprev = s;                    /* Use a2 */
  else
    xxprev = &s5;
  xxprev->bcurs = 0;               /* Cursor at column 1 on exit */
  bytes = defend - defpos;         /* Size remaining */
  bufend = defend;                 /* End address of file */
  prvpos = bufpos = defpos;        /* Data starts here */
  if (num == 1)
  {
    if (prclin())
      deferd = false;
    else
      endLin();
  }                                /* if(num==1) */
  else
    for (l = 0; l < num; l++)      /* Loop on lines */
    {
      if (prclin())                /* Eof */
      {
        deferd = false;
        break;
      }                            /* if()cntrlc&&ctlcak())||prclin()) */
      endLin();
    }                              /* for(l=0;l<num;l++) */
/* Finished with deferred file & want total lines */
  if (!deferd && num == LONG_MAX)
    printf("%ld lines read.\r\n", count);
  dcount = count;                  /* Lines read in from file */
  defpos = bufpos;                 /* Start of still-deferred data */
  setptr(savpos);                  /* Restore old file pos'n */
}                                  /* void dfread(long num,scrbuf5*s) */
