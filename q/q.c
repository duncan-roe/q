/* Q
 *
 *
 * Copyright (C) 1981 D. C. Roe
 * Copyright (C) 2002,2007,2012-2018 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * This is the Master Q module
 *
 *
 */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <memory.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include "prototypes.h"
#include "edmast.h"
#include "macros.h"
#include "fmode.h"
#include "c1in.h"
#include "q_pipe.h"
#include "alu.h"
#include "isacharspecial.h"

/* Macros */

#define REREAD_CMD goto p1025
#define ERR1025(x) do {fprintf(stderr, "%s", (x)); REREAD_CMD;} while (0)
#define READ_NEXT_COMMAND goto p1004
#define ERRRTN(x) do {fprintf(stderr, "%s", (x)); return false;} while (0)
#define PIPE_NAME "qpipeXXXXXX"

/* Typedefs */

typedef enum q_yesno
{
  Q_YES,
  Q_NO,
  Q_MAYBE,
  Q_UNREC
} q_yesno;                         /* typedef enum q_yesno */

typedef enum command_state
{
  RUNNING,
  Q_ARG1,
  TRY_INITIAL_COMMAND,
/* Below here for line number states only (i.e. q <file>:<line number>) */
  LINE_NUMBER_BASE,
  LINE_NUMBER_SAVED,
  DO_V_NEXT,
  HAVE_LINE_NUMBER,
} command_state;

/* Externals that are not in any header */

long timlst;
unsigned char fxtabl[128];
scrbuf5 b1, b2, b3, b4;            /* 2 line & 2 command buffers */

/* Instantiate externals */

int tbstat;
bool offline = false;
bool piping = true;                /* Needs to start off true for quthan() */
int stack_size = 16;               /* Register stack initial depth */
long *rs = NULL;                   /* The register stack */
double *fs = NULL;                 /* The FP register stack */
long xreg = 0;                     /* Index Register */
int rsidx = -1;                    /* No current register yet */
int fsidx = -1;                    /* No current FP register yet */
bool index_next = false;
int effaddr;
bool alu_skip = false;
int num_ops = 0;
alu_dict_ent root_alu_dict_ent = { NULL, NULL, -2, 0 };
int *alu_table_index;
bool alu_macros_only = false;      /* N- was N-- */
unsigned long fmode;
unsigned long zmode;
bool zmode_valid = false;
char FPformat[40];
char DTformat[256];
long lintot = 0;
long ptrpos = 0;

/* Static Variables */

static long count = 0;             /* Returned by GETNUM seq */
static char *help_dir;
static char *help_cmd;
static bool nofile;
static int tmode;                  /* Mode of file */
static int towner;                 /* Owner of file */
static int tgroup;                 /* Group of file */
static int tmask = 0;              /* Current umask */
static char tmtree[PTHSIZ];        /* Name of HELP file */
static struct stat statbuf;
static char tmfile[PTHSIZ];        /* Name of .TM file */
static bool q_new_file;
static long wrtnum = 0;            /* # of lines to write */
static int rdwr = 0;               /* Mode for file opens */
static long savpos = 0;            /* Remembered pointer during S, B & Y */
static int saved_pipe_stdout;
static int pipe_temp_fd;
static char pipe_temp_name[sizeof PIPE_NAME] = { 0 };
static struct sigaction act;

/* Static functions */

/* ********************************* pushmac ******************************** */

static bool
pushmac(bool set_u_use)
{
  if (mcnxfr == MCLMIT)
    return false;
  mcstck[mcnxfr].mcprev = curmac;
  mcstck[mcnxfr].mcposn = mcposn;
  mcstck[mcnxfr].u_use = set_u_use;
  mcnxfr++;
  return true;
}                                  /* static bool pushmac(bool set_u_use) */
/* ********************************** eolok ********************************* */

/* Check no extra params */

static bool
eolok(void)
{
  scrdtk(1, (unsigned char *)NULL, 0, oldcom);
  if (oldcom->toktyp == eoltok)    /* OK */
    return true;
  printf("%s", "Too many arguments for this command");
  return false;
}                                  /* static bool eolok(void) */

/* ******************************* get_answer ******************************* */

/* Parse rest of line for yes / no indication */

static q_yesno
get_answer(void)
{
  if (scrdtk(1, (unsigned char *)buf, 6, oldcom))
  {
    fprintf(stderr, "%s. (scrdtk)", strerror(errno));
    return Q_UNREC;
  }                        /* if (scrdtk(1, (unsigned char *)buf, 6, oldcom)) */
  if (oldcom->toktyp == eoltok)
    return Q_MAYBE;
  if (oldcom->toktyp != nortok)    /* I.e. null token */
  {
    printf("%s", "Bad parameter for command");
    return Q_UNREC;
  }                                /* if (oldcom->toktyp != nortok) */
  if (!eolok())
    return Q_UNREC;
  switch (buf[0] & 0337)
  {
    case 'O':
      if ((buf[1] & 0337) == 'N')
        return Q_YES;
      if ((buf[1] & 0337) == 'F')
        return Q_NO;
      break;
    case 'Y':
    case 'T':
      return Q_YES;
    case 'F':
    case 'N':
      return Q_NO;
  }
  printf("%s", "Parameter not recognised");
  return Q_UNREC;
}                                  /* get_answer(void) */

/* ****************************** get_file_arg ****************************** */
static bool
get_file_arg(void)
{
  if (scrdtk(2, (unsigned char *)buf, PTHMAX, oldcom)) /* Read a f/n */
  {
    perror("scrdtk");
    putchar('\r');
    return false;
  }
  nofile = oldcom->toktyp == eoltok;
  if (!nofile && oldcom->toktyp != nortok)
    return false;
  tildexpn(buf);                   /* Do tilde expansion */
  return true;
}                                  /* get_file_arg() */

/* &************************** get_opt_lines2count ************************** */

static bool
get_opt_lines2count(void)
{
  if (getnum(0))                   /* Format of optional # of lines OK */
  {
    count = oldcom->decval;
    if (oldcom->toktyp == eoltok || eolok()) /* EOL already or next */
      return true;
  }                                /* if(getnum(0)) */
  return false;
}                                  /* get_opt_lines2count() */

/* ***************************** do_stat_symlink **************************** */

static bool
do_stat_symlink(void)
{
  int i;
  unsigned char *p;

/* For S B & Q, if the file exists then use its mode from now on. Don't complain
 * here if it doesn't exist. To check whether the file is a symlink, we need to
 * call readlink to find its real name. The only real error here is a symlink
 * loop */

  errno = 0;                       /* Ensure valid */
  if (!stat(buf, &statbuf))
  {
    tmode = statbuf.st_mode;
    tgroup = statbuf.st_gid;
    towner = statbuf.st_uid;
  }                                /* if (!stat(buf, &statbuf)) */
  else
  {
    if (!tmask)
    {
      tmask = umask(0);            /* Get current umask */
      umask(tmask);                /* Reinstate umask */
    }                              /* if (!tmask) */
    tmode = ~tmask & 0666;         /* Assume no execute on new file */
    tgroup = towner = 0;
  }                                /* if (!stat(buf, &statbuf)) else */
  if (!lstat(buf, &statbuf) && S_ISLNK(statbuf.st_mode) && errno != ELOOP)
    for (;;)
    {
      if (0 < (i = readlink(buf, tmfile, (size_t)PTHSIZ)))
      {                            /* S, B or Q on a symlink */
        tmfile[i] = 0;             /* No trlg NUL from readlink */
        if (tmfile[0] == '/' || tmfile[0] == '~')
          strcpy(buf, tmfile);
        else
        {
          p = (unsigned char *)strrchr(buf, '/'); /* Find last '/' if any */
          if (!p)
            p = (unsigned char *)buf - 1; /* Filename at buf start */
          *(p + 1) = '\0';         /* Throw away filename */
          strcat(buf, tmfile);     /* Append linked name */
        }                          /* if(tmfile[0]=='/'||tmfile[0]=='~') else */
        printf("Symbolic link resolves to %s", buf);
        newlin();
/* See if symlink points to another symlink... */
        if (lstat(buf, &statbuf))
          break;                   /* B link to a new file */
        if (!S_ISLNK(statbuf.st_mode))
          break;                   /* B now not on a symlink */
      }                            /* if(0<(i=readlink(buf,tmfile,... */
      else
      {
        fprintf(stderr, "%s. %s (readlink)", strerror(errno), buf);
        return false;              /* Bad readlink */
      }                            /* if(0<(i=readlink(buf,tmfile,... else */
    }                              /* if(!lstat(buf,&statbuf)&&S_ISLNK(... */
  if (!(errno || S_ISREG(statbuf.st_mode)))
  {
    fprintf(stderr, "Not a regular file. %s", buf);
    return false;
  }                                /* if(!S_ISREG(statbuf.st_mode)) */
  return true;
}                                  /* do_stat_symlink() */

/* ******************************** open_buf ******************************** */

static int
open_buf(int flags, mode_t mode)
{
  int retcod;

  do
    retcod = open(buf, flags, mode);
  while (retcod == -1 && errno == EINTR);
  return retcod;
}                                  /* open_buf() */

/* ******************************** my_close ******************************** */

static int
my_close(int fd)
{
  int retcod;

  do
    retcod = close(fd);
  while (retcod == -1 && errno == EINTR);
  return retcod;
}                                  /* my_close() */

/* *************************** s_b_w_common_write *************************** */

static bool
s_b_w_common_write(void)
{
  if ((funit = open_buf(rdwr, tmode)) == -1)
  {
    fprintf(stderr, "%s. %s (open)", strerror(errno), buf);
    return false;                  /* Bad open */
  }                             /* if ((funit = open_buf(rdwr, tmode)) == -1) */
  if (fstat(funit, &statbuf))
    fprintf(stderr, "%s. funit %d (fstat)", strerror(errno), funit);
  else if (ismapd(statbuf.st_ino))
    fprintf(stderr, "%s is mmap'd", buf);
  else if (ftruncate(funit, 0))
    fprintf(stderr, "%s. funit %d (ftruncate)", strerror(errno), funit);
  else
    writfl(wrtnum);                /* Write lines to o/p file */
  if (fscode != 0)                 /* Some kind of failure above */
    my_close(funit);
  return fscode == 0;
}                                  /* s_b_w_common_write() */

/* ****************************** do_b_or_s ********************************* */
static bool
do_b_or_s(bool is_b)
{
  bool bspar;                      /* BACKUP/SAVE had a param */

  savpos = ptrpos;                 /* So we can leave pos'n same at end */
  setptr((long)1);                 /* Pos'n 1st line */
  if (deferd)
    dfread(LONG_MAX, NULL);        /* Ensure all file in */
  wrtnum = lintot;                 /* Write all lines */
  rdwr = O_WRONLY + O_CREAT;       /* Don't truncate yet in case mmap'd */
  if (!get_file_arg())
    ERRRTN("Error in filename");
  if (nofile)                      /* B or S no filename arg */
  {
    if (!pcnta[0])                 /* We have no default f/n */
      ERRRTN("filename must be specified");
    bspar = false;                 /* Don't have a param */
    strcpy(buf, pcnta);      /* Duplicate f/n in buf so B-BACKUP can merge in */
  bkup_with_fn_arg:               /*  B-Backup with a filename arg joins here */
/*
 * Use TMFILE for name of backup file
 */
    if (snprintf(tmfile, sizeof tmfile, "%s.%s", buf,
      is_b ? "bu" : "tm") >= sizeof tmfile)
      ERRRTN("Filename too long");

/* ---------------------------------------------------- */
/* We used to rely on link failing if the file existed. */
/* But then, link() always fails in a DOS file system.  */
/* So now we stat() the file to see if it exists,       */
/* then use rename() if it doesn't.                     */
/* (rename() would overwrite an old file)               */
/* ---------------------------------------------------- */

    while (!stat(tmfile, &statbuf))
    {
      if (is_b && !ysno5a("Do you want to delete the old backup file", A5NDEF))
        ERRRTN("Need another filename to take backup");
      if (unlink(tmfile))
      {
        fprintf(stderr, "%s. %s (unlink)", strerror(errno), tmfile);
        my_close(funit);
        return false;
      }                            /* if(unlink(tmfile)) */
      if (is_b)
        printf("Previous backup file deleted:- ");
    }                              /* if(!stat(tmfile,&statbuf)) */
    if (rename(buf, tmfile))       /* If rename fails */
    {
/* OK nofile if B with param */
      if (bspar && errno == ENOENT)
      {
        puts("New file - no backup taken\r");
        goto new_bkup_file;
      }                            /* if(bspar&&(errno==ENOENT)) */
      fprintf(stderr, "%s. %s to %s (rename)", strerror(errno), buf, tmfile);
      my_close(funit);             /* In case anything left open */
      return false;                /* Get corrected command */
    }                              /* if (rename(buf, tmfile)) */
/*
 * File renamed - now open new file of same type as original
 */
    rdwr = O_WRONLY + O_CREAT + O_EXCL; /* Must be new file */
    if (is_b)
      printf("Backup file is %s\r\n", tmfile); /* Report backup f/n */
  new_bkup_file:
    if (!s_b_w_common_write())
      return false;
  }
  else
  {
    if (!do_stat_symlink() || !eolok())
      return false;
    bspar = true;                  /* B or S has a parameter */
/*
 * If B-BACKUP, back up supplied param anyway
 */
    if (is_b)
      goto bkup_with_fn_arg;       /* Join S&B with no params */
    if (!s_b_w_common_write())
      return false;
  }
  setptr(savpos);                  /* Repos'n file as before */
  if (bspar)
    strcpy(pcnta, buf);            /* We had a param. Set as dflt */
  else if (!is_b)
  {
    printf("%s\r\n", pcnta);       /* Remind user what file he's editing */
    if (unlink(tmfile) == -1)
      fprintf(stderr, "%s. %s (unlink)\r\n", strerror(errno), tmfile);
  }

/* ---------------------------------------------------------------------- */
/* Attempt to restore as many attributes of the original file as possible */
/* current file attributes are in statbuf                                 */
/* ---------------------------------------------------------------------- */

  if (towner)                      /* If there *was* an original file */
  {
    fscode = 0;
    if (tgroup != statbuf.st_gid)
    {
      if (chown(pcnta, -1, tgroup) == -1)
      {
        fscode = 1;
        fprintf(stderr, "Warning - original group not restored\r\n");
      }                            /* if (chown(pcnta, -1, tgroup) == -1) */
    }                              /* if (tgroup != statbuf.st_gid) */
    if (towner != statbuf.st_uid)
    {
/* Don't try to change user if group failed, but do warn */
      if (fscode || chown(pcnta, towner, -1) == -1)
      {
        fscode = 1;
        fprintf(stderr, "Warning - original owner not restored\r\n");
      }                      /* if (fscode || chown(pcnta, towner, -1) == -1) */
    }                              /* if (towner != statbuf.st_gid) */
/* If there were no problems above, set any extra original mode bits */
    if (tmode != statbuf.st_mode && (fscode || chmod(pcnta, tmode) == -1))
      printf("Warning - original mode not restored\r\n");
  }                                /* if (towner) */
  mods = false;                    /* S or B succeeded */
  return true;
}                                  /* do_b_or_s() */

/* ****************************** rm_pipe_temp ****************************** */

static void
rm_pipe_temp(void)
{
  int retcod;

  if (*pipe_temp_name)
  {
    retcod = unlink(pipe_temp_name);
    if (retcod == -1 && errno != ENOENT)
      fprintf(stderr, "%s. %s (unlink)\r\n", strerror(errno), pipe_temp_name);
  }                                /* if (pipe_temp_name && *pipe_temp_name) */
}                                  /* rm_pipe_temp() */

/* ************************ write_workfile_to_stdout ************************ */

static void
write_workfile_to_stdout(void)
{
/*
 * Because this is potentially lengthy, re -enable signals
 */
  sigset_t omask = act.sa_mask;    /* Will have wanted bits 1st time thru */

/* Before changing signal handlers ,delete the temp file */
/* (it's mmpap'd) */
  rm_pipe_temp();

/* Reset TERM & INT to default actions */
  act.sa_flags = 0;
  act.sa_handler = SIG_DFL;
  sigemptyset(&act.sa_mask);
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGTERM, &act, NULL);

/* Enable signals that will be blocked if we came here via a handler */
  sigprocmask(SIG_UNBLOCK, &omask, NULL);
/*
 * Do a subset of a regular save, to the original stdout
 */
  setptr((long)1);                 /* Pos'n 1st line */
  if (deferd)
    dfread(LONG_MAX, NULL);        /* Ensure all file in */
  funit = saved_pipe_stdout;
  writfl(lintot);
}                                  /* write_workfile_to_stdout() */

/* ***************************** dev_null_stdout **************************** */

static void
dev_null_stdout(void)
{
  int retcod;

  if (my_close(STDOUT5FD))
  {
    fprintf(stderr, "%s. fd %d (close)\n", strerror(errno), STDOUT5FD);
    exit(1);
  }                                /* if (my_close (STDOUT5FD)) */
  do
    retcod = open("/dev/null", O_WRONLY);
  while (retcod == -1 && errno == EINTR);
  if (retcod == -1)
  {
    fprintf(stderr, "%s. /dev/null (open)\n", strerror(errno));
    exit(1);
  }                                /* if (retcod == -1) */
  if (retcod != STDOUT5FD)
  {
    fprintf(stderr, "/dev/null was opened on fd %d when it needed to be"
      " opened on fd%d\n", retcod, STDOUT5FD);
    exit(1);
  }                                /* if (retcod != STDOUT5FD) */
}                                  /* dev_null_stdout() */

/* ******************************** make_node ******************************* */

static alu_dict_ent *
make_node(int initial_fn_idx)
{
  alu_dict_ent *result;

  result = malloc(sizeof *result);
  if (!result)
  {
    fprintf(stderr, "%s. (malloc)\n", strerror(errno));
    exit(1);
  }                                /* if (!result) */
  memset(result, 0, sizeof *result);
  result->fn_idx = initial_fn_idx;
  return result;
}                                  /* make_node() */

/* ********************************* add_op ********************************* */

static void
add_op(alu_dict_ent * root, char *opcode)
{
  static int next_fn_idx = 0;
  alu_dict_ent *ptr, *old_alt;
  char thisch = toupper(*(unsigned char *)opcode);

/* Return condition for the recursive function */
  if (!thisch)
  {
    if (root->fn_idx != -1)
    {
      fprintf(stderr, "fn_idx (=%d) already claimed by another opcode!\r\n",
        root->fn_idx);
      exit(1);
    }                              /* if (root->fn_idx != -1) */
    root->fn_idx = next_fn_idx++;
    return;
  }                                /* if (!thisch) */

  ptr = root;
  while (ptr->letter != thisch)
  {
    if (ptr->alt)
    {
      if (thisch < ptr->alt->letter)
      {
        old_alt = ptr->alt;
        ptr->alt = make_node(-3);
        ptr = ptr->alt;
        ptr->alt = old_alt;
        ptr->letter = thisch;
      }                            /* if (thisch < ptr->alt->letter) */
      else
        ptr = ptr->alt;
    }                              /* if (ptr->alt) */
    else
    {
      ptr->alt = make_node(-3);
      ptr = ptr->alt;
      ptr->letter = thisch;
    }                              /* if (ptr->alt) else */
  }                                /* while (ptr->letter != thisch) */
  if (!ptr->next)
    ptr->next = make_node(-1);

/* Make the recursive call */
  add_op(ptr->next, opcode + 1);
  return;
}                                  /* add_op() */

/* ******************************** init_alu ******************************** */

static void
init_alu(void)
{
  int i, j;

/* Allocate the register stack */
  rs = malloc(stack_size * sizeof *rs);
  if (!rs)
  {
    fprintf(stderr, "%s. (malloc)\n", strerror(errno));
    exit(1);
  }                                /* if (!rs) */
  fs = malloc(stack_size * sizeof *fs);
  if (!fs)
  {
    fprintf(stderr, "%s. (malloc)\n", strerror(errno));
    exit(1);
  }                                /* if (!fs) */

/* Count how many opcodes there are */
  for (i = num_alu_opcode_table_entries - 1; i >= 0; i--)
    if (opcode_defs[i].func)
      num_ops++;

/* Allocate the array of function pointers for the run machine */
  alu_table_index = malloc(sizeof *alu_table_index * num_ops);
  if (!alu_table_index)
  {
    fprintf(stderr, "%s. (malloc)\n", strerror(errno));
    exit(1);
  }                                /* if (!alu_table_index) */

/* Set up initial floating point format */
  strcpy(FPformat, "%g");

/* Set up initial date format */
  strcpy(DTformat, "%F_%T");

/* Set up indicies and lookup dictionary */
  for (i = 0, j = 0; i < num_alu_opcode_table_entries; i++)
    if (opcode_defs[i].func)
    {
      alu_table_index[j++] = i;
      add_op(&root_alu_dict_ent, opcode_defs[i].name);
    }                              /* if (opcode_defs[i].func) */
}                                  /* init_alu() */

/* ***************************** display_opcodes **************************** */
static void
display_opcodes(void)
{
  int i;
  char tbuf[16];
  char *p;

  printf("\r\n"
    "\t Instructions to Access Tabs\r\n"
    "\t ============ == ====== ====\r\n"
    "\t (x is a tab ID; type of tab is not examined)\r\n"
    "PSHTAB x Push value of tab x to R\r\n"
    "POPTAB x Pop R to set value of tab x;\r\n"
    "                  set tab type to file or cursor pos'n"
    " as set by SCPT / SFPT\r\n"
    "\r\n"
    "\t Memory Reference Instructions\r\n"
    "\t ====== ========= ============\r\n"
    "PSH  xxx  Push contents of N7xxx to R\r\n"
    "POP  xxx  Pop R to define N7xxx\r\n"
    "PSHF xxx  Push contents of N13xxx to F\r\n"
    "POPF xxx  Pop F to define N13xxx\r\n");

  for (i = 0; i < num_alu_opcode_table_entries; i++)
  {
    if (opcode_defs[i].func)
    {
      strcpy(tbuf, opcode_defs[i].name);
      for (p = tbuf; *p; p++)
        *p = toupper(*(unsigned char *)p);
      printf("%s\t %s\r\n", tbuf, opcode_defs[i].description);
    }                              /* if (opcode_defs[i].func) */
    else
      printf("\t %s\r\n", opcode_defs[i].description);
  }                     /* for (i = 0; i < num_alu_opcode_table_entries; i++) */
}                                  /* display_opcodes() */

/* ********************************** main ********************************** */
int
main(int xargc, char **xargv)
{
  long timnow;
  struct tms tloc;
  char oldkey[3];                  /* 1st param to FX command */
  char xkey[2], newkey[3];         /* 2nd param to FX command */
  char *r;                         /* Scratch */
  char ndel[33];                   /* Table of non-delimiters FL & FY */
  int newl = 0;                    /* Label to go if Nl (MOD, INS, AP) */
  int numok;                     /* Label to go if # of lines OK & last param */
  int rtn = 0;                     /* Return from INS/MOD/APPE common code */
  int i, j, k = 0, l, m, n, dummy; /* Scratch - I most so */
  int nonum;                       /* Label if no # of lines */
  int oldlen = 0;                  /* Length of OLDSTR */
  int newlen = 0;                  /* Length of NEWSTR */
  int ydiff = 0;                   /* OLDLEN-NEWLEN */
  int yposn;                       /* How far we are along a line (Y) */
  int locpos;                   /* Position in line of string found by LOCATE */
  int minlen = 0;                  /* Min line length (L&Y) */
  int firstpos = 0;                /* First pos to search (L&Y) */
  int lastpos = 0;                 /* Last pos to search (L&Y) */ ;
  bool revrse;                     /* Locate backwards */
  int savtok;                      /* Saved token type */
  int colonline;                   /* Line number from <file>:<line> */
/* */
/* For those commands that take 2 #'s of lines */
  long count2 = 0;
  long i4, j4 = 0, k4 = 0;         /* Scratch */
  long revpos;                     /* Remembered pointer during backwards L */
  long xcount = 0;                 /* For V-View */
/* */
  char oldstr[Q_BUFSIZ], newstr[Q_BUFSIZ]; /* YCHANGEALL. !!AMENDED USAGE!! */
  unsigned char *p, *q;            /* Scratch */
  char *colonpos;                  /* Pos'n of ":" in q filename */
/* */
  bool splt;                       /* Last line ended ^T (for MODIFY) */
  bool logtmp = false, lgtmp2 = false, lgtmp3 = false, lgtmp4; /* Scratch */
  bool repos = false;              /* We are R-REPOSITION (not C-COPY) */
  bool linmod;                     /* This line modified (Y) */
  bool tokens = false;             /* Token search yes/no */
  bool do_rc = true;
  bool errflg = false;             /* Illegal switch seen */
  bool vrsflg = false;             /* -V seen */
  bool aluflg = false;             /* -A seen */
  bool quiet_flag = false;         /* -q seen */
  bool verbose_flag = false;       /* -v seen */
  command_state cmd_state = TRY_INITIAL_COMMAND;
  bool fullv = false;              /* Fulll VIEW wanted */
  q_yesno answer;
  char *initial_command = NULL;
  bool P, Q;                      /* For determining whether we are in a pipe */
/*
 * Initial Tasks
 */
  argc = xargc;                    /* Xfer invocation arg to common */
  argv = xargv;                    /* Xfer invocation arg to common */
  dfltmode = 01212005;             /* +e +m +* +tr +dr +i +a */
  end_seq = normal_end_sequence;
  init_alu();
/* Pick up any option arguments and set cmd_state if more args follow */
  while ((i = getopt(argc, argv, "AVbdei:mnoqtv")) != -1)
    switch (i)
    {
      case 'A':
        aluflg = true;
        break;

      case 'V':
        vrsflg = true;
        break;

      case 'b':
        binary = true;
        dfltmode |= 0400;          /* +f */
        break;

      case 'd':
        dfltmode ^= 1;             /* dr */
        break;

      case 'e':
        dfltmode ^= 010000;        /* e */
        break;

      case 'i':
        initial_command = optarg;
        break;

      case 'm':
        dfltmode ^= 02000;         /* m */
        break;

      case 'n':
        do_rc = false;
        break;

      case 'o':
        offline = true;
        break;

      case 'q':
        quiet_flag = true;
        break;

      case 't':
        dfltmode ^= 4;             /* tr */
        break;

      case 'v':
        verbose_flag = true;
        break;

      case '?':
        errflg = true;
        break;
    }                              /* switch(i) */
  if (errflg)
  {
    fprintf(stderr, "%s",
      "Usage: q [-AVbdemnto] [-i <macro definition>] [+<n> file]"
      " [file[:<n>]]...\n");
    return 1;
  }
  if (aluflg)
  {
    display_opcodes();
    if (argc == 2)
      return 0;
  }                                /* if (aluflg) */
  if (vrsflg)
  {
    q_version();
    if (argc == 2)
      return 0;
  }                                /* if (vrsflg) */
  if (offline && initial_command == NULL)
  {
    fprintf(stderr, "%s", "Can only use -o with -i\n");
    return 1;
  }                                /* if (offline && initial_command == NULL) */
  if (verbose_flag && quiet_flag)
  {
    fprintf(stderr, "%s\n", "-q[uiet] and -v[erbose] are mutually exclusive");
    return 1;
  }                                /* if (verbose_flag && quiet_flag) */
  if (!(sh = getenv("SHELL")))
    sh = "/bin/sh";
  if (!(help_dir = getenv("Q_HELP_DIR")))
    help_dir = "/usr/local/lib/q";
  if (!(macro_dir = getenv("Q_MACRO_DIR")))
    macro_dir = help_dir;
  if (!(help_cmd = getenv("Q_HELP_CMD")) && !(help_cmd = getenv("PAGER")))
    help_cmd = "more";
  fmode = dfltmode;                /* Assert defaults */
  if (optind < argc)
    cmd_state = Q_ARG1;
  else
    argno = -1;                    /* No filenames */
  memset(&act, 0, sizeof act);
  act.sa_sigaction = quthan;
  act.sa_flags = SA_SIGINFO;
  sigemptyset(&act.sa_mask);
  sigaddset(&act.sa_mask, SIGINT);
  sigaddset(&act.sa_mask, SIGTERM);
#ifdef SIGWINCH
  sigaddset(&act.sa_mask, SIGWINCH);
#endif
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGTERM, &act, NULL);
#ifdef SIGWINCH
  sigaction(SIGWINCH, &act, NULL);
#endif

/* Check for running in a pipe (or with redirection) */
  P = isatty(STDIN5FD);
  Q = isatty(STDOUT5FD);
  if (!P && !Q)
  {
    if (initial_command == NULL)
    {
      fprintf(stderr, "%s\n",
        "You must supply an initial command to run Q in a pipe");
      return 1;
    }                              /* if (initial_command == NULL) */

/* If either stdin or stdout is a character special (typically /dev/null),
 * we are not in fact in a pipe.*/
    if (isacharspecial(STDIN5FD) || isacharspecial(STDOUT5FD))
      goto not_pipe;
    if (argno != -1)
    {
      fprintf(stderr, "%s\n",
        "You may not give Q a file name when running in a pipe");
      return 1;
    }                              /* if (argno != -1) */
/*
 * Deal with stdout
 */
    do
      saved_pipe_stdout = dup(STDOUT5FD);
    while (saved_pipe_stdout == -1 && errno == EINTR);
    if (saved_pipe_stdout == -1)
    {
      fprintf(stderr, "%s. fd %d (dup)\n", strerror(errno), STDOUT5FD);
      return 1;
    }                              /* if (saved_pipe_stdout == -1) */

/* If verbose, dup stderr to stdout. Otherwise, stdout is /dev/null */
    if (verbose_flag)
    {
      do
        i = dup2(STDERR5FD, STDOUT5FD);
      while (i == -1 && errno == EINTR);
      if (i == -1)
      {
        fprintf(stderr, "%s. fd %d to fd %d (dup2)\n", strerror(errno),
          STDERR5FD, STDOUT5FD);
        return 1;
      }                            /* if (i == -1) */
    }                              /* if (verbose_flag) */
    else
      dev_null_stdout();
/*
 * Deal with stdin
 */

/* Create & open a temporary file to buffer the entire pipe */
    strcpy(pipe_temp_name, PIPE_NAME);
    atexit(rm_pipe_temp);
    pipe_temp_fd = mkstemp(pipe_temp_name);
    if (pipe_temp_fd == -1)
    {
      fprintf(stderr, "%s. %s (open)\n", strerror(errno), pipe_temp_name);
      return 1;
    }                              /* if (pipe_temp_fd == -1) */

/* Populate the temporary file */
    while (true)
    {
      ssize_t nc, todo;
      char *write_from;

      do
        todo = read(STDIN5FD, buf, sizeof buf);
      while (todo == -1 && errno == EINTR);
      if (todo == -1)
      {
        fprintf(stderr, "%s. stdin (read)", strerror(errno));
        return 1;
      }                            /* if (todo == -1) */
      if (!todo)
        break;                     /* Reached EOF */
      write_from = buf;
      while (todo)
      {
        do
          nc = write(pipe_temp_fd, write_from, todo);
        while (nc == -1 && errno == EINTR);
        if (nc == -1)
        {
          fprintf(stderr, "%s. %s (write)\n", strerror(errno), pipe_temp_name);
          return 1;
        }                          /* if (nc == -1) */
        if (nc == 0)               /* Is this possible? */
        {
          fprintf(stderr, "Zero characters written. %s (write)\n",
            pipe_temp_name);
          return 1;
        }                          /* if (nc == 0) */
        write_from += nc;
        todo -= nc;
      }                            /* while(todo) */
    }                              /* while (true) */
    my_close(pipe_temp_fd);
    cmd_state = Q_ARG1;

/* Never ask for input from stdin */
    offline = true;
  }                           /* if (!isatty(STDIN5FD) && !isatty(STDOUT5FD)) */
  else
/*
 * Not in a pipe
 */
  {
  not_pipe:
    if (!(P && Q) && !offline)
    {
      fprintf(stderr, "%s\n",
        "stdin & stdout must either both be a tty or both not");
      return 1;
    }                        /* if (!(isatty(STDIN5FD) && isatty(STDOUT5FD))) */

/* Implement quiet operation if requested */
    if (quiet_flag)
    {
      if (!offline)
      {
        fprintf(stderr, "%s\n", "-q[uiet] is only available with -o[ffline]");
        return 1;
      }                            /* if (!offline) */
      dev_null_stdout();
    }                              /* if (quiet_flag) */

/* Ctrl-C just sets a flag (rather than exitting) */
    piping = false;
  }                      /* if (!isatty(STDIN5FD) && !isatty(STDOUT5FD)) else */
/*
 * Common initialisation
 */
  cntrlc = false;                  /* Not yet seen ^C */
  ndel[0] = '\0';                  /* No FT commands yet */
  for (i = 127; i >= 0; i--)
    fxtabl[i] = i;                 /* No FX commands yet */
  oldcom = &b1;
  newcom = &b2;
  curr = &b3;
  prev = &b4;
  init5();                         /* Set half duplex &c */
/*
 * Set up Screenedit buffers
 */
  oldcom->bcurs = 0;
  oldcom->bchars = 0;              /* Initialise OLDCOM only this once */
  prev->bchars = 0;
  prev->bcurs = 0;                 /* Initialise PREV only this once */
  oldcom->bmxch = BUFMAX;
  newcom->bmxch = BUFMAX;
  curr->bmxch = BUFMAX;
  prev->bmxch = BUFMAX;
/* */
  finitl();                        /* Initialise workfile system */
  sinitl();                        /* Initialise screen system */
  newlin();                        /* Screen displaying blanks */
  splt = false;                    /* Not splitting a line initially */
  mods = false;                    /* No mods to i/p file yet */
  pcnta[0] = 0;                    /* No default filename yet */
  locpos = 0;                      /* Stays at 1 except after a LOCATE */
  locerr = false;                  /* 1st error might not be LOCATE */
  noRereadIfMacro = false;
  forych = false;                  /* Start off VERBOSE */

/* Invite to type H, but only if taking input */
  if (!offline)
    puts("Type H for help\r");

  if (size5)
    printf("Noted screen dimensions %u x %u\r\n", col5, row5);
  orig_stdout = -1;                /* Haven't dup'd stdout */
/*
 * Initially INDENT switched OFF
 */
  ndntch = 0;                      /* For when it's first switched on */
  tbstat = -1;                     /* Xlation table not set up */
  stdidx = -1;                     /* No U-use file */
/*
 * Use .qrc if it exists here or in $HOME
 */
  if (do_rc)
  {
/* Forge a u-use: push current stdin */
    stdidx = 0;
    do
      stdinfo[stdidx].funit = dup(0);
    while (stdinfo[stdidx].funit == -1 && errno == EINTR);
    if (stdinfo[stdidx].funit == -1)
    {
      fprintf(stderr, "\r\n%s. (dup(0))\r\n", strerror(errno));
      refrsh(NULL);
      stdidx--;
      READ_NEXT_COMMAND;           /* Don't try to open .qrc */
    }                              /* if (stdinfo[stdidx].funit == -1) */
    my_close(0);

/* Try for .qrc or ~/.qrc */
    logtmp = true;                 /* retry on failure */
    strcpy(buf, ".qrc");
  retry_qrc:
    do
      i = open_buf(O_RDONLY, 0);
    while (i == -1 && errno == EINTR);
    if (i == -1)
    {
      if (logtmp)
      {
        logtmp = false;
        strcpy(buf, "~/.qrc");
        tildexpn(buf);
        goto retry_qrc;
      }                            /* if (logtmp) */
      pop_stdin();
    }                              /* if (i == -1) */
    else
    {
      if (i)
      {
        do
          j = dup2(i, 0);
        while (j == -1 && errno == EINTR);
        if (j == -1)
        {
          fprintf(stderr, "\r\n%s. (dup2(%d, 0))\r\n", strerror(errno), i);
          fprintf(stderr, "Serious problem - new stdin opened on funit %d\r\n",
            i);
          refrsh(NULL);
          pop_stdin();
          READ_NEXT_COMMAND;
        }                          /* if (j == -1) */
        else
          my_close(i);
      }                            /* if (i) */
      duplx5(true);                /* Assert XOFF recognition */
      printf("> u %s\r\n", buf);   /* Simulate a command */
    }                              /* if (i == -1) else */
  }                                /* if (do_rc) */
/*
 * Main command reading loop
 */
p1004:
  if ((!USING_FILE && curmac < 0) || cmd_state > LINE_NUMBER_BASE)
  {
    switch (cmd_state)
    {
      case TRY_INITIAL_COMMAND:
        cmd_state = RUNNING;
        if (initial_command != NULL)
        {
          oldcom->bchars = snprintf((char *)oldcom->bdata, sizeof oldcom->bdata,
            "fi %s", initial_command);
          initial_command = NULL;
          verb = 'i';
          oldcom->bcurs = 3;
          break;
        }                          /* if (initial_command != NULL) */
/* Drop through */
      case RUNNING:
        goto read_command_normally;

      case Q_ARG1:

/* Q-quit into the first file on the command line. Pagers (e.g. less) may
 * precede this with +<line#> so deal with this too. Cause command to be
 * actioned & displayed by setting up oldcom, also set up "verb" since sccmd is
 * not being called */

        if (piping)
          cmd_state = TRY_INITIAL_COMMAND;
        else
/* assume if the first arg starts "+" and there is at least 1 more arg then the
 * first arg is a line number */
        {
          if (**(argv + optind) == '+' && strlen(*(argv + optind)) > 1 &&
            argc - optind >= 2)
          {
            optind++;              /* "hide" +# arg */
            cmd_state = LINE_NUMBER_SAVED; /* Have a +# arg */
          }                        /* if(**(argv+optind)=='+'&&... */
          else
            cmd_state = TRY_INITIAL_COMMAND;
        }                          /* if (!piping) */

/* Open the file whether line number supplied or not */
        oldcom->bchars =
          snprintf((char *)oldcom->bdata, sizeof oldcom->bdata, "q %s",
          piping ? pipe_temp_name : *(argv + optind));
        verb = 'Q';
        oldcom->bcurs = 2;
        if (piping)
          atexit(write_workfile_to_stdout);
        break;

/* The line number states are used on "q file:line",
 * either initially or any time subsequently.
 * They are also used on an initial "q +line file". */

      case LINE_NUMBER_BASE:
        fprintf(stderr, "Illegal command state in %s:%d\r\n", __FILE__,
          __LINE__);
        exit(1);

      case LINE_NUMBER_SAVED:
        oldcom->bchars =
          snprintf((char *)oldcom->bdata, sizeof oldcom->bdata, "g %s",
          *(argv + optind - 1) + 1);
        goto setup_goto_cmd;

      case HAVE_LINE_NUMBER:
        oldcom->bchars =
          snprintf((char *)oldcom->bdata, sizeof oldcom->bdata, "g %d",
          colonline);
      setup_goto_cmd:
        cmd_state = DO_V_NEXT;
        verb = 'G';
        oldcom->bcurs = 2;
        break;

      case DO_V_NEXT:
        cmd_state = TRY_INITIAL_COMMAND;
        oldcom->bchars = 1;
        oldcom->bdata[0] = 'v';
        oldcom->bdata[1] = 0;
        verb = 'V';
        oldcom->bcurs = 1;
        break;
    }                              /* switch(cmd_state) */
    printf("> %s\r\n", oldcom->bdata);
  }                               /* if ((!USING_FILE && curmac < 0) || ... ) */
  else
  read_command_normally:
    sccmnd();                      /* Read a command; set VERB */
p1201:
  if (cntrlc)                      /* There has been a ^C or BRK */
  {
    cntrlc = false;                /* Reset flag */
    if (USING_FILE || curmac >= 0) /* If in macro, force an error */
    {
      (void)write(1, "Keyboard interrupt", 18);
      REREAD_CMD;
    }
  }                                /* Else ignore the quit */
  revrse = false;                  /* Guarantee validity if true */
  switch (verb)
  {
    case 'A':
      goto p1005;
    case 'B':
      if (do_b_or_s(true))
        READ_NEXT_COMMAND;
      REREAD_CMD;
    case 'C':
      goto p1007;
    case 'D':
      goto p1008;
    case 'E':
      goto p1009;
    case 'G':
      goto p1010;
    case 'H':
      goto p1011;
    case 'I':
      goto p1012;
    case 'J':
      goto p1013;
    case 'L':
      goto p1014;
    case 'M':
      goto p1015;
    case 'P':
      goto p1016;
    case 'Q':
      goto p1017;
    case 'q':
      goto p1017;
    case 'R':
      goto p1018;
    case 'S':
      if (do_b_or_s(false))
        READ_NEXT_COMMAND;
      REREAD_CMD;
    case 'U':
      goto p1020;
    case 'V':
      goto p1021;
    case 'W':
      goto p1022;
    case 'X':
      goto p1023;
    case 'T':
      goto p1101;
    case 'Z':
      goto p1102;
    case 'O':
      goto p1401;
    case 'N':
      goto p1501;
    case 'K':
      goto p1525;
    case 'Y':
      goto p1607;
    case 'b':
      goto p1701;
    case 'v':
      goto p1702;
    case 'n':
      goto p1703;
    case 'o':
      goto p1707;
    case '!':
      goto p1801;
    case 'x':
      goto p1904;
    case 'l':
      goto p1014;                  /* Same as L */
    case 'y':
      goto p2002;
    case 't':
      goto p2003;
    case 'f':
      goto p2013;
    case 'c':
      goto p2016;
    case 'm':                      /* "FM"ode */
      if (setmode())
        READ_NEXT_COMMAND;
/* Get Yes / No (no default) */
      REREAD_CMD;
    case 'i':                      /* "FI'mmediate macro */
      if (scrdtk(4, (unsigned char *)buf, BUFMAX, oldcom))
      {
        perror("SCRDTK of macro text");
        putchar('\r');
        printf("Unexpected error");
        REREAD_CMD;
      }
/* Decide which macro this will be. */
/* Some nesting of FI macros is allowed, */
/* to support e.g. nested U-use files which contain FI cmds */
      if (immnxfr > LAST_IMMEDIATE_MACRO)
      {
        printf("%s", "Too many nested FI commands");
        REREAD_CMD;
      }                            /* if (immnxfr > LAST_IMMEDIATE_MACRO) */
      verb = immnxfr++;
      if (!newmac2(true))
        REREAD_CMD;

/* FI does an implied ^ND */
      if (curmac >= 0 && !pushmac(false))
        REREAD_CMD;

      curmac = verb;
      mcposn = 0;
      READ_NEXT_COMMAND;
    case 'd':                      /* "FD"evnull */
      switch (get_answer())
      {
        case Q_UNREC:
          REREAD_CMD;
        case Q_MAYBE:
        case Q_NO:
          restore_stdout();
          break;
        case Q_YES:
          if (USING_FILE)
          {
            devnull_stdout();
            break;
          }                        /* if (USING_FILE) */
          else
          {
            printf("%s", "fd y is not available from the keyboard");
            REREAD_CMD;
          }                        /* if (USING_FILE) else */
      }                            /* switch (get_answer()) */
      READ_NEXT_COMMAND;
  }
  printf("%s", "unknown command"); /* Dropped out of switch */
p1025:
  if (cmd_state == LINE_NUMBER_SAVED) /* Deal with early errors specially */
  {
    cmd_state = TRY_INITIAL_COMMAND;
    optind--;
  }                                /* if (cmd_state == LINE_NUMBER_SAVED) */
  rerdcm();
  goto p1201;
/* ******************************************************************
 *                        Start line modifiers
 * ******************************************************************
 *
 * A - Append
 */
p1005:
  if (!eolok())
    REREAD_CMD;                    /* Reread command */
  if (deferd)                      /* File not all read in yet */
    dfread(LONG_MAX, NULL);
  setptr(lintot + 1);              /* Ptr after last line in file */
/*
 * Code used by INSERT and APPEND
 */
p1034:modify = false;
  lstvld = false;                  /* Previous line not valid */
  newl = 1026;
p1026:
  curr->bchars = 0;
  curr->bcurs = 0;                 /* Set up new empty line */
/*
 * Code used by INSERT, APPEND, MODIFY
 */
  sprmpt(ptrpos);                  /* PROMPT = # of new line */
/*
 * See if ^C has been typed - get user out of INSERT/APPEND/MODIFY
 * if it has. (Particularly user 1, who can't input an ESC)
 */
p1027:if (cntrlc)
    goto p1901;                    /* J ^C keyed */
  scrdit(curr, prev, (char *)prmpt, pchrs, false); /* Edit the line */
  lgtmp3 = curmac < 0 || !BRIEF;   /* Do a DISPLY if true */
  switch (verb)                    /* Check EOL type */
  {
    case 'J':
      goto p1029;
    case '[':
      goto p1301;
    case 'T':
      goto p1030;
  }
  (void)write(1, "Internal error - EOL char not recognised", 40);
  newlin();
  READ_NEXT_COMMAND;
p1029:
  rtn = newl;
p1033:
  lgtmp4 = (modify || splt);       /* We are not inserting */
  if (lgtmp4 && modlin)
    delete(false);                 /* Delete CHANGED existing line */
  splt = false;                    /* Not a split this time */
  if (lgtmp3)
    disply(prev, false);           /* Display final line */
  if ((!lgtmp4) || modlin)
    inslin(prev);                  /* Insert changed or new line */
  goto asg2rtn;                    /* ^M & ^T part here */
p1030:
  rtn = 1032;
  goto p1033;                      /* Display & update file */
p1032:
  splt = true;                     /* Force a delete next time */
  inslin(curr);                    /* In case ESC next time */
  sprmpt(ptrpos - 1);
  goto p1027;                      /* New line for all 3 (A,I,M) */

/* P1301 - ESC. If we were changing an existing line, display the original */

p1301:
  if (modify || splt)
  {
    if (lgtmp3)                    /* Display req'd */
    {
      setptr(ptrpos - 1);
      rdlin(prev, false);
      disply(prev, false);
    }                              /* if(lgtmp3) */
    splt = false;
  }                                /* if(modify||splt) */
  if (revrse)                      /* "L"ocate backwards */
    setptr(revpos);
  READ_NEXT_COMMAND;
/*
 * I - Insert
 */
p1012:
  if (!getlin(true, true))
    REREAD_CMD;                    /* J line # u/s */
  setptr(oldcom->decval);          /* Get ready to insert */
  if (eolok())                     /* No extra params */
    goto p1034;
  REREAD_CMD;
/*
 * M - MODIFY
 */
p1015:
  modify = true;
  if (getlin(false, false))
  {
    j4 = oldcom->decval;           /* 1st line to be altered */
    lstvld = false;                /* Previous line not valid */
    if (!get_opt_lines2count())
      REREAD_CMD;
  }
  else
  {
    locerr = true;                 /* An scmac can detect this error */
    if (curmac < 0 || !BRIEF)      /* Report err unless brief macro */
      fprintf(stderr, "%s", ermess);
    REREAD_CMD;                    /* Reread command */
  }
p1036:
  newl = 1037;                  /* Eventually come back here after any splits */
  if (verb == 'M')                 /* Was M-modify */
    setptr(j4);                    /* Position on 1st line to alter */
  for (i = count; i > 0; i--)
  {
    if (!rdlin(curr, false))       /* Get lin to mod / EOF */
    {
      printf("E - O - F\r\n");
      READ_NEXT_COMMAND;
    }                              /* if(!rdlin(curr, false)) */
    curr->bcurs = locpos;          /* In case just come from LOCATE */
    locpos = 0;                    /* In case just come from LOCATE */
    sprmpt(ptrpos - 1);            /* Set up prompt lin # just read */
    goto p1027;
  p1037:;                          /* Nl typed (possibly after ^T(s)) */
  }

/* If doing a reverse, locate move pointer back to where locate was, ready for
 * the next one. Also has to be done after Ec) */

  if (revrse)
    setptr(revpos);

  READ_NEXT_COMMAND;               /* Finished this MODIFY */
/* ******************************************************************
 * End line modifiers
 * ******************************************************************
 *
 *
 *
 *
 * ******************************************************************
 * Start file handlers
 * ******************************************************************
 *
 * W - WRITEFILE
 */
p1022:
  if (!get_file_arg() || nofile)
    ERR1025("Error in filename");
  if (!getlin(true, false))
    REREAD_CMD;                    /* J line # u/s */
  setptr(oldcom->decval);          /* Get ready to write */
  if (!get_opt_lines2count() || !eolok())
    REREAD_CMD;
  wrtnum = count;
  rdwr = O_WRONLY + O_CREAT;
  if (!s_b_w_common_write())
    REREAD_CMD;
  READ_NEXT_COMMAND;
/*
 * E - Enter
 */
p1009:
  if (!get_file_arg() || nofile)
    ERR1025("Error in filename");
  if (!eolok())
    REREAD_CMD;
  if ((funit = open_buf(O_RDONLY, 0)) == -1)
  {
    fprintf(stderr, "%s. %s (open)", strerror(errno), buf);
    REREAD_CMD;                    /* Bad open */
  }                         /* if ((funit = open_buf(O_RDONLY, tmode)) == -1) */
e_q_common:                        /* Q <filename> joins here */
  savpos = ptrpos;                 /* Return here with file open. read it... */

/* Use mmap if requested */

  if (fmode & 02000 && (verb != 'E' || fmode & 020000))
  {
    if (fstat(funit, &statbuf))
    {
      fprintf(stderr, "%s. funit %d (fstat)", strerror(errno), funit);
      my_close(funit);             /* Bad fstat */
      REREAD_CMD;
    }                              /* if(fstat(funit,&statbuf)) */
    if (statbuf.st_size)
    {
      if ((p = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE
#ifdef MAP_DENYWRITE
        | MAP_DENYWRITE      /* Doesn't appear to work (need MAP_EXECUTABLE?) */
#endif
        , funit, (off_t) 0)) == (void *)-1)
      {
        fprintf(stderr, "%s. funit %d (mmap)", strerror(errno), funit);
        my_close(funit);
        REREAD_CMD;
      }                            /* if(!(p=mmap(NULL,statbuf.st_size... */
      mapfil(statbuf.st_ino, statbuf.st_size, p);
    }                              /* if(statbuf.st_size) */
    else
      printf("0 lines read.\r\n");
  }                                /* if(fmode&02000&&... */
  else
    readfl();
  if (my_close(funit))             /* Failure */
    fprintf(stderr, "%s. funit %d (close)", strerror(errno), funit);
  if (verb == 'Q')
    mods = false;                  /* No mods yet if that was Q-QUIT */
  setptr(savpos);                  /*  ...retaining old file pos'n... */
  READ_NEXT_COMMAND;               /* ... and finish (!) */
/*
 * Q - Quit
 *
 * We accept a filename - starts user off editing a fresh file
 */
p1017:
  if (mods &&
    !ysno5a("file modified since last b-backup/s-save, ok to quit (y,n,Cr [n])",
    A5DNO))
    READ_NEXT_COMMAND;             /* J user changed his mind */
  Tcl_DumpActiveMemory("t5mem");
  if (!get_file_arg())
    ERR1025("Error in filename");
  if (nofile)
  {
/*
 * If in a macro, only action solitary Q if mode says so.
 * Otherwise, convert to ^NU...
 */
    if (curmac >= 0 && !(fmode & 0100) && verb == 'Q')
    {
      macdef(64, (unsigned char *)"", 0, true); /* Macro is ^NU only */
      curmac = 64;
      mcposn = 0;
      READ_NEXT_COMMAND;
    }
    return 0;                      /* ACTUALLY EXIT Q */
  }
  rdwr = O_RDONLY;
  q_new_file = false;              /* Q-QUIT into existing file */
colontrunc:
  if (!do_stat_symlink() || !eolok())
    REREAD_CMD;
try_open:
  if ((funit = open_buf(rdwr, tmode)) == -1)
  {
/*
 * The file may not exist as user wishes to create a new one.
 * Or the file may not exist because it's of the form <filename>:<line number>
 */
    if (errno == ENOENT && !q_new_file)
    {
      if (cmd_state == RUNNING || cmd_state == TRY_INITIAL_COMMAND)
      {
        if ((colonpos = strchr(buf, ':')) &&
          sscanf(colonpos + 1, "%d", &colonline) == 1)
        {
          cmd_state = HAVE_LINE_NUMBER; /* line # in colonline */
          *colonpos = '\0';        /* Truncate filename */
          if (!do_stat_symlink() || !eolok())
          {
            cmd_state = RUNNING;
            REREAD_CMD;
          }                        /* if (!do_stat_symlink() || !eolok()) */
          goto colontrunc;         /* Try with truncated buf */
        }                          /* if((colonpos=strchr(buf,':'))&&... */
      }                            /* if (cmd_state == RUNNING) */
      else if (cmd_state == HAVE_LINE_NUMBER) /* Just tried truncating at ":" */
        *colonpos = ':';           /* Undo truncation */
      cmd_state = RUNNING;

/* Look for Q command-line option that is enabled while running */
      if (strlen(buf) == 2 && buf[0] == '-' && ( /* Could be option */
        buf[1] == 'A' ||           /* Display opcodes */
        buf[1] == 'V'))            /* Display version */
      {
        switch (buf[1])
        {
          case 'A':
            display_opcodes();
            break;

          case 'V':
            q_version();
            break;
        }                          /* switch (buf[1]) */
        READ_NEXT_COMMAND;
      }                            /* if (strlen(buf) == 2 && ... */

      if (ysno5a("Do you want to create a new file (y,n,Cr [n])", A5DNO))
      {
        cmd_state = TRY_INITIAL_COMMAND;
        q_new_file = true;         /* Q-QUIT into new file */
        rdwr = O_WRONLY + O_CREAT + O_EXCL; /* File should *not* exist */
        goto try_open;             /* So create file */
      }  /* if(ysno5a("Do you want to create a new file (y,n,Cr [n])",A5DNO)) */
    }                              /* if (errno == ENOENT && !q_new_file) */
    fprintf(stderr, "%s. %s (open)", strerror(errno), buf);
    REREAD_CMD;                    /* Bad open */
  }                             /* if ((funit = open_buf(rdwr, tmode)) == -1) */

  if (q_new_file)                  /* Have just created file for Q-QUIT */
  {
    mods = false;                  /* No mods to new file yet */
    my_close(funit);               /* New empty file open in wrong mode */
  }                                /* if(q_new_file) */
  finitl();                        /* Erases old file but keeps segments */
  verb = 'Q';                      /* In case was 'q' */
/*
 * Set up new default for S&B
 */
  (void)strcpy(pcnta, buf);        /* Filename & length now remembered */
  if (q_new_file)                  /* Q-QUIT into new file */
    READ_NEXT_COMMAND;             /* Read next command */
  goto e_q_common;                 /* Join E-ENTER */
/*
 * ******************************************************************
 * End file handlers (except HELP & USE)
 * ******************************************************************
 *
 * D - DELETE
 */
p1008:
  if (!getlin(true, false))
    REREAD_CMD;                    /* J bad line # */
  k4 = oldcom->decval + 1;         /* Pos here for each delete */
  if (!get_opt_lines2count())
    REREAD_CMD;
  clrfgt();                        /* In case lines from last D */
  for (i4 = count; i4 > 0; i4--)
  {

/* Next line checks if there are deferred lines then try to get one. We need to
 * re-check lintot: "deferd" will get cleared in the process if the last line
 * was unterminated */

    if (k4 == lintot + 2 && !(deferd && (dfread(1, NULL), k4 != lintot + 2)))
      goto p1078;
    setptr(k4);                    /* Pos 1 past 1st line to go */
    delete(false);                 /* Knock off line. Use normal ptr */
  }
  READ_NEXT_COMMAND;               /* Finished if get here */
p1078:
  rtn = 1079;
p1112:
  i4 = count - i4 + 1;             /* This many *not* deleted */
q1112:
  printf("%s of file reached:- ", revrse ? "start" : "end");
p1120:
  printf("%ld lines ", i4 - 1);
  goto asg2rtn;
p1079:
  puts("deleted\r");
  READ_NEXT_COMMAND;               /* End DELETE */
/*
 * G - GOTO
 */
p1010:
  if (getlin(true, true) && eolok())
  {
    setptr(oldcom->decval);
    READ_NEXT_COMMAND;             /* Finished GOTO */
  }                                /* if(getlin(true, ... */
  REREAD_CMD;
/*
 * U - USE
 */
p1020:
  if (!get_file_arg() || nofile)
    ERR1025("Error in filename");
  if (!eolok())
    REREAD_CMD;
  duplx5(true);                    /* Assert XOFF recognition */

/* NULLSTDOUT setting is same as parent */
  stdinfo[stdidx + 1].nullstdout =
    stdidx < 0 ? false : stdinfo[stdidx].nullstdout;

/* Save current stdin */
  stdidx++;
  do
    stdinfo[stdidx].funit = dup(0);
  while (stdinfo[stdidx].funit == -1 && errno == EINTR);
  if (stdinfo[stdidx].funit == -1)
  {
    stdidx--;
    fprintf(stderr, "%s. (dup(0))", strerror(errno));
    REREAD_CMD;
  }                                /* if (stdinfo[stdidx].funit == -1) */

/* Close funit 0 */
  do
    i = close(0);
  while (i == -1 && errno == EINTR);

/* Open new input source */
  do
    i = open_buf(O_RDONLY, 0);
  while (i == -1 && errno == EINTR);
  if (i == -1)
  {
    pop_stdin();
    fprintf(stderr, "%s. %s (open)", strerror(errno), buf);
    REREAD_CMD;
  }                                /* if (i == -1) */

/* Verify new input opened on funit 0. Try to rectify if not */
  if (i)
  {
    do
      j = dup2(i, 0);
    while (j == -1 && errno == EINTR);
    if (j == -1)
    {
      fprintf(stderr, "%s.(dup2(%d, 0))", strerror(errno), i);
      do
        j = close(i);
      while (j == -1 && errno == EINTR);
      pop_stdin();
      REREAD_CMD;
    }                              /* if (j == -1) */
    do
      j = close(i);
    while (j == -1 && errno == EINTR);
  }                                /* if (i) */

/* If invoked from a macro, suspend that macro */
  if (curmac >= 0)
  {
    if (!pushmac(true))
      REREAD_CMD;
    curmac = -1;
    stdinfo[stdidx].frommac = true;
  }                                /* if (curmac >= 0) */
  else
    stdinfo[stdidx].frommac = false;

  buf5len = 0;                     /* Flush any input left over */
  READ_NEXT_COMMAND;
/*
 * H - HELP
 */
p1011:
/*
 * We have some extra work here, because HELP doesn't actually take
 * a filename, and doesn't want one.
 */
  if (scrdtk(2, (unsigned char *)tmfile, 17, oldcom))
  {
  p11042:
    fprintf(stderr, "%s. (scrdtk)", strerror(errno));
    REREAD_CMD;
  }
  k = oldcom->toklen;              /* Get length of HELP topic */
  if (k == 0)
  {                                /* If no topic given */
    k = 1;
    tmfile[0] = '#';               /* Dummy topic of "#" */
    tmfile[1] = '\0';
  }
  if (!eolok())
    REREAD_CMD;
  for (i = k - 1; i >= 0; i--)
  {
    if (tmfile[i] >= 'A' && tmfile[i] <= 'Z')
      tmfile[i] += 040;
  }
/* GCC 8.1 complained when the next line was an sprintf. */
/* To keep gcc quiet, we must test for overflow. */
/* If truncation happens, there should be a useful message... */
  if (snprintf(tmtree, sizeof tmtree, "%s/%s", help_dir,
    tmfile) >= sizeof tmtree)
    ;
  if (stat(tmtree, &statbuf))
  {
/* Output a potted message if he "typed h for help" */
    if (tmfile[0] == '#' && tmfile[1] == 0)
    {
      printf("\r\n");
      printf("Sorry - I can't find my HELP files.\r\n");
      printf("If you have them installed somewhere,\r\n"
        "please put that path in your shell environment"
        " with the name Q_HELP_DIR.\r\n\n");
      READ_NEXT_COMMAND;
    }
    fprintf(stderr, "%s. %s (HELP)", strerror(errno), tmtree);
    REREAD_CMD;
  }
  sprintf(buf, "%s %s", help_cmd, tmtree);
  final5();                        /* For some pagers */
  if (system(buf) < 0)
  {
    fprintf(stderr, "%s. %s (system)", strerror(errno), buf);
    init5();
    REREAD_CMD;
  }
  init5();
  READ_NEXT_COMMAND;               /* Finished */
/*
 * X - Set terminal characteristics
 */
p1023:
  if (!eolok())
    REREAD_CMD;
  puts("Switching off screenedit mode\r");
  xistcs();                        /* So set them */
  puts("Re-enabling screenedit\r");
  READ_NEXT_COMMAND;               /* Finished X */
/*
 * P - Print
 */
p1016:
  lstlin = ptrpos;                 /* -TO strt curr lin */
  if (!get_opt_lines2count())
    REREAD_CMD;
  rtn = 1093;
p1104:
  for (i = count; i > 0; i--)
  {
    if (!rdlin(curr, false))       /* EOF. Normal ptr */
    {
      puts(" E - O - F\r");
      break;
    }
    sprmpt(ptrpos - 1);
    pdsply(curr, prmpt, pchrs);
    if (cntrlc || kbd5())          /* User wants out */
    {
      newlin();
      puts(" *** keyboard interrupt *** \r");
      cntrlc = false;
      break;
    }
  }
  goto asg2rtn;                    /* All req'd lines printed */
p1093:
  READ_NEXT_COMMAND;               /* Finished P */
/*
 * V - View
 *
 *
 * Print n lines before current line, then current line, then n lines
 * after. N may be specified as zero - show current line only.
 * If no n, assume enough lines to completely fill the screen.
 * Afterwards, move pointer back to posn of entry.
 */
p1021:
  numok = 10965;
  nonum = 1097;
  xcount = 0;                      /* Only show requested # */
  lstlin = -1;                     /* Not allowed -TO */
  logtmp = true;                   /* Accept 0 lines */
  fullv = false;                   /* Assume not just "V" */
p1107:
  if (!getnum(logtmp))
    REREAD_CMD;                    /* J format err on # of lines */
  count = oldcom->decval;
  if (oldcom->toktyp == eoltok)
    goto asg2nonum;                /* J no # given */
/*
 * !CAUTION! Tricky coding. LOGTMP is really for whether 0 is allowed
 * but it is also used here, in the reverse sense, to see whether extra
 * parameters are allowed or not. I.E. if true then 0 is allowed
 * but extra parameters aren't; false is the other way round.
 */
  savtok = oldcom->toktyp;         /* eolok overwrites */
  if (!logtmp || eolok())          /* More allowed or eol now */
  {
    if (savtok == nultok)          /* No # given */
      goto asg2nonum;
    else
      goto asg2numok;
  }                                /* if(!logtmp||eolok()) */
  REREAD_CMD;
/* Print enough lines to fill the screen... */
p1097:
  fullv = true;                    /* Try very hard to fill screen */
  count = (row5 / 2) - 1;          /* No count given so assume 1/2 screen */
  xcount = row5 & 1;               /* Extra line if odd screen length */
p10965:
  if (!lintot && !(deferd && (dfread(1, NULL), lintot))) /* Empty file */
  {
    puts("THE FILE IS EMPTY\r");
    READ_NEXT_COMMAND;             /* End V */
  }
  k4 = ptrpos;                     /* Remember pos'n */
  j4 = k4 - count;
  if (j4 < 1)
    j4 = 1;                        /* J4=1st line to list */
  setptr(j4);
  j4 = k4 + count + xcount;        /* Extra line if odd length */
  if (j4 > lintot && deferd)
    dfread(j4 - lintot, NULL);     /* Not -1, want that extra line */
  if (j4 > lintot + 1)
    j4 = lintot + 1;               /* J4=last line to list */
  count = j4 - ptrpos + 1;         /* Set up # lines to print */
/* Want to fill screen but not enough lines yet */
  if (fullv && count < row5 - 1)
  {
    if (ptrpos == 1)               /* Viewing from file start */
    {
      if (deferd && lintot < row5)
        dfread(row5, NULL);        /* Ensure lintot is adequate */
      count = lintot + 1 >= row5 - 1 ? row5 - 1 : lintot + 1;
    }                              /* if(ptrpos==1) */
    else                           /* Viewing to file end */
    {
      if (deferd)
        dfread(row5, NULL);        /* Ensure lintot is adequate */
      j4 = lintot + 3 - row5;
      if (j4 < 1)
        j4 = 1;
      setptr(j4);
      count = lintot + 3 - j4;
    }                              /* if(ptrpos==1) else */
  }
  rtn = 1103;
  goto p1104;                      /* Print them */
p1103:
  setptr(k4);                      /* Reposition */
  READ_NEXT_COMMAND;               /* Finished */
/*
 * L - Locate
 */
p1014:
  tokens = verb == 'l';            /* Whether FL */
  if ((revrse = (fmode & 04000) != 0))
    revpos = ptrpos;
  lgtmp2 = !BRIEF || curmac < 0;   /* Display error messages if true */
  if (revrse ? ptrpos <= 1 : ptrpos > lintot && !(deferd &&
    (dfread(1, NULL), ptrpos <= lintot)))
  {
    if (lgtmp2)
      printf("At %s of file already - no lines to search\r\n",
        revrse ? "start" : "end");
    READ_NEXT_COMMAND;             /* Next command */
  }                                /* if(revrse?ptrpos<=1:ptrpos>lintot&&... */

/* Get the string to locate. The string is read to the ermess array, as buf will
 * get overwritten and we aren't going to use getlin, so ermess is spare. */

  if (scrdtk(2, (unsigned char *)ermess, BUFMAX, oldcom))
    goto p11042;                   /* J bad RDTK */
  if (oldcom->toktyp == eoltok)
    goto p11043;
  if (!(l = oldcom->toklen))       /* String is length zero */
  {
  p11043:
    write(1, "Null string to locate", 21);
    REREAD_CMD;                    /* Reread command */
  }                                /* if(!(l=oldcom->toklen)) */
  numok = 1105;
  nonum = 1106;
  lstlin = ptrpos;                 /* -TO rel currnt line */
  logtmp = false;                  /* Don't accept 0 lines */
  goto p1107;                      /* Get number lines to search */
p1106:                             /* No num, so search to [se]of */
  if (revrse)
    count = ptrpos - 1;
  else if (deferd)
    count = LONG_MAX;
  else
    count = lintot - ptrpos + 1;
p1105:count2 = count;              /* We have another COUNT to get */
  lstlin = -1;                     /* Not allowed -TO */
  numok = 1514;
  nonum = 1514;
  goto p1107;                      /* Get # lines to mod on location */
p1514:
  rtn = 1715;                      /* Shared 1st/last code */
p1722:
  numok = 1716;
  nonum = 1716;                    /* 1st pos 0 default */
  goto p17165;                     /* Get 1st pos to search */
p1716:
  firstpos = oldcom->decval - 1;   /* Columns start at 0 */
  numok = 1717;
  nonum = 1718;                    /* Last max lin len default */
  goto p17165;                     /* Get last pos'n */
p1718:
  lastpos = BUFMAX;                /* BUFMAX does not include trlg null */
  goto p1719;
p1717:
  lastpos = oldcom->decval - 1;    /* Last start position */
  if (lastpos < firstpos)          /* Impossible combination of columns */
  {
    printf("Last pos'n < first\r\n");
    REREAD_CMD;
  }                                /* if(lastpos < firstpos) */
  lastpos += l;                    /* Add search length to get wanted length */
p1719:
  minlen = firstpos + l;           /* Get minimum line length to search */
  if (eolok())
    goto asg2rtn;
  REREAD_CMD;
p1715:
  savpos = ptrpos;                 /* Remember pos in case no match */

/* Start of search */

  for (i4 = count2; i4 > 0; i4--)
  {
    if (cntrlc)                    /* User abort */
    {
      count = count2;              /* Fit in with Y's error printing */
      goto p1622;
    }                              /* if(cntrlc) */
    if (revrse)
    {
      if (revpos <= 1)             /* Sof (< shouldn't happen) */
        goto s1112;
      setptr(--revpos);            /* Read previous line */
      rdlin(curr, false);          /* "can't" hit eof */
    }                              /* if(revrse) */
    else if (!rdlin(curr, false))  /* If eof */
    {
    s1112:
      if (!lgtmp2 || count2 == LONG_MAX) /* No message wanted */
        break;                     /* for(i4=count2;i4>0;i4--) */
      rtn = 1111;
    r1112:
      i4 = count2 - i4 + 1;        /* This many *not* searched */
      goto q1112;
    p1111:
      puts("searched\r");
      break;                       /* for(i4=count2;i4>0;i4--) */
    }                              /* if(!rdlin(curr, false)) */
    m = curr->bchars;
    if (m < minlen)
      continue;                    /* Skip search if too short */
    if (m > lastpos)
      m = lastpos;                 /* Get length to search */
    if (tokens ? ltok5a((unsigned char *)ermess, l, curr->bdata, firstpos, m,
      &locpos, &dummy, (unsigned char *)ndel) : lsub5a((unsigned char *)ermess,
      l, curr->bdata, firstpos, m, &locpos, &dummy))
    {                              /* Line located */
      if (revrse)
        setptr(revpos);
      else
      p1110:                       /* Joined here by "J"oin */
        setptr(ptrpos - 1);
      lstvld = false;              /* Previous line not valid */
      modify = true;               /* For "M"odify code */
      goto p1036;                  /* End L & J - carry on as M */
    }                              /* if((tokens?ltok5a((... */
  }                                /* for(i4=count2;i4>0;i4--) */

/* Didn't locate it if we get here */

  setptr(savpos);                  /* Move pointer back */
  locpos = 0;                      /* zeroised by lstr5a */
  if (lgtmp2)
    (void)write(1, "Specified string not found", 26);
p1810:locerr = true;               /* Picked up by RERDCM */
p1811:
/* Reset screen cursor */
  (void)scrdtk(5, (unsigned char *)NULL, 0, oldcom);
/* Move past command & 1st param */
  (void)scrdtk(1, (unsigned char *)NULL, 0, oldcom);
  (void)scrdtk(1, (unsigned char *)NULL, 0, oldcom);
  REREAD_CMD;
/*
 * P17165 - Get 1st & last posn's for L & Y
 */
p17165:
  if (!getnum(0))
    REREAD_CMD;
  if (oldcom->toktyp != nortok)
    goto asg2nonum;                /* J no number given */
  goto asg2numok;
/*
 * J - Join
 */
p1013:
  if (!getlin(true, false))
    REREAD_CMD;                    /* J bad line # */
  setptr(oldcom->decval);          /* Pos'n on line to be joined onto */
  numok = 1114;
  nonum = 1114;
  logtmp = false;                  /* 0 not allowed - not eol yet */
  goto p1107;                      /* Get opt. # lines to join in COUNT */
p1114:count2 = count;              /* Another # to get */
  lstlin = -1;                     /* Not allowed -TO */
  if (!get_opt_lines2count())
    REREAD_CMD;
/* At eof */
  if (ptrpos >= lintot && !(deferd && (dfread(1, NULL), ptrpos < lintot)))
  {
    if (curmac < 0 || !BRIEF)      /* Message wanted */
      puts("Can't join anything - no lines follow\r");
    READ_NEXT_COMMAND;             /* Next command */
  }                                /* if(ptrpos>=lintot&&... */
  rdlin(prev, false);              /* 1st line */
  delete(false);                   /* Del lin before normal ptr */
  for (i4 = count2; i4 > 0; i4--)
  {
    if (!rdlin(curr, false))       /* If eof */
    {
      rtn = 1118;
      goto r1112;
    p1118:
      puts("joined\r");
      break;
    }
    j = curr->bchars + prev->bchars;
    if (j > prev->bmxch)
      goto p1117;                  /* J would bust line */
    r = (char *)&prev->bdata[prev->bchars]; /* Appending posn */
    prev->bchars = j;              /* New length */
    delete(false);                 /* Delete line just read */
/* Append line just read */
    memcpy(r, (char *)curr->bdata, (size_t)curr->bchars);
  }
  inslin(prev);                    /* Put composed line back */
  goto p1110;                      /* Join M-MODIFY eventually */
p1117:
  setptr(ptrpos - 1);              /* P1117 - bust line */
  (void)write(1, "joining next line would exceed line size :- ", 44);
  rtn = 1118;
  goto p1120;                      /* End JOIN */
/*
 * R - Re position
 *
 *
 * Most of the code is common to C-COPY
 */
p1018:
  rtn = 1121;
  repos = true;
p1129:
  if (!getlin(true, false))        /* Bad source. C joins here */
  {
    (void)write(1, " in source line", 15);
    REREAD_CMD;
  }
  k4 = oldcom->decval;             /* Remember source */
  if (!getlin(true, true))         /* Bad dest'n */
  {
    (void)write(1, " in dest'n line", 15);
    REREAD_CMD;
  }
  j4 = oldcom->decval;             /* Remember dest'n */
  lstlin = k4;                     /* -TO refers from source line */
  if (j4 == k4)                    /* Error if equal */
  {
    (void)write(1, "Source and destination line #'s must be different", 49);
    REREAD_CMD;
  }
  goto asg2rtn;                    /* End 1st common part */
p1121:
  if (k4 == j4 - 1)
  {
    (void)write(1, "moving a line to before the next line is a no-op", 48);
    REREAD_CMD;
  }
  rtn = 1125;                      /* Only used if hit eof */
p1131:                             /* C joins us here */
  if (!get_opt_lines2count())
    REREAD_CMD;
  setptr(j4);                      /* Set main ptr at dest'n */
/*
 * Note:- When setting both pointers, always set AUX second
 */
  setaux(k4);                      /* Set AUX ptr at source */
  for (i4 = count; i4 > 0; i4--)
  {
    if (!rdlin(prev, true))
      goto p1112;                  /* Read AUX, j eof to mess code */
    if (repos)
      delete(true);                /* For reposition only, delete line read */
    inslin(prev);
  }
  READ_NEXT_COMMAND;               /* Finished C or R */
p1125:
  puts("repositioned\r");          /* Eof on repos. most of mess done */
  READ_NEXT_COMMAND;               /* End R */
/*
 * C-COPY
 */
p1007:
  repos = false;                   /* C-COPY, not R-REPOS */
  rtn = 1128;
  goto p1129;                      /* Phase 1 - get source & dest'n #'s */
p1128:
  rtn = 1130;                      /* Come back if hit eof */
  goto p1131;                      /* Get 3rd param & do copy */
p1130:
  puts("copied\r");                /* Eof. Most of message o/p */
  READ_NEXT_COMMAND;               /* End C */
/*
 * T - Tabset
 */
/* Enter SCREENEDIT subsystem to complete command */
p1101:
  if (tabset(oldcom))
    READ_NEXT_COMMAND;
  REREAD_CMD;                      /* Allow user to correct any errors */
/*
 * Z - Return from a Use file
 */
p1102:
  if (!eolok() || !pop_stdin())
    REREAD_CMD;

/* If U-use was in a macro, resume that macro */
  if (stdinfo[stdidx + 1].frommac)
  {
    mcnxfr--;
    curmac = mcstck[mcnxfr].mcprev;
    mcposn = mcstck[mcnxfr].mcposn;
  }                                /* if (stdinfo[stdidx + 1].frommac) */
  READ_NEXT_COMMAND;
/*
 * O - Switch On INDENT
 */
p1401:
  logtmp = INDENT != 0;
  rtn = 1407;
p2015:
  answer = get_answer();
  switch (answer)
  {
    case Q_YES:
      logtmp = true;
      break;
    case Q_NO:
      logtmp = false;
      break;
    case Q_MAYBE:
      logtmp = !logtmp;
      break;
    case Q_UNREC:
      REREAD_CMD;
  }                                /* switch (answer) */
  goto asg2rtn;
p1407:
  if (logtmp)
    fmode |= 04000000000;
  else
    fmode &= ~04000000000;
  READ_NEXT_COMMAND;
/*
 * P2013 - FFortran. Use the O-indent code...
 */
p2013:
  rtn = 2014;
  logtmp = FTNMOD != 0;
  goto p2015;
p2014:
  if (logtmp)
    fmode |= 01000000000;
  else
    fmode &= ~01000000000;
  READ_NEXT_COMMAND;
/*
 * P2016 - FCaseind. Use the O-indent code...
 */
p2016:
  rtn = 2017;
  logtmp = !CASDEP;
  goto p2015;
p2017:
  if (logtmp)
    fmode |= 02000000000;
  else
    fmode &= ~02000000000;
  READ_NEXT_COMMAND;
/*
 * N - NEWMACRO
 */
p1501:
  i = newmac();
  if (i < 0)                       /* Token was "-" */
  {
    scrdtk(2, (unsigned char *)buf, BUFMAX, oldcom);
    if (oldcom->toktyp == eoltok)
      typmac();
    else
    {
      if (!eolok())
      {
        alu_macros_only = false;
        REREAD_CMD;
      }                            /* if (!eolok()) */
/* Need to preserve stdout for this */
      if (orig_stdout == -1)
      {
        do
          orig_stdout = dup(1);
        while (orig_stdout == -1 && errno == EINTR);
      }                            /* if (orig_stdout == -1) */
      if (orig_stdout == -1)
      {
        fprintf(stderr, "\r\n%s. (dup(1))\r\n", strerror(errno));
        refrsh(NULL);
        alu_macros_only = false;
        REREAD_CMD;
      }                            /* if (orig_stdout == -1) */
      else
      {
        do
          i = close(1);
        while (i == -1 && errno == EINTR);
        i = open_buf(O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (i == 1)
        {
          lstmac();
          i = 0;                   /* Success */
        }                          /* if (i == 1) */
        else
          i = errno;
        restore_stdout();
        if (i)
          fprintf(stderr, "%s. %s (freopen)", strerror(i), buf);
        i = !i;                    /* Required below */
      }                            /* if (orig_stdout == -1) else */
    }                              /* if (oldcom->toktyp == eoltok) else */
    alu_macros_only = false;
  }                                /* if (i < 0) */
  if (i)
    READ_NEXT_COMMAND;             /* No error */
  REREAD_CMD;
/*
 * K - pop a shell prompt
 */
p1525:
  if (!eolok())
    REREAD_CMD;
  if (!USING_FILE)
    puts("Exit from shell to restart\r");
  final5();
  i = system(sh);
  init5();
  if (i < 0)
    fprintf(stderr, "%s. (system(\"%s\"))", strerror(errno), sh);
  if (!USING_FILE)
    puts("Re-entering Q\r");
  READ_NEXT_COMMAND;
/*
 * FX - Exchange the functions of 2 keyboard keys
 */
p1904:
  if (scrdtk(2, (unsigned char *)oldkey, 3, oldcom))
    goto p11042;
  if (oldcom->toktyp != eoltok)
    goto p1905;                    /* J line wasn't empty */
/*
 * FX with no params - Re-initialise the table, subject to
 *                     confirmation
 */
  if (!ysno5a("Re-initialise keyboard table to default - ok", A5NDEF))
    READ_NEXT_COMMAND;             /* Finish if changed his mind */
  for (i = 127; i >= 0; i--)
    fxtabl[i] = i;                 /* No FX commands yet */
  READ_NEXT_COMMAND;               /* Finished after reset */
/*
 * Validate parameter just read ...
 */
p1905:
  rtn = 1909;
  xkey[0] = oldkey[0];
  xkey[1] = oldkey[1];
p1915:
  i = oldcom->toklen;
  if (i == 0)
    goto p11043;                   /* Null param not allowed */
  if (i == 2)
    goto p1908;                    /* ^<char> allowed */
  k = xkey[0];                     /* Convert to subscript */
  if (k < 128)
    goto p1916;                    /* Continue */
  (void)write(1, "parity-high \"keys\" not allowed", 30);
  REREAD_CMD;
p1908:
  if (xkey[0] == CARAT)
    goto p1910;                    /* J starts "^" (legal) */
  (void)write(1, "may only have single char or ^ char", 35);
  REREAD_CMD;
p1910:
  k = xkey[1];                     /* Isolate putative control */
  if (k >= 0100 && k <= 0137)
    goto p1911;                    /* J a real control char */
  if (k == ASTRSK)
    goto p1912;                    /* ^* (=^) */
  if (k == QM)
    goto p1913;                    /* ^? (=rubout) */
  (void)write(1, "Illegal control character representation", 40);
  REREAD_CMD;
p1911:
  k = k - 0100;                    /* Control char to subscript */
  goto p1916;
p1912:
  k = CARAT;                       /* "^" as subscript */
  goto p1916;
p1913:
  k = 127;                         /* Rubout is the last character */
p1916:
  if (k > 31 && k != 127)
    puts("Warning: non-control char as argument\r");
  goto asg2rtn;
p1909:
  oldkey[0] = k;                   /* Now a subscript */
  if (scrdtk(2, (unsigned char *)newkey, 3, oldcom))
    goto p11042;
  if (oldcom->toktyp == eoltok)
    goto p11043;                   /* Must have another parameter */
  xkey[0] = newkey[0];
  xkey[1] = newkey[1];
  rtn = 1914;
  goto p1915;                      /* Validate parameter */
p1914:
  newkey[0] = k;                   /* Now a subscript */
  if (!eolok())
    REREAD_CMD;
  i = fxtabl[(int)oldkey[0]];
  fxtabl[(int)oldkey[0]] = fxtabl[(int)newkey[0]];
  fxtabl[(int)newkey[0]] = i;
  READ_NEXT_COMMAND;
/*
 * Y - Change all occurrences of 1 string to another
 *
 *
 * P1607 - Get string to look for
 */
p2002:
  tokens = true;                   /* FY */
  goto p2005;
p1607:
  tokens = false;                  /* Not FY */
p2005:
  if (scrdtk(2, (unsigned char *)oldstr, BUFMAX, oldcom))
    goto p11042;
  if (oldcom->toktyp == eoltok)
    goto p11043;
  oldlen = oldcom->toklen;         /* Length of string */
  if (oldlen == 0)
    goto p11043;                   /* J null string (error) */
/*
 * Get string with which to replace it
 */
  if (scrdtk(2, (unsigned char *)newstr, BUFMAX, oldcom))
    goto p11042;
  newlen = oldcom->toklen;
  ydiff = newlen - oldlen;
/* strings must be equal length if Fixed-Length mode */
  if (ydiff && fmode & 0400)
  {
    printf("Replace string must be same length in FIXED LENGTH mode");
    REREAD_CMD;                    /* Report error */
  }
  if (oldcom->toktyp == eoltok)
    goto p1608;                    /* J no more params */
/* */
  if (getlin(false, false))
    goto p1609;                    /* J definitely ok 1st line # */
  if (oldcom->toktyp != nortok)
    goto p1608;                    /* J ok after all */
  fprintf(stderr, "%s", ermess);
  REREAD_CMD;
p1608:j4 = 1;                      /* Start looking at line 1 */
  if (oldcom->toktyp == eoltok)
    goto p1610;                    /* Join no # lines code if eol now */
  goto p16105;
p1609:
  j4 = oldcom->decval;
p16105:if (!getnum(0))
    REREAD_CMD;                    /* Get # lines, 0 not allowed */
  count = oldcom->decval;
  if (oldcom->toktyp != nortok)    /* No number given */
  p1610:
    count = deferd ? LONG_MAX : lintot + 1 - j4; /* Process to eof */
  rtn = 1612;
  lstlin = -1;                     /* -TO not allowed for column pos'ns */
  l = oldlen;                      /* Req'd by code for L-LOCATE */
  goto p1722;                      /* Look for 1st & last pos'ns in line */

/* Error messages */

p1622:
  cntrlc = false;                  /* ^C noticed */
  printf("Command abandoned :-");
p16221:
  printf(" %ld lines ", count - i4);
p1616:
  (void)write(1, "scanned", 7);
  newlin();
p1712:setptr(savpos);              /* Restore file pos'n */
  READ_NEXT_COMMAND;               /* Leave Y */
p16175:
  savpos = ptrpos - 1;             /* Point to too big line */
  printf("Next line would exceed max size:-");
  goto p16221;
p1620:
  if (curmac < 0 || !BRIEF)
    (void)write(1, "specified string not found", 26);
  setptr(savpos);                  /* Restore file pos'n */
  goto p1810;

p1612:
  if (!lintot && !(deferd && (dfread(1, NULL), lintot))) /* Empty file */
  {
    (void)write(1, "Empty file - can't changeall any lines", 38);
    REREAD_CMD;
  }                                /* if(!lintot&&... */

/* We act on BRIEF or NONE if in a macro without question. Otherwise, BRIEF or
 * NONE is queried, and we reset to VERBOSE if we don't get confirmation */

  if (curmac >= 0)
    goto p1710;                    /* J in a macro */
  if (!BRIEF)
    goto p1710;                    /* J VERBOSE already */
  if (ysno5a("Use brief/none in this command (y,n,Cr [n])", A5DNO))
    goto p1710;
  puts("Reverting to verbose\r");
  fmode &= 07777777777;
p1710:savpos = ptrpos;             /* Remember so we can get back */
  setptr(j4);                      /* First line to look at */
  lgtmp2 = newlen != 0;
  lgtmp3 = false;                  /* No lines changed yet */
/*
 * Main loop on specified lines
 */
  for (i4 = count; i4 > 0; i4--)
  {
    if (cntrlc)                    /* User has interrupted */
      goto p1622;
    if (!rdlin(curr, false))       /* Eof on main pointer */
    {
      if (count == LONG_MAX)       /* Was going to deferred eof */
        break;                     /* for(i4=count;i4>0;i4--) */
      rtn = 1616;
      goto p1112;                  /* Print most of end of file message */
    }
/*
 * Initial tasks for each line
 */
    yposn = firstpos;              /* Search from 1st position spec'd */
    linmod = false;                /* No match this line yet */
    k = curr->bchars;              /* Remembers line length */
    if (k < minlen)
      continue;                    /* J line shorter than minimum */
    n = 0;
    if (k > lastpos)
      n = k - lastpos;             /* N=# at end not to search */
/* */
  p1619:if (tokens)
      goto p2011;                  /* J FY */
/* J no more occurrences this line */
    if (!lsub5a((unsigned char *)oldstr, oldlen, curr->bdata, yposn, k - n, &l,
      &m))
      goto p1617;
    goto p2012;
  p2011:
/* J no more occurrences this line */
    if (!ltok5a((unsigned char *)oldstr, oldlen, curr->bdata, yposn, k - n, &l,
      &m, (unsigned char *)ndel))
      goto p1617;
  p2012:
    if (k + ydiff > curr->bmxch)
      goto p16175;                 /* J would bust line */
    if (m == k - 1)
      goto p1618;                  /* J no chars after string */
    if (ydiff > 0)
/* Create a gap of the right length. Overlapping r/h move */
    {
      p = &curr->bdata[k - 1];     /* End char to pick up */
      q = &curr->bdata[k - 1 + ydiff]; /* End char to set down */
      for (i = k - m - 1; i > 0; i--)
        *q-- = *p--;
    }
    else if (ydiff < 0)
/* Close up by the right length. Overlapping l/h move */
    {
      p = &curr->bdata[m + 1];     /* Start char to pick up */
      q = &curr->bdata[m + 1 + ydiff]; /* Start char to set down */
      for (i = k - m - 1; i > 0; i--)
        *q++ = *p++;
    }
  p1618:
/* Move in new string if not null */
    if (lgtmp2)
      memcpy((char *)&curr->bdata[l], (char *)newstr, (size_t)newlen);
    k = k + ydiff;                 /* Get new line length */
    linmod = true;                 /* This line has been modified */
    yposn = m + 1 + ydiff;         /* Resume search after new string */
/* Seek more occurrences if room */
    if (k - n - yposn >= oldlen)
      goto p1619;
  p1617:
    if (!linmod)
      continue;                    /* J no mods to this line */
    linmod = false;
    lgtmp3 = true;                 /* A line changed now */
    curr->bchars = k;
    if (!NONE)                     /* Some display may be req'd */
    {
/*
 * SPEEDUP - If BRIEF, only display every 1/5th sec
 */
      if (BRIEF)                   /* only display if time to (or 1st) */
      {
        if ((timnow = times(&tloc)) == -1)
        {
          perror("times");
          putchar('\r');
          REREAD_CMD;
        }
        if (timnow - timlst < 20)
          goto p1711;              /* J not yet time to display */
        timlst = timnow;           /* Displaying */
        forych = true;             /* Tell PDSPLY short display */
      }                            /* if(BRIEF) */
      sprmpt(ptrpos - 1);          /* set up line # */
      pdsply(curr, prmpt, pchrs);  /* Display the modified line */
    }                              /* if(!NONE) */
  p1711:
    delete(false);                 /* Remove old version of line */
    inslin(curr);                  /* Insert new version */
  }                                /* for(i4=count;i4>0;i4--) */
  if (!lgtmp3)
    goto p1620;                    /* J no lines changed */
  goto p1712;                      /* End Y */
/*
 * FB - BRIEF
 */
p1701:
  if (!eolok())
    REREAD_CMD;
  if (fmode & 01000)
  {
  q1704:
    printf("f%c ignored (mode +v)\r\n", verb);
    READ_NEXT_COMMAND;
  }
  fmode |= 010000000000;
  fmode &= 017777777777;
  READ_NEXT_COMMAND;               /* Finished */
/*
 * FV - VERBOSE
 */
p1702:
  if (!eolok())
    REREAD_CMD;
  fmode &= 07777777777;
  READ_NEXT_COMMAND;               /* Finished */
/*
 * FN - NONE
 */
p1703:
  if (!eolok())
    REREAD_CMD;
  if (fmode & 01000)
    goto q1704;
  fmode |= 030000000000u;
  READ_NEXT_COMMAND;               /* Finished */
/*
 * FO - FORGET
 */
p1707:
  if (!eolok())
    REREAD_CMD;
  forget();                        /* In fact implemented by workfile */
  READ_NEXT_COMMAND;
/*
 * FT - TOKENCHAR
 */
p2003:
  if (strlen(ndel) < 32)
    goto p2007;                    /* J table not full */
  (void)write(1, "no room for further entries", 27);
  REREAD_CMD;
p2007:
/* Get character to add */
  if (scrdtk(1, (unsigned char *)buf, 40, oldcom))
    goto p11042;
  if (oldcom->toktyp != eoltok)
    goto p2008;                    /* J not EOL */
  (void)write(1, "command requires a parameter", 28);
  REREAD_CMD;
p2008:
  if (oldcom->toklen == 1)
    goto p2009;                    /* J 1-char param (good) */
  (void)write(1, "parameter must be single character", 34);
  REREAD_CMD;
p2009:
  if (!eolok())
    REREAD_CMD;
  strcat(ndel, buf);
  READ_NEXT_COMMAND;
/*
 * FP or ! - DO SHELL COMMAND
 */
p1801:
  if (do_cmd())
  {
    (void)write(1, "bad luck", 8);
    noRereadIfMacro = true;
    goto p1811;                    /* Error has been reported */
  }
  if (!cntrlc)
    READ_NEXT_COMMAND;             /* J no ^C in command */
  newlin();
p1901:
  puts("Quit,\r");
  if (!USING_FILE && curmac < 0)
    cntrlc = false;                /* Forget ^C now unless macro */
  READ_NEXT_COMMAND;
asg2rtn:switch (rtn)
  {
    case 1037:
      goto p1037;
    case 1026:
      goto p1026;
    case 1616:
      goto p1616;
    case 1612:
      goto p1612;
    case 1715:
      goto p1715;
    case 1914:
      goto p1914;
    case 1909:
      goto p1909;
    case 2017:
      goto p2017;
    case 2014:
      goto p2014;
    case 1407:
      goto p1407;
    case 1130:
      goto p1130;
    case 1128:
      goto p1128;
    case 1125:
      goto p1125;
    case 1121:
      goto p1121;
    case 1118:
      goto p1118;
    case 1111:
      goto p1111;
    case 1103:
      goto p1103;
    case 1093:
      goto p1093;
    case 1079:
      goto p1079;
    case 1032:
      goto p1032;
    default:
      printf("Assigned Goto failure, rtn = %d\r\n", rtn);
      return 1;
  }
asg2numok:switch (numok)
  {
    case 1114:
      goto p1114;
    case 1717:
      goto p1717;
    case 1515:
      goto p1114;
    case 1716:
      goto p1716;
    case 1514:
      goto p1514;
    case 1105:
      goto p1105;
    case 10965:
      goto p10965;
    default:
      printf("Assigned Goto failure, numok = %d\r\n", numok);
      return 1;
  }
asg2nonum:switch (nonum)
  {
    case 1114:
      goto p1114;
    case 1718:
      goto p1718;
    case 1716:
      goto p1716;
    case 1514:
      goto p1514;
    case 1106:
      goto p1106;
    case 1097:
      goto p1097;
    default:
      printf("Assigned Goto failure, nonum = %d\r\n", nonum);
      return 1;
  }
}
