/* E X E C _ A L U _ O P C O D E . C
 *
 * Copyright (C) 2014 Duncan Roe
 */

/* Headers */

#include <string.h>
#include "alu.h"

/* **************************** Static Functions **************************** */

static bool
push(long val, char **err)
{
  if (rsidx >= stack_size - 1)
  {
    *err = "Register stack overflow";
    return false;
  }                                /* if (rsidx >= stack_size - 1) */
  rs[++rsidx] = val;
  return true;
}                                  /* push() */

static bool
r_valid(char **err)
{
  if (rsidx >= 0)
    return true;
  *err = "Register stack underflow";
  return false;
}                                  /* r_valid() */

static bool
r_valid2(char **err)
{
  if (!r_valid(err))
    return false;
  if (rsidx >= 1)
    return true;
  *err = "Register stack underflow (no 2nd argument pushed)";
  return false;
}                                  /* r_valid() */

static bool
nop(char **err)
{
  return true;
}                                  /* nop() */

static bool
skp(char **err)
{
  alu_skip = true;
  return true;
}                                  /* skp() */

static bool
not(char **err)
{
  if (!r_valid(err))
    return false;
  rs[rsidx] = ~rs[rsidx];
  return true;
}                                  /* not() */

static bool
tc(char **err)
{
  if (!r_valid(err))
    return false;
  rs[rsidx] = -rs[rsidx];
  return true;
}                                  /* tc() */

static bool
add_any(char **err, long val)
{
  if (!r_valid(err))
    return false;
  rs[rsidx] += val;
  return true;
}                                  /* add_any() */

static bool
a1(char **err)
{
  return add_any(err, 1);
}                                  /* a1() */

static bool
a2(char **err)
{
  return add_any(err, 2);
}                                  /* a2() */

static bool
s2(char **err)
{
  return add_any(err, -2);
}                                  /* s2() */

static bool
s1(char **err)
{
  return add_any(err, -1);
}                                  /* s1() */

static bool
ls(char **err)
{
  if (!r_valid(err))
    return false;
  rs[rsidx] <<= 1;
  return true;
}                                  /* ls() */

static bool
rss(char **err)
{
  if (!r_valid(err))
    return false;
  rs[rsidx] >>= 1;
  return true;
}                                  /* rss() */

static bool
rsu(char **err)
{
  if (!r_valid(err))
    return false;
  *((unsigned long *)&rs[rsidx]) >>= 1;
  return true;
}                                  /* rsu() */

static bool
seq(char **err)
{
  if (!r_valid(err))
    return false;
  if (rs[rsidx] == 0)
    alu_skip = true;
  return true;
}                                  /* seq() */

static bool
sne(char **err)
{
  if (!r_valid(err))
    return false;
  if (rs[rsidx] != 0)
    alu_skip = true;
  return true;
}                                  /* sne() */

static bool
sge(char **err)
{
  if (!r_valid(err))
    return false;
  if (rs[rsidx] >= 0)
    alu_skip = true;
  return true;
}                                  /* sge() */

static bool
sle(char **err)
{
  if (!r_valid(err))
    return false;
  if (rs[rsidx] <= 0)
    alu_skip = true;
  return true;
}                                  /* sle() */

static bool
sgt(char **err)
{
  if (!r_valid(err))
    return false;
  if (rs[rsidx] > 0)
    alu_skip = true;
  return true;
}                                  /* sgt() */

static bool
slt(char **err)
{
  if (!r_valid(err))
    return false;
  if (rs[rsidx] < 0)
    alu_skip = true;
  return true;
}                                  /* slt() */

static bool
sxeq(char **err)
{
  if (xreg == 0)
    alu_skip = true;
  return true;
}                                  /* seq() */

static bool
sxne(char **err)
{
  if (xreg != 0)
    alu_skip = true;
  return true;
}                                  /* sxne() */

static bool
sxge(char **err)
{
  if (xreg >= 0)
    alu_skip = true;
  return true;
}                                  /* sxge() */

static bool
sxle(char **err)
{
  if (xreg <= 0)
    alu_skip = true;
  return true;
}                                  /* sxle() */

static bool
sxgt(char **err)
{
  if (xreg > 0)
    alu_skip = true;
  return true;
}                                  /* sxgt() */

static bool
sxlt(char **err)
{
  if (xreg < 0)
    alu_skip = true;
  return true;
}                                  /* sxlt() */

static bool
a1x(char **err)
{
  xreg++;
  return true;
}                                  /* a1x() */

static bool
a2x(char **err)
{
  xreg += 2;
  return true;
}                                  /* a2x() */

static bool
s2x(char **err)
{
  xreg -= 2;
  return true;
}                                  /* s2x() */

static bool
s1x(char **err)
{
  xreg--;
  return true;
}                                  /* s1x() */

static bool
and(char **err)
{
  if (!r_valid2(err))
    return false;
  rs[rsidx - 1] = rs[rsidx] & rs[rsidx - 1];
  rsidx--;
  return true;
}                                  /* and() */

static bool
or(char **err)
{
  if (!r_valid2(err))
    return false;
  rs[rsidx - 1] = rs[rsidx] | rs[rsidx - 1];
  rsidx--;
  return true;
}                                  /* or() */

static bool
xor(char **err)
{
  if (!r_valid2(err))
    return false;
  rs[rsidx - 1] = rs[rsidx] ^ rs[rsidx - 1];
  rsidx--;
  return true;
}                                  /* xor() */

static bool
add(char **err)
{
  if (!r_valid2(err))
    return false;
  rs[rsidx - 1] = rs[rsidx] + rs[rsidx - 1];
  rsidx--;
  return true;
}                                  /* add() */

static bool
sub(char **err)
{
  if (!r_valid2(err))
    return false;
  rs[rsidx - 1] = rs[rsidx] - rs[rsidx - 1];
  rsidx--;
  return true;
}                                  /* sub() */

static bool
mpy(char **err)
{
  if (!r_valid2(err))
    return false;
  rs[rsidx - 1] = rs[rsidx] * rs[rsidx - 1];
  rsidx--;
  return true;
}                                  /* mpy() */

static bool
div(char **err)
{
  if (!r_valid2(err))
    return false;
  if (rs[rsidx - 1] == 0)
  {
    *err="Attempt to divide by zero";
    return false;
  }                                /* if (rs[rsidx - 1] == 0) */
  rs[rsidx - 1] = rs[rsidx] / rs[rsidx - 1];
  rsidx--;
  return true;
}                                  /* div() */

static bool
mod(char **err)
{
  if (!r_valid2(err))
    return false;
  if (rs[rsidx - 1] == 0)
  {
    *err="Attempt to take modulo zero";
    return false;
  }                                /* if (rs[rsidx - 1] == 0) */
  rs[rsidx - 1] = rs[rsidx] % rs[rsidx - 1];
  rsidx--;
  return true;
}                                  /* mod() */

static bool
indx(char **err)
{
  index_next = true;
  return true;
}                                  /* indx() */

static bool
pshx(char **err)
{
  return push(xreg, err);
}                                  /* pshx() */

static bool
popx(char **err)
{
  if (!r_valid(err))
    return false;
  xreg = rs[rsidx--];
  return true;
}                                  /* popx() */

static bool
popn(char **err)
{
  if (!r_valid(err))
    return false;
  rsidx--;
  return true;
}                                  /* popn() */

static bool
dup(char **err)
{
  if (!r_valid(err))
    return false;
  return push(rs[rsidx], err);
}                                  /* dup() */

static bool
dmp(char **err)
{
  dump_registers(true);
  return true;
}                                  /* dmp() */

static bool
rst(char **err)
{
  rsidx = -1;
  xreg = 0;
  index_next = false;
  alu_skip = false;
  store_file_pos = false;
  return true;
}                                  /* rst() */

static bool
zam(char **err)
{
  memset(ALU_memory, 0, sizeof ALU_memory);
  return true;
}                                  /* zam() */

static bool
scpt(char **err)
{
  return !(store_file_pos = false);
}                                  /* scpt() */

static bool
sfpt(char **err)
{
  return store_file_pos = true;
}                                  /* sfpt() */

static bool
ps0(char **err)
{
  return push(0, err);
}                                  /* ps0() */

static bool
ps1(char **err)
{
  return push(1, err);
}                                  /* ps1() */

static bool
ps2(char **err)
{
  return push(2, err);
}                                  /* ps2() */

static bool
ps4(char **err)
{
  return push(4, err);
}                                  /* ps4() */

static bool
ps8(char **err)
{
  return push(8, err);
}                                  /* ps8() */

static bool
ps16(char **err)
{
  return push(16, err);
}                                  /* ps16() */

static bool
ps32(char **err)
{
  return push(32, err);
}                                  /* ps32() */

static bool
ps64(char **err)
{
  return push(64, err);
}                                  /* ps64() */

static bool
ps128(char **err)
{
  return push(128, err);
}                                  /* ps128() */

static bool
ps256(char **err)
{
  return push(256, err);
}                                  /* ps256() */

static bool
ps512(char **err)
{
  return push(512, err);
}                                  /* ps512() */

static bool
ps1024(char **err)
{
  return push(1024, err);
}                                  /* ps1024() */

static bool
ps2048(char **err)
{
  return push(2048, err);
}                                  /* ps2048() */

static bool
ps4096(char **err)
{
  return push(4096, err);
}                                  /* ps4096() */

static bool
ps8192(char **err)
{
  return push(8192, err);
}                                  /* ps8192() */

/* **************************** The Opcode Table **************************** */

alu_opcode opcode_defs[] = {
  CAPTION(""),
  CAPTION("Skip Instructions"),
  CAPTION("==== ============"),
  OPCODE(nop, "Do nothing"),
  OPCODE(skp, "Unconditional skip"),
  OPCODE(seq, "Skip if R is zero"),
  OPCODE(sne, "Skip if R is not zero"),
  OPCODE(sge, "Skip if R is greater than or equal to zero"),
  OPCODE(sle, "Skip if R is less than or equal to zero"),
  OPCODE(sgt, "Skip if R is greater than zero"),
  OPCODE(slt, "Skip if R is less than zero"),
  CAPTION(""),
  CAPTION("Instructions that Modify R"),
  CAPTION("============ ==== ====== ="),
  OPCODE(not, "Bitwise Invert"),
  OPCODE(tc, "Negate (2's complement)"),
  OPCODE(a1, "R = R + 1"),
  OPCODE(a2, "R = R + 2"),
  OPCODE(s2, "R = R - 2"),
  OPCODE(s1, "R = R - 1"),
  OPCODE(ls, "R = R << 1"),
  OPCODE(rss, "R = R >> 1 (signed)"),
  OPCODE(rsu, "R = R >> 1 (unsigned)"),
  OPCODE(popn, "Pop R to nowhere (value is discarded)"),
  OPCODE(dup, "Push a copy of R"),
  CAPTION(""),
  CAPTION("Immediate Data Instructions"),
  CAPTION("========= ==== ============"),
  OPCODE(ps0, "Push constant 0 to R"),
  OPCODE(ps1, "Push constant 1 to R"),
  OPCODE(ps2, "Push constant 2 to R"),
  OPCODE(ps4, "Push constant 4 to R"),
  OPCODE(ps8, "Push constant 8 to R"),
  OPCODE(ps16, "Push constant 16 to R"),
  OPCODE(ps32, "Push constant 32 to R"),
  OPCODE(ps64, "Push constant 64 to R"),
  OPCODE(ps128, "Push constant 128 to R"),
  OPCODE(ps256, "Push constant 256 to R"),
  OPCODE(ps512, "Push constant 512 to R"),
  OPCODE(ps1024, "Push constant 1024 to R"),
  OPCODE(ps2048, "Push constant 2048 to R"),
  OPCODE(ps4096, "Push constant 4096 to R"),
  OPCODE(ps8192, "Push constant 8192 to R"),
  CAPTION(""),
  CAPTION("Instructions with 2 operands"),
  CAPTION("============ ==== = ========"),
  CAPTION("(These have the same effect as:-"),
  CAPTION("    pop A; pop B; push A {instr} B"),
  CAPTION("except attempted divide by zero leaves the registers unchanged)"),
  OPCODE(add, "Add"),
  OPCODE(sub, "Subtract"),
  OPCODE(mpy, "Multiply"),
  OPCODE(div, "Divide"),
  OPCODE(mod, "Modulus"),
  OPCODE(and, "Bitwise AND"),
  OPCODE(or, "Bitwise OR"),
  OPCODE(xor, "Bitwise EXCLUSIVE OR"),
  CAPTION(""),
  CAPTION("Index Register Instructions"),
  CAPTION("===== ======== ============"),
  OPCODE(indx, "Index next PSH or POP"),
  OPCODE(pshx, "Push contents of X to R"),
  OPCODE(popx, "Pop R to define value of X"),
  OPCODE(sxeq, "Skip if X is zero"),
  OPCODE(sxne, "Skip if X is not zero"),
  OPCODE(sxge, "Skip if X is greater than or equal to zero"),
  OPCODE(sxle, "Skip if X is less than or equal to zero"),
  OPCODE(sxgt, "Skip if X is greater than zero"),
  OPCODE(sxlt, "Skip if X is less than zero"),
  OPCODE(a1x, "X = X + 1"),
  OPCODE(a2x, "X = X + 2"),
  OPCODE(s2x, "X = X - 2"),
  OPCODE(s1x, "X = X - 1"),
  CAPTION(""),
  CAPTION("Control Instructions"),
  CAPTION("======= ============"),
  OPCODE(dmp, "Dump Registers"),
  OPCODE(rst, "Reset Registers to initial state"),
  OPCODE(zam, "Zeroise All Memory"),
  OPCODE(scpt, "Store Cursor Position Tabs (initial setting, also after RST)"),
  OPCODE(sfpt, "Store File Position Tabs"),
};                                 /* alu_opcode opcode_defs[] = */

/* Instantiate other externals */

int num_alu_opcode_table_entries = sizeof opcode_defs / sizeof *opcode_defs;

/* ***************************** exec_alu_opcode **************************** */

bool
exec_alu_opcode(int opcode, char **err)
{
  return opcode_defs[alu_table_index[opcode - FIRST_ALU_OP]].func(err);
}                                  /* exec_alu_opcode(int opcode) */