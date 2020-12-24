/* A L U . H */
#ifndef ALU_H
#define ALU_H
/*
 * Copyright (C) 2014,2016,2018 Duncan Roe
 *
 * This header file contains items relating to support for the ALU
 * (Arithmetic and Logic Unit) a.k.a. the Reverse Polish Calculator
 */

/* Headers required by this header */

#include <stddef.h>
#include "typedefs.h"

/* Macro definitions */

#define OPCODE(op, desc) {#op, op, desc}
#define CAPTION(desc) {"", NULL, desc}
#define FIRST_ALU_OP 04200

/* Typedefs */

typedef struct alu_opcode
{
  char *name;
  bool (*func) (char **err);
  char *description;
} alu_opcode;                      /* typedef struct alu_opcode */

typedef struct alu_dict_ent
{
  struct alu_dict_ent *alt;        /* Alternative letter */
  struct alu_dict_ent *next;       /* Next possible letter(s) */
  int fn_idx;
  char letter;
} alu_dict_ent;                    /* typedef struct alu_dict_ent */

/* Prototypes */

void dump_registers(bool append_newline);
bool exec_alu_opcode(int opcode, char **err);
bool get_inp(double *fval, long *val, long *len, char **err);

/* External variables */

extern long ALU_memory[01000];
extern double FPU_memory[01000];
extern int stack_size;
extern int f_stack_size;
extern long *rs;                   /* Register stack */
extern double *fs;                 /* FP register stack */
extern long xreg;                  /* Index Register */
extern long qreg;                  /* Q Result Register */
extern int rsidx;                  /* Index of current register */
extern int fsidx;                  /* Index of current FP register */
extern bool index_next;            /* Next PSH or POP is indexed */
extern bool xmode_pending;
extern int effaddr;                /* Effective address */
extern alu_opcode opcode_defs[];
extern bool alu_skip;              /* Macro should skip 2 chars */
extern int num_ops;
extern alu_dict_ent root_alu_dict_ent;
extern int num_alu_opcode_table_entries;
extern int *alu_table_index;
extern bool alu_macros_only;       /* N-- lstmac & typmac only show n7000+ */
extern char FPformat[40];          /* Floating-point format */
extern char Iformat[40];           /* Integer format */
extern char DTformat[256];         /* Date format (can be really huge) */
extern scrbuf5 *last_Curr;         /* Line that scrdit is working on */

#endif
