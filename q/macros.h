/* M A C R O S
 *
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012, Duncan Roe
 *
 * Universal statements for segments accessing SCREENEDIT macros
 * */
#define DBGMAC 40                  /* How many macro chars dbg shows */
#define STKSIZ 512                 /* Should be enough for anybody */
#define MCLMIT STKSIZ              /* 1st wd off end off stack */
#define MCDTUM 0                   /* No words used yet */
#define TOPMAC 2047                /* Highest available macro */
#define FIRST_PSEUDO 64            /* First macro shadowed by pseudos */
#define LAST_PSEUDO 127            /* Last macro shadowed by pseudos */
#define BASEMAC sizeof(macro5)-(sizeof(short))*DBGMAC
#define FIRST_INLINE_MACRO (FIRST_PSEUDO + 1)
#define LAST_INLINE_MACRO (FIRST_PSEUDO + 15)
int newmac2(int mcchrs);
typedef struct
{
  short mcsize;                    /* # chars in macro */
  short maclen;                    /* # chars macro could hold */
  unsigned short data[DBGMAC];     /* The macro chars, 16-bit each */
}
macro5;
macro5 *scmacs[TOPMAC + 1];        /* Pointers to macros */
bool mctrst;                       /* Entrust user (various) */
bool nodup;                        /* DUPLX$ done if false */
int curmac;                        /* Macro being expanded */
int mcposn;                        /* Pos'n in macro */
int mcnxfr;                        /* Addr next free wd in stack */
/* Macro return stack */
struct
{
  int mcprev;
  int mcposn;
} mcstck[STKSIZ];
