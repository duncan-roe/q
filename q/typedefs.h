#ifndef TYPEDEFS_H
#define TYPEDEFS_H

/* Headers required by this header */

#include <stdint.h>

/* Macro definitions */

#define Q_BUFSIZ 65536
#define BUFMAX Q_BUFSIZ-1
#define BLKCAP (64-sizeof(void*))  /* Capacity of a data block */
#define PTHSIZ 256
#define PTHMAX PTHSIZ-4            /* Allow for .tm */
#define SYSCALL(x, y) do x = y; while(x == -1 && errno == EINTR)

#define CARAT '^'
#define SPACE ' '
#define LT '<'
#define ESC '\33'
#define DEL 0177
#define CTL_N 016
#define CTL_U 025
#define GT '>'
#define SLASH '/'
#define QM '?'
#define COMMA ','
#define QUOTE '\''
#define ASTRSK '*'

#define A5NDEF 0
#define A5DYES 1
#define A5DNO -1

#define STDIN5FD 0
#define STDOUT5FD 1
#define STDERR5FD 2

/* Typedefs */

typedef enum bool
{
  false,
  true
} bool;

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
  uint8_t bdata[Q_BUFSIZ];         /* Data for the line */
}
scrbuf5;

/* Prototype */

int scrdtk(int key, uint8_t * buf, int bufcap, scrbuf5 *scline);
#endif
