#ifndef FMODE_H
#define FMODE_H
/*
 * Copyright (C) 2014,2017 Duncan Roe
 */

/* Macro definitions */

#define FMODE (zmode_valid ? zmode : fmode)
#define FTNMOD (FMODE & 01000000000)
#define CASDEP ((FMODE & 02000000000) == 0)
#define INDENT (FMODE & 04000000000)
#define INTERPRET_ALU_OPCODES (FMODE & 00000200000)
#define WARN_NONZERO_MEMORY (FMODE & 00000400000)
#define FILE_POS_BIT 00400000000
#define STORE_FILE_POS (FMODE & FILE_POS_BIT)

/* BRIEF and NONE are bits in fmode. Both are set for NONE */
#define BRIEF (FMODE & 010000000000)
#define NONE (FMODE & 020000000000u)

/* External variables */

extern unsigned long fmode;
extern unsigned long zmode;        /* Copy of fmode by scmnrd */
extern bool zmode_valid;
#endif
