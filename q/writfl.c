/* W R I T F L
 *
 * Copyright (C) 1993, 1995, 1998, 1999 Duncan Roe & Associates P/L
 * Copyright (C) 2003,2012,2014 Duncan Roe
 *
 * This routine writes out the spec'd # of lines to the file open on
 * FUNIT. If EOF is reached, it reports how many lines were written...
 *
 * PARAMETER
 * =========
 *
 * WRTNUM - # of lines to write (INTEGER*4)
 *
 * On success, return with the external variable CODE set zero, else set
 * it to ERRNO
 */
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include "alledit.h"
#include "edmast.h"
/* */
#define STC(c) do {*q++ = c;\
  if (!--i) {if (p1() <0) goto errlbl; q = fbuf; i = Q_BUFSIZ;}} while (0)

extern int tabsiz;                 /* How many spaces per tab */

static unsigned char fbuf[Q_BUFSIZ]; /* I/o buffer */
static int i;                      /* Bytes in f/s buffer */
static int p1(void);               /* File writing routine */
void
writfl(long wrtnum)
{
  int bytes;
  long todo, count;
  unsigned char *p, *q, thisch;
  short tabfnd, spacnt, chrpos;
  int inindent;                    /* Inside indenting whitespace */
/*
 * Initialise
 */
  count = 0;                       /* No lines written yet */
  todo = wrtnum;                   /* Max # to write */
  q = fbuf;                        /* 1st char goes here */
  i = Q_BUFSIZ;                    /* Room for this many in file buf */
  fscode = 0;
/*
 * Main loop on lines
 */
  for (; todo > 0; todo--)
  {
    if (!rdlin(curr, 0))
    {
      printf("End of file reached:- %ld lines written\r\n", count);
      break;
    }
    count++;
    p = curr->bdata;               /* Next char from here */
    bytes = curr->bchars;          /* Bytes in this line */
/*
 * Strip trailing spaces (usually)...
 */
    if (!(fmode & 020 || binary))
      while (bytes > 0)
      {
        if (curr->bdata[bytes - 1] != SPACE)
          break;
        bytes--;
      }
    spacnt = 0;                    /* Not pending any spaces */
    tabfnd = 0;                    /* Not seen a real tab yet */
    chrpos = 0;                    /* At line start */
/* In putative w/s at line start, if we care */
    inindent = fmode & 040000;
/*
 * Inner loop for this line
 */
    for (; bytes > 0; bytes--)
    {
      thisch = *p++;               /* Get editor char */
/*
 * Compress spaces to tabs (usually)...
 */
      if (!binary && (fmode & 010 || fmode & inindent) && !tabfnd)
      {
/* If on 8-char bdry & have spaces */
        if (!(chrpos % tabsiz) && spacnt)
        {
          if (spacnt == 1)
            STC(SPACE);
          else
            STC('\t');
          spacnt = 0;
        }                          /* if(!(chrpos%tabsiz)&&spacnt) */
        chrpos++;                  /* Track pos'n in line */
        if (thisch == '\t')
          tabfnd = 1;
        if (thisch == SPACE)
/* Always increase space count, even if zero previously... */
        {
          spacnt++;
          continue;                /* for(;bytes>0;bytes--) */
        }                          /* if(thisch==SPACE) */
        else if (inindent)
          inindent = 0;
        for (; spacnt > 0; spacnt--)
          STC(SPACE);              /* Flush any spaces */
      }                            /* if(!binary&&fmode&010&&!tabfnd) */
      STC(thisch);                 /* O/p the char */
    }                              /* for(;bytes>0;bytes--) */
/*
 * Finish off line
 */
    if (!binary)
    {
      if (spacnt)
      {
        if (!(chrpos % tabsiz))
        {
          if (spacnt == 1)
            STC(SPACE);
          else
            STC('\t');
        }                          /* if(!(chrpos%tabsiz)) */
        else
          for (; spacnt > 0; spacnt--)
            STC(SPACE);
      }                            /* if(spacnt) */
      if (fmode & 2)
        STC('\r');                 /* If DOS o/p wanted */
      STC('\n');
    }                              /* if(!binary) */
  }                                /* for(;todo>0;todo--) */
/*
 * All requested lines read from Workfile. Write out any partial file
 * buffer and close the file. Then we are finished...
 */
  if (i != Q_BUFSIZ)               /* If any chars in buffer */
    if (p1() < 0)
    errlbl:
      fscode = errno;
  for (;;)                         /* Loop past interrupts */
  {
    if (!close(funit))
      break;
    if (errno == EINTR)
      continue;
    perror("Closing file");
    putchar('\r');
    break;
  }
  return;
}
/*
 * P1 - File writing subroutine. Usually writes a full block, but
 *      caters for a partial block at end.
 */
static int
p1()
{
  int k;                           /* Scratch */
  int todo = Q_BUFSIZ - i;         /* Chars in buffer. Usually Q_BUFSIZ */
  k = write(funit, fbuf,
#ifdef ANSI5
    (size_t)
#endif
    todo);
  if (k < 0)
    return k;                      /* Error */
  if (k != todo)
  {
    printf("\aFile writing problem\r\nRequested: %d\r\n    Wrote: %d\r\n",
      todo, k);
    errno = EDOM;                  /* Impossible from f/s normally */
    return -1;
  }
  return 0;                        /* Ok */
}
