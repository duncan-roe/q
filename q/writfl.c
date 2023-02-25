/* >%---- CODE_STARTS ./writfl.c */
/* W R I T F L
 *
 * Copyright (C) 1993, 1995, 1998, 1999 Duncan Roe & Associates P/L
 * Copyright (C) 2003,2012,2014,2017-2021,2023 Duncan Roe
 *
 * This routine writes out the spec'd # of lines to the file open on
 * funit. If EOF is reached, it reports how many lines were written */

/* >%---- KEEP2HERE ./writfl.c */
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "prototypes.h"
#include "edmast.h"
#include "tabsiz.h"
#include "fmode.h"
/* >%---- CUT_HERE ./writfl.c */
/* */
#define STC(c) do {*q++ = c;\
  if (!--unused) \
  {if (do_write() <0) goto errlbl; q = fbuf; unused = Q_BUFSIZ;}} while (0)

static uint8_t fbuf[Q_BUFSIZ];     /* I/o buffer */
static int unused;                 /* Bytes in f/s buffer */
static int do_write(void);         /* File writing routine */

void
writfl(long wrtnum, bool leave_open)
{
  int bytes;
  long todo, count;
  uint8_t *p, *q, thisch;
  short tabfnd, spacnt, chrpos;
  int inindent;                    /* Inside indenting whitespace */
  const bool fm_plus_y = (fmode & FM_PLUS_Y_BIT) != 0;

/* Initialise */
  count = 0;                       /* No lines written yet */
  todo = wrtnum;                   /* Max # to write */
  q = fbuf;                        /* 1st char goes here */
  unused = Q_BUFSIZ;               /* Room for this many in file fbuf */
  fscode = 0;

/* Main loop on lines. Obey 1 of 2 loops: a fast loop for binary or good-as */
/* and the original per-character loop otherwise                            */

  for (; todo > 0; todo--)
  {
    if (!rdlin(curr, false))
    {
      fprintf(stderr, "End of file reached:- %ld lines written\r\n", count);
      break;
    }
    count++;
    p = curr->bdata;               /* Next char from here */
    bytes = curr->bchars;          /* Bytes in this line */
    if (binary || ((fmode & (TAB_WRITE_BIT | FM_PLUS_L_BIT)) == 0 &&
      (fmode & FM_PLUS_S_BIT)))
    {
      while (bytes >= unused)
      {
        memmove(q, p, unused);
        p += unused;
        bytes -= unused;
        unused = 0;                /* "arg" to do_write :( */
        if (do_write() < 0)
          goto errlbl;
        q = fbuf;
        unused = Q_BUFSIZ;
      }                            /* while (bytes > unused) */
      memmove(q, p, bytes);
      unused -= bytes;
      q += bytes;
    }                              /* if (binary || ...) */
    else
    {
/* Strip trailing whitespace (usually)... */
      if (!(fmode & FM_PLUS_S_BIT))
        while (bytes > 0)
        {
          if (!isspace(curr->bdata[bytes - 1]))
            break;
          bytes--;
        }
      spacnt = 0;                  /* Not pending any spaces */
      tabfnd = 0;                  /* Not seen a real tab yet */
      chrpos = 0;                  /* At line start */

/* In putative whitespace at line start, if we care */
      inindent = fmode & FM_PLUS_L_BIT;

/* Inner loop for this line */
      for (; bytes > 0; bytes--)
      {
        thisch = *p++;             /* Get editor char */

/* Compress spaces to tabs if requested */
        if ((fmode & TAB_WRITE_BIT || inindent) && !tabfnd)
        {
/* If on 8-char bdry & have spaces */
          if (!(chrpos % tabsiz) && spacnt)
          {
            if (spacnt == 1 && !fm_plus_y)
              STC(SPACE);
            else
              STC('\t');
            spacnt = 0;
          }                        /* if(!(chrpos%tabsiz)&&spacnt) */
          chrpos++;                /* Track pos'n in line */
          if (thisch == '\t')
            tabfnd = 1;
          if (thisch == SPACE)

/* Always increase space count, even if zero previously... */
          {
            spacnt++;
            continue;              /* for(;bytes>0;bytes--) */
          }                        /* if(thisch==SPACE) */
          else if (inindent)
            inindent = 0;
          for (; spacnt > 0; spacnt--)
            STC(SPACE);            /* Flush any spaces */
        }                          /* if (!binary && ... */
        STC(thisch);               /* O/p the char */
      }                            /* for(; bytes > 0; bytes--) */

/* Finish off line */
      if (spacnt)
      {
        if (!(chrpos % tabsiz))
        {
          if (spacnt == 1 && !fm_plus_y)
            STC(SPACE);
          else
            STC('\t');
        }                          /* if(!(chrpos%tabsiz)) */
        else
          for (; spacnt > 0; spacnt--)
            STC(SPACE);
      }                            /* if(spacnt) */
    }                              /* if (binary || ...) else */
    if (!binary)
    {
      if (fmode & DOS_WRITE_BIT)
        STC('\r');                 /* If DOS o/p wanted */
      STC('\n');
    }                              /* if (!binary) */
  }                                /* for(;todo>0;todo--) */

/* All requested lines read from Workfile. Write out any partial file
 * buffer and close the file. Then we are finished... */
  if (unused != Q_BUFSIZ)          /* If any chars in buffer */
    if (do_write() < 0)
    errlbl:
      fscode = errno;
  if (fscode)
    fprintf(stderr, "%s. fd %d (write)\r\n", strerror(fscode), funit);
  if (!leave_open)
  {
    SYSCALL(bytes, close(funit));
    if (bytes == -1)
      fprintf(stderr, "%s. fd %d (close)\r\n", strerror(errno), funit);
  }                                /* if (!leave_open) */
  return;
}

/* ******************************** do_write ******************************** */

/*
 * File writing subroutine. Usually writes a full block,
 * but caters for a partial block at end.
 *
 * Because output may be to a pipe,
 * we cater for a write only being partially satisfied.
 */
static int
do_write()
{
  int nc;
  int todo = Q_BUFSIZ - unused;    /* Chars in buffer. Usually Q_BUFSIZ */
  uint8_t *write_from = fbuf;

  while (todo)
  {
    SYSCALL(nc, write(funit, write_from, todo));
    if (nc < 0)
      return nc;                   /* Error */
    if (!nc)
    {
      errno = EDOM;                /* Impossible from f/s normally */
      return -1;
    }                              /* if (!nc) */
    todo -= nc;
    write_from += nc;
  }                                /* while (todo) */
  return 0;                        /* Ok */
}
