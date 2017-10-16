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
void ordch(unsigned char, scrbuf5 *);
void pdsply(scrbuf5 *, unsigned char *, int);
short getlin(int, int);
short getnum(int), setmode(void);
bool tabset(scrbuf5 *scbuf);
int newmac(void), rdlin(scrbuf5 *, int), ysno5a(char *, int);
bool kbd5(void);
int lsub5a(unsigned char *, int, unsigned char *, int, int, int *, int *);
int ltok5a(unsigned char *, int, unsigned char *, int, int, int *, int *,
  unsigned char *);
int do_cmd(void);
void duplx5(bool enable_IXON);
void lstmac(void), typmac(void), sccmnd(void), scmnrd(void);
void restore_stdout(void), sinitl(void);
void notmac(bool err);
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
