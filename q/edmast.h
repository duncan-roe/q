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
extern bool modify;                /* Modifying rather than inserting */
extern uint8_t prmpt[10];          /* Prompt etc. */
extern scrbuf5 *newcom;            /* Accepts latest command */
extern scrbuf5 *oldcom;            /* Holds prev. (&returned) command */
extern scrbuf5 *curr;              /* Current line */
extern scrbuf5 *prev;              /* Previous (& returned) line */
extern int pchrs;                  /* # of chrs in the prompt */
extern long lstlin;                /* Last parsed line # (for -TO) */
extern int fscode;                 /* File system &c code */
extern char ermess[Q_BUFSIZ];      /* Text of message from GETLIN */
extern char ubuf[Q_BUFSIZ];        /* Utility buffer for general use */
extern unsigned long dfltmode;
extern int argc;                   /* Copy of invocation arg */
extern char **argv;                /* Copy of invocation arg */
extern int argno;                  /* File # from cmdline (0-based) */
extern int previous_argno;
extern char *sh;                   /* The shell we should use */
#endif
