/* S H O W C H A R . C
 *
 * Copyright (C) 1994, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2018,2019 Duncan Roe
 *
 * This routine outputs a character or its printable interpretation to
 * standard output.
 */
#include <stdio.h>
#include "prototypes.h"
void
showchar(uint8_t c)
{
  if (c >= ' ')
  {
    if (c < 0177)
      putchar(c);
    else
    {
      putchar(CARAT);
      if (c == 0177)
        putchar(QM);
      else
        printf("<%o>", (int)c);
    }
  }
  else
  {
    if (c == '\n')
      puts("\r");
    else
    {
      putchar(CARAT);
      putchar(c + 0100);
    }
  }
}
