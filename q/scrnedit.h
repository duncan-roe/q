#ifndef SCRNEDIT_H
#define SCRNEDIT_H
/* S C R N E D I T
 *
 * Copyright (C) 1981, 2011 D. C. Roe
 * Copyright (C) 2014 Duncan Roe
 *
 * Universal Statements for Screenedit Subsystem Segments
 */

/* Allow for up to 512 screen width - should be good for a while... */
#define SCRMAX 512
#define PRSIZ 16
#define PRMAX 15
#define WCHRS col5-1
unsigned char screen[SCRMAX],reqd[SCRMAX],prompt[PRSIZ],crsbuf[SCRMAX];
unsigned char rtchrs,backsp,cachrs[PRSIZ];
int cursr,scurs,pchars,icurs,crscnt,rtcnt;
int cacnt,tabcnt,cdone,partno,mxchrs;
bool insert,rfrsh,endlin;
bool bspace,cntrlw;
unsigned long zmode;               /* Copy of fmode by scmnrd */
double delay;
/*
 * SCREEN - What's on the screen now
 * REQD   - What we want on the screen
 * CURSR  - Where the screen cursor should be
 * SCURS  - Where the screen cursor is
 * WCHRS  - Usable width of screen (including prompts etc) (chars)
 * CHRSOP - # of chars o/p to screen since last delay
 * DELAY  - seconds we guess it takes to o/p a char.
 * PCHARS - # of chars in supplied prompt - zero if none supplied
 * PROMPT - Text of the supplied prompt
 * ICURS  - Local cursor variable: pos'n on screen of last char done
 * INSERT - We are in insert mode
 * RFRSH  - true if no need for a further REFRSH
 * ENDLIN - Set if EOL encountered
 * CRSBUF - Chars to set cursor to required position
 * CRSCNT - No. of sig chars in CRSBUF
 * BSPACE - This terminal can backspace
 * RTCNT  - # of chars to effect NL (max 2)
 * RTCHRS - The characters to effect NL
 * CACNT  - # of chars to effect ^A (max 40)
 * CACHRS - Chars to effect ^A (primitive, i.e. get last posn on screen)
 *          CACNT includes any number of backspaces + these chars (max 2)
 * TABCNT - # of tabs currently defined by the "T" command
 * TABS   - The tabs, in ascending positional order
 * CDONE  - # of chars dealt with by REFRSH (for DISPLY)
 * PARTNO - Part # of line being displayed (for DISPLY)
 * MXCHRS - Max # chars ever in line (for ^R)
 * BACKSP - Value of 2 backspaces. Variable for terminals that use ^Y
 * CNTRLW - true if ^W seen
 */
 #endif
