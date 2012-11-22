/* E D M A S T
 *
 * Copyright (C) 1981; D. C. Roe
 * Copyright (C) 2012, Duncan Roe
 *
 * Universal Statements for all Master Subsystem Segments
 */
bool modify;                       /* Modifying rather than inserting */
unsigned char prmpt[10];           /* Prompt etc. */
scrbuf5 *newcom;                   /* Accepts latest command */
scrbuf5 *oldcom;                   /* Holds prev. (&returned) command */
scrbuf5 *curr;                     /* Current line */
scrbuf5 *prev;                     /* Previous (& returned) line */
int pchrs;                         /* # of chrs in the prompt */
long lstlin;                       /* Last parsed line # (for -TO) */
unsigned errlen;                   /* Length of message from GETLIN */
int code;                          /* File system &c code */
char ermess[Q_BUFSIZ];             /* Text of message from GETLIN */
char buf[Q_BUFSIZ];                /* Utility buffer for general use */
unsigned long dfltmode;
int argc;                          /* Copy of invocation arg */
char **argv;                       /* Copy of invocation arg */
int argno;                         /* File # from cmd line (0-based) */
char *sh;                          /* The shell we should use */
