#ifndef ALLEDIT_H
#define ALLEDIT_H
/* A L L E D I T
 */

/* Headers required by prototypes &c. */
#include <signal.h>
#include <sys/types.h>
#include "ckalloc.h"
#include "bool.h"

/* Macros */
#define CARAT '^'
#define SPACE ' '
#define LT '<'
#define ESC '\33'
#define DEL 0177
#define CTL_U 025
#define GT '>'
#define SLASH '/'
#define QM '?'
#define COMMA ','
#define A5NDEF 0
#define A5DYES 1
#define A5DNO -1
/* */
#define Q_BUFSIZ 8192
#define BUFMAX Q_BUFSIZ-1
#define PTHSIZ 256
#define PTHMAX PTHSIZ-4            /* Allow for .tm */
#define FTNMOD (fmode&01000000000)
#define CASDEP ((fmode&02000000000)==0)
#define INDENT (fmode&04000000000)
/* BRIEF and NONE are now bits in fmode. Both are set for NONE... */
#define BRIEF (fmode&010000000000)
#define NONE (fmode&020000000000u)
#define QUOTE '\''
#define ASTRSK '*'
#define BLKCAP (64-sizeof(void*))  /* Capacity of a data block */
/*
 * Typedefs
 */
typedef struct scrbuf5             /* Screenedit buffer */
{
  int bmxch;                       /* Buffer capacity (chars) */
  int bchars;                      /* No. of chars of data */
  int bcurs;                       /* Cursor posn (0-based) */
  int tokbeg;                      /* Where last token from scrdtk started */
  long decval;                     /* Decimal value from scrdtk */
  unsigned long octval;            /* Octal value from scrdtk */
  enum
  { nortok, nultok, eoltok } toktyp; /* Token type from scrdtk */
  int toklen;                      /* Token length from scrdtk */
  bool decok;                      /* decval is valid */
  bool octok;                      /* octval is valid */
  bool plusf;                      /* Token started with unquoted leading '+' */
  bool minusf;                     /* Token started with unquoted leading '-' */
  bool nulcma;                     /* If scrdtk sees a COMMA then null token */
  unsigned char bdata[Q_BUFSIZ];   /* Data for the line */
}
scrbuf5;
/*
 * Prototypes
 */
void memrec(unsigned char *start, unsigned char *end, unsigned long mode,
  scrbuf5 *a1);
void mapfil(ino_t inode, off_t size, unsigned char *addr);
void dfread(long num, scrbuf5 *s);
int ismapd(ino_t inode);
void newmap(ino_t inode, off_t size, unsigned char *addr);
short trytab(short, unsigned char *, scrbuf5 *);
void scrdit(scrbuf5 *curr, scrbuf5 *prev, char *prmpt, int pchrs, int cmmand);
void insmem(unsigned char *linptr, unsigned char *last);
void ordch(unsigned char, scrbuf5 *);
void pdsply(scrbuf5 *, unsigned char *, int);
short getlin(int, int);
short gettab(int, int, long *);
short getnum(int), tabset(scrbuf5 *), setmode(void);
int scrdtk(int, unsigned char *, int, scrbuf5 *);
int newmac(void), rdlin(scrbuf5 *, int), ysno5a(char *, int);
bool kbd5(void);
int lsub5a(unsigned char *, int, unsigned char *, int, int, int *, int *);
int ltok5a(unsigned char *, int, unsigned char *, int, int, int *, int *,
  unsigned char *);
int do_cmd(void);
int macdef(unsigned int, unsigned char *, int, bool);
int macdefw(unsigned int, unsigned short *, int, bool);
void duplx5(bool enable_IXON);
void lstmac(void), typmac(void), showmac(int), sccmnd(void), scmnrd(void);
void restore_stdout(void), notmac(int), sinitl(void);
void disply(scrbuf5 *, int), refrsh(scrbuf5 *);
void scrset(scrbuf5 *), setcrs(int), setptr(long);
void delete(long), clrfgt(void), inslin(scrbuf5 *), forget(void), setaux(long);
void finitl(void), sprmpt(long);
void quthan(int signum, siginfo_t *siginfo, void *ucontext);
void sdsply(void);
bool cl5get(char *buf, int bufcap, bool action_eof, bool read_macros);
void readfl(void), writfl(long);
void xlateset(void), showchar(unsigned char), tildexpn(char *);
void winchhan(int), setwinsz(int), newlin(void), rerdcm(void), xistcs(void);
void sindnt(void);
char c1in5(bool *eof_encountered);
int cmd(char *buf);
bool pop_stdin(void);
void devnull_stdout(void);
/* */
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
unsigned long fmode;
char *macro_dir;                   /* Where macros are */
int orig_stdout;                   /* stdout funit at start */
#endif
