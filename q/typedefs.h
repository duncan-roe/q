#ifndef TYPEDEFS_H
#define TYPEDEFS_H

/* Macros */
#define Q_BUFSIZ 8192
#define BUFMAX Q_BUFSIZ-1

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
  unsigned char bdata[Q_BUFSIZ];   /* Data for the line */
}
scrbuf5;
#endif
