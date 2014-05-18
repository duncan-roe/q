/*  S C R D T K
 * Retrieve tokens from screenedit buffers
 * Copyright (C) 1998, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2014 Duncan Roe
 */
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include "alledit.h"
int scrdtk                         /* errno is int, so we are */
  (int key,                        /* What to do */
  unsigned char *buf,              /* Receives token */
  int bufcap,                      /* Capacity of buf */
  scrbuf5 *scline)                 /* Screenedit buffer */
{

/* Possible key values
 * ======== === ======
 *
 * 1:  read token, force to uppercase
 * 2:  Read token, leave case alone
 * 3:  Reset to parse from buffer start
 * 4:  Read raw input line from cursor pos'n to buffer end
 * 5:  Initialize buffer state (in fact the same as 3) */

  int chr;                         /* Scratch */
  int tknidx;                      /* Copy of bcurs for efficiency */
  int tbegin;                      /* Token start */
  int bufidx;                      /* Buffer pointer */
  int littxt;                      /* Inside quotes */
  int trunca;                      /* Returned token is truncated */
/*  */
  errno = 0;                       /* Set good return code */

/* The rest of the function is a switch on the value of key */

  if (bufcap < 0)                  /* Invalid arg */
    errno = EINVAL;
  else
    switch (key)
    {
      case 3:
      case 5:
        scline->nulcma = true;
        scline->tokbeg = scline->bcurs = 0;
        break;
      case 4:
      case 1:
      case 2:
        trunca = littxt = 0;       /* Internal flags */
        scline->plusf = scline->minusf = scline->decok = scline->octok = false;
        scline->toktyp = nortok;   /* Start w/normal type token */
        bufidx = 0;
        tknidx = scline->bcurs;
        tbegin = key == 4 ? tknidx : -1; /* -1 indicates no token found */
        for (; tknidx < scline->bchars; tknidx++)
        {
          chr = scline->bdata[tknidx];
          if (key != 4)
          {
            if (tbegin < 0)        /* Looking for token start */
            {
              if (chr == SPACE)    /* Another SPACE */
                continue;          /* Keep looking for token start */
              if (chr == COMMA)
              {
                if (scline->nulcma) /* Comma implies null token */
                {
                  scline->toktyp = nultok;
                  tbegin = tknidx; /* Where null token is */
                  break;
                }                  /* if(scline->nulcma) */
                scline->nulcma = true; /* Note COMMA encountered */
                continue;          /* Keep looking for token start */
              }                    /* if(chr=',') */
              if (chr == SLASH && tknidx < scline->bchars - 1 &&
                scline->bdata[tknidx + 1] == ASTRSK)
              {                    /* Token is SLASH-star comment */
                tknidx = scline->bchars; /* Look like end-of-line */
                break;
              }                    /* else if(chr==SLASH&&... */
              tbegin = tknidx;     /* Token starts here */
              scline->nulcma = false;
              if (chr == QUOTE)    /* Token starts with quote */
              {
                littxt = 1;
                continue;          /* Don't store quote as part of token */
              }                    /* if (chr == QUOTE) */
              if (chr == '+')
                scline->plusf = true;
              else if (chr == '-')
                scline->minusf = true;
              else if (chr == '9' || chr == '8')
              {
                scline->decok = true;
                scline->decval = chr - '0';
              }                    /* else if(chr=='9'||chr='8') */
              else if (chr >= '0' && chr <= '7')
              {
                scline->decok = scline->octok = true;
                scline->decval = scline->octval = chr - '0';
              }                    /* else if(chr>='0'&&chr<='7') */
              if (scline->plusf || scline->minusf)
              {
                scline->decok = true;
                scline->decval = 0;
              }                    /* if(scline->plusf||scline->minusf) */
            }                      /* if(tbegin<0) */
            else
            {
              if (littxt)          /* Only significant char is another quote */
              {
                if (chr == QUOTE)
                {                  /* See if quote is 1st of a pair */
                  if (tknidx < scline->bchars - 1 &&
                    scline->bdata[tknidx + 1] == QUOTE)
                    tknidx++;      /* Address 2nd quote */
                  else
                  {
                    littxt = 0;    /* Back to normal processing */
                    continue;      /* Don't store quote as part of token */
                  }                /* if(tknidx<scline->bchars-1&&... else */
                }                  /* if(chr == QUOTE) */
              }                    /* if(littxt) */
              else
              {
                if (chr == SPACE)
                  break;           /* End of token */
                if (chr == COMMA)
                {
                  scline->nulcma = true; /* Null token if COMMA next */
                  break;           /* Token end */
                }                  /* if(chr==COMMA) */
                if (chr == QUOTE)
                {
                  littxt = 1;
                  scline->minusf = false;
                  scline->plusf = false;
                  scline->decok = false;
                  scline->octok = false;
                  continue;        /* Don't store as part of token */
                }                  /* if (chr == QUOTE) */
                if (chr == SLASH && tknidx < scline->bchars - 1 &&
                  scline->bdata[tknidx + 1] == ASTRSK)
                {
                  tknidx--;        /* Want to see '/' next time */
                  break;           /* End of token */
                }                  /* else if(chr==SLASH&&... */
                if (scline->decok)
                {
                  if (chr >= '0' && chr <= '9')
                  {
                    if (scline->decval < LONG_MAX / 10) /* *10 won't overflow */
                      scline->decval = scline->decval * 10 + chr - '0';
                    else
                      scline->decok = false; /* Overflowed */
                  }                /* if(chr>='0'&&chr<='9') */
                  else
                    scline->decok = false;
                }                  /* if(scline->decok) */
                if (scline->octok)
                {
                  if (chr >= '0' && chr <= '7')
                    scline->octval = (scline->octval << 3) + chr - '0';
                  else
                    scline->octok = false;
                }                  /* if(scline->octok) */
              }                    /* if(littxt) else */
            }                      /* if(tbegin<0) else */
          }                        /* if(key!=4) */
          if (bufcap)              /* Buffer supplied for token */
          {
            if (bufidx < bufcap - 1)
            {                   /* Room to store in buf leaving room for null */
              if (!littxt && key == 1 && chr >= 'a' && chr <= 'z')
                chr -= 040;        /* Convert to uppercase */
              buf[bufidx++] = chr; /* Xfer char */
            }                      /* if(bufidx<bufcap-1) */
            else
              trunca = 1;          /* Returned token truncated */
          }                        /* if(bufcap) */
        }                          /* for(;tknptr<linend;tknptr++) */
        if (tknidx < scline->bchars) /* Not reached line end */
          tknidx++;                /* Skip token delimiter */
        if (tbegin < 0)            /* No token before line end */
        {
          if (scline->nulcma && scline->bcurs)
/* I.e. scline->nulcma set other than at line start */
            scline->toktyp = nultok;
          else
            scline->toktyp = eoltok;
          scline->nulcma = false;  /* Return 1 null token max */
        }                          /* if(tbegin<0) */
        else
          scline->tokbeg = tbegin;
        scline->bcurs = tknidx;    /* For next call */
        if (bufcap)
        {
          scline->toklen = bufidx;
          buf[bufidx] = '\0';      /* Terminate string */
          if (trunca)              /* Returned token is truncated */
            errno = ENOMEM;
        }                          /* if(bufcap) */
        else
          scline->toklen = 0;
        if (scline->decok && scline->minusf)
          scline->decval = -scline->decval;
        break;                     /* switch (key) */
      default:
        errno = EINVAL;            /* Bad key */
    }                              /* switch (key) */
  return errno;                    /* Exit */
}          /* int scrdtk(int key,unsigned char*buf,int bufcap,scrbuf5*scline) */
