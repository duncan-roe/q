#ifndef FMODE_H
#define FMODE_H
/*
 * Copyright (C) 2014 Duncan Roe
 */

/* Macro definitions */

#define FTNMOD (fmode & 01000000000)
#define CASDEP ((fmode & 02000000000) == 0)
#define INDENT (fmode & 04000000000)
#define INTERPRET_ALU_OPCODES (fmode & 00000200000)
#define WARN_NONZERO_MEMORY (fmode & 00000400000)

/* BRIEF and NONE are bits in fmode. Both are set for NONE */
#define BRIEF (fmode & 010000000000)
#define NONE (fmode & 020000000000u)

/* External variables */

extern unsigned long fmode;
extern unsigned long zmode;        /* Copy of fmode by scmnrd */
extern bool zmode_valid;
#endif
