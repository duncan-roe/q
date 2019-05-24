/* T I M E _ N O W . C
 *
 * Copyright (C) 2019 Duncan Roe */

/* Return wall-clock time since Q started */

/* Headers */

#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "prototypes.h"
#include "typedefs.h"

/* Static Variables */

static bool first_call = true;
struct timespec start_time;

double
time_now(void)
{
  struct timespec tp;

  if (first_call)
  {
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &start_time))
    {
      fprintf(stderr, "%s. CLOCK_MONOTONIC_RAW (clock_gettime)\r\n",
        strerror(errno));
      return 0;
    }                 /* if (clock_gettime(CLOCK_MONOTONIC_RAW, &start_time)) */
    first_call = false;
  }                                /* if (first_call) */
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &tp))
    {
      fprintf(stderr, "%s. CLOCK_MONOTONIC_RAW (clock_gettime)\r\n",
        strerror(errno));
      return 0;
    }                 /* if (clock_gettime(CLOCK_MONOTONIC_RAW, &start_time)) */
    tp.tv_sec -= start_time.tv_sec;
    return (double)tp.tv_sec + tp.tv_nsec / 1000000000.0;
}                                  /* double time_now(void) */
