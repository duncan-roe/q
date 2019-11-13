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

void memrec(uint8_t *start, uint8_t *end, unsigned long mode, scrbuf5 *a1);
void mapfil(ino_t inode, off_t size, uint8_t *addr);
void dfread(long num, scrbuf5 *s);
int ismapd(ino_t inode);
void newmap(ino_t inode, off_t size, uint8_t *addr);
bool trytab(uint8_t *zvuf, scrbuf5 *scline, bool filpos);
void scrdit(scrbuf5 *Curr, scrbuf5 *Prev, char *prmpt, int pchrs, bool in_cmd);
void insmem(uint8_t *linptr, uint8_t *last);
void ordch(uint8_t chr, scrbuf5 *scbuf);
void pdsply(scrbuf5 *buf, uint8_t *prm, int pch);
bool getlin(bool reperr, bool eofok);
bool getnum(bool okzero, bool filpos);
bool setmode(void);
bool tabset(scrbuf5 *scbuf);
int newmac(void);
bool rdlin(scrbuf5 *a1, bool aux);
bool ysno5a(char *mess, int key);
bool kbd5(void);
bool lsub5a(uint8_t *srchstr, int srchlen, uint8_t *string,
  int first, int len, int *strtpos, int *endpos);
bool ltok5a(uint8_t *srchstr, int srchlen, uint8_t *string,
  int first, int len, int *strtpos, int *endpos, uint8_t *ndel);
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
void delete(bool aux, long num2del, bool forgettable);
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
void showchar(uint8_t c);
void tildexpn(char *path, int pthsiz);
void setwinsz(int msg);
void newlin(void);
void rerdcm(void);
void xistcs(void);
uint8_t *sindnt(void);
char c1in5(bool *eof_encountered);
int cmd(char *buf, bool backtick);
bool pop_stdin(void);
void devnull_stdout(void);
void visbel(void);
double time_now(void);

/* Global variables */

extern uint32_t verb;        /*Command Processing - COMANL, ONEOF, NEWMAC &c. */
extern int ndntch;                 /* # of chars to indent */
extern bool vt100;                 /* Enable VT100-style curpos */
extern bool deferd;                /* Deferred indexing of mmap'd input file */
extern bool locerr;                /* LOCATE error only - macro can detect */
extern bool noRereadIfMacro;       /* Don't re-read command if in macro &c */
extern bool forych;             /* PDSPLY only required to do a BRIEF display */
extern bool lstvld;                /* "previous" buffer has valid data */
extern bool modlin;                /* This line has actually changed */
extern bool binary;                /* Q invoked -b */
extern int funit;                  /* File i/o ptr */
extern char pcnta[PTHSIZ];         /* Pathname we are editing */
extern bool cntrlc;                /* SIGINT handler invoked */
extern bool seenwinch;             /* SIGWINCH handler invoked */
extern char *macro_dir;            /* Where macros are */
extern int orig_stdout;            /* stdout funit at start */
#endif
