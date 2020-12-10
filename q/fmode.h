#ifndef FMODE_H
#define FMODE_H
/*
 * Copyright (C) 2014,2017,2019 Duncan Roe
 */

/* Headers required by this header */

#include "prototypes.h"

/* Macro definitions */

#define FMODE_C(x) INT32_C(x)
#define PRIofmode PRIo32

/* This macro is because the ALU can modify the mode at any time */
#define FMODE (zmode_valid ? zmode : fmode)

#define NOWRAP (bool)((FMODE & FF_ON_BIT) != 0)
#define CASDEP (bool)((FMODE & FC_ON_BIT) == 0)

/* INDENT is special - must always test fmode */
#define INDENT (bool)((fmode & INDENT_BIT) != 0)

#define INTERPRET_ALU_OPCODES (bool)((FMODE & FM_PLUS_I_BIT) != 0)
#define WARN_NONZERO_MEMORY (bool)((FMODE & FM_PLUS_W_BIT) != 0)
#define STORE_FILE_POS (bool)((FMODE & FILE_POS_BIT) != 0)
#define MATCH_ANY_WHSP (bool)((FMODE & FM_PLUS_A_BIT) != 0)
#define EXCLUSIVE_L_BOOL (bool)((FMODE & FM_PLUS_X_BIT) != 0)

/* Bit settings. Code may safely tilde these for a mask of the correct width */
#define DOS_READ_BIT FMODE_C(01)
#define DOS_WRITE_BIT FMODE_C(02)
#define TAB_READ_BIT FMODE_C(04)
#define TAB_WRITE_BIT FMODE_C(010)
#define FM_PLUS_S_BIT FMODE_C(020)
#define FM_PLUS_STAR_BIT FMODE_C(040)
#define FM_PLUS_Q_BIT FMODE_C(0100)
#define FM_PLUS_HASH_BIT FMODE_C(0200)
#define FM_PLUS_F_BIT FMODE_C(0400)
#define FM_PLUS_V_BIT FMODE_C(01000)
#define FM_PLUS_M_BIT FMODE_C(02000)
#define FM_PLUS_R_BIT FMODE_C(04000)
#define FM_PLUS_E_BIT FMODE_C(010000)
#define FM_PLUS_N_BIT FMODE_C(020000)
#define FM_PLUS_L_BIT FMODE_C(040000)
#define FM_PLUS_H_BIT FMODE_C(0100000)
#define FM_PLUS_I_BIT FMODE_C(0200000)
#define FM_PLUS_W_BIT FMODE_C(0400000)
#define FM_PLUS_A_BIT FMODE_C(01000000)
#define FM_PLUS_X_BIT FMODE_C(02000000)
#define FM_PLUS_Y_BIT FMODE_C(04000000)
#define FM_PLUS_G_BIT FMODE_C(010000000)
#define FM_PLUS_8_BIT FMODE_C(020000000)
#define FM_PLUS_9_BIT FMODE_C(040000000)
#define FM_PLUS_0_BIT FMODE_C(0100000000)
#define FILE_POS_BIT FMODE_C(0400000000)
#define FF_ON_BIT FMODE_C(01000000000)
#define FC_ON_BIT FMODE_C(02000000000)
#define INDENT_BIT FMODE_C(04000000000)
#define FB_CMD_BIT FMODE_C(010000000000)
#define FN_CMD_BITS FMODE_C(030000000000)

/* BRIEF and NONE are bits in fmode. Both are set for NONE */
#define BRIEF (bool)((FMODE & FB_CMD_BIT) != 0)
#define NONE (bool)((FMODE & FN_CMD_BITS) == FN_CMD_BITS)

/* External variables */

extern fmode_t fmode;
extern fmode_t zmode;             /* Copy of fmode by scmnrd */
extern uint8_t xlatable[256];
extern bool zmode_valid;
extern int tbstat;
extern bool mods;                  /* Mods done since last SAVE */
#endif
