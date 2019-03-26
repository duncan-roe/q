/* N E W M A C 2
 *
 * Copyright (C) 2012,2014,2019 Duncan Roe
 *
 * Generates a 16-bit-char macro from the string in ubuf
 */
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "prototypes.h"
#include "edmast.h"
#include "macros.h"
#include "tabs.h"
#include "alu.h"

/* Macros */

#define GIVE_UP return false
#define MAX_OPCODE_LEN 7

/* Static Variables */

static uint16_t xpnsion[Q_BUFSIZ];

/* ******************************* get_op_idx ******************************* */

static int
get_op_idx(alu_dict_ent * root, char *op)
{
  alu_dict_ent *ptr;

/* Return condition for the recursive function */
  if (!*op)
    return root->fn_idx;

/* Find the node of the first letter */
  ptr = root;
  while (ptr->letter != *op)
    if (ptr->alt)
    {
      if (*op < ptr->alt->letter)
        return -4;                 /* Mnemonic not in dictionary */
      ptr = ptr->alt;
    }                              /* if (ptr->alt) */
    else
      return -4;

  if (!ptr->next)
    return -4;

/* Make the recursive call */
  return get_op_idx(ptr->next, op + 1);
}                                  /* get_op_idx() */

/* ******************************* get_opcode ******************************* */

static bool
get_opcode(char *op, uint16_t *result)
{
  int idx;

  idx = get_op_idx(&root_alu_dict_ent, op);
  if (idx >= 0)
  {
    *result = FIRST_ALU_OP + idx;
    return true;
  }                                /* if (idx >= 0) */
  if (idx == -4)
    fprintf(stderr, "Unrecognised opcode mnemonic \"%s\"\r\n", op);
  else
    fprintf(stderr, "Internal error - get_op_idx(%s) returned %d\r\n", op, idx);
  return false;
}                                  /* get_opcode(); */

/* ******************************* get_alu_op ******************************* */

static bool
get_alu_op(char *mybuf, uint16_t *result, char **endptr)
{
  long locn;
  char tbuf[16];
  int i;
  char *p;

/* Special-case the memory acccessors as usual */
  if (!strncasecmp(mybuf, "PSH ", 4) || !strncasecmp(mybuf, "POP ", 4))
  {
    errno = 0;
    locn = strtol(mybuf + 4, endptr, 8);
    if (errno)
    {
      fprintf(stderr, "%s. %s (strtol)\r\n", strerror(errno), mybuf);
      GIVE_UP;
    }                              /* if (errno) */
    if (**endptr != GT)
    {
      fprintf(stderr, "Character '%c' illegal\r\n", **endptr);
      GIVE_UP;
    }                              /* if (**endptr != GT) */
    if (locn < 0 || locn > 0777)
    {
      fprintf(stderr, "Value %lo is out of range (not between 0 & 0777)\r\n",
        locn);
      GIVE_UP;
    }                              /* if (locn < 0 || locn > 0777) */
    *result = (strncasecmp(mybuf, "POP ", 4) ? 05000 : 06000) | locn;
    return true;
  }                                /* if (!strncasecmp(mybuf, "PSH ", 4 ... */
  if (!strncasecmp(mybuf, "PSHF ", 5) || !strncasecmp(mybuf, "POPF ", 5))
  {
    errno = 0;
    locn = strtol(mybuf + 5, endptr, 8);
    if (errno)
    {
      fprintf(stderr, "%s. %s (strtol)\r\n", strerror(errno), mybuf);
      GIVE_UP;
    }                              /* if (errno) */
    if (**endptr != GT)
    {
      fprintf(stderr, "Character '%c' illegal\r\n", **endptr);
      GIVE_UP;
    }                              /* if (**endptr != GT) */
    if (locn < 0 || locn > 0777)
    {
      fprintf(stderr, "Value %lo is out of range (not between 0 & 0777)\r\n",
        locn);
      GIVE_UP;
    }                              /* if (locn < 0 || locn > 0777) */
    *result = (strncasecmp(mybuf, "POPF ", 5) ? 011000 : 012000) | locn;
    return true;
  }                                /* if (!strncasecmp(mybuf, "PSHF ", 5 ... */

/* Tab accessors are similar to memory accessors */
  if (!strncasecmp(mybuf, "PSHTAB ", 7) || !strncasecmp(mybuf, "POPTAB ", 7))
  {
    long tabidx;
    uint8_t tuch;

    if (mybuf[8] == GT)
    {
      tuch = (uint8_t)mybuf[7];
      *endptr = mybuf + 8;
    }                              /* if (mybuf[8] == GT) */
    else if (!strncmp(mybuf + 7, "^?>", 3))
    {
      *endptr = mybuf + 9;
      tuch = DEL;
    }                              /* else if (!strncmp(mybuf + 7, "^?>", 3)) */
    else
    {
      fprintf(stderr, "Character '%c' illegal\r\n", mybuf[7]);
      GIVE_UP;
    }                              /* if (mybuf[8] == GT) */
    if (gettab(tuch, false, &tabidx, true))
    {
      *result = strncasecmp(mybuf, "POPTAB ", 7)
        ? FIRST_ALU_OP + num_ops + tabidx
        : FIRST_ALU_OP + num_ops + NUM_TABS + tabidx;
      return true;
    }                 /* if (gettab((uint8_t)mybuf[7], false, &tabidx, true)) */
    fprintf(stderr, "\r\n");
    GIVE_UP;
  }                           /* if (!strncasecmp(mybuf, "PSHTAB ", 7) || ... */

/* Get opcode to tbuf, in upper case, advancing caller's pointer */
  for (i = MAX_OPCODE_LEN, p = tbuf; i > 0; i--, p++, (*endptr)++)
  {
    *p = toupper((uint8_t)**endptr);
    if (!*p)
    {
      fprintf(stderr, "Macro specification ends mid-opcode\r\n");
      GIVE_UP;
    }                              /* if (!*p) */
    if (*p == GT)
      break;
  }       /* for (i = MAX_OPCODE_LEN, p = tbuf; i > 0; i--, p++, (*endptr)++) */
  *p = 0;
  if (**endptr != GT)
  {
    fprintf(stderr, "Opcode mnemonic too long (%d chars max)\r\n",
      MAX_OPCODE_LEN);
    GIVE_UP;
  }                                /* if (**endptr != GT) */

  return get_opcode(tbuf, result);
}                                  /* get_alu_op() */

/* ********************************* newmac2 ******************************** */

bool
newmac2(bool appnu)
{
  int k, m, l;
  uint16_t thisch;                 /* Character being examined */
  char *bfp;

  m = 0;                           /* Accumulates macro size */
  for (bfp = ubuf; *bfp; bfp++)    /* Loop on chars stored */
  {
    if (*bfp != CARAT)
    {
      xpnsion[m++] = *bfp;
      continue;
    }                              /* if (thisch != CARAT) */
    if (!*++bfp)                   /* If nothing to precurse (error) */
    {                              /* (and point to next i/p char) */
      fprintf(stderr, "Macro specification ends mid-sequence");
      GIVE_UP;
    }                              /* if (!*++bfp) */
    thisch = *bfp;                 /* Get control */

/* Allow control characters to be spec'd as lower case letters ... */
    if ((thisch >= 'a' && thisch <= 'z') || (thisch >= 0100 && thisch <= 0137))
    {
      thisch = (thisch & 037);     /* Convert to a control */
      xpnsion[m++] = thisch;       /* Store control char */
      continue;
    }                          /* if ((thisch >= 'a' && thisch <= 'z') || ... */

/* Char is not a normal control. May have ^* (^) or ^? (rubout) */
/* May also have up to 16-bit octal value ^<ooo...> */
    if (thisch == LT)
    {                              /* Try for an octal sequence */
      for (thisch = 0;;)
      {                            /* thisch will accumulate result */
        if (!*++bfp)
        {
          fprintf(stderr, "Macro specification ends mid octal number");
          GIVE_UP;
        }                          /* if (!*++bfp) */
        k = (l = *bfp) - '0';      /* Get octal digit */
        if (k < 0 || k > 7)
          break;                   /* B wasn't octal, may be ">" */
        if (thisch & 0160000)
        {                          /* Would overflow */
          fprintf(stderr, "Octal macro char out of range (>177777)");
          GIVE_UP;
        }                          /* if (thisch & 0160000) */
        thisch = (thisch << 3) + k; /* Accumulate digit */
      }                            /* for (thisch = 0;;) */
      if (l == GT)
      {
        xpnsion[m++] = thisch;
        continue;
      }                            /* if (l == GT) */
      if (bfp > ubuf && *(bfp - 1) == LT && get_alu_op(bfp, &thisch, &bfp))
      {
        xpnsion[m++] = CTL_N;
        xpnsion[m++] = thisch;
        continue;
      }                            /* if (bfp > ubuf && *(bfp - 1) == LT ... */
      fprintf(stderr, "Bad octal character (%c) in macro definition", l);
      GIVE_UP;
    }                              /* if (thisch == LT) */
    else if (thisch == ASTRSK)
      thisch = CARAT;
    else if (thisch == QM)
      thisch = 0177;               /* ^? -> DEL */
    else
    {
      fprintf(stderr, "unrecognised control character: ^%c", thisch);
      GIVE_UP;
    }                              /* if (thisch == LT) ... else */

    xpnsion[m++] = thisch;
  }                                /* for (bfp = ubuf; *bfp; bfp++) */
/* Now try to store macro */
  if (!macdefw(verb, xpnsion, m, appnu))
    GIVE_UP;
/*
 * If the current macro has just redefined itself, get out of it now
 * We need to do this so Q massage will work on the first arg, with
 * certain .qrc's
 */
  if (curmac == verb)
    notmac(false);
  return true;                     /* Successful end */
}                                  /* newmac2() */
