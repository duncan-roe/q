#ifndef C1IN_H
#define C1IN_H
/* c 1 i n . h */
/*
 * Copyright (C) 1995, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2014,2019 Duncan Roe
 *
 * This header file contains items of interest to the character input
 * subsystem
 */

/* Headers */

#include "typedefs.h"

/* Macros */

#define BUF5MAX 40
#ifndef OPEN_MAX
#  define OPEN_MAX 2048            /* Surely more than anyone would want */
#endif
#define USING_FILE (bool)(stdidx >= 0)

/* External variables */

extern int buf5len, buf5idx;
extern struct termios tio5save, tio5;
extern bool size5;                 /* Screen was sized */
extern int ttyfd;                  /* fd to do iocltls on */
extern int stdidx;
extern struct stdinfo
{
  int funit;
  bool nullstdout;
  bool frommac;
}
stdinfo[OPEN_MAX];
extern bool offline;
extern bool simulate_q;
extern int simulate_q_idx;
extern const char *end_seq;
extern const char *const normal_end_sequence;
extern double timlst;
extern uint8_t fxtabl[128];        /* FX command implementation */

/* Prototypes */

void init5(void), final5(void);
void change_attr(int fd, struct termios *wanted);
#endif
