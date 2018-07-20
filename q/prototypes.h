/* P R O T O T Y P E S . H */
#ifndef PROTOTYPES_H
#define PROTOTYPES_H

/* Headers required by this header */

#include <signal.h>
#include <sys/types.h>
#include "ckalloc.h"
#include "typedefs.h"
#include "pushable_values.h"

/* Function Prototypes */

void memrec(unsigned char *start, unsigned char *end, unsigned long mode,
  scrbuf5 *a1);
void mapfil(ino_t inode, off_t size, unsigned char *addr);
void dfread(long num, scrbuf5 *s);
int ismapd(ino_t inode);
void newmap(ino_t inode, off_t size, unsigned char *addr);
bool trytab(unsigned char *zvuf, scrbuf5 *scline);
void scrdit(scrbuf5 *Curr, scrbuf5 *Prev, char *prmpt, int pchrs, bool in_cmd);
void insmem(unsigned char *linptr, unsigned char *last);
void ordch(unsigned char chr, scrbuf5 *scbuf);
void pdsply(scrbuf5 *buf, unsigned char *prm, int pch);
bool getlin(bool reperr, bool eofok);
bool getnum(bool okzero);
bool setmode(void);
bool tabset(scrbuf5 *scbuf);
int newmac(void);
bool rdlin(scrbuf5 *a1, bool aux);
bool ysno5a(char *mess, int key);
bool kbd5(void);
bool lsub5a(unsigned char *srchstr, int srchlen, unsigned char *string, \
  int first, int len, int *strtpos, int *endpos);
bool ltok5a(unsigned char *srchstr, int srchlen, unsigned char *string, \
  int first, int len, int *strtpos, int *endpos, unsigned char *ndel);
int do_cmd(void);
void duplx5(bool enable_IXON);
void lstmac(void);
void typmac(void);
void sccmnd(void);
void scmnrd(void);
void restore_stdout(void);
void sinitl(void);
void notmac(bool err);
void disply(scrbuf5 *line, bool savecurs);
void refrsh(scrbuf5 *xline);
void scrset(scrbuf5 *line);
void setcrs(int posn);
void setptr(long a1);
void delete(bool aux);
void clrfgt(void);
void inslin(scrbuf5 *a1);
void forget(void);
void setaux(long a1);
void finitl(void);
void sprmpt(long number);
void quthan(int signum, siginfo_t *siginfo, void *ucontext);
void sdsply(void);
bool cl5get(char *buf, int bufcap, bool action_eof, bool read_macros);
void readfl(void);
void writfl(long wrtnum);
void xlateset(void);
void showchar(unsigned char c);
void tildexpn(char *path, int pthsiz);
void setwinsz(int msg);
void newlin(void);
void rerdcm(void);
void xistcs(void);
unsigned char *sindnt(void);
char c1in5(bool *eof_encountered);
int cmd(char *buf);
bool pop_stdin(void);
void devnull_stdout(void);

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
int funit;                         /* File i/o ptr */
char pcnta[256];                   /* Pathname we are editing */
unsigned int row5, col5;           /* Screen / window geometry */
bool cntrlc, seenwinch;
char *macro_dir;                   /* Where macros are */
int orig_stdout;                   /* stdout funit at start */
#endif
