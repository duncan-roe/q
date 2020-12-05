/* L O G _ C H A R . C
 *
 * Copyright (C) 2020 Duncan Roe
 */

/* Headers */

#include "macros.h"
#include "fmode.h"
#include "c1in.h"
#include "alu.h"

/* Static Variables */

static bool ctl_n_pending = false;

/* ******************************** log_char ******************************** */

void
log_char(uint16_t thisch)
{
/* Look for an opcode if previous char was ^N */
  if (ctl_n_pending)
  {
    ctl_n_pending = false;
    if (display_opcode(thisch, log_fd))
      return;
    fputs("^N", log_fd);
  }                                /* if (ctl_n_pending) */
/* Pend o/p of ^N if in a macro (LOG macro checks fmode for logging wanted) */
  else if (thisch == CTL_N && curmac >= 0)
  {
    ctl_n_pending = true;
    return;
  }                               /* else if (thisch == CTL_N && curmac >= 0) */

  if (thisch < SPACE)
  {
    fprintf(log_fd, "^%c", thisch | 0100);
/* Newline after ^J, ^M, ^T or ^[ */
    if (thisch == 012 || thisch == 015 || thisch == 024 || thisch == ESC)
      fputc('\n', log_fd);
  }                                /* if (thisch < SPACE) */
  else if (thisch == CARAT)
  {
    fputc(CARAT, log_fd);
    if (fmode & 040000000)
      fputc(ASTRSK, log_fd);
  }                                /* else if (thisch == CARAT) */
  else if (thisch < DEL)
    fputc(thisch, log_fd);
  else if (thisch == DEL)
    fputs("^?", log_fd);
  else
    fprintf(log_fd, "^<%o>", thisch);
}                                  /* log_char(uint16_t thisch) */
