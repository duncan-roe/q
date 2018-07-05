#ifndef FMODE_H
#define FMODE_H
/*
 * Copyright (C) 2014,2017 Duncan Roe
 */

/* Macro definitions */

/* This macro is because the ALU can modify the mode at any time */
#define FMODE (zmode_valid ? zmode : fmode)

#define NOWRAP ((FMODE & 01000000000) != 0)
#define CASDEP ((FMODE & 02000000000) == 0)

/* INDENT is special - must always test fmode */
#define INDENT (fmode & 04000000000)

#define INTERPRET_ALU_OPCODES (FMODE & 00000200000)
#define WARN_NONZERO_MEMORY (FMODE & 00000400000)
#define FILE_POS_BIT 00400000000
#define STORE_FILE_POS (FMODE & FILE_POS_BIT)
#define MATCH_ANY_WHSP (FMODE & 00001000000)

/* BRIEF and NONE are bits in fmode. Both are set for NONE */
#define BRIEF (FMODE & 010000000000)
#define NONE (FMODE & 020000000000u)

/* External variables */

extern unsigned long fmode;
extern unsigned long zmode;        /* Copy of fmode by scmnrd */
extern bool zmode_valid;
#endif
