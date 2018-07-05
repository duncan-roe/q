#ifndef MACROS_H
#define MACROS_H
/* M A C R O S
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012- 2014, Duncan Roe
 *
 * Universal statements for segments accessing SCREENEDIT macros
 * */

/* Macros */

#define DBGMAC 40                  /* How many macro chars dbg shows */
#define STKSIZ 512                 /* Should be enough for anybody */
#define MCLMIT STKSIZ              /* 1st entry off end off stack */
#define MCDTUM 0                   /* No entries used yet */
#define TOPMAC 2047                /* Highest available macro */
#define FIRST_PSEUDO 64            /* First macro shadowed by pseudos */
#define LAST_PSEUDO 127            /* Last macro shadowed by pseudos */
#define BASEMAC sizeof(macro5)-(sizeof(short))*DBGMAC
#define FIRST_IMMEDIATE_MACRO (FIRST_PSEUDO + 1)
#define LAST_IMMEDIATE_MACRO (FIRST_PSEUDO + 15)

/* Typedefs */

typedef struct
{
  short mcsize;                    /* # chars in macro */
  short maclen;                    /* # chars macro could hold */
  unsigned short data[DBGMAC];     /* The macro chars, 16-bit each */
}
macro5;

/* Macro return stack */
struct macinfo
{
  int mcprev;
  int mcposn;
  bool u_use;
} mcstck[STKSIZ];

/* COMMON variables */

macro5 *scmacs[TOPMAC + 1];        /* Pointers to macros */
bool mctrst;                       /* Entrust user (various) */
bool nodup;                        /* DUPLX$ done if false */
int curmac;                        /* Macro being expanded */
int mcposn;                        /* Pos'n in macro */
int mcnxfr;                        /* Index of next free entry in stack */
int immnxfr;                       /* Next free slot for an immediate macro */

/* Prototypes */

bool newmac2(bool appnu);
bool macdef(unsigned int mcnum, unsigned char *buff, int buflen, bool appnu);
bool macdefw(unsigned int mcnum, unsigned short *buff, int buflen, bool appnu);
void showmac(int i);
#endif
