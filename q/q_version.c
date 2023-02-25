/* >%---- CODE_STARTS ./q_version.c */
#include <stdio.h>
/* >%---- KEEP2HERE ./q_version.c */
#include <string.h>
#include "edmast.h"
#include "q_version.h"
/* >%---- CUT_HERE ./q_version.c */
void
q_version()
{
  printf("Q version %g\r\n", (double)Q_VERSION);

/* q -V from the command line prints extra stuff */

  if (argc != 2 || strcmp(argv[1], "-V"))
    return;
  fputs("Copyright (C) 1995-2023 Duncan Roe\r\n", stdout);
  fputs("License GPLv3+: GNU GPL version 3 or later "
    "<http://gnu.org/licenses/gpl.html>\r\n", stdout);
  fputs("q comes with NO WARRANTY, to the extent permitted by law.\r\n",
    stdout);
}                                  /* q_version() */
