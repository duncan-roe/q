#ifndef SCRNEDIT_H
#define SCRNEDIT_H
/* S C R N E D I T
 *
 * Copyright (C) 1981, 2011 D. C. Roe
 * Copyright (C) 2014,2015,2019 Duncan Roe
 *
 * Universal Statements for Screenedit Subsystem Segments
 */

/* Headers required by this header */

#include "prototypes.h"

/* Macros */

/* Allow for up to 4096 screen width - should be good for a while... */
#define SCRMAX 4096
#define PRSIZ 16
#define PRMAX 15
#define WCHRS (col5 - 1)

extern uint8_t screen[SCRMAX];     /* What's on the screen now */
extern uint8_t reqd[SCRMAX];       /* What we want on the screen */
extern uint8_t prompt[PRSIZ];      /* Text of the supplied prompt */
extern uint8_t crsbuf[SCRMAX];    /* Chars to set cursor to required position */
extern uint8_t backsp;             /* Character that does a backspace */
extern uint8_t cachrs[PRSIZ];  /* Chars to effect ^A (-> last posn on screen) */
extern int cursr;                  /* Where the screen cursor should be */
extern int scurs;                  /* Where the screen cursor is */
extern int pchars;                 /* # of chars in supplied prompt else 0 */
extern int crscnt;                 /* # significant chars in crsbuf */
extern int cacnt;                  /* # chars to effect ^A */
extern int tabcnt;                 /* # tabs currently defined by T-TABSET */
extern int cdone;            /* # chars dealt with by scrset() (for disply()) */
extern int partno;           /* Part # of line being displayed (for disply()) */
extern int mxchrs;                 /* Max # chars ever in line (for ^R) */
extern bool insert;                /* We are in insert mode */
extern bool rfrsh;                 /* True if no need for a further refrsh() */
extern bool endlin;                /* Set if EOL encountered */
#endif
