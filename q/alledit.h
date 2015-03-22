/* A L L E D I T . H */
#ifndef ALLEDIT_H
#define ALLEDIT_H

/* Headers required by this header */

#include <signal.h>
#include <sys/types.h>
#include "ckalloc.h"
#include "prototypes.h"     /* New code uses this unless it needs stuff below */

/* Global variables */

unsigned int verb;           /*Command Processing - COMANL, ONEOF, NEWMAC &c. */
int ndntch;                        /* # of chars to indent */
bool vt100;                        /* Enable VT100-style curpos */
bool deferd;                       /* Deferred indexing of mmap'd input file */
bool locerr;                       /* LOCATE error only - macro can detect */
bool noRereadIfMacro;              /* Don't re-read cmd if in macro &c */
bool forych;   /* PDSPLY only required to do a BRIEF display (for YCHANGEALL) */
bool lstvld;                       /* "previous" buffer has valid data */
bool modlin;                       /* This line has actually changed */
bool mods;                         /* Mods done since last SAVE */
bool binary;                       /* Q invoked -b */
long lintot;                       /* Total # of lines known to system */
long ptrpos;                       /* Line # where pointer is now */
int funit;                         /* File i/o ptr */
char pcnta[256];                   /* Pathname we are editing */
unsigned int row5, col5;           /* Screen / window geometry */
bool cntrlc, seenwinch;
char *macro_dir;                   /* Where macros are */
int orig_stdout;                   /* stdout funit at start */
#endif
