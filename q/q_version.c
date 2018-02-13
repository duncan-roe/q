#define Q_VERSION 48
#include <stdio.h>
#include "edmast.h"
void
q_version()
{
  printf("Q version %g\r\n", (double)Q_VERSION);
}                                  /* q_version() */
