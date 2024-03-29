/* E X E C _ A L U _ O P C O D E . C
 *
 * Copyright (C) 2014-2017,2019-2021,2023 Duncan Roe
 */

/* Headers */

#include <math.h>
#include <time.h>
#include <string.h>
#include "pushable_values.h"
#include "q_version.h"
#include "typedefs.h"
#include "tabsiz.h"
#include "fmode.h"
#include "alu.h"

/* **************************** Static Functions **************************** */

static bool
room(char **err)
{
  if (rsidx >= stack_size - 1)
  {
    *err = "Register stack overflow";
    return false;
  }                                /* if (rsidx >= stack_size - 1) */
  return true;
}                                  /* room() */

static bool
push(long val, char **err)
{
  if (!room(err))
    return false;
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
fpush(double val, char **err)
{
  if (fsidx >= stack_size - 1)
  {
    *err = "FP register stack overflow";
    return false;
  }                                /* if (fsidx >= stack_size - 1) */
  fs[++fsidx] = val;
  return true;
}                                  /* fpush() */

static bool
f_valid(char **err)
{
  if (fsidx >= 0)
    return true;
  *err = "FP register stack underflow";
  return false;
}                                  /* f_valid() */

static bool
f_valid2(char **err)
{
  if (!f_valid(err))
    return false;
  if (fsidx >= 1)
    return true;
  *err = "FP register stack underflow (no 2nd argument pushed)";
  return false;
}                                  /* f_valid() */

static bool
pshwdth(char **err)
{
  if (!room(err))
    return false;
  return push(col5, err);
}                                  /* pshwdth) */

static bool
pshhght(char **err)
{
  if (!room(err))
    return false;
  return push(row5, err);
}                                  /* pshhght) */

static bool
pshtbsz(char **err)
{
  if (!room(err))
    return false;
  return push(tabsiz, err);
}                                  /* pshtbsz) */

static bool
poptbsz(char **err)
{
  if (!r_valid(err))
    return false;
  tabsiz = rs[rsidx--];
  return true;
}                                  /* poptbsz() */

static bool
pshmode(char **err)
{
  if (!room(err))
    return false;
  return push(fmode, err);
}                                  /* pshmode() */

static bool
popmode(char **err)
{
  if (!r_valid(err))
    return false;
/* Must not set FB OR FN if fm+v */
    if (rs[rsidx] & FM_PLUS_V_BIT)
      rs[rsidx] &= ~FN_CMD_BITS;
  fmode = rs[rsidx--];
  return true;
}                                  /* popmode() */

static bool
pshcrs(char **err)
{
  if (!room(err))
    return false;
  return push(last_Curr->bcurs, err);
}                                  /* pshcrs() */

static bool
pshlnln(char **err)
{
  if (!room(err))
    return false;
  return push(last_Curr->bchars, err);
}                                  /* pshlnln() */

static bool
pshnbln(char **err)
{
  if (!room(err))
    return false;
  return push(lintot, err);
}                                  /* pshnbln() */

static bool
pshlnnb(char **err)
{
  if (!room(err))
    return false;
  return push(ptrpos, err);
}                                  /* pshlnnb() */

static bool
inp(char **err)
{
  long val, len;

  if (rsidx >= stack_size - 2)
  {
    *err = "Register stack overflow";
    return false;
  }                                /* if (rsidx >= stack_size - 2) */
  if (!get_inp(NULL, &val, &len, err))
    return false;
  rs[++rsidx] = val;
  rs[++rsidx] = len;
  return true;
}                                  /* inp() */

static bool
inpf(char **err)
{
  long len;
  double fval;

  if (!room(err))
    return false;
  if (fsidx >= stack_size - 1)
  {
    *err = "FP register stack overflow";
    return false;
  }                                /* if (fsidx >= stack_size - 1) */
  if (!get_inp(&fval, NULL, &len, err))
    return false;
  fs[++fsidx] = fval;
  rs[++rsidx] = len;
  return true;
}                                  /* inpf() */

static bool
fqversn(char **err)
{
  if (fsidx >= stack_size - 1)
  {
    *err = "FP register stack overflow";
    return false;
  }                                /* if (fsidx >= stack_size - 1) */
  fs[++fsidx] = Q_VERSION;
  return true;
}                                  /* fqversn() */

static bool
psvbint(char **err)
{
  if (fsidx >= stack_size - 1)
  {
    *err = "FP register stack overflow";
    return false;
  }                                /* if (fsidx >= stack_size - 1) */
  fs[++fsidx] = visbel_interval;
  return true;
}                                  /* psvbint() */

static bool
psfbint(char **err)
{
  if (fsidx >= stack_size - 1)
  {
    *err = "FP register stack overflow";
    return false;
  }                                /* if (fsidx >= stack_size - 1) */
  fs[++fsidx] = fbrief_interval;
  return true;
}                                  /* psfbint() */

static bool
psclock(char **err)
{
  if (fsidx >= stack_size - 1)
  {
    *err = "FP register stack overflow";
    return false;
  }                                /* if (fsidx >= stack_size - 1) */
  fs[++fsidx] = clock() / (double)CLOCKS_PER_SEC;
  return true;
}                                  /* psclock() */

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
xmode(char **err)
{
  xmode_pending = true;
  return true;
}                                  /* xmode() */

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
tcf(char **err)
{
  if (!f_valid(err))
    return false;
  fs[fsidx] = -fs[fsidx];
  return true;
}                                  /* tcf() */

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
sfeq(char **err)
{
  if (!f_valid(err))
    return false;
  if (fs[fsidx] == 0.0)
    alu_skip = true;
  return true;
}                                  /* sfeq() */

static bool
sfne(char **err)
{
  if (!f_valid(err))
    return false;
  if (fs[fsidx] != 0.0)
    alu_skip = true;
  return true;
}                                  /* sfne() */

static bool
sfge(char **err)
{
  if (!f_valid(err))
    return false;
  if (fs[fsidx] >= 0.0)
    alu_skip = true;
  return true;
}                                  /* sfge() */

static bool
sfle(char **err)
{
  if (!f_valid(err))
    return false;
  if (fs[fsidx] <= 0.0)
    alu_skip = true;
  return true;
}                                  /* sfle() */

static bool
sfgt(char **err)
{
  if (!f_valid(err))
    return false;
  if (fs[fsidx] > 0.0)
    alu_skip = true;
  return true;
}                                  /* sfgt() */

static bool
sflt(char **err)
{
  if (!f_valid(err))
    return false;
  if (fs[fsidx] < 0.0)
    alu_skip = true;
  return true;
}                                  /* sflt() */

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
sqeq(char **err)
{
  if (qreg == 0)
    alu_skip = true;
  return true;
}                                  /* seq() */

static bool
sqne(char **err)
{
  if (qreg != 0)
    alu_skip = true;
  return true;
}                                  /* sqne() */

static bool
sqge(char **err)
{
  if (qreg >= 0)
    alu_skip = true;
  return true;
}                                  /* sqge() */

static bool
sqle(char **err)
{
  if (qreg <= 0)
    alu_skip = true;
  return true;
}                                  /* sqle() */

static bool
sqgt(char **err)
{
  if (qreg > 0)
    alu_skip = true;
  return true;
}                                  /* sqgt() */

static bool
sqlt(char **err)
{
  if (qreg < 0)
    alu_skip = true;
  return true;
}                                  /* sqlt() */

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
sfmod(char **err)
{
  alu_skip = mods;
  return true;
}                                  /* sfmod() */

static bool
sfnmod(char **err)
{
  alu_skip = !mods;
  return true;
}                                  /* sfnmod() */

static bool
sbin(char **err)
{
  alu_skip = binary;
  return true;
}                                  /* sbin() */

static bool
snbin(char **err)
{
  alu_skip = !binary;
  return true;
}                                  /* snbin() */

static bool
clrfmod(char **err)
{
  mods = false;
  return true;
}                                  /* clrfmod() */

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
    *err = "Attempt to divide by zero";
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
    *err = "Attempt to take modulo zero";
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
pshq(char **err)
{
  return push(qreg, err);
}                                  /* pshq() */

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
popnf(char **err)
{
  if (!f_valid(err))
    return false;
  fsidx--;
  return true;
}                                  /* popnf() */

static bool
ppvbint(char **err)
{
  if (!f_valid(err))
    return false;
  visbel_interval = fs[fsidx--];
  return true;
}                                  /* ppvbint() */

static bool
ppfbint(char **err)
{
  if (!f_valid(err))
    return false;
  fbrief_interval = fs[fsidx--];
  return true;
}                                  /* ppfbint() */

static bool
dupf(char **err)
{
  if (!f_valid(err))
    return false;
  return fpush(fs[fsidx], err);
}                                  /* dupf() */

static bool
frnd(char **err)
{
  if (!f_valid(err))
    return false;
  fs[fsidx] = rint(fs[fsidx]);
  return true;
}                                  /* frnd() */

static bool
ffloor(char **err)
{
  if (!f_valid(err))
    return false;
  fs[fsidx] = floor(fs[fsidx]);
  return true;
}                                  /* ffloor() */

static bool
fceil(char **err)
{
  if (!f_valid(err))
    return false;
  fs[fsidx] = ceil(fs[fsidx]);
  return true;
}                                  /* fceil() */

static bool
fsin(char **err)
{
  if (!f_valid(err))
    return false;
  fs[fsidx] = sin(fs[fsidx]);
  return true;
}                                  /* fsin() */

static bool
fcos(char **err)
{
  if (!f_valid(err))
    return false;
  fs[fsidx] = cos(fs[fsidx]);
  return true;
}                                  /* fcos() */

static bool
ftan(char **err)
{
  if (!f_valid(err))
    return false;
  fs[fsidx] = tan(fs[fsidx]);
  return true;
}                                  /* ftan() */

static bool
flog(char **err)
{
  if (!f_valid(err))
    return false;
  fs[fsidx] = log(fs[fsidx]);
  return true;
}                                  /* flog() */

static bool
fexp(char **err)
{
  if (!f_valid(err))
    return false;
  fs[fsidx] = exp(fs[fsidx]);
  return true;
}                                  /* fexp() */

static bool
fsqrt(char **err)
{
  if (!f_valid(err))
    return false;
  fs[fsidx] = sqrt(fs[fsidx]);
  return true;
}                                  /* fsqrt() */

static bool
addf(char **err)
{
  if (!f_valid2(err))
    return false;
  fs[fsidx - 1] = fs[fsidx] + fs[fsidx - 1];
  fsidx--;
  return true;
}                                  /* addf() */

static bool
subf(char **err)
{
  if (!f_valid2(err))
    return false;
  fs[fsidx - 1] = fs[fsidx] - fs[fsidx - 1];
  fsidx--;
  return true;
}                                  /* subf() */

static bool
mpyf(char **err)
{
  if (!f_valid2(err))
    return false;
  fs[fsidx - 1] = fs[fsidx] * fs[fsidx - 1];
  fsidx--;
  return true;
}                                  /* mpyf() */

static bool
divf(char **err)
{
  if (!f_valid2(err))
    return false;
  if (fs[fsidx - 1] == 0.0)
  {
    *err = "Attempt to divide by zero";
    return false;
  }                                /* if (fs[fsidx - 1] == 0) */
  fs[fsidx - 1] = fs[fsidx] / fs[fsidx - 1];
  fsidx--;
  return true;
}                                  /* divf() */

static bool
popfr(char **err)
{
  if (!f_valid(err))
    return false;
  if (!push((long)fs[fsidx], err))
    return false;
  fsidx--;
  return true;
}                                  /* popfr(char **err) */

static bool
poprf(char **err)
{
  if (!r_valid(err))
    return false;
  if (!fpush((double)rs[rsidx], err))
    return false;
  rsidx--;
  return true;
}                                  /* poprf(char **err) */

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
  fsidx = -1;
  xreg = 0;
  index_next = false;
  alu_skip = false;
  fmode &= ~FILE_POS_BIT;
  return true;
}                                  /* rst() */

static bool
zam(char **err)
{
  memset(ALU_memory, 0, sizeof ALU_memory);
  memset(FPU_memory, 0, sizeof FPU_memory);
  return true;
}                                  /* zam() */

static bool
scpt(char **err)
{
  fmode &= ~FILE_POS_BIT;
  return true;
}                                  /* scpt() */

static bool
sfpt(char **err)
{
  fmode |= FILE_POS_BIT;
  return true;
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

static bool
ps2p14(char **err)
{
  return push(16384, err);
}                                  /* ps2p14() */

static bool
ps2p15(char **err)
{
  return push(32768, err);
}                                  /* ps2p15() */

static bool
ps2p16(char **err)
{
  return push(65536, err);
}                                  /* ps2p16() */

static bool
ps2p17(char **err)
{
  return push(131072, err);
}                                  /* ps2p17() */

static bool
ps2p18(char **err)
{
  return push(262144, err);
}                                  /* ps2p18() */

static bool
ps2p19(char **err)
{
  return push(524288, err);
}                                  /* ps2p19() */

static bool
ps2p20(char **err)
{
  return push(1048576, err);
}                                  /* ps2p20() */

static bool
ps2p21(char **err)
{
  return push(2097152, err);
}                                  /* ps2p21() */

static bool
ps2p22(char **err)
{
  return push(4194304, err);
}                                  /* ps2p22() */

static bool
ps2p23(char **err)
{
  return push(8388608, err);
}                                  /* ps2p23() */

static bool
ps2p24(char **err)
{
  return push(16777216, err);
}                                  /* ps2p24() */

static bool
ps2p25(char **err)
{
  return push(33554432, err);
}                                  /* ps2p25() */

static bool
ps2p26(char **err)
{
  return push(67108864, err);
}                                  /* ps2p26() */

static bool
ps2p27(char **err)
{
  return push(134217728, err);
}                                  /* ps2p27() */

static bool
ps2p28(char **err)
{
  return push(268435456, err);
}                                  /* ps2p28() */

static bool
ps2p29(char **err)
{
  return push(536870912, err);
}                                  /* ps2p29() */

static bool
ps2p30(char **err)
{
  return push(1073741824, err);
}                                  /* ps2p30() */

static bool
ps2p31(char **err)
{
  return push(2147483648, err);
}                                  /* ps2p31() */

/* **************************** The Opcode Table **************************** */

alu_opcode opcode_defs[] = {
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
  OPCODE(sfeq, "Skip if F is zero"),
  OPCODE(sfne, "Skip if F is not zero"),
  OPCODE(sfge, "Skip if F is greater than or equal to zero"),
  OPCODE(sfle, "Skip if F is less than or equal to zero"),
  OPCODE(sfgt, "Skip if F is greater than zero"),
  OPCODE(sflt, "Skip if F is less than zero"),
  OPCODE(sfmod, "Skip if file modified"),
  OPCODE(sfnmod, "Skip if file not modified"),
  OPCODE(sbin, "Skip if q -b "),
  OPCODE(snbin, "Skip if not q -b "),
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
  OPCODE(inp, "Read next integer in line, push value & length"),
  CAPTION("(leaves cursor on 1st char of number)"),
  OPCODE(pshmode, "Push mode (as per n4000) to R"),
  OPCODE(popmode, "Pop R to set mode (as per n4000)"),
  OPCODE(pshcrs, "Push cursor position to R (zero-based)"),
  OPCODE(pshlnln, "Push line length to R"),
  OPCODE(pshnbln, "Push number of lines in file to R (i.e. # read so far)"),
  OPCODE(pshlnnb, "Push line number to R (same as ^NF / PSHTAB)"),
  OPCODE(pshtbsz, "Push number of spaces between tabstops in file to R"),
  OPCODE(poptbsz, "Pop R to number of spaces between tabstops in file"),
  OPCODE(pshwdth, "Push screen width to R"),
  OPCODE(pshhght, "Push screen height to R"),
  CAPTION(""),
  CAPTION("Instructions that Modify F"),
  CAPTION("============ ==== ====== ="),
  OPCODE(tcf, "Negate (2's complement)"),
  OPCODE(popnf, "Pop F to nowhere (value is discarded)"),
  OPCODE(dupf, "Push a copy of F"),
  OPCODE(frnd, "F = rint(F)"),
  OPCODE(ffloor, "F = floor(F)"),
  OPCODE(fceil, "F = ceil(F)"),
  OPCODE(fsin, "F = sin(F)"),
  OPCODE(fcos, "F = cos(F)"),
  OPCODE(ftan, "F = tan(F)"),
  OPCODE(flog, "F = log(F)"),
  OPCODE(fexp, "F = exp(F)"),
  OPCODE(fsqrt, "F = sqrt(F)"),
  OPCODE(inpf, "Read next number in line, push value to F & length to R"),
  CAPTION("(leaves cursor on 1st char of number)"),
  OPCODE(fqversn, "Push Q version to F"),
  OPCODE(psvbint, "Push Visible Bell Interval to F"),
  OPCODE(ppvbint, "Pop F to Visible Bell Interval"),
  OPCODE(psfbint, "Push Fbrief Interval to F"),
  OPCODE(ppfbint, "Pop F to Fbrief Interval"),
  OPCODE(psclock, "Push result from clock(3) to F as seconds"),
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
  OPCODE(ps2p14, "Push constant 16384 (2**14) to R"),
  OPCODE(ps2p15, "Push constant 32768 (2**15) to R"),
  OPCODE(ps2p16, "Push constant 65536 (2**16) to R"),
  OPCODE(ps2p17, "Push constant 131072 (2**17) to R"),
  OPCODE(ps2p18, "Push constant 262144 (2**18) to R"),
  OPCODE(ps2p19, "Push constant 524288 (2**19) to R"),
  OPCODE(ps2p20, "Push constant 1048576 (2**20) to R"),
  OPCODE(ps2p21, "Push constant 2097152 (2**21) to R"),
  OPCODE(ps2p22, "Push constant 4194304 (2**22) to R"),
  OPCODE(ps2p23, "Push constant 8388608 (2**23) to R"),
  OPCODE(ps2p24, "Push constant 16777216 (2**24) to R"),
  OPCODE(ps2p25, "Push constant 33554432 (2**25) to R"),
  OPCODE(ps2p26, "Push constant 67108864 (2**26) to R"),
  OPCODE(ps2p27, "Push constant 134217728 (2**27) to R"),
  OPCODE(ps2p28, "Push constant 268435456 (2**28) to R"),
  OPCODE(ps2p29, "Push constant 536870912 (2**29) to R"),
  OPCODE(ps2p30, "Push constant 1073741824 (2**30) to R"),
  OPCODE(ps2p31, "Push constant 2147483648 (2**31) to R"),
  CAPTION(""),
  CAPTION("Instructions with 2 operands"),
  CAPTION("============ ==== = ========"),
  CAPTION("(These have the same effect as:-"),
  CAPTION("    pop A; pop B; push A {instr} B (or FP equivalents)"),
  CAPTION
    ("    except attempted divide by zero leaves the registers unchanged)"),
  OPCODE(add, "Add"),
  OPCODE(sub, "Subtract"),
  OPCODE(mpy, "Multiply"),
  OPCODE(div, "Divide"),
  OPCODE(mod, "Modulus"),
  OPCODE(and, "Bitwise AND"),
  OPCODE(or, "Bitwise OR"),
  OPCODE(xor, "Bitwise EXCLUSIVE OR"),
  OPCODE(addf, "Add F"),
  OPCODE(subf, "Subtract F"),
  OPCODE(mpyf, "Multiply F"),
  OPCODE(divf, "Divide F"),
  CAPTION(""),
  CAPTION("FP (double) <==> Integer (long)"),
  CAPTION("== ======== ==== ======= ======"),
  OPCODE(popfr, "Pop F; push (long) to R"),
  OPCODE(poprf, "Pop R; push (double) to F"),
  CAPTION(""),
  CAPTION("Index Register Instructions"),
  CAPTION("===== ======== ============"),
  OPCODE(indx, "Index next PSH[F] or POP[F]"),
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
  OPCODE(xmode, "Mainline is to set mode from X after next cmd read (for ^N7)"),
  CAPTION(""),
  CAPTION("Q Result Register Instructions"),
  CAPTION("= ====== ======== ============"),
  OPCODE(pshq, "Push contents of Q to R"),
  OPCODE(sqeq, "Skip if Q is zero"),
  OPCODE(sqne, "Skip if Q is not zero"),
  OPCODE(sqge, "Skip if Q is greater than or equal to zero"),
  OPCODE(sqle, "Skip if Q is less than or equal to zero"),
  OPCODE(sqgt, "Skip if Q is greater than zero"),
  OPCODE(sqlt, "Skip if Q is less than zero"),
  CAPTION(""),
  CAPTION("Control Instructions"),
  CAPTION("======= ============"),
  OPCODE(dmp, "Dump Registers"),
  OPCODE(rst, "Reset Registers to initial state (except Q)"),
  OPCODE(zam, "Zeroise All Memory"),
  OPCODE(scpt, "Store Cursor Position Tabs (initial setting, also after RST)"),
  OPCODE(sfpt, "Store File Position Tabs"),
  OPCODE(clrfmod, "Set file to be unmodified"),
};                                 /* alu_opcode opcode_defs[] = */

/* Instantiate other externals */

int num_alu_opcode_table_entries = sizeof opcode_defs / sizeof *opcode_defs;

/* ***************************** exec_alu_opcode **************************** */

bool
exec_alu_opcode(int opcode, char **err)
{
  return opcode_defs[alu_table_index[opcode - FIRST_ALU_OP]].func(err);
}                                  /* exec_alu_opcode(int opcode) */
