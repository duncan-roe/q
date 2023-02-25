/* >%---- CODE_STARTS ./workfile.c */
/* W O R K F I L E
 *
 * Copyright (C) 2012,2014,2018-2020 Duncan Roe
 *
 * Dual-mode Workfile System
 * ========= ======== ======
 *
 * Lines are either held in-memory (the same as Linedit did), or are referenced
 * by their address in an mmap'ed file.
 *
 * The workfile consists of a bidirectional ring of Index blocks. An in-memory
 * line has one index block, and enough data blocks to contain it: if the line
 * is shorter than the size of a pointer, then it is held in the index block and
 * there are no data blocks. Index blocks for in-memory lines have a count of
 * data blocks, a pointer to the 1st data block, and a count of the # of chars
 * in the last data block.
 *
 * An Index block for mmap'ed lines usually manages many such lines. The data
 * block count is set to -1 to distinguish the mmap usage. The count is the
 * number of lines controlled. This count has to be explicitly checked for
 * overflow when adding a line, since the same line can be referenced any number
 * of times (e.g. if file is E-entered more than once(?)). The pointer is to the
 * supplementary index block.
 *
 * Data blocks are mainly used for in-memory lines and are always 64 bytes in
 * length. when the block is in use, it starts with a pointer to the next block
 * or NULL if it is the last block for the line; the remainder of the block
 * holds character data. When the block is free, it is chained in the regular
 * fashion. Data blocks also hold mapbk structures, since these are bigger than
 * an index block.
 *
 * Supplementary index blocks are longer than index blocks, so are held in data
 * blocks. The next and prev members are actually the base of the offset block
 * chain for the group of records managed by the index block. There is a pointer
 * to the file (memory mapped) address of the first record in the group, whether
 * that record is deleted or not. Then there is a pointer to just beyond the end
 * of the memory mapping, required to give an upper bound to the record length.
 * There is a copy of the mode in which the group was created (for DOS read &
 * tab expand), and a count of how many records in the first block are deleted.
 *
 * Offset blocks are 1024 bytes in length. Apart from their next & prev
 * pointers, they can be filled with uint16_t offsets. These are the
 * distances from the address in the supplementary index block to the record
 * start point.
 *
 * If pointr addreses an mmap'd index block, poffset says how many lines into
 * the line group we are. At other times, it is invalid.
 *
 * The lengths of the larger blocks are deliberately powers of 2. A "page"
 * (large power of 2) of blocks is allocated at a time, to avoid the overhead
 * of numerous small malloc's is avoided. mallopt is no longer used, even if
 * available. To some extent, the strategy may minimise page faults when moving
 * through the file. This will be more the case if malloc is "page aware",
 * aligning the returned block when the request is an integral multiple of the
 * hardware page size.
 *
 * The workfile system can handle lines up to length 64K - 1. The workfile
 * system tracks the number of the current line and the total number of lines.
 */
/* Pragmas */

#pragma GCC optimize 0             /* Avoid having to guess volatiles */

/* Headers */

/* >%---- KEEP2HERE ./workfile.c */
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/mman.h>
#include "prototypes.h"
#include "macros.h"
#include "fmode.h"
/* >%---- CUT_HERE ./workfile.c */

/* Macros */

#define NOM_PAGE 8192              /* Largest known pagesize on any system */
#define FULLPAGES (NOM_PAGE / sizeof(void*) - 2)
#define OFF_DIM ((1024 - 2 * sizeof(void*)) / sizeof(uint16_t))

/* Typedefs */

typedef struct databk
{
  struct databk *next;
  uint8_t data1[BLKCAP];           /* Actually a BPTR when free */
} databk;

typedef struct indxbk
{
  struct indxbk *next;
  struct indxbk *prev;
  databk *dtaptr;                  /* Or may hold up to 4 chars */
  short blocks;
  uint16_t chars;                  /* Chars in last data block */
} indxbk;

typedef struct ofstbk
{
  struct ofstbk *next;
  struct ofstbk *prev;
  uint16_t offset[OFF_DIM];
} ofstbk;

typedef struct suppbk
{
  ofstbk *next;
  ofstbk *prev;
  uint8_t *filptr;
  uint8_t *endptr;
  fmode_t mode;
  short dlrecs;
} suppbk;

typedef struct pagebk
{
  struct pagebk *next;
  struct pagebk *prev;
  void *pages[FULLPAGES];
} pagebk;

typedef struct mapbk
{
  struct mapbk *next;
  struct mapbk *prev;
  uint8_t *start;
  unsigned long length;
  unsigned long inode;
} mapbk;

typedef struct chainbase
{
  struct chainbase *next;
  struct chainbase *prev;
} chainbase;

typedef struct hddr                /* Used by the chaining routines */
{
  struct hddr *next;
  struct hddr *prev;
} hddr;                            /* typedef struct hddr */

/* Static prototypes */

static void move_chain(void *firstblk, void *lastblk, void *beforeblk);

/* Data private to workfile system */

static indxbk *pointr;             /* W/F ptr addr */
static indxbk *auxptr;             /* W/F aux ptr addr */
static ofstbk *xxoffs;        /* Offset block containing last gtofst() result */
static int poffst;                 /* # lines ptrpos is on from pointr */
static int aoffst;                 /* # lines on from auxptr */

/* The workfile base is a real index block rather than a chainbase, so the code
 * can tell it's not on an mmap'ed line when positioned on it */

static indxbk bwork = { NULL, NULL, NULL, 0, 0 }; /* W/f chainbase */

static chainbase bfrgt;            /* Forgotten chainbase */
static chainbase bifree;           /* Free index block chainbase */
static chainbase bdfree;           /* Free data block chainbase */
static chainbase bofree;           /* Free offsets block chainbase */
static chainbase bpage;            /* Allocated page database */
static chainbase bmap;             /* Mapped file database (uses data blks) */
static long forpos;                /* 1st line # in forgotten chain */
static long auxpos = 0;            /* Line # of aux ptr */
static short count;                /* Used by clrfgt, inslin, rdlin &c */
static int lastpages;              /* # entries in last pagebk */
static int first = 1;              /* First-time flag for finitl */
static long fortot = 0;            /* # lines in forgotten chain */

/*  Brief externally visible subroutine specifications:-
 *
 * finitl [re-]initialises the workfile system
 *
 * setaux Sets the auxiliary pointer to the line # requested
 *
 * setptr Sets the workfile pointer to the line # requested. setptr attempts to
 *        get there by the fastest route: from where it is or the start or end
 *        of file as appropriate. The optimisation may be less than perfect if
 *        there are a mixture of mmap'd and in-memory lines
 *
 * forget Implements the FOrget command for the main editor
 *
 * rdlin Reads the line at either the main or auxiliary pointer and then leaves
 *       that pointer after that line. Returns 0 if at eof, else 1
 *
 * delete Moves the line before the [aux] pointer from the workfile chain to the
 *        end of the forgotten chain.
 *        Argument says which pointer to use: 1 - auxiliary, 0 - normal
 *
 * clrfgt Clears the forgotten chain, which may be empty, putting blocks back on
 *        the end of the free chains without upsetting their order too much
 *
 * inslin Inserts the text in the screenedit buffer A1 before the main pointer
 */

/* ******************************* auxxch ****************************** */

static void
auxxch(void)
{
  indxbk *ix;                      /* Scratch */
  long i;                          /* Scratch */
/*  */
  ix = pointr;
  pointr = auxptr;
  auxptr = ix;
  i = poffst;
  poffst = aoffst;
  aoffst = i;
  i = ptrpos;
  ptrpos = auxpos;
  auxpos = i;
}

/* ******************************* qunchn ****************************** */

/* Unchain block "blk" */

static void *
qunchn(void *blk)
{
  ((hddr *)blk)->next->prev = ((hddr *)blk)->prev;
  ((hddr *)blk)->prev->next = ((hddr *)blk)->next;
  return blk;
}                                  /* static void qunchn(void*blk) */

/* ******************************* qchain ****************************** */

/* Chain block "new" before block "old" */

static void
qchain(void *new, void *old)
{
  ((hddr *)new)->next = (hddr *)old;
  ((hddr *)new)->prev = ((hddr *)old)->prev;
  ((hddr *)old)->prev->next = (hddr *)new;
  ((hddr *)old)->prev = (hddr *)new;
}                                  /* static void qchain(void*old,void,new) */

/* ******************************* getmem ****************************** */

static void *
getmem(chainbase *cb, size_t sz, char *desc)
{
  size_t roomleft;                 /* Amount of "page" left */
  uint8_t *pg;                     /* Addresses "page"-size blocks */
/*  */
  if (cb->next != cb)              /* There are free blocks */
    return qunchn(cb->prev);       /* Use last block */

/* Need to malloc more space */

  for (;;)                         /* May need more than 1 malloc */
  {
    if (!(pg = malloc(NOM_PAGE)))
    {
      printf("\n\rNo memory for %s block (malloc)\r\n", desc);
      return NULL;
    }                              /* if(!(pg=malloc(NOM_PAGE))) */
/* Database needs this page */
    if (bpage.next == &bpage || lastpages == FULLPAGES)
    {
      qchain(pg, &bpage);
      lastpages = 0;               /* No entries in this block */
      continue;                    /* for(;;) */
    }                              /* if(bpage.next==&bpage) */
    ((pagebk *)bpage.prev)->pages[lastpages++] = pg; /* Record new page */
    for (roomleft = NOM_PAGE; roomleft >= sz; pg += sz, roomleft -= sz)
      qchain(pg, cb);              /* Add the next chunk to the chain */
    return qunchn(cb->prev);
  }                                /* for(;;) */
}                     /* static void*getmem(chainbase*cb,size_t sz,char*desc) */


/* ******************************** get2 ******************************* */

/* Get the pair of blocks required to create an empty mmap'd group, initialising
 * them as much as possible */

static indxbk *
get2(uint8_t *linptr)
{
  indxbk *ix;                      /* Scratch */
  suppbk *sp;                      /* Scratch */
/*  */
  if (!(ix = getmem(&bifree, sizeof(indxbk), "index")))
    return NULL;
  if (!(sp = getmem(&bdfree, sizeof(databk), "supp")))
  {
    qchain(ix, &bifree);           /* Return allocated index block */
    return NULL;
  }                       /* if(!(*sp=getmem(&bifree,sizeof(indxbk),"supp"))) */
  ix->dtaptr = (void *)sp;         /* Addr supp blk */
  ix->blocks = -1;                 /* Is an mmap'd group */
  ix->chars = 0;                   /* No lines controlled */
  sp->next = sp->prev = (void *)sp; /* Offset chain empty */

  sp->filptr = linptr;
  sp->endptr = NULL;
  sp->mode = fmode;                /* Only want low-order bits */
  sp->dlrecs = 0;                  /* No deleted lines */
  return ix;
}                                  /* static ofstbk*get2(uint8_t*linptr) */

/* ******************************* gtofst ****************************** */

/* Given a mmapping index block and line offset, return the address of the
 * short offset for that line. The line # may be at most one more than the # in
 * the group: that is how we append. The address of the offset block found is
 * available to other functions in this compilation unit as xxoffs */

static uint16_t *
gtofst(indxbk *ix, int offset)
{
  suppbk *sp;                      /* Scratch */
  int absoff;                      /* Offset including deleted lines */
  int totbks;                      /* Total # of offset blocks in this group */
  int blknum;                      /* 0-based block # we weant */
  int offidx;                      /* Index of offset in this block */
  int i;                           /* Scratch */

/* Sanity checks */

  if (offset < 0 || offset > ix->chars)
  {
    printf("\r\nBad call of gtofst: offset is %d when group has %hu lines\r\n",
      offset, ix->chars);
    return NULL;
  }                                /* if(offset<0||offset>ix->chars) */
  if (ix->blocks != -1)
  {
    fputs("\r\nBad call of gtofst: block is not mmapping\r\n", stdout);
    return NULL;
  }                                /* if(ix->blocks!=-1) */

  sp = (void *)ix->dtaptr;
  absoff = offset + sp->dlrecs;
  totbks = (ix->chars + sp->dlrecs + OFF_DIM - 1) / OFF_DIM;
  blknum = absoff / OFF_DIM;
  offidx = absoff % OFF_DIM;
  if (blknum == totbks)            /* Need a new one */
  {
    if (offidx)
    {
      printf("\r\nInternal error in gtofst: new block has offset %d (should"
        " be zero)\r\n", offidx);
      return NULL;
    }                              /* if(offidx) */
    if (!(xxoffs = getmem(&bofree, sizeof(ofstbk), "offsets")))
      return NULL;
    qchain(xxoffs, sp);            /* Place at end of offset block chain */
    return xxoffs->offset;         /* 1st element is required one */
  }                                /* if(blknum==totbks) */

/* Decide which direction to go in, and go */

  if (blknum < totbks / 2)
    for (xxoffs = sp->next, i = 0; i < blknum; i++)
      xxoffs = xxoffs->next;
  else
    for (xxoffs = sp->prev, i = totbks - 1; i > blknum; i--)
      xxoffs = xxoffs->prev;

  return xxoffs->offset + offidx;
}                                  /* uint16_t*gtofst(indxbk*ix,int offset) */

/* ******************************* splitb ****************************** */

static bool
splitb(void)
/* Split the mmapping block addressed by pointr such that poffst becomes zero
 * (pointr points to a block whose 1st non-deleted record is ptrpos)  */
/* The initial implementation simply duplicates the offset block where the split
 * is (except if the split is on the first line in that block). The 2 new groups
 * have the same file addresses, and block contents are unaltered. */
{
  ofstbk *sof;                     /* Splitting offset block */
  ofstbk *nof;                     /* New offset block */
  indxbk *nix;                     /* New index block */
  suppbk *nsp;                     /* New supp block */
  volatile suppbk *sp;             /* Old supp block */
  uint16_t *us;                    /* Scratch */
  int offidx;                      /* Index of offset in this block */

/* Sanity checks */

  if (pointr->blocks != -1)
  {
    fputs("\r\nBad call of splitb: pointr does not address a mapping block\r\n",
      stdout);
    return false;                  /* Error */
  }                                /* if(pointr->blocks!=-1) */
  if (!poffst)                     /* Nothing to do */
    return true;                   /* Success(?) */

/* Discover address of splitting block. Use gtofst() to avoid duplicating code
 */

  if (!(us = gtofst(pointr, poffst)))
    return false;                  /* Shouldn't happen */

/* We will need a new index pair. Put it before the current group. The
 * duplicated offsets block (if any) will be the last */

  sp = (void *)pointr->dtaptr;     /* Current supp blk */
  if (!(nix = get2(sp->filptr)))
    return false;                  /* No memory */
  qchain(nix, pointr);

/* See if we need to duplicate a block. If so, force its creation by calling
 * gtofst on the new group */

  sof = xxoffs;                    /* Remember splitting blk addr */
  offidx = us - sof->offset;     /* Index of returned value, also # to delete */
  if (offidx)                      /* Some deletes, so must duplicate block */
  {
    if (!gtofst(nix, 0))           /* No memory */
      return false;
    memcpy(xxoffs->offset, sof->offset, sizeof sof->offset);
    nix->chars = offidx;           /* Assume this is not 1st blk (no deletes) */
  }                                /* if(offidx) */

/* Fix up delete counts, move whole blocks up to but excluding the split block
 */

  nsp = (void *)nix->dtaptr;       /* New supp blk */
/* Move blocks */
  for (nof = nsp->next; sp->next != sof; nix->chars += OFF_DIM)
    qchain(qunchn(sp->next), nof);
  nix->chars -= sp->dlrecs;        /* Account for short 1st block */
  nsp->dlrecs = sp->dlrecs;        /* 1st block is moved / dup'd */
  nsp->mode = sp->mode;            /* Mode at map time */
  nsp->filptr = sp->filptr;        /* Pointer from get2 may be wrong */
  nsp->endptr = sp->endptr;
  pointr->chars -= nix->chars;     /* Total recs stay the same */
  sp->dlrecs = offidx;

/* Fix up the aux pointer, if it was in the split group */

  if (auxptr == pointr && (aoffst -= poffst) < 0)
  {
/* Aux line now in previous group */
    auxptr = auxptr->prev;
    aoffst += auxptr->chars;
  }                                /* if(auxptr==pointr&&(aoffst-=poffst)<0) */

  poffst = 0;
  return true;                     /* Success */
}                                  /* static int splitb(void) */

/* ******************************* delete ****************************** */

void
delete(bool aux, long num2del, bool forgettable)
{
/* On entry, the pointer is 1 line after the first line to delete */

  long savpos[2];                  /* Saved ptrpos */
  indxbk *savptr[2];               /* Saved pointr */
  long nextpos;                    /* 1st line that is not deleted */

/* Delete lines, starting at line before that addressed by pointr */

/* If line to not stay on forgotten chain, num2del m.b. 1 */
  if (!forgettable || aux)
  {
    if (num2del != 1)
    {
      fprintf(stderr,
        "\r\nSERIOUS ERROR: !forgettable or aux & num2del != 1\r\n");
      return;
    }                              /* if (num2del != 1) */
  }                                /* if (forgettable) */

  mods = true;                     /* File has been modified */
  if (aux)                         /* Delete line before aux pointer */
  {
    if (auxpos <= ptrpos)      /* Main pointer must decrease when aux deletes */
      ptrpos--;
    auxxch();                      /* Exchange pointers &c. */
  }                                /* if(aux) */

/* The first line to delete and the line after the last line to delete both
 * need to be at the start of a group (no-op if either not memory-mapped) */

  setptr(ptrpos - 1);              /* Point to 1st line 2B deleted */
  if (pointr->blocks < 0 && poffst && !splitb())
    return;                        /* No memory for splitb() */
  savpos[0] = ptrpos;
  nextpos = ptrpos + num2del;      /* 1st un-deleted line */

  setptr(nextpos);
  if (pointr->blocks < 0 && poffst && !splitb())
    return;                        /* No memory for splitb() */
  savpos[1] = ptrpos;
  savptr[1] = pointr;

  setptr(savpos[0]);
  savptr[0] = pointr;

/* Move deleted lines to the forgotten chain whether forgettable or not. */

/* If we're going to leave stuff in the forgotten chain, */
/* better make sure it's empty first */
  if (forgettable && fortot)
    clrfgt();

/* pointr is addressing the first index block that is going to be removed, */
/* so need to move it elsewhere. */
/* Point at the first line that will not be deleted, then reduce ptrpos. */
  setptr(savpos[1]);
  ptrpos -= num2del;

  move_chain(savptr[0], savptr[1]->prev, &bfrgt);
  fortot = num2del;
  forpos = savpos[0];
  lintot -= num2del;
  if (!forgettable)
    clrfgt();
  setptr(savpos[0]);               /* Decrement line # */

  if (aux)                         /* Was aux delete */
    auxxch();                      /* Restore pointers &c. */
}

/* ******************************* clrfgt ****************************** */

/* This subroutine clears the forgotten chain, which may be empty, putting
 * blocks back on the end of the free chain, preserving their order, mostly */

void
clrfgt()
{
  indxbk *ix;                      /* Scratch */
  databk *dt, *nxtdt;              /* Scratch */
  suppbk *sp;                      /* Scratch */

/* There is a ring of index blocks, with data blocks hanging off them.
 * First of all, free the data blocks... */

  forpos = 0;                      /* Invalidate */
  fortot = 0;                      /* Forgotten chain (will be) empty */
  if (bfrgt.next == &bfrgt)        /* Ring is empty */
    return;
  for (ix = (void *)bfrgt.next; ix != (void *)&bfrgt; ix = ix->next)
  {
    if (ix->blocks < 0)            /* Memory-mapped line group */
    {
      sp = (void *)ix->dtaptr;
      if (sp->next != (ofstbk *)sp) /* Is it possible to have no blocks? */
        move_chain(sp->next, sp->prev, &bofree);
      qchain(sp, &bdfree);         /* Return supp blk */
    }                              /* if (ix->blocks < 0) */
    else                           /* In-memory line */
    {
      if (!(count = ix->blocks))   /* Empty line */
        continue;

/* Free data blocks in this line */
      for (dt = ix->dtaptr; count > 0; count--, dt = nxtdt)
      {
        nxtdt = dt->next;          /* Addr next block in chain */
        qchain(dt, &bdfree);       /* Chain B4 free data block base */
      }                            /* for (dt = ix->dtaptr; ... */
    }                              /* if (ix->blocks < 0) else */
  }                                /* for (ix = (void *)bfrgt.next; ... */

/* Free the index blocks in the forgotten chain, by moving the forgotten chain
 * into free chain in one go */
  move_chain(bfrgt.next, bfrgt.prev, &bifree);
}

/* ******************************* inslin ****************************** */

void
inslin(scrbuf5 *a1)
{
  int i;                           /* Scratch */
  uint8_t *p;                      /* Scratch */
  indxbk *ix;                      /* Addr new block */
  databk *dt;                      /* Addr data blocks */

/* If in the middle of an mmapping block, must split it */

  if (pointr->blocks < 0 && poffst && !splitb())
    return;                        /* No memory for split block */

  mods = true;                     /* File being modified */
  if (fortot)
    clrfgt();                      /* Empty forgotten chain */
/*
 * Insert a new line before the workfile pointer; update # lines; if auxptr was
 * same as pointr then make it point to new block
 */
  if (!(ix = getmem(&bifree, sizeof(indxbk), "index")))
    return;
  qchain(ix, pointr);              /* Chain this block before current */
  lintot++;                        /* Update total lines */
  ptrpos++;                        /* Update line # */

/* If apparently inserting before the aux pointer, increment it */

  if (auxpos >= ptrpos)
    auxpos++;

/* If inserting at the Aux pointer, update it to point to the new line */

  if (pointr == auxptr && (pointr->blocks >= 0 || aoffst == poffst))
    auxptr = ix;                   /* In-memory so aoffset now irrelevant */
/*
 * Partially set up block, in case we take a fault when try to get
 * 1st data block.
 */
  ix->blocks = 0;                  /* No blocks in last block */
  ix->chars = 0;                   /* No chars in last block */
  ix->dtaptr = NULL;               /* Null pointer */
  dt = (databk *)&ix->dtaptr;      /* Where addr 1st data blk is 2 go */

/* Get line length, if it will fit where pointer would go then put it there */

  count = a1->bchars;              /* Running count # to do */
  if (count <= sizeof(void *))
  {
    if (count)
      memcpy(&ix->dtaptr, a1->bdata, (size_t)count);
    ix->chars = count;             /* No blocks (done), 0-4 chars */
    return;                        /* Finished */
  }
/* Remember where the data starts, will be used as running pointer */
  p = a1->bdata;

/* Xfer line into data blocks */

  for (;;)
  {
    if (!(dt = dt->next = getmem(&bdfree, sizeof(databk), "data")))
      return;
    ix->blocks++;                  /* Count extra data block */
    i = count > BLKCAP ? BLKCAP : count; /* # chars to do this time */
    memcpy(dt->data1, p, (size_t)i);
    if (count <= BLKCAP)
      break;                       /* Store this then we're done */
    p += i;
    count -= i;
  }                                /* for(;;) */
  ix->chars = count;               /* Of chars in last block */
}

/* ******************************* rdlin ******************************* */

bool
rdlin(scrbuf5 *a1, bool aux)
{
  uint8_t *p;                      /* Scratch */
  int i;                           /* Scratch */
  databk *dt;                      /* Scratch */
  suppbk *sp;                      /* Scratch */
  bool result;                     /* iff data returned (else eof) */
/*  */
  if (aux)                         /* Reading aux pointer */
    auxxch();
  if (pointr == &bwork)            /* At eof, maybe */
  {
    if (deferd)
    {
      dfread(1, a1);               /* Read line into w/f & into a1 */
      if (pointr != &bwork)        /* Extra line read */
      {                           /* Step pointer past new line (back to eof) */
        ptrpos++;
        pointr = &bwork;
        result = true;
      }                            /* if(pointr!=&bwork) */
      else
        result = false;
    }                              /* if(deferd) */
    else
      result = false;
  }                                /* if(pointr==&bwork) */
  else
  {
    result = true;                 /* Indicate not eof */
    if (pointr->blocks < 0)        /* Mmapping */
    {

/* Unpack line from mapped memory */

      sp = (void *)pointr->dtaptr; /* Supp info for group */
      memrec(sp->filptr + *gtofst(pointr, poffst), sp->endptr, sp->mode, a1);

/* Step on to the next line */

      if (poffst == pointr->chars - 1) /* Last line in group */
      {
        pointr = pointr->next;     /* Next group or in-memory line */
        poffst = 0;                /* Whether mmapping or no */
      }                            /* if(poffst==pointr->chars+1) */
      else
        poffst++;                  /* Next line in group */
    }                              /* if(pointr->blocks<0) */
    else
    {
      count = pointr->chars;       /* Remember chars in last block */
      if (!(i = pointr->blocks))   /* If there are no blocks */
      {                 /* Short line - data is in index block's data pointer */
        if (count)
          memcpy(a1->bdata, &pointr->dtaptr, (size_t)count);
        a1->bchars = pointr->chars;
      }                            /* if(!(i=pointr->blocks)) */
      else                         /* Line with at least 1 data block */
      {
        p = a1->bdata;             /* P is next addr for data */
        dt = pointr->dtaptr;       /* Addr 1st data block */
        for (; i > 1; i--)         /* Transfer full blocks */
        {
          memcpy(p, dt->data1, BLKCAP); /* Move this block */
          p += BLKCAP;             /* Up "to" address */
          dt = dt->next;           /* Up "from" address */
        }                          /* for(;i>1;i--) */
        memcpy(p, dt->data1, (size_t)count); /* Move last block */
        a1->bchars = p - a1->bdata + count; /* Store # of chars in line */
      }                            /* if(!(i=pointr->blocks)) else */
      pointr = pointr->next;       /* Move file pointer on 1 line */
      poffst = 0;                  /* Whether mmapping or no */
    }                              /* if(pointr->blocks<0) else */
    ptrpos++;                      /* Update line # */
    a1->bcurs = 0;                 /* Leave cursor at col 1 for main */
  }                                /* if(pointr==&bwork) else */
  if (aux)
    auxxch();                      /* Restore pointers, line # */
  return result;                   /* Whether line read or not */
}                                  /* Finished */

/* ******************************* forget ****************************** */

void
forget()
{
  if (!fortot)
  {
    puts("Nothing to forget\r");
    return;
  }                                /* if (!fortot) */
  setptr(forpos);                  /* Pos'n to insert lines */

/* Should mever need to split this block (since the line before it was deleted)
 * so output a warning if we do */
  if (pointr->blocks == -1 && poffst)
  {
    puts("Unexpected call of splitb()!!\r");
    if (!splitb())
      return;                      /* If no memory for split */
  }                                /* if (pointr->blocks == -1 && poffst) */
  move_chain(bfrgt.next, bfrgt.prev, pointr);
  lintot += fortot;
  ptrpos += fortot;
  setptr(forpos);
  if (curmac < 0 || !BRIEF)
  printf("%ld line%s reinstated from line %ld\r\n", fortot,
    fortot == 1 ? "" : "s", forpos);
  forpos = fortot = 0;             /* Forgotten chain now empty */
  mods = true;                     /* File modified */
}

/* ******************************* setptr ****************************** */

/* Move the workfile pointer "pointr" so it points to the requested line */

void
setptr(long a1)
{
  long todo;                       /* Scratch */
  int back;                        /* 1 for move backwards */
/*
 * First - check request within limits.
 */
  if (a1 <= 0 || a1 > lintot + 1)  /* arg <= 0 or > line total + 1 (illegal) */
  {
    puts("!Pointer move request outside file - ignored [SETPTR]\r");
    return;
  }
/*
 * Determine whether to go back ,return at once, or go forward
 */
  if (a1 == ptrpos)                /* Already there */
    return;
  if (a1 < ptrpos)                 /* Pointer back */
  {
    if (a1 * 2 < ptrpos)           /* Move fwd from sof faster */
    {
      todo = a1;                   /* # to do */
      pointr = &bwork;             /* Addr 1st (imaginary) line */
      back = 0;                    /* Move forward */
    }                              /* if(a1*2<ptrpos) */
    else
    {
      todo = ptrpos - a1;          /* # to do, pointr ready */
      back = 1;                    /* Move back */
    }                              /* if(a1*2<ptrpos) else */
  }                                /* if(a1<ptrpos) */
  else                             /* Pointer forward */
  {
    if (a1 * 2 > ptrpos + lintot + 1) /* Back from eof is faster */
    {
      pointr = &bwork;             /* addr last (imaginary) line */
      todo = lintot + 1 - a1;      /* # to do */
      back = 1;
    }                              /* if(a1*2>ptrpos+lintot+1) */
    else
    {
      todo = a1 - ptrpos;          /* Get # to do, pointr right */
      back = 0;                    /* Move forward */
    }                              /* if(a1*2>ptrpos+lintot+1) else */
  }                                /* if(a1<ptrpos) else */
  if (back)
  {
    if (pointr->blocks < 0)        /* Mmapping group */
      todo -= poffst;              /* Go back from 1st line in group */
    for (; todo > 0;)
    {
      pointr = pointr->prev;       /* Previous line or group */
      if (pointr->blocks >= 0)
        todo--;
      else
        todo -= pointr->chars;     /* Went back by # lines in group */
    }                              /* for(;todo>0;) */
    if (todo < 0)
    {
      if (pointr->blocks < 0)
      {
        if (pointr->chars + todo <= 0)
        {
          printf("\r\nInternal error in setptr back: todo=%ld, chars=%hu\r\n",
            todo, pointr->chars);
          poffst = 0;              /* In range, anyway */
        }                          /* if(pointr->chars+todo<0) */
        else
          poffst = -todo;
      }                            /* if(pointr->blocks<0) */
      else
        printf("\r\nInternal error in setptr back: todo=%ld but not on an"
          " mmapping block\r\n", todo);
    }                              /* if(todo<0) */
    else
      poffst = 0;                  /* Whether mmapping or no */
  }                                /* if(back) */
  else
  {
    if (pointr->blocks < 0)        /* Mmapping group */
/* Go forward from last line in group */
      todo -= pointr->chars - 1 - poffst;
    for (; todo > 0;)
    {
      pointr = pointr->next;       /* Next line or group */
      if (pointr->blocks >= 0)
        todo--;
      else
        todo -= pointr->chars;     /* Went forward by # lines in group */
    }                              /* for(;todo>0;) */
    if (todo < 0)
    {
      if (pointr->blocks < 0)
      {
        if (pointr->chars + todo <= 0)
        {
          printf("\r\nInternal error in setptr fwd: todo=%ld, chars=%hu\r\n",
            todo, pointr->chars);
          poffst = 0;              /* In range, anyway */
        }                          /* if(pointr->chars+todo<0) */
        else
          poffst = pointr->chars - 1 + todo;
      }                            /* if(pointr->blocks<0) */
      else
        printf("\r\nInternal error in setptr back: todo=%ld but not on an"
          " mmapping block\r\n", todo);
    }                              /* if(todo<0) */
    else
      poffst = pointr->chars - 1;  /* Whether mmapping or no */
  }                                /* if(back) else */
  ptrpos = a1;                     /* Set new line # */
  return;                          /* Finished */
}

/* ******************************* setaux ****************************** */

void
setaux(long a1)
{

/* Set aux values = main values, set main pointer to aux request, exchange */

  auxpos = ptrpos;                 /* Keep main line # */
  auxptr = pointr;                 /* Keep main file pointer */
  aoffst = poffst;                 /* Keep main file pointer */
  setptr(a1);                      /* Position the pointer */
  auxxch();                        /* Switch ptrs; restore line # */
}

/* ******************************* finitl ****************************** */

void
finitl()
{
  pagebk *pg;                      /* Scratch */
  mapbk *mp;                       /* Scratch */
  int i, j;                        /* Scratch */
/*  */
  if (first)                       /* 1st time call */
    first = 0;
  else
/* Free all memory acquired last time, and unmap all files */

  {
    for (mp = (void *)bmap.next; mp != (void *)&bmap; mp = mp->next)
      munmap(mp->start, mp->length);
    for (pg = (void *)bpage.next; pg != (void *)&bpage;)
    {
/* How many in this blk */
      j = pg == (void *)bpage.prev ? lastpages : FULLPAGES;
      for (i = 0; i < j; i++)
        free(pg->pages[i]);
      pg = pg->next;
      free(pg->prev);              /* No need to unchain */
    }                              /* for(pg=bpage.next;pg!=(void*)&bpage;) */
  }                                /* if(first) else */

/* Empty all chains */

  bpage.next = bpage.prev = &bpage; /* Page database is empty */
  bmap.next = bmap.prev = &bmap;   /* Mapped file database is empty */
  bwork.next = bwork.prev = &bwork;
  bfrgt.next = bfrgt.prev = &bfrgt; /* Forgotten chain */
  bifree.next = bifree.prev = &bifree; /* Free index chain */
  bdfree.next = bdfree.prev = &bdfree; /* Free data chain */
  bofree.next = bofree.prev = &bofree; /* Free offsets chain */
/*
 * Initialise universals
 */
  lintot = 0;                      /* Empty file */
  forpos = 0;                      /* Invalidate */
  ptrpos = 1;                      /* Ready for appending */
  pointr = &bwork;                 /* Init w/f internal pointer */
  deferd = false;                  /* No deferred file */
}

/* ******************************* newmap ****************************** */

void
newmap(ino_t inode, off_t size, uint8_t *addr)
{
  mapbk *mp;                       /* Scratch */

/* Store details in a data block, since index blocks are too small for the mapbk
 * structure */

  if (!(mp = getmem(&bdfree, sizeof(databk), "map")))
    return;                        /* Hopefully won't happen often */
  mp->inode = inode;
  mp->start = addr;
  mp->length = size;
  qchain(mp, &bmap);

  clrfgt();      /* Empty forgotten chain here once (rather than in insmem()) */
  mods = true;                     /* File being modified */
  return;
}                         /* void newmap(ino_t inode,off_t size,uint8_t*addr) */

/* ******************************* insmem ****************************** */

/* Insert a reference to a memory-mapped line before the workfile pointer;
 * update # lines; if auxptr was same as pointr then make it point to new line.
 * insmem can be called at any time to do deferred reads, but in these cases:-
 * - we do not want to set "mods"
 * - we do not want to call "clrfgt"
 * - the pointer (aux or main) being used must be at eof, so there is no
 *   question of having to update the "other" pointer above it */

void
insmem(uint8_t *linptr, uint8_t *last)
{
  indxbk *ix;                      /* Scratch */
  suppbk *sp;                      /* Scratch */
  uint16_t *us;                    /* Scratch */
  int auxflg = 0;                  /* If aux pointer started same as main one */

/* See if aux & main pointers are the same */

  if (auxptr == pointr && (pointr->blocks != -1 || poffst == aoffst))
    auxflg = 1;


/* Lines can only be appended to the "current" mmap'd index (group). Since we
 * insert before the file pointer, pointr itself must be on an in-memory block
 * or at the start of an mmap'd one (poffset 0). If these conditions are not
 * met, split the mmap'd block addressed by pointr */

/* Step 1 - make sure pointr is on in-mem block or at a group start */

  if (pointr->blocks == -1 && poffst && !splitb())
    return;                        /* If no memory for split */

/* Step 2 - ensure block before pointer is mmap'd */

  if (pointr->prev->blocks != -1)
  {
    if (!(ix = get2(linptr)))
      return;                      /* No memory */
    qchain(ix, pointr);            /* Insert new empty group */
  }                                /* if(pointr->prev->blocks!=-1) */

/* Step 3 - ensure block before pointer can accept this line */

  sp = (void *)pointr->prev->dtaptr;
  if (linptr - sp->filptr > USHRT_MAX || pointr->prev->chars == USHRT_MAX ||
    !(!sp->endptr || sp->endptr == last) || sp->mode != fmode)
  {
    if (!(ix = get2(linptr)))
      return;                      /* No memory */
    qchain(ix, pointr);            /* Insert new empty group */
    sp = (void *)ix->dtaptr;
  }                                /* if(linptr-sp->data>USHRT_MAX||... */

/* Step 4 - put line reference in block before pointer */

  ix = pointr->prev;               /* Block being appended to */
  if (!sp->endptr)                 /* New block */
    sp->endptr = last;
  if (!(us = gtofst(ix, ix->chars))) /* No memory for extra offsets blk */
    return;
  *us = linptr - sp->filptr;       /* Store record offset */

/* Line address inserted. Update counts */

  lintot++;                        /* Update total lines */
  ptrpos++;                        /* Update line # */
  ix->chars++;                     /* Lines in group increases */

/* Make aux pointer address newly created line if it was the same as the main
 * pointer previously */

  if (auxflg)
  {
    auxptr = ix;
    aoffst = ix->chars - 1;        /* Line just appended */
  }                                /* if(auxflg) */

}                                  /* void insmem(scrbuf5*a1) */

/* ******************************* ismapd ****************************** */

bool
ismapd(ino_t inode)
{
  mapbk *mp;                       /* Scratch */
/*  */
  for (mp = (void *)bmap.next; mp != (void *)&bmap; mp = mp->next)
    if (mp->inode == inode)
      break;
  return mp != (void *)&bmap;
}                                  /* int ismapd(ino_t inode) */
/* ******************************* move_chain ******************************* */

static void
move_chain(void *firstblk, void *lastblk, void *beforeblk)
{
  ((hddr *)lastblk)->next->prev = ((hddr *)firstblk)->prev;
  ((hddr *)firstblk)->prev->next = ((hddr *)lastblk)->next;
  ((hddr *)firstblk)->prev = ((hddr *)beforeblk)->prev;
  ((hddr *)lastblk)->next = (hddr *)beforeblk;
  ((hddr *)beforeblk)->prev->next = (hddr *)firstblk;
  ((hddr *)beforeblk)->prev = (hddr *)lastblk;
}                                  /* move_chain() */
