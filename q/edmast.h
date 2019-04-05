#ifndef EDMAST_H
#define EDMAST_H
/* E D M A S T
 *
 * Copyright (C) 1981; D. C. Roe
 * Copyright (C) 2012,2014,2019 Duncan Roe
 *
 * Universal Statements for all Master Subsystem Segments
 */

/* Headers required by prototypes &c. */
#include "typedefs.h"

/* Prototypes */
void q_version(void);

/* Variables */
bool modify;                       /* Modifying rather than inserting */
uint8_t prmpt[10];                 /* Prompt etc. */
scrbuf5 *newcom;                   /* Accepts latest command */
scrbuf5 *oldcom;                   /* Holds prev. (&returned) command */
scrbuf5 *curr;                     /* Current line */
scrbuf5 *prev;                     /* Previous (& returned) line */
int pchrs;                         /* # of chrs in the prompt */
long lstlin;                       /* Last parsed line # (for -TO) */
int fscode;                        /* File system &c code */
char ermess[Q_BUFSIZ];             /* Text of message from GETLIN */
char ubuf[Q_BUFSIZ];               /* Utility buffer for general use */
unsigned long dfltmode;
int argc;                          /* Copy of invocation arg */
char **argv;                       /* Copy of invocation arg */
int argno;                         /* File # from cmdline (0-based) */
char *sh;                          /* The shell we should use */
#endif
