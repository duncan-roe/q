/* L O G _ C H A R . C
 *
 * Copyright (C) 2020 Duncan Roe
 */

/* Headers */

#include "c1in.h"
#include "fmode.h"
#include "macros.h"

/* Static Variables */

static bool nseen = false;

/* Static prototypes */

static bool display_opcode(uint16_t thisch);

/* ******************************** log_char ******************************** */

void
log_char(uint16_t thisch)
{
/* Look for an opcode if previous char was ^N */
  if (nseen)
  {
    nseen = false;
    if (display_opcode(thisch))
      return;
    fputs("^N", log_fd);
  }                                /* if (nseen) */
/* Pend o/p of ^N if in a macro (LOG macro checks fmode for logging wanted) */
  else if (thisch == 016 && curmac >= 0)
  {
    nseen = true;
    return;
  }                                /* else if (thisch == 016 && curmac >= 0) */

  if (thisch < 040)
  {
    fprintf(log_fd, "^%c", thisch | 0100);
/* Newline after ^J, ^M, ^T or ^[ */
    if (thisch == 012 || thisch == 015 || thisch == 024 || thisch == 033)
      fputc('\n', log_fd);
  }                                /* if (thisch < 040) */
  else if (thisch == CARAT)
  {
    fputc(CARAT, log_fd);
    if (fmode & 040000000)
      fputc(ASTRSK, log_fd);
  }                                /* else if (thisch == CARAT) */
  else if (thisch < 0177)
    fputc(thisch, log_fd);
  else if (thisch == 0177)
    fputs("^?", log_fd);
  else
    fprintf(log_fd, "^<%o>", thisch);
}                                  /* log_char(uint16_t thisch) */

/* ***************************** display_opcode ***************************** */

static bool display_opcode(uint16_t thisch)
{
  return false;
}                                  /* static bool display_opcode() */
