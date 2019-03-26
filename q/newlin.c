/* N E W L I N */
/*
 * Copyright (C) 1981, D. C. Roe
 * Copyright (C) 2012,2019 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 *
 * O/p's a newline (CR/NL) and spacefills the internal screen image
 * to reflect this.
 * May be called by user program
 */
#include <stdio.h>
#include "prototypes.h"
#include "scrnedit.h"
void
newlin()
{
  int i;                           /* Scratch */
  uint8_t *p;
  puts("\r");                      /* CR/NL */
/* */
  p = screen;
  for (i = 0; i < WCHRS; i++)
    *p++ = SPACE;                  /* Fill buffer to configured width */
  *p = '\0';                       /* Do we need this? */
  scurs = 0;                       /*Screen cursor at start */
}
