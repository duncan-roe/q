#ifndef SCRNEDIT_H
#define SCRNEDIT_H
/* S C R N E D I T
 *
 * Copyright (C) 1981, 2011 D. C. Roe
 * Copyright (C) 2014,2015,2019 Duncan Roe
 *
 * Universal Statements for Screenedit Subsystem Segments
 */

/* Allow for up to 512 screen width - should be good for a while... */
#define SCRMAX 4096
#define PRSIZ 16
#define PRMAX 15
#define WCHRS (col5 - 1)
uint8_t screen[SCRMAX], reqd[SCRMAX], prompt[PRSIZ], crsbuf[SCRMAX];
uint8_t rtchrs, backsp, cachrs[PRSIZ];
int cursr, scurs, pchars, icurs, crscnt, rtcnt;
int cacnt, tabcnt, cdone, partno, mxchrs;
bool insert, rfrsh, endlin;
bool bspace;
double delay;
/*
 * screen - What's on the screen now
 * reqd   - What we want on the screen
 * cursr  - Where the screen cursor should be
 * scurs  - Where the screen cursor is
 * wchrs  - Usable width of screen (including prompts etc) (chars)
 * chrsop - # of chars o/p to screen since last delay
 * delay  - seconds we guess it takes to o/p a char.
 * pchars - # of chars in supplied prompt - zero if none supplied
 * prompt - Text of the supplied prompt
 * icurs  - Local cursor variable: pos'n on screen of last char done
 * insert - We are in insert mode
 * rfrsh  - true if no need for a further REFRSH
 * endlin - Set if EOL encountered
 * crsbuf - Chars to set cursor to required position
 * crscnt - No. of sig chars in CRSBUF
 * bspace - This terminal can backspace
 * rtcnt  - # of chars to effect NL (max 2)
 * rtchrs - The characters to effect NL
 * cacnt  - # of chars to effect ^A (max 40)
 * cachrs - Chars to effect ^A (primitive, i.e. get last posn on screen)
 *          cacnt includes any number of backspaces + these chars (max 2)
 * tabcnt - # of tabs currently defined by the "T" command
 * tabs   - The tabs, in ascending positional order
 * cdone  - # of chars dealt with by REFRSH (for DISPLY)
 * partno - Part # of line being displayed (for DISPLY)
 * mxchrs - Max # chars ever in line (for ^R)
 * backsp - Value of 2 backspaces. Variable for terminals that use ^Y
 */
#endif
