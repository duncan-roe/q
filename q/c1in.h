#ifndef C1IN_H
#define C1IN_H
/* c 1 i n . h */
/*
 * Copyright (C) 1995, Duncan Roe & Associates P/L
 * Copyright (C) 2012,2014 Duncan Roe
 *
 * This header file contains items of interest to the character input
 * subsystem
 */
#include "termio5.hl"
#include "bool.h"
#define BUF5MAX 40
#define STDIN5FD 0
#define STDOUT5FD 1
#define STDERR5FD 2
#ifndef OPEN_MAX
#  define OPEN_MAX 2048            /* Surely more than anyone would want */
#endif
#define USING_FILE (stdidx >= 0)
int buf5len, buf5idx;
void init5(void), final5(void);
struct TIO55 tio5save, tio5;
char buf5[BUF5MAX];
bool size5;                        /* Screen was sized */
int ttyfd;                         /* fd to do iocltls on */
int stdidx;
struct
{
  int funit;
  bool nullstdout;
  bool frommac;
}
stdinfo[OPEN_MAX];
extern bool offline;
extern bool simulate_q;
extern int simulate_q_idx;
extern char *end_seq;
extern char *normal_end_sequence;
extern char *xistics_end_sequence;
#endif
