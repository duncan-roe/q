#ifndef FMODE_H
#define FMODE_H
/*
 * Copyright (C) 2014,2017,2019 Duncan Roe
 */

/* Headers required by this header */

#include "prototypes.h"

/* Macro definitions */

/* This macro is because the ALU can modify the mode at any time */
#define FMODE (zmode_valid ? zmode : fmode)

#define NOWRAP (bool)((FMODE & 01000000000) != 0)
#define CASDEP (bool)((FMODE & 02000000000) == 0)

/* INDENT is special - must always test fmode */
#define INDENT (bool)((fmode & 04000000000) != 0)

#define INTERPRET_ALU_OPCODES (bool)((FMODE & 00000200000) != 0)
#define WARN_NONZERO_MEMORY (bool)((FMODE & 00000400000) != 0)
#define FILE_POS_BIT INT32_C(00400000000)
#define STORE_FILE_POS (bool)((FMODE & FILE_POS_BIT) != 0)
#define MATCH_ANY_WHSP (bool)((FMODE & 00001000000) != 0)
#define EXCLUSIVE_L_BOOL (bool)((FMODE & 00002000000) != 0)

/* BRIEF and NONE are bits in fmode. Both are set for NONE */
#define BRIEF (bool)((FMODE & 010000000000) != 0)
#define NONE (bool)((FMODE & 020000000000) != 0)

/* External variables */

extern uint32_t fmode;
extern uint32_t zmode;             /* Copy of fmode by scmnrd */
extern uint8_t xlatable[256];
extern bool zmode_valid;
extern int tbstat;
extern bool mods;                  /* Mods done since last SAVE */
#endif
