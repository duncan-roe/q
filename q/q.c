/* Q
 *
 *
 * Copyright (C) 1981 D. C. Roe
 * Copyright (C) 2002,2007,2012-2021 Duncan Roe
 *
 * Written by Duncan Roe while a staff member & part time student at
 * Caulfield Institute of Technology, Melbourne, Australia.
 * Support from Des Fitzgerald & Associates gratefully acknowledged.
 * Project started 1980.
 *
 * This is the Master Q module */

/* Headers */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <regex.h>
#include <signal.h>
#include <memory.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/types.h>
#include "isacharspecial.h"
#include "prototypes.h"
#include "backtick.h"
#include "cmndcmmn.h"
#include "edmast.h"
#include "macros.h"
#include "q_pipe.h"
#include "fmode.h"
#include "c1in.h"
#include "alu.h"

/* Macros */

#define ERRRTN(x) do {fprintf(stderr, "%s", (x)); return false;} while (0)
#define PIPE_NAME "/tmp/qpipeXXXXXX"
#define REVRSE (fmode & FM_PLUS_R_BIT)
#define PRINTF_IGNORED printf("f%c ignored (mode +v)\r\n", verb)
#define RESET_ARGNO do {if (previous_argno >= 0) argno = previous_argno;\
  return false;}while (0)
#define STDERROUT (curmac < 0 ? stderr : stdout)
#define NUM_RFRAMES 8

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

typedef enum qrc_state
{
  LOCAL,
  HOME,
  ETC,
  GIVE_UP,
} qrc_state;                       /* typedef enum qrc_state */

/* Externals that are not in any header */

scrbuf5 b1, b2, b3, b4;            /* 2 line & 2 command buffers */

/* Instantiate externals */

int tbstat;
bool offline = false;
bool piping = true;                /* Needs to start off true for quthan() */
bool recursing_copy;
int stack_size = 16;               /* Register stack initial depth */
long *rs = NULL;                   /* The register stack */
double *fs = NULL;                 /* The FP register stack */
long xreg = 0;                     /* Index Register */
long qreg = 0;                     /* Q Result Register */
int rsidx = -1;                    /* No current register yet */
int fsidx = -1;                    /* No current FP register yet */
bool index_next = false;
bool xmode_pending = false;
int effaddr;
bool alu_skip = false;
int num_ops = 0;
alu_dict_ent root_alu_dict_ent = { NULL, NULL, -2, 0 };
int *alu_table_index;
bool alu_macros_only = false;      /* N- was N-- */
fmode_t fmode;
char FPformat[40];
char Iformat[40];
char DTformat[256];
long lintot = 0;
long ptrpos = 0;
uint8_t stdoutbuf[Q_BUFSIZ];
uint8_t stderrbuf[Q_BUFSIZ];
uint8_t fxtabl[128];
uint32_t verb, row5, col5;
int ndntch, funit, orig_stdout;
bool vt100, deferd, locerr, noRereadIfMacro, forych, lstvld, modlin, mods;
bool binary, cntrlc, seenwinch;
char pcnta[PTHSIZ], *macro_dir;
bool modify;
uint8_t prmpt[10];
scrbuf5 *newcom, *oldcom, *curr, *prev;
int pchrs, fscode, argc, argno = 0;
int previous_argno = -1;
long lstlin;
char ermess[Q_BUFSIZ], ubuf[Q_BUFSIZ], **argv, *sh;
fmode_t dfltmode = 0;
char *log_name = NULL;
int log_name_len = 0;

/* Static Variables */

static int colonline;              /* Line number from <file>:<line> */
static command_state cmd_state;
static bool verbose_flag = false;  /* -v seen */
static bool very_verbose_flag = false; /* -vv seen */
static long count2;            /* For those commands that take 2 #'s of lines */
static int locpos;              /* Position in line of string found by LOCATE */
static long revpos;                /* Remembered pointer during backwards L */
static bool is_locate;
static bool splt;                  /* Last line ended ^T (for MODIFY) */
static bool display_wanted;
static bool repos;                 /* We are R-REPOSITION (not C-COPY) */
static char xkey[3];               /* 2nd param to FX command */
static char ndel[33];              /* Table of non-delimiters FL & FY */
static int lastpos;                /* Last pos to search (L&Y) */ ;
static int minlen;                 /* Min line length (L&Y) */
static int firstpos;               /* First pos to search (L&Y) */
static long count;
static int i, j, k;                /* Scratch */
static int srch_str_len;
static long i4, j4, k4;            /* Scratch */
static bool tokens;                /* Token search yes/no */
static bool regs;                  /* RE search yes/no */
static char *help_dir;
static char *help_cmd;
static char *etc_dir;
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
static int retcod;                 /* Short-term use */
static bool quiet_flag = false;    /* -q seen */
static bool vrsflg = false;        /* -V seen */
static bool aluflg = false;        /* -A seen */
static qrc_state e_state = LOCAL;
static char *initial_command = NULL;
static bool P, Q;                 /* For determining whether we are in a pipe */
static bool errflg = false;        /* Illegal switch seen */
static bool fq_from_fr;
static bool q_from_fr;
static struct reprompt_frame
{
  bool saved_offline;
  bool saved_piping;
  int fds[3];
  int saved_curmac;
  int saved_mcposn;
  int saved_immdtum;
  int saved_mcdtum;
  int saved_stdbase;
  fmode_t saved_fmode;
}
rframe[NUM_RFRAMES];
static int ridx = -1;
static regex_t preg = { 0 };
static regmatch_t *pmatch;
static size_t pmatch_len = 0;
static bool unmatched_substring;
static int saved_tokbeg;

/* Static prototypes */

static int xmain(int xargc, char **xargv);
static bool close_log_file(void);
static bool open_log_file(char *fn);
static bool do_keylog(void);
static bool get_nmatch(size_t *nmatch);
static bool match_regexp(uint8_t *string, int stringlen, int offset,
  int *matchpos, int *matchlen, size_t nmatch);
static bool compile_regexp(char *regex);
static bool do_freprompt(void);
static bool check_pipe(void);
static void not_pipe(void);
static void do_initial_tsks(bool *do_rc_p);
static bool do_fdevnull(void);
static bool do_fimmediate_macro(void);
static bool do_fmode(void);
static bool do_fnone(void);
static bool do_forget(void);
static bool do_ftokenchar(void);
static bool do_fverbose(void);
static bool do_fxchange(void);
static bool do_goto(void);
static bool do_help(void);
static bool do_insert(void);
static bool do_join(void);
static bool do_locate(void);
static bool do_modify(void);
static bool do_newmacro(void);
static bool do_o_ff_or_fc(fmode_t bit);
static bool do_print(void);
static bool do_quit(bool recursing);
static bool do_reposition(void);
static bool do_shell_command(void);
static bool do_tabset(void);
static bool do_usefile(void);
static bool do_view(void);
static bool do_writefile(void);
static bool do_xistics(void);
static bool do_copy(void);
static bool do_delete(void);
static bool do_enter(void);
static bool do_fbrief(void);
static bool do_append(void);
static void A_I_common(void);
static bool E_Q_common(void);
static void P_V_common(void);
static void J_L_common(void);
static void J_L_M_common(void);
static void display_and_update_file(void);
static bool A_I_M_common(void);
static bool finish_c_or_r(char *which);
static bool get_c_or_r_args(void);
static void report_control_c(void);
static int get_num(bool okzero, long *result);
static bool valid_FX_arg(void);
static void move_cursor_back(int wanted_tokbeg);
static void printf_eof_reached(long ct, char *action_str);
static void print_scanned_lines(long which);
static bool get_search_columns(bool more_allowed);
static bool do_ychangeall(void);
static bool do_zenduse(void);
static bool pushmac(bool set_u_use);
static bool eolok(void);
static q_yesno get_answer(void);
static bool get_file_arg(bool *no_file);
static bool get_opt_lines(long *result);
static bool do_stat_symlink(void);
static int open_buf(int flags, mode_t mode);
static int my_close(int fd);
static bool s_b_w_common_write(void);
static bool do_b_or_s(bool is_b);
static void rm_pipe_temp(void);
static void write_workfile_to_stdout(void);
static void dev_null_stdout(void);
static alu_dict_ent *make_node(int initial_fn_idx);
static void add_op(alu_dict_ent * root, char *opcode);
static void init_alu(void);
static bool bad_rdtk(void);
static void display_opcodes(void);

/* ********************************** main ********************************** */

int
main(int real_argc, char **real_argv)
{
  return xmain(real_argc, real_argv);
}                                  /* main() */

/* ********************************** xmain ********************************* */

static int
xmain(int xargc, char **xargv)
{
  bool recursing = !xargv;         /* In FR-FReprompt */
  bool do_rc = !recursing;
  bool cmd_reread = false;

/* [Re-]initialise static variables */

  recursing_copy = recursing;
  tokens = false;
  repos = false;
  k = 0;
  lastpos = 0;
  minlen = 0;
  firstpos = 0;
  j4 = k4 = 0;
  count2 = 0;

/* Initial Tasks */
  if (recursing)
    printf("\r\nType Q to continue macro %03o; FQ to abandon\r\n", xargc);
  else
  {
    argc = xargc;                  /* Xfer invocation arg to common */
    argv = xargv;                  /* Xfer invocation arg to common */
    do_initial_tsks(&do_rc);
    if (!check_pipe())
      not_pipe();

/* Common initialisation */
    ndel[0] = '\0';                /* No FT commands yet */
    for (i = 127; i >= 0; i--)
      fxtabl[i] = i;               /* No FX commands yet */
    oldcom = &b1;
    newcom = &b2;
    curr = &b3;
    prev = &b4;

/* Set up Screenedit buffers */
    oldcom->bcurs = 0;
    oldcom->bchars = 0;            /* Initialise OLDCOM only this once */
    prev->bchars = 0;
    prev->bcurs = 0;               /* Initialise PREV only this once */
    oldcom->bmxch = BUFMAX;
    newcom->bmxch = BUFMAX;
    curr->bmxch = BUFMAX;
    prev->bmxch = BUFMAX;

    finitl();                      /* Initialise workfile system */
    sinitl();                      /* Initialise screen system */

/* Initially INDENT switched OFF */
    ndntch = 0;                    /* For when it's first switched on */
    tbstat = -1;                   /* Xlation table not set up */
    stdidx = -1;                   /* No U-use file */

    mods = false;                  /* No mods to o/p file yet */
    init5();                       /* Set half duplex &c */
    newlin();                      /* Screen displaying blanks */

    pcnta[0] = 0;                  /* No default filename yet */
    locerr = false;                /* 1st error might not be LOCATE */
    orig_stdout = -1;              /* Haven't dup'd stdout */

/* Invite to type H, but only if taking input */
    if (!offline)
      puts("Type H for help\r");
  }                                /* if (recursing) else */

  cntrlc = false;                  /* Not yet seen ^C */
  splt = false;                    /* Not splitting a line initially */
  noRereadIfMacro = false;
  forych = false;                  /* Not in middle of brief Y */

  if (size5)
    printf("Noted screen dimensions %u x %u\r\n", col5, row5);
/*
 * Use .qrc if it exists here or in $HOME; otherwise use /etc/qrc
 */
  if (do_rc)                       /* (Forced to false if recursing) */
  {
/* Forge a u-use: push current stdin */
    stdidx = stdbase;
    SYSCALL(stdinfo[stdidx].funit, dup(0));
    if (stdinfo[stdidx].funit == -1)
    {
      fprintf(stderr, "\r\n%s. (dup(0))\r\n", strerror(errno));
      refrsh(NULL);
      stdidx--;
    }                              /* if (stdinfo[stdidx].funit == -1) */
    else
    {
      my_close(0);

/* Try for .qrc, ~/.qrc or ... */
      for (;;)
      {
        switch (e_state)
        {
          case LOCAL:
            strcpy(ubuf, ".qrc");
            break;
          case HOME:
            strcpy(ubuf, "~/.qrc");
            tildexpn(ubuf, Q_BUFSIZ);
            break;
          case ETC:
            snprintf(ubuf, sizeof ubuf, "%s%s", etc_dir, "/qrc");
            break;
          case GIVE_UP:
            pop_stdin();

/* The /etc file should exist. so output an error message */
            fprintf(stderr, "%s. %s (open) (Installation problem?)\r\n",
              strerror(errno), ubuf);
            break;
        }                          /* switch (e_state) */
        if (e_state == GIVE_UP)
          break;
        i = open_buf(O_RDONLY, 0);
        if (i != -1)
          break;
        e_state++;
      }                            /* for (;;) */
      if (e_state != GIVE_UP)
      {
        if (i)
        {
          my_close(i);
          fprintf(stderr, "Serious problem - new stdin opened on funit %d\r\n",
            i);
          pop_stdin();
        }                          /* if (i) */
        else
        {
          duplx5(true);            /* Assert XOFF recognition */
          printf("> u %s\r\n", ubuf); /* Simulate a command */
        }                          /* if (i) else */
      }                            /* if (e_state != GIVE_UP) */
    }                              /* if (stdinfo[stdidx].funit == -1) else */
  }                                /* if (do_rc) */

/* MAIN COMMAND READING LOOP */

  for (;;)
  {
    if (cmd_reread)
      cmd_reread = false;
    else
    {
      if ((!USING_FILE && curmac < 0) || cmd_state > LINE_NUMBER_BASE)
      {
        bool printf_wanted = true;
        bool oldcom_done = false;

        switch (cmd_state)
        {
          case TRY_INITIAL_COMMAND:
            cmd_state = RUNNING;
            if (initial_command != NULL)
            {
              oldcom->bchars =
                snprintf((char *)oldcom->bdata, sizeof oldcom->bdata, "fi %s",
                initial_command);
              initial_command = NULL;
              verb = 'i';
              oldcom->bcurs = 3;
              break;
            }                      /* if (initial_command != NULL) */

          case RUNNING:            /* Drop thru */
            sccmnd();              /* Read a command; set VERB */
            printf_wanted = false;
            break;

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
                optind++;          /* "hide" +# arg */
                cmd_state = LINE_NUMBER_SAVED; /* Have a +# arg */
              }                    /* if(**(argv+optind)=='+'&&... */
              else
                cmd_state = TRY_INITIAL_COMMAND;
            }                      /* if (!piping) */

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
            oldcom_done = true;    /* Drop thru */

          case HAVE_LINE_NUMBER:
            if (!oldcom_done)
              oldcom->bchars =
                snprintf((char *)oldcom->bdata, sizeof oldcom->bdata, "g %d",
                colonline);
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
        }                          /* switch(cmd_state) */
        if (printf_wanted)
          printf("> %s\r\n", oldcom->bdata);
      }                           /* if ((!USING_FILE && curmac < 0) || ... ) */
      else
        sccmnd();                  /* Read a command; set VERB */
    }                              /* if (cmd_reread) else */
    retcod = true;
    if (cntrlc)                    /* There has been a ^C or BRK */
    {
      cntrlc = false;              /* Reset flag */
      if (USING_FILE || curmac >= 0) /* If in macro, force an error */
      {
        fputs("Keyboard interrupt", stdout);
        retcod = false;            /* Skip switch */
      }
    }                              /* Else ignore the quit */
    is_locate = false;
    if (retcod)
    {
      switch (verb)
      {
        case 'A':
          retcod = do_append();
          break;

        case 'B':
          retcod = do_b_or_s(true);
          break;

        case 'C':
          retcod = do_copy();
          break;

        case 'D':
          retcod = do_delete();
          break;

        case 'E':
          retcod = do_enter();
          break;

        case 'G':
          retcod = do_goto();
          break;

        case 'H':
          retcod = do_help();
          break;

        case 'I':
          retcod = do_insert();
          break;

        case 'J':
          retcod = do_join();
          break;

        case 'K':
          retcod = do_keylog();
          break;

        case 'L':
        case 'l':                  /* Same as L */
          retcod = do_locate();
          break;

        case 'M':
          retcod = do_modify();
          break;

        case 'P':
          retcod = do_print();
          break;

        case 'Q':                  /* Drop thru */
        case 'q':
          retcod = do_quit(recursing);

/* do_quit() is only called from here, in lieu of lots of inline code. */
/* The following is part of that inline code */
          previous_argno = -1;
          if (q_from_fr)
          {
            q_from_fr = false;
            return 0;              /* To do_freprompt() */
          }                        /* if (q_from_fr) */

          break;

        case 'R':
          retcod = do_reposition();
          break;

        case 'S':
          retcod = do_b_or_s(false);
          break;

        case 'U':                  /* U - USE */
          retcod = do_usefile();
          break;

        case 'V':
          retcod = do_view();
          break;

        case 'W':
          retcod = do_writefile();
          break;

        case 'X':
          retcod = do_xistics();
          break;

        case 'T':
          retcod = do_tabset();
          break;

        case 'Z':
          retcod = do_zenduse();
          break;

        case 'O':
          retcod = do_o_ff_or_fc(04000000000);
          break;

        case 'N':
          retcod = do_newmacro();
          break;

        case 'Y':                  /* Drop thru */
        case 'y':
          retcod = do_ychangeall();
          break;

        case 'b':                  /* FBRIEF */
          retcod = do_fbrief();
          break;

        case 'v':                  /* FVERBOSE */
          retcod = do_fverbose();
          break;

        case 'n':                  /* FNONE */
          retcod = do_fnone();
          break;

        case 'o':                  /* FO - FORGET */
          retcod = do_forget();
          break;

        case '!':                  /* DO SHELL COMMAND */
          retcod = do_shell_command();
          break;

        case 'x':           /* FX - Exchange the functions of 2 keyboard keys */
          retcod = do_fxchange();
          break;

        case 't':                  /* FT - TOKENCHAR */
          retcod = do_ftokenchar();
          break;

        case 'f':
          retcod = do_o_ff_or_fc(01000000000);
          break;

        case 'c':
          retcod = do_o_ff_or_fc(02000000000);
          break;

        case 'm':                  /* "FM"ode */
          retcod = do_fmode();
          break;

        case 'i':                  /* "FI'mmediate macro */
          retcod = do_fimmediate_macro();
          break;

        case 'd':                  /* "FD"evnull */
          retcod = do_fdevnull();
          break;

        case 'r':                  /* "FR"eprompt (calls q recursively) */
          retcod = do_freprompt();
          recursing_copy = recursing; /* do_freprompt() sets recursing */
          if (!retcod)
            break;
          if (fq_from_fr)
          {
            if (recursing)
              return 0;
            fq_from_fr = false;
            notmac(FQ_FROM_FR);
          }                        /* if (fq_from_fr) */
          break;

        default:                   /* Coding error if we get here */
          fputs("unknown command", stderr);
          retcod = false;
          break;
      }                            /* switch (verb) */
    }                              /* if (retcod) */
    if (!retcod)
    {
      if (cmd_state == LINE_NUMBER_SAVED) /* Deal with early errors specially */
      {
        cmd_state = TRY_INITIAL_COMMAND;
        optind--;
      }                            /* if (cmd_state == LINE_NUMBER_SAVED) */
      rerdcm();
      cmd_reread = true;
    }                              /* if (!retcod) */
  }                                /* for(;;) */
}                                  /* xmain() */

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
  scrdtk(1, NULL, 0, oldcom);
  if (oldcom->toktyp == eoltok)    /* OK */
    return true;
  fputs("Too many arguments for this command", stderr);
  return false;
}                                  /* static bool eolok(void) */

/* ******************************* get_answer ******************************* */

/* Parse rest of line for yes / no indication */

static q_yesno
get_answer(void)
{
  if (scrdtk(1, (uint8_t *)ubuf, 6, oldcom))
  {
    fprintf(stderr, "%s. (scrdtk)", strerror(errno));
    return Q_UNREC;
  }                             /* if (scrdtk(1, (uint8_t *)ubuf, 6, oldcom)) */
  if (oldcom->toktyp == eoltok)
    return Q_MAYBE;
  if (oldcom->toktyp != nortok)    /* I.e. null token */
  {
    fputs("Bad parameter for command", stderr);
    return Q_UNREC;
  }                                /* if (oldcom->toktyp != nortok) */
  if (!eolok())
    return Q_UNREC;
  switch (toupper(ubuf[0]))
  {
    case 'O':
      if (toupper(ubuf[1]) == 'N')
        return Q_YES;
      if (toupper(ubuf[1]) == 'F')
        return Q_NO;
      break;
    case 'Y':
    case 'T':
      return Q_YES;
    case 'F':
    case 'N':
      return Q_NO;
  }
  fputs("Parameter not recognised", stderr);
  return Q_UNREC;
}                                  /* get_answer(void) */

/* ****************************** get_file_arg ****************************** */
static bool
get_file_arg(bool *no_file)
{
  if (scrdtk(2, (uint8_t *)ubuf, PTHMAX, oldcom)) /* Read a f/n */
  {
    fprintf(stderr, "%s. (scrdtk)\r\n", strerror(errno));
    return false;
  }
  if (!(*no_file = oldcom->toktyp == eoltok))
  {
    if (oldcom->toktyp == nultok)
      return false;
    tildexpn(ubuf, PTHMAX);        /* Do tilde expansion */
  }                            /* if (!(*no_file = oldcom->toktyp == eoltok)) */
  return true;
}                                  /* get_file_arg() */

/* ****************************** get_opt_lines ***************************** */

static bool
get_opt_lines(long *result)
{
  if (getnum(false, true))         /* Format of optional # of lines OK */
  {
    *result = oldcom->decval;
    if (oldcom->toktyp == eoltok || eolok()) /* EOL already or next */
      return true;
  }                                /* if(getnum()) */
  return false;
}                                  /* get_opt_lines() */

/* ***************************** do_stat_symlink **************************** */

static bool
do_stat_symlink(void)
{
  uint8_t *p;

/* For S B & Q, if the file exists then use its mode from now on. Don't complain
 * here if it doesn't exist. To check whether the file is a symlink, we need to
 * call readlink to find its real name. The only real error here is a symlink
 * loop */

  errno = 0;                       /* Ensure valid */
  if (!stat(ubuf, &statbuf))
  {
    tmode = statbuf.st_mode;
    tgroup = statbuf.st_gid;
    towner = statbuf.st_uid;
  }                                /* if (!stat(ubuf, &statbuf)) */
  else
    tgroup = towner = 0;
  if (!lstat(ubuf, &statbuf) && S_ISLNK(statbuf.st_mode) && errno != ELOOP)
    for (;;)
    {
      if (0 < (i = readlink(ubuf, tmfile, (size_t)PTHSIZ)))
      {                            /* S, B or Q on a symlink */
        tmfile[i] = 0;             /* No trlg NUL from readlink */
        if (tmfile[0] == '/' || tmfile[0] == '~')
          strcpy(ubuf, tmfile);
        else
        {
          p = (uint8_t *)strrchr(ubuf, '/'); /* Find last '/' if any */
          if (!p)
            p = (uint8_t *)ubuf - 1; /* Filename at ubuf start */
          *(p + 1) = '\0';         /* Throw away filename */
          strcat(ubuf, tmfile);    /* Append linked name */
        }                          /* if(tmfile[0]=='/'||tmfile[0]=='~') else */
        printf("Symbolic link resolves to %s", ubuf);
        newlin();
/* See if symlink points to another symlink... */
        if (lstat(ubuf, &statbuf))
          break;                   /* B link to a new file */
        if (!S_ISLNK(statbuf.st_mode))
          break;                   /* B now not on a symlink */
      }                            /* if(0<(i=readlink(ubuf,tmfile,... */
      else
      {
        fprintf(stderr, "%s. %s (readlink)", strerror(errno), ubuf);
        return false;              /* Bad readlink */
      }                            /* if(0<(i=readlink(ubuf,tmfile,... else */
    }                              /* if(!lstat(ubuf,&statbuf)&&S_ISLNK(... */
  if (!(errno || S_ISREG(statbuf.st_mode)))
  {
    fprintf(stderr, "Not a regular file. %s", ubuf);
    return false;
  }                                /* if(!S_ISREG(statbuf.st_mode)) */
  return true;
}                                  /* do_stat_symlink() */

/* ******************************** open_buf ******************************** */

static int
open_buf(int flags, mode_t mode)
{

  SYSCALL(retcod, open(ubuf, flags, mode));
  return retcod;
}                                  /* open_buf() */

/* ******************************** my_close ******************************** */

static int
my_close(int fd)
{

  SYSCALL(retcod, close(fd));
  return retcod;
}                                  /* my_close() */

/* *************************** s_b_w_common_write *************************** */

static bool
s_b_w_common_write(void)
{
  if ((funit = open_buf(rdwr, tmode)) == -1)
  {
    fprintf(stderr, "%s. %s (open)", strerror(errno), ubuf);
    return false;                  /* Bad open */
  }                             /* if ((funit = open_buf(rdwr, tmode)) == -1) */
  if ((fscode = fstat(funit, &statbuf)))
    fprintf(stderr, "%s. funit %d (fstat)", strerror(errno), funit);
  else if (ismapd(statbuf.st_ino))
    fprintf(stderr, "%s is mmap'd", ubuf), fscode = 1;
  else
  {
    SYSCALL(fscode, ftruncate(funit, 0));
    if (fscode)
      fprintf(stderr, "%s. funit %d (ftruncate)", strerror(errno), funit);
  }                                /* else */
  if (fscode == 0)
    writfl(wrtnum);                /* Write lines to o/p file */
  else                             /* Some kind of failure above */
    my_close(funit);
  return fscode == 0;
}                                  /* s_b_w_common_write() */

/* ****************************** do_b_or_s ********************************* */

static bool
do_b_or_s(bool is_b)
{
  bool new_bkup_file = false;

  savpos = ptrpos;                 /* So we can leave pos'n same at end */
  setptr(1);                       /* Pos'n 1st line */
  if (deferd)
    dfread(LONG_MAX, NULL);        /* Ensure all file in */
  wrtnum = lintot;                 /* Write all lines */
  rdwr = O_WRONLY + O_CREAT;       /* Don't truncate yet in case mmap'd */
  if (!get_file_arg(&nofile))
    ERRRTN("Error in filename");
  if (!nofile)                     /* B or S with param */
  {
    if (!do_stat_symlink() || !eolok())
      return false;

/* B-BACKUP will back up supplied param if it exists, but not now */
    if (!is_b && !s_b_w_common_write())
      return false;
  }                                /* if (!nofile) */

  if (nofile || is_b)
  {
    if (nofile)
    {
      if (!pcnta[0])               /* We have no default f/n */
        ERRRTN("filename must be specified");
      strcpy(ubuf, pcnta);         /* ubuf has b f/n if !nofile */
    }                              /* if (nofile) */

/* Use .tm or .bu for name of backup file */
    if (snprintf(tmfile, sizeof tmfile, "%s.%s", ubuf,
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
        fputs("Previous backup file deleted:- ", stdout);
    }                              /* while (!stat(tmfile, &statbuf)) */
    if (rename(ubuf, tmfile))      /* If rename fails */
    {
/* No such file is OK if B with param */
      if (!nofile && errno == ENOENT)
      {
        puts("New file - no backup taken\r");
        new_bkup_file = true;
      }                            /* if (!nofile && errno == ENOENT) */
      else
      {
        fprintf(stderr, "%s. %s to %s (rename)", strerror(errno), ubuf, tmfile);
        my_close(funit);           /* In case anything left open */
        return false;              /* Get corrected command */
      }                            /* if (!nofile && errno == ENOENT) else */
    }                              /* if (rename(ubuf, tmfile)) */
/*
 * File renamed - now open new file of same type as original
 */
    if (!new_bkup_file)
    {
      rdwr = O_WRONLY + O_CREAT + O_EXCL; /* Must be new file */
      if (is_b)
        printf("Backup file is %s\r\n", tmfile); /* Report backup f/n */
    }                              /* if (!new_bkup_file) */
    if (!s_b_w_common_write())
      return false;
  }                                /* if(nofile) */
  setptr(savpos);                  /* Repos'n file as before */
  if (!nofile)
    strcpy(pcnta, ubuf);           /* We had a param. Set as dflt */
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
      puts("Warning - original mode not restored\r");
  }                                /* if (towner) */
  mods = false;                    /* S or B succeeded */
  return true;
}                                  /* do_b_or_s() */

/* ****************************** rm_pipe_temp ****************************** */

static void
rm_pipe_temp(void)
{

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

/* Before changing signal handlers, delete the temp file */
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
  setptr(1);                       /* Pos'n 1st line */
  if (deferd)
    dfread(LONG_MAX, NULL);        /* Ensure all file in */
  funit = saved_pipe_stdout;
  writfl(lintot);
}                                  /* write_workfile_to_stdout() */

/* ***************************** dev_null_stdout **************************** */

static void
dev_null_stdout(void)
{

  if (my_close(STDOUT5FD))
  {
    fprintf(stderr, "%s. fd %d (close)\n", strerror(errno), STDOUT5FD);
    exit(1);
  }                                /* if (my_close (STDOUT5FD)) */
  SYSCALL(retcod, open("/dev/null", O_WRONLY));
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
  char thisch = toupper(*(uint8_t *)opcode);

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
  int g;

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
  strcpy(Iformat, "%ld");

/* Set up initial date format */
  strcpy(DTformat, "%F %T %z");

/* Set up indicies and lookup dictionary */
  for (i = 0, g = 0; i < num_alu_opcode_table_entries; i++)
    if (opcode_defs[i].func)
    {
      alu_table_index[g++] = i;
      add_op(&root_alu_dict_ent, opcode_defs[i].name);
    }                              /* if (opcode_defs[i].func) */
}                                  /* init_alu() */

/* ******************************** bad_rdtk ******************************** */

static bool
bad_rdtk(void)
{
  fprintf(stderr, "%s. (scrdtk)", strerror(errno));
/* Most callers were returning false next so let them return bad_rdtk instead */
  return false;
}                                  /* bad_rdtk(void) */

/* ***************************** display_opcodes **************************** */
static void
display_opcodes(void)
{
  char tbuf[16];
  char *p;

  puts("\r\n"
    "\t Instructions to Access Tabs\r\n"
    "\t ============ == ====== ====\r\n"
    "PSHTAB x Push value of tab x to R\r\n"
    "\t (x is a tab ID; type of tab is not examined)\r\n"
    "POPTAB x Pop R to set value of tab x;\r\n"
    "\t (x is a tab ID; type of tab is from last SCPT / SFPT)\r\n"
    "\r\n"
    "\t Memory Reference Instructions\r\n"
    "\t ====== ========= ============\r\n"
    "PSH  xxx Push contents of N7xxx to R\r\n"
    "POP  xxx Pop R to define N7xxx\r\n"
    "PSHF xxx Push contents of N13xxx to F\r\n"
    "POPF xxx Pop F to define N13xxx\r\n");

  for (i = 0; i < num_alu_opcode_table_entries; i++)
  {
    if (opcode_defs[i].func)
    {
      strcpy(tbuf, opcode_defs[i].name);
      for (p = tbuf; *p; p++)
        *p = toupper(*(uint8_t *)p);
      printf("%s\t %s\r\n", tbuf, opcode_defs[i].description);
    }                              /* if (opcode_defs[i].func) */
    else
      printf("\t %s\r\n", opcode_defs[i].description);
  }                     /* for (i = 0; i < num_alu_opcode_table_entries; i++) */
}                                  /* display_opcodes() */

/* ******************************* do_zenduse ******************************* */

static bool
do_zenduse(void)
{
  if (!eolok() || !pop_stdin())
    return false;

/* If U-use was in a macro, resume that macro */
  if (stdinfo[stdidx + 1].frommac)
  {
    mcnxfr--;
    curmac = mcstck[mcnxfr].mcprev;
    mcposn = mcstck[mcnxfr].mcposn;
  }                                /* if (stdinfo[stdidx + 1].frommac) */
  return true;
}                                  /* bool do_zenduse(void) */

/* ****************************** do_ychangeall ***************************** */

static bool
do_ychangeall(void)
{

/* Y or FY - Change all occurrences of 1 string to another */

  bool linmod;                     /* This line modified */
  int yposn;                       /* How far we are along a line */
  int ydiff = 0;                   /* = 0 not needed but compiler can't tell */
  int oldlen, newlen;
  char oldstr[Q_BUFSIZ], newstr[Q_BUFSIZ];
  bool lines_changed = false;
  int n;                           /* #chars at end not to search */
  int match_start;
  int match_end;
  size_t nmatch;

  tokens = verb == 'y';            /* Differentiate fy */
  unmatched_substring = false;
  saved_tokbeg = 0;
  if (tokens)
    regs = false;
  else
    regs = (fmode & FM_PLUS_G_BIT) != 0;
  if (scrdtk(2, (uint8_t *)oldstr, BUFMAX, oldcom))
    return bad_rdtk();
  if (oldcom->toktyp == eoltok || !(oldlen = oldcom->toklen))
  {
    fputs("Null string to replace", stderr);
    return false;
  }

/* Get string with which to replace it */
  if (scrdtk(2, (uint8_t *)newstr, BUFMAX, oldcom))
    return bad_rdtk();
  newlen = oldcom->toklen;

/* RE does this once matched length for current line is known */
  if (!regs)
  {
    ydiff = newlen - oldlen;

/* strings must be equal length if Fixed-Length mode */
    if (ydiff && fmode & FM_PLUS_F_BIT)
    {
      fputs("Replace string must be same length in FIXED LENGTH mode", stderr);
      return false;
    }
  }                                /* if (!regs) */

  if (oldcom->toktyp == eoltok)
    j4 = 1;
  else
  {
    if (getlin(false, false))
      j4 = oldcom->decval;
    else
    {
      if (oldcom->toktyp == nortok)
      {
        fprintf(stderr, "%s", ermess);
        return false;
      }                            /* if (oldcom->toktyp == nortok) */
      j4 = 1;                      /* Start looking at line 1 */
    }                              /* if (getlin(false, false)) else */
    if (!getnum(false, true))      /* Get # lines, 0 not allowed */
      return false;
  }                                /* if (oldcom->toktyp == eoltok) else */
  if (oldcom->toktyp != nortok)    /* No number given */
    count = deferd ? LONG_MAX : lintot + 1 - j4; /* Process to eof */
  else
    count = oldcom->decval;
  lstlin = -1;                     /* -TO not allowed for column pos'ns */
  srch_str_len = regs ? 0 : oldlen; /* Used by get_search_columns */
  if (!get_search_columns(regs))   /* Look for 1st & last pos'ns in line */
    return false;
  if (!lintot && !(deferd && (dfread(1, NULL), lintot))) /* Empty file */
  {
    fputs("Empty file - can't changeall any lines", stderr);
    return false;
  }

  if (regs && !(get_nmatch(&nmatch) && compile_regexp(oldstr)))
    return false;

/* We act on BRIEF or NONE if in a macro without question. Otherwise, BRIEF or
 * NONE is queried, and we reset to VERBOSE if we don't get confirmation */
  if (curmac < 0 && BRIEF &&
    !ysno5a("Use brief/none in this command (y,n,Cr [n])", A5DNO))
  {
    puts("Reverting to verbose\r");
    fmode &= ~FN_CMD_BITS;
  }

  savpos = ptrpos;                 /* Remember so we can get back */
  setptr(j4);                      /* First line to look at */
  lines_changed = false;
/*
 * Main loop on specified lines
 */
  for (i4 = count; i4 > 0; i4--)
  {
    if (cntrlc)                    /* User has interrupted */
    {
      cntrlc = false;              /* ^C noticed */
      fputs("Command abandoned :-", stdout);
      print_scanned_lines(count);
      return true;                 /* Leave Y */
    }                              /* if (cntrlc) */
    if (!rdlin(curr, false))       /* Eof on main pointer */
    {
      if (count == LONG_MAX)       /* Was going to deferred eof */
        break;                     /* for(i4=count;i4>0;i4--) */
      printf_eof_reached(count, "scanned");
      newlin();
      setptr(savpos);              /* Restore file pos'n */
      return true;                 /* Leave Y */
    }

/* Initial tasks for each line */
    yposn = firstpos;              /* Search from 1st position spec'd */
    linmod = false;                /* No match this line yet */
    if (curr->bchars < minlen)
      continue;                    /* J line shorter than minimum */
/* n stays valid as curr->bchars changes with replacements */
/* along the line */
    n = curr->bchars > lastpos ? curr->bchars - lastpos : 0;

    do
    {
      if (tokens)
        retcod =
          ltok5a((uint8_t *)oldstr, oldlen, curr->bdata, yposn,
          curr->bchars - n, &match_start, &match_end, (uint8_t *)ndel);
      else if (regs)
        retcod = match_regexp
          (curr->bdata, curr->bchars, yposn, &match_start, &oldlen, nmatch);
      else
        retcod =
          lsub5a((uint8_t *)oldstr, oldlen, curr->bdata, yposn,
          curr->bchars - n, &match_start, &match_end);
      if (!retcod)
        break;
      else if (regs)
      {
        ydiff = newlen - oldlen;
        match_end = match_start + oldlen - 1;
        if (ydiff && fmode & FM_PLUS_F_BIT)
        {
          fputs("Replace string must be same length in FIXED LENGTH mode",
            stderr);
          return false;
        }                          /* if (ydiff && fmode & FM_PLUS_F_BIT) */
      }                            /* else if (regs) */
      if (curr->bchars + ydiff > curr->bmxch) /* Would exceed line capacity */
      {
        savpos = ptrpos - 1;       /* Point to too big line */
        fputs("Next line would exceed max size:-", stdout);
        print_scanned_lines(count);
        return true;
      }                            /* if (curr->bchars + ydiff > curr->bmxch) */
      memmove(&curr->bdata[match_end + 1 + ydiff], &curr->bdata[match_end + 1],
        curr->bchars - match_end - 1);

/* Move in new string if not null */
      if (newlen != 0)
        memcpy(&curr->bdata[match_start], newstr, newlen);
      curr->bchars = curr->bchars + ydiff; /* Get new line length */
      linmod = true;               /* This line has been modified */
      yposn = match_end + 1 + ydiff; /* Resume search after new string */

/* Seek more occurrences if room */
    }                              /* do */
    while (curr->bchars - n - yposn >= (regs ? 0 : oldlen));

    if (!linmod)
      continue;                    /* J no mods to this line */
/*  linmod = false;                 * (done at loop start) */
    lines_changed = true;
    if (!NONE)                     /* Some display may be req'd */
    {
      bool want_display = true;
      double t;

/* If BRIEF, only display every fbrief_interval secs */
      if (BRIEF)                   /* only display if time to (or 1st) */
      {
        if ((t = time_now()) - timlst < fbrief_interval)
          want_display = false;
        else
        {
          timlst = t;              /* Displaying */
          forych = true;           /* Tell PDSPLY short display */
        }            /* if ((t = time_now()) - timlst < fbrief_interval) else */
      }                            /* if(BRIEF) */
      if (want_display)
      {
        sprmpt(ptrpos - 1);        /* set up line # */
        pdsply(curr, prmpt, pchrs); /* Display the modified line */
      }                            /* if (want_display) */
    }                              /* if(!NONE) */
    delete(false, 1, false);       /* Remove old version of line */
    inslin(curr);                  /* Insert new version */
  }                                /* for(i4=count;i4>0;i4--) */
  setptr(savpos);                  /* End Y */
  if (!lines_changed)
  {
    if (curmac < 0 || !BRIEF)
      fprintf(STDERROUT, "Specified %s not found",
        unmatched_substring ? "sub-expression" : (regs ? "pattern" : "string"));
    locerr = true;                 /* Picked up by RERDCM */
    move_cursor_back(saved_tokbeg);
    return false;
  }                                /* if (!lines_changed) */
  return true;
}                                  /* bool do_ychangeall(void) */

/* *************************** get_search_columns *************************** */

static bool
get_search_columns(bool more_allowed)
{
  if (!getnum(false, false))       /* Get 1st pos to search */
    return false;
  firstpos = oldcom->decval - 1;   /* Columns start at 0 */
  if (!getnum(false, false))       /* Get last pos'n */
    return false;
  if (oldcom->toktyp != nortok)
    lastpos = BUFMAX;              /* BUFMAX does not include trlg null */
  else
  {
    lastpos = oldcom->decval - 1;  /* Last start position */
    if (lastpos < firstpos)        /* Impossible combination of columns */
    {
      fputs("Last pos'n < first\r", stderr);
      return false;
    }                              /* if(lastpos < firstpos) */
    lastpos += srch_str_len;       /* Add search length to get wanted length */
  }                                /* if (oldcom->toktyp != nortok) else */
/* --------------------------------------------------------------------- */
/* FNDTN MACRO BUG                                                       */
/* Comment on next line was split with ^N6, so next line had no slash-*  */
/* and following line had no star-slash. Comment was lined up micely,    */
/* but N1 pushed it to the right. It should have left that line alone.   */
/* Progress:                                                             */
/* This is not caused by N1. GNU indent doesn't like the single space    */
/* between semicolon and slash-star, so inserts some extra spaces.       */
/* It then (helpfully, not) recognises and indents the comment           */
/* continaution. N1 will have to clean this up.                          */
/* THIS IS NOW DONE                                                      */
/* Subsidiary Issue:  (NEEDS ACTION)                                     */
/* After line lengthened beyond cc 80, N1 moved unterminated comment     */
/* fragment to before that line, causing line to be not executed         */
/* with no gcc warning. N1 needs to action this specially (FR perhaps?)  */
/* THIS IS NOW DONE (with FR)                                            */
/* --------------------------------------------------------------------- */

/* Get minimum line length to search (can't determine for RE search) */
  minlen = regs ? 0 : firstpos + srch_str_len;

  if (!more_allowed && !eolok())
    return false;
  return true;
}                               /* bool get_search_columns(bool more_allowed) */

/* *************************** print_scanned_lines ************************** */

static void
print_scanned_lines(long which)
{
  printf(" %ld lines ", which - i4);
  fputs("scanned", stdout);
  newlin();
  setptr(savpos);                  /* Restore file pos'n */
}                                  /* void print_scanned_lines(void) */

/* *************************** printf_eof_reached *************************** */

static void
printf_eof_reached(long ct, char *action_str)
{
  i4 = ct - i4 + 1;                /* Lines not actioned */
  printf("%s of file reached:- ", REVRSE ? "start" : "end");
  printf("%ld line%s ", i4 - 1, i4 == 2 ? "" : "s");
  printf("%s\r", action_str);
}                       /* void printf_eof_reached(long ct, char *action_str) */

/* **************************** move_cursor_back **************************** */

static void
move_cursor_back(int wanted_tokbeg)
{
/* Reset screen cursor */
  scrdtk(5, NULL, 0, oldcom);

/* Move past command & 1st param */
  scrdtk(1, NULL, 0, oldcom);
  scrdtk(1, NULL, 0, oldcom);

/* Move on to wanted token, if specified */
  if (wanted_tokbeg)
    while (oldcom->tokbeg < wanted_tokbeg && oldcom->toktyp != eoltok)
      scrdtk(1, NULL, 0, oldcom);
}                                  /* move_cursor_back() */

/* ****************************** valid_FX_arg ****************************** */

static bool
valid_FX_arg(void)
{
  if (oldcom->toklen == 0)
  {
    fputs("Null argument not allowed", stderr);
    return false;
  }                                /* if (oldcom->toklen == 0) */
  if (oldcom->toklen != 2)         /* Can't be ^<char> */
  {
    k = xkey[0];                   /* Convert to subscript */
    if (k >= 128)
    {
      fputs("parity-high \"keys\" not allowed", stderr);
      return false;
    }                              /* if (k >= 128) */
  }                                /* if (oldcom->toklen != 2) */
  else
  {
    if (xkey[0] != CARAT)
    {
      fputs("may only have single char or ^ char", stderr);
      return false;
    }                              /* if (xkey[0] != CARAT) */
    k = xkey[1];                   /* Isolate putative control */
    if (k >= 0100 && k <= 0137)    /* A real control char */
      k = k - 0100;                /* Control char to subscript */
    else if (k == ASTRSK)          /* ^* (real uparrow) */
      k = CARAT;
    else if (k == QM)              /* ^? (Rubout) */
      k = 127;
    else
    {
      fputs("Illegal control character representation", stderr);
      return false;
    }
  }                                /* if (oldcom->toklen != 2) else */
  if (k > 31 && k != 127)
    puts("Warning: non-control char as argument\r");
  return true;
}                                  /* bool valid_FX_arg(void) */

/* ********************************* get_num ******************************** */

static int
get_num(bool okzero, long *result)
/* Returns: 1: no number found and 1 returned in result
 *          0: number found and returned in result
 *         -1: error */
{
  if (!getnum(okzero, true))
    return -1;                     /* Format err on # of lines */
  *result = oldcom->decval;        /* getnum() assures validity */
  return oldcom->toktyp != nortok;
}                                  /* int get_num(bool okzero, long *result) */

/* **************************** report_control_c **************************** */

static void
report_control_c(void)
{
  puts("Quit,\r");
  if (!USING_FILE && curmac < 0)
    cntrlc = false;                /* Forget ^C now unless macro */
}                                  /* void report_control_c(void) */

/* ***************************** get_c_or_r_args **************************** */

static bool
get_c_or_r_args(void)
{
  if (!getlin(true, false))        /* Bad source */
  {
    fputs(" in source line", stderr);
    return false;
  }
  k4 = oldcom->decval;             /* Remember source */
  if (!getlin(true, true))         /* Bad dest'n */
  {
    fputs(" in dest'n line", stderr);
    return false;
  }
  j4 = oldcom->decval;             /* Remember dest'n */
  lstlin = k4;                     /* -TO refers from source line */
  if (j4 == k4)                    /* Error if equal */
  {
    fputs("Source and destination line #'s must be different", stderr);
    return false;
  }
  return true;
}                                  /* bool get_c_or_r_args(void) */

/* ****************************** finish_c_or_r ***************************** */

static bool
finish_c_or_r(char *which)
{
  if (!get_opt_lines(&count))
    return false;
  setptr(j4);                      /* Set main ptr at dest'n */

/* Note:- When setting both pointers, always set AUX second */
  setaux(k4);                      /* Set AUX ptr at source */

  for (i4 = count; i4 > 0; i4--)
  {
    if (rdlin(prev, true))         /* Read AUX */
    {
      if (repos)
        delete(true, 1, false);    /* For reposition only, delete line read */
      inslin(prev);
    }                              /* if (rdlin(prev, true)) */
    else
    {
      printf_eof_reached(count, which);
      newlin();
      break;
    }                              /* if (rdlin(prev, true)) else */
  }
  return true;                     /* Finished C or R */
}                                  /* bool finish_c_or_r(char *which) */

/* ****************************** A_I_M_common ****************************** */

static bool
A_I_M_common(void)
{
  bool repeat;

  do
  {
/* Get user out of INSERT/APPEND/MODIFY if ^C has been typed */
    if (cntrlc)
    {
      report_control_c();
      return true;
    }                              /* if (cntrlc) */
    repeat = false;
    scrdit(curr, prev, (char *)prmpt, pchrs); /* Edit the line */
    display_wanted = curmac < 0 || !BRIEF;
    switch (verb)                  /* Check EOL type */
    {
      case 'J':
        display_and_update_file();
        break;

/* ESC. If we were changing an existing line, display the original */
      case '[':
        if (modify || splt)
        {
          if (display_wanted)
          {
            setptr(ptrpos - 1);
            rdlin(prev, false);
            disply(prev, false);
          }                        /* if(display_wanted) */
          splt = false;
        }                          /* if(modify||splt) */
        if (REVRSE && is_locate)   /* "L"ocate backwards */
          setptr(revpos);
        return true;

      case 'T':
        display_and_update_file();
        splt = true;               /* Force a delete next time */
        inslin(curr);              /* In case ESC next time */
        sprmpt(ptrpos - 1);
        repeat = true;             /* New line for all 3 (A,I,M) */
        break;

      default:
        fputs("Internal error - EOL char not recognised", stderr);
        newlin();
        return true;
    }                              /* switch (verb) */
  }                                /* do */
  while (repeat);
  return false;
}                                  /* bool A_I_M_common(void) */

/* ************************* display_and_update_file ************************ */

static void
display_and_update_file(void)
{
  if ((modify || splt) && modlin)
    delete(false, 1, false);       /* Delete CHANGED existing line */
  if (display_wanted)
    disply(prev, false);           /* Display final line */
  if (!(modify || splt) || modlin)
    inslin(prev);                  /* Insert changed or new line */
  splt = false;                    /* Not a split any more */
}                                  /* void display_and_update_file(void) */

/* ****************************** J_L_M_common ****************************** */

static void
J_L_M_common(void)
{
  for (i = count; i > 0; i--)
  {
    if (!rdlin(curr, false))       /* Get lin to mod / EOF */
    {
      puts("E - O - F\r");
      return;
    }                              /* if(!rdlin(curr, false)) */
    if (is_locate && !EXCLUSIVE_L_BOOL)
      curr->bcurs = locpos;        /* Set cursor to start of found string */
    sprmpt(ptrpos - 1);            /* Set up prompt lin # just read */
    if (A_I_M_common())
      return;
  }
/* If doing a reverse locate, move pointer back to where locate was, ready for
 * the next one. Also has to be done after Ec) */
  if (is_locate && REVRSE)
    setptr(revpos);
}                                  /* void J_L_M_common(void) */

/* ******************************* J_L_common ******************************* */

static void
J_L_common(void)
{
  lstvld = false;                  /* Previous line not valid */
  modify = true;                   /* For "M"odify code */
  J_L_M_common();
}                                  /* void J_L_common(void) */

/* ******************************* P_V_common ******************************* */

static void
P_V_common(void)
{
  for (i = count; i > 0; i--)
  {
    if (!rdlin(curr, false))       /* EOF. Normal ptr */
    {
      puts(" E - O - F\r");
      break;
    }
    sprmpt(ptrpos - 1);
    pdsply(curr, prmpt, pchrs);
    if (cntrlc || (i > 1 && kbd5())) /* User wants out */
    {
      newlin();
      puts(" *** keyboard interrupt *** \r");
      cntrlc = false;
      break;
    }
  }
}                                  /* void P_V_common(void) */

/* ******************************* E_Q_common ******************************* */

static bool
E_Q_common(void)
{
  uint8_t *p;                      /* Scratch */

  savpos = ptrpos;                 /* Return here with file open. read it... */

/* Use mmap if requested */

  if (fmode & FM_PLUS_M_BIT && (verb != 'E' || fmode & FM_PLUS_N_BIT))
  {
    if (fstat(funit, &statbuf))
    {
      fprintf(stderr, "%s. funit %d (fstat)", strerror(errno), funit);
      my_close(funit);             /* Bad fstat */
      return false;
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
        return false;
      }                            /* if(!(p=mmap(NULL,statbuf.st_size... */
      mapfil(statbuf.st_ino, statbuf.st_size, p);
    }                              /* if(statbuf.st_size) */
    else
      puts("0 lines read.\r");
  }                                /* if(fmode & FM_PLUS_M_BIT &&... */
  else
    readfl();
  if (my_close(funit))             /* Failure */
    fprintf(stderr, "%s. funit %d (close)", strerror(errno), funit);
  setptr(savpos);
  return true;
}                                  /* bool E_Q_common(void) */

/* ******************************* A_I_common ******************************* */

static void
A_I_common(void)
{
  modify = false;
  lstvld = false;                  /* Previous line not valid */
  for (;;)
  {
    curr->bchars = 0;
    curr->bcurs = 0;               /* Set up new empty line */
    sprmpt(ptrpos);                /* PROMPT = # of new line */
    if (A_I_M_common())
      return;
  }                                /* for (;;) */
}                                  /* void A_I_common(void) */

/* IMPLEMENT Q COMMANDS (In alphabetical order) */

/* ******************************** do_append ******************************* */

static bool
do_append(void)
{
  if (!eolok())
    return false;
  if (deferd)                      /* File not all read in yet */
    dfread(LONG_MAX, NULL);
  setptr(lintot + 1);              /* Ptr after last line in file */
  A_I_common();
  return true;
}                                  /* bool do_append(void) */

/* ********************************* do_copy ******************************** */

static bool
do_copy(void)
{
  repos = false;                   /* C-COPY, not R-REPOS */
  if (!get_c_or_r_args())
    return false;
  if (finish_c_or_r("copied"))
    return true;                   /* End C */
  return false;
}                                  /* bool do_copy(void) */

/* ******************************** do_delete ******************************* */

static bool
do_delete(void)
{
  if (!getlin(true, false))
    return false;                  /* J bad line # */
  k4 = oldcom->decval + 1;         /* Pos here for delete */
  if (!get_opt_lines(&count))
    return false;
  if (k4 - 2 + count > lintot)
  {
    if (deferd)
      dfread(k4 - 2 + count - lintot, NULL);
    if (k4 - 2 + count > lintot)
    {
      count = lintot - k4 + 2;
      printf("end of file reached:- %ld line%s deleted\r", count,
        count == 1 ? "" : "s");
      newlin();
    }                              /* if (k4 -2 + count >lintot) */
  }                                /* if (k4 -2 + count >lintot) */
  setptr(k4);                      /* Pos 1 past 1st line to go */
  delete(false, count, true);      /* Knock off line(s). Use normal ptr */
  return true;                     /* Finished if get here */
}                                  /* bool do_delete(void) */

/* ******************************** do_enter ******************************** */

static bool
do_enter(void)
{
  if (!get_file_arg(&nofile) || nofile)
    ERRRTN("Error in filename");
  if (!eolok())
    return false;
  if ((funit = open_buf(O_RDONLY, 0)) == -1)
  {
    fprintf(stderr, "%s. %s (open)", strerror(errno), ubuf);
    return false;                  /* Bad open */
  }                         /* if ((funit = open_buf(O_RDONLY, tmode)) == -1) */
  return E_Q_common();
}                                  /* bool do_enter(void) */

/* ******************************** do_fbrief ******************************* */

static bool
do_fbrief(void)
{
  if (!eolok())
    return false;
  if (!(fmode & FM_PLUS_V_BIT))
  {
    fmode &= ~FN_CMD_BITS;
    fmode |= FB_CMD_BIT;
  }                                /* if (!(fmode & FM_PLUS_V_BIT)) */
  else
    PRINTF_IGNORED;
  return true;                     /* Finished */
}                                  /* bool do_fbrief(void) */

/* ******************************* do_fdevnull ****************************** */

static bool
do_fdevnull(void)
{
  if (very_verbose_flag)           /* FD ineffective in q -vv */
  {
    PRINTF_IGNORED;
    return true;
  }                                /* if (very_verbose_flag) */
  switch (get_answer())
  {
    case Q_UNREC:
      return false;
    case Q_MAYBE:
    case Q_NO:
      restore_stdout();
      break;
    case Q_YES:
      if (USING_FILE)
      {
        devnull_stdout();
        break;
      }                            /* if (USING_FILE) */
      else
      {
        fputs("fd y is not available from the keyboard", stderr);
        return false;
      }
  }                                /* switch (get_answer()) */
  return true;
}                                  /* bool do_fdevnull(void) */

/* *************************** do_fimmediate_macro ************************** */

static bool
do_fimmediate_macro(void)
{
  if (scrdtk(4, (uint8_t *)ubuf, BUFMAX, oldcom))
  {
    perror("SCRDTK of macro text");
    fprintf(stderr, "\rUnexpected error");
    return false;
  }
/* Decide which macro this will be. */
/* Some nesting of FI macros is allowed, */
/* to support e.g. nested U-use files which contain FI cmds */
  if (immnxfr > LAST_IMMEDIATE_MACRO)
  {
    fputs("Too many nested FI commands", stderr);
    return false;
  }
  verb = immnxfr++;
  if (!newmac2(true))
    return false;

/* FI does an implied ^ND */
  if (curmac >= 0 && !pushmac(false))
    return false;

  curmac = verb;
  mcposn = 0;
  return true;
}                                  /* bool do_fimmediate_macro(void) */

/* ******************************** do_fmode ******************************** */

static bool
do_fmode(void)
{
  return setqmode();
}                                  /* bool do_fmode(void) */

/* ******************************** do_fnone ******************************** */

static bool
do_fnone(void)
{
  if (!eolok())
    return false;
  if (fmode & FM_PLUS_V_BIT)
    PRINTF_IGNORED;
  else
    fmode |= FN_CMD_BITS;
  return true;                     /* Finished */
}                                  /* bool do_fnone(void) */

/* ******************************** do_forget ******************************* */

static bool
do_forget(void)
{
  if (!eolok())
    return false;
  forget();                        /* Implemented by workfile */
  return true;
}                                  /* bool do_forget(void) */

/* ****************************** do_ftokenchar ***************************** */

static bool
do_ftokenchar(void)
{
  if (strlen(ndel) >= 32)
  {
    fputs("no room for further entries", stderr);
    return false;
  }
  if (scrdtk(1, (uint8_t *)ubuf, 40, oldcom)) /* Get character to add */
    return bad_rdtk();
  if (oldcom->toktyp == eoltok)
  {
    fputs("command requires a parameter", stderr);
    return false;
  }
  if (oldcom->toklen != 1)
  {
    fputs("parameter must be single character", stderr);
    return false;
  }
  if (!eolok())
    return false;
  strcat(ndel, ubuf);
  return true;
}                                  /* bool do_ftokenchar(void) */

/* ******************************* do_fverbose ****************************** */

static bool
do_fverbose(void)
{
  if (!eolok())
    return false;
  fmode &= ~FN_CMD_BITS;
  return true;                     /* Finished */
}                                  /* bool do_fverbose(void) */

/* ******************************* do_fxchange ****************************** */

static bool
do_fxchange(void)
{
  char oldkey[3];                  /* 1st param to FX command */
  char newkey[3];                  /* 2nd param to FX command */

  if (scrdtk(2, (uint8_t *)oldkey, 3, oldcom))
    return bad_rdtk();
  if (oldcom->toktyp == eoltok)    /* Empty line */
  {
/* FX with no params - Re-initialise the table, subject to
 *                     confirmation */
    if (!ysno5a("Re-initialise keyboard table to default - ok", A5NDEF))
      return true;                 /* Finish if changed his mind */
    for (i = 127; i >= 0; i--)
      fxtabl[i] = i;               /* No FX commands yet */
    return true;                   /* Finished after reset */
  }                                /* if (oldcom->toktyp != eoltok) */
  xkey[0] = oldkey[0];
  xkey[1] = oldkey[1];
  if (!valid_FX_arg())
    return false;
  oldkey[0] = k;                   /* Now a subscript */
  if (scrdtk(2, (uint8_t *)newkey, 3, oldcom))
    return bad_rdtk();
  if (oldcom->toktyp == eoltok)
  {
    fputs("FX must have 2 parameters", stderr);
    return false;
  }
  xkey[0] = newkey[0];
  xkey[1] = newkey[1];
  if (!valid_FX_arg())
    return false;
  newkey[0] = k;                   /* Now a subscript */
  if (!eolok())
    return false;
  i = fxtabl[(int)oldkey[0]];
  fxtabl[(int)oldkey[0]] = fxtabl[(int)newkey[0]];
  fxtabl[(int)newkey[0]] = i;
  return true;
}                                  /* bool do_fxchange(void) */

/* ********************************* do_goto ******************************** */

static bool
do_goto(void)
{
  if (getlin(true, true) && eolok())
  {
    setptr(oldcom->decval);
    return true;                   /* Finished GOTO */
  }                                /* if(getlin(true, ... */
  return false;
}                                  /* bool do_goto(void) */

/* ********************************* do_help ******************************** */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wformat-truncation"

static bool
do_help(void)
{
/* We have some extra work here, because HELP doesn't actually take
 * a filename, and doesn't want one. */
  if (scrdtk(2, (uint8_t *)tmfile, 17, oldcom))
    return bad_rdtk();
  k = oldcom->toklen;              /* Get length of HELP topic */
  if (!eolok())
    return false;
  if (k == 0)                      /* No topic given */
  {
    k = 1;
    tmfile[0] = '#';               /* Dummy topic of "#" */
    tmfile[1] = '\0';
  }                                /* if (k == 0) */
  else
  {
    for (i = k - 1; i >= 0; i--)
      if (tmfile[i] >= 'A' && tmfile[i] <= 'Z')
        tmfile[i] = tolower(tmfile[i]); /* No u/c filenames allowed */
  }                                /* if (k == 0) else */

/* GCC 8.1 complained when the next line was an sprintf. */
/* To keep gcc quiet, we tested for overflow. */
/* But GCC 8.3.0 complained amnyway */
/* So now we compile with -Wno-format-truncation. */
/* If truncation happens, there should be a useful message... */
  snprintf(tmtree, sizeof tmtree, "%s/%s", help_dir, tmfile);
  if (stat(tmtree, &statbuf))
  {
/* Output a potted message if he "typed h for help" */
    if (tmfile[0] == '#' && tmfile[1] == 0)
    {
      puts("\r");
      puts("Sorry - I can't find my HELP files.\r");
      puts("If you have them installed somewhere,\r"
        "please put that path in your shell environment"
        " with the name Q_HELP_DIR.\r\n\n");
      return true;
    }
    fprintf(stderr, "%s. %s (HELP)", strerror(errno), tmtree);
    return false;
  }
  sprintf(ubuf, "%s %s", help_cmd, tmtree);
  final5();                        /* For some pagers */
  if (system(ubuf) < 0)
  {
    fprintf(stderr, "%s. %s (system)", strerror(errno), ubuf);
    init5();
    return false;
  }
  init5();
  return true;                     /* Finished */
}                                  /* bool do_help(void) */

#pragma GCC diagnostic pop

/* ******************************** do_insert ******************************* */

static bool
do_insert(void)
{
  if (!getlin(true, true))
    return false;                  /* J line # u/s */
  setptr(oldcom->decval);          /* Get ready to insert */
  if (!eolok())                    /* Extra params */
    return false;
  A_I_common();
  return true;
}                                  /* bool do_insert(void) */

/* ********************************* do_join ******************************** */

static bool
do_join(void)
{
  char *r;                         /* Scratch */

  if (!getlin(true, false))
    return false;                  /* J bad line # */
  setptr(oldcom->decval);          /* Pos'n on line to be joined onto */
  if ((retcod = get_num(false, &count2)) < 0)
    return false;
  lstlin = -1;                     /* Not allowed -TO */
  if (!get_opt_lines(&count))
    return false;

/* At eof? */
  if (ptrpos >= lintot && !(deferd && (dfread(1, NULL), ptrpos < lintot)))
  {
    if (curmac < 0 || !BRIEF)      /* Message wanted */
      puts("Can't join anything - no lines follow\r");
    return true;                   /* Next command */
  }                                /* if(ptrpos>=lintot&&... */
  rdlin(prev, false);              /* 1st line */
  delete(false, 1, false);         /* Del lin before normal ptr */
  for (i4 = count2; i4 > 0; i4--)
  {
    if (!rdlin(curr, false))       /* If eof */
    {
      printf_eof_reached(count2, "joined");
      break;
    }
    j = curr->bchars + prev->bchars;
    if (j > prev->bmxch)           /* Overflowed line capacity */
    {
      setptr(ptrpos - 1);
      fputs("joining next line would exceed line size :- ", stdout);
      printf("%ld lines ", i4 - 1);
      puts("joined\r");
      break;
    }                              /* if (j > prev->bmxch) */
    r = (char *)&prev->bdata[prev->bchars]; /* Appending posn */
    prev->bchars = j;              /* New length */
    delete(false, 1, false);       /* Delete line just read */
/* Append line just read */
    memcpy(r, (char *)curr->bdata, (size_t)curr->bchars);
  }
  inslin(prev);                    /* Put composed line back */
  setptr(ptrpos - 1);
  J_L_common();
  return true;
}                                  /* bool do_join(void) */

/* ***************************** close_log_file ***************************** */

static bool
close_log_file(void)
{
  SYSCALL(retcod, fclose(log_fd));
  if (retcod)
  {
    fprintf(stderr, "%s. (fclose)", strerror(errno));
    return false;
  }                                /* if (retcod) */
  log_fd = NULL;
  return true;
}                                  /* static bool close_log_file(void) */

/* ****************************** open_log_file ***************************** */

static bool
open_log_file(char *fn)
{
  int new_len = strlen(fn) + 1;

  do
    log_fd = fopen(fn, "a");
  while (!log_fd && errno == EINTR);
  if (!log_fd)
  {
    fprintf(stderr, "%s. %s (fopen)", strerror(errno), optarg);
    return false;
  }                                /* if (!log_fd) */
  if (setvbuf(log_fd, NULL, _IONBF, 0))
  {
    fprintf(stderr, "%s. %s (setvbuf)", strerror(errno), optarg);
    return false;
  }                                /* if (setvbuf(log_fd, NULL, _IONBF, 0)) */
  if (new_len > log_name_len)
  {
    if (log_name)
      free(log_name);
    log_name = malloc(new_len);
    if (!log_name)
    {
      fprintf(stderr, "%s. (malloc)", strerror(errno));
      return false;
    }                              /* if (!log_name) */
    log_name_len = new_len;
  }                                /* if (new_len < log_name_len) */
  strcpy(log_name, fn);
  return true;
}                                  /* static bool open_log_file(char *fn) */

/* ******************************** do_keylog ******************************* */

static bool
do_keylog(void)
{
  if (scrdtk(2, (uint8_t *)ubuf, BUFMAX, oldcom))
  {
    fprintf(stderr, "%s. (scrdtk)", strerror(errno));
    return false;
  }                        /* if (scrdtk(2, (uint8_t *)ubuf, BUFMAX, oldcom)) */
  switch (oldcom->toktyp)
  {
    case nultok:
      fputs("Null arg meaningless", stderr);
      return false;

    case eoltok:
      if (!log_fd)
      {
        fputs("No keylog file to close", stderr);
        return false;
      }                            /* if (!log_fd) */
      if ((curmac >= 0 || ysno5a("Stop keylogging [yes]", A5DYES))
        && !close_log_file())
        return false;
      break;

    case nortok:
      if (oldcom->minusf && oldcom->toklen == 1) /* "k -" */
      {
        if (log_fd)
        {
          printf("Logging to %s\r\n", log_name && *log_name ? log_name :
            "(unknown)");
        }                          /* if (log_fd) */
        else
          fputs("Not logging\r\n", stdout);
      }                            /* if (oldcom->minusf ...) */
      else
      {

        if (!eolok())
          return false;
/* Have name: close any existing keylog file */
        if (log_fd && !close_log_file())
          return false;

        return open_log_file(ubuf);
      }                            /* if (oldcom->minusf ...) else */
  }                                /* switch (oldcom->toktyp) */
  return true;
}                                  /* static bool do_keylog(void) */

/* ******************************** do_locate ******************************* */

static bool
do_locate(void)
{
  bool found;
  int srch_len;
  size_t nmatch;

  is_locate = true;
  unmatched_substring = false;
  saved_tokbeg = 0;
  tokens = verb == 'l';            /* Whether FL */
  if (tokens)
    regs = false;
  else
    regs = (fmode & FM_PLUS_G_BIT) != 0;
  if (REVRSE)
    revpos = ptrpos;
  display_wanted = !BRIEF || curmac < 0; /* Display error messages if true */
  if (REVRSE ? ptrpos <= 1 : ptrpos > lintot && !(deferd &&
    (dfread(1, NULL), ptrpos <= lintot)))
  {
    if (display_wanted)
      printf("At %s of file already - no lines to search\r\n",
        REVRSE ? "start" : "end");
    return true;                   /* Next command */
  }                                /* if(REVRSE?ptrpos<=1:ptrpos>lintot&&... */

/* Get the string to locate.
 * The string is read to the ermess array,
 * as ubuf will get overwritten and we aren't going to use getlin,
 * so ermess is spare. */
  if (scrdtk(2, (uint8_t *)ermess, BUFMAX, oldcom))
    return bad_rdtk();
  if (oldcom->toktyp == eoltok || !(oldcom->toklen))
  {
    fputs("Null string to locate", stderr);
    return false;
  }
  if (regs)
    srch_str_len = 0;              /* lastpos will be where string may start */
  else
    srch_str_len = oldcom->toklen; /* lastpos will be where string may end */
  if (regs && !compile_regexp(ermess))
    return false;

  lstlin = ptrpos;                 /* -TO rel currnt line */
  if ((retcod = get_num(false, &count2)) < 0) /* Get number lines to search */
    return false;
  if (retcod)                      /* No number given */
  {
    if (REVRSE)
      count2 = ptrpos - 1;
    else if (deferd)
      count2 = LONG_MAX;
    else
      count2 = lintot - ptrpos + 1;
  }                                /* if (retcod) */
  lstlin = -1;                     /* Not allowed -TO */
  if (get_num(false, &count) < 0)  /* Get # lines to mod on location */
    return false;
  if (!get_search_columns(regs))
    return false;
  savpos = ptrpos;                 /* Remember pos in case no match */

  if (regs && !get_nmatch(&nmatch))
    return false;

/* Start of search */

  for (i4 = count2; i4 > 0; i4--)
  {
    if (cntrlc)                    /* User abort */
    {
      cntrlc = false;              /* ^C noticed */
      fputs("Command abandoned :-", stdout);
      print_scanned_lines(count2);
      return true;
    }                              /* if(cntrlc) */
    if (REVRSE && revpos > 1)
    {
      setptr(--revpos);            /* Read previous line */
      rdlin(curr, false);          /* "can't" hit eof */
    }                              /* if(REVRSE) */
    else if ((REVRSE && revpos <= 1) || !rdlin(curr, false)) /* If eof */
    {
      if (display_wanted && count2 != LONG_MAX) /* Message wanted */
        printf_eof_reached(count2, "searched");
      break;                       /* for(i4=count2;i4>0;i4--) */
    }                              /* if(!rdlin(curr, false)) */
    srch_len = curr->bchars;
    if (srch_len < minlen)         /* Line too short for a match */
    {
      if (EXCLUSIVE_L_BOOL)
        found = true;              /* Short line == non-match */
      else
        continue;
    }                              /* if (srch_len < minlen) */
    else
    {
      if (srch_len > lastpos)
        srch_len = lastpos;        /* Get length to search */
      found =
        (tokens ?
        ltok5a((uint8_t *)ermess, srch_str_len, curr->bdata, firstpos, srch_len,
        &locpos, NULL, (uint8_t *)ndel) :
        (regs ?
        match_regexp
        (curr->bdata, curr->bchars, firstpos, &locpos, NULL, nmatch) :
        lsub5a((uint8_t *)ermess, srch_str_len, curr->bdata, firstpos, srch_len,
        &locpos, NULL))) ^ EXCLUSIVE_L_BOOL;
    }                              /* if (srch_len < minlen) else */
    if (found)
    {                              /* Line located */
      if (REVRSE)
        setptr(revpos);
      else
        setptr(ptrpos - 1);
      J_L_common();                /* End L & J - carry on as M */
      return true;
    }                              /* if (found) */
  }                                /* for(i4=count2;i4>0;i4--) */

/* Didn't locate it if we get here */

  setptr(savpos);                  /* Move pointer back */
  if (display_wanted)
    fprintf(STDERROUT, "Specified %s not found",
      unmatched_substring ? "sub-expression" : (regs ? "pattern" : "string"));
  locerr = true;                   /* Picked up by RERDCM */
  move_cursor_back(saved_tokbeg);
  return false;
}                                  /* bool do_locate(void) */

/* ******************************** do_modify ******************************* */

static bool
do_modify(void)
{
  modify = true;
  if (getlin(false, false))
  {
    j4 = oldcom->decval;           /* 1st line to be altered */
    lstvld = false;                /* Previous line not valid */
    if (!get_opt_lines(&count))
      return false;
  }
  else
  {
    locerr = true;                 /* An scmac can detect this error */
    if (curmac < 0 || !BRIEF)      /* Report err unless brief macro */
      fprintf(stderr, "%s", ermess);
    return false;
  }
  setptr(j4);                      /* Position on 1st line to alter */
  J_L_M_common();
  return true;
}                                  /* bool do_modify(void) */

/* ******************************* do_newmacro ****************************** */

static bool
do_newmacro(void)
{
  i = newmac();
  if (i < 0)                       /* Token was "-" */
  {
    scrdtk(2, (uint8_t *)ubuf, BUFMAX, oldcom);
    if (oldcom->toktyp == eoltok)
      typmac();
    else
    {
      if (!eolok())
      {
        alu_macros_only = false;
        return false;
      }                            /* if (!eolok()) */
/* Need to preserve stdout for this */
      if (orig_stdout == -1)
      {
        SYSCALL(orig_stdout, dup(1));
      }                            /* if (orig_stdout == -1) */
      if (orig_stdout == -1)
      {
        fprintf(stderr, "\r\n%s. (dup(1))\r\n", strerror(errno));
        refrsh(NULL);
        alu_macros_only = false;
        return false;
      }                            /* if (orig_stdout == -1) */
      else
      {
        SYSCALL(i, close(1));
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
          fprintf(stderr, "%s. %s (freopen)", strerror(i), ubuf);
        i = !i;                    /* Required below */
      }                            /* if (orig_stdout == -1) else */
    }                              /* if (oldcom->toktyp == eoltok) else */
    alu_macros_only = false;
  }                                /* if (i < 0) */
  return i != 0;
}                                  /* bool do_newmacro(void) */

/* ****************************** do_o_ff_or_fc ***************************** */

static bool
do_o_ff_or_fc(fmode_t bit)
{
  bool bit_is_set = false;

  bit_is_set = fmode & bit;
  switch (get_answer())
  {
    case Q_YES:
      bit_is_set = true;
      break;
    case Q_NO:
      bit_is_set = false;
      break;
    case Q_MAYBE:
      bit_is_set = !bit_is_set;
      break;
    case Q_UNREC:
      return false;
  }                                /* switch (answer) */
  if (bit_is_set)
    fmode |= bit;
  else
    fmode &= ~bit;
  return true;
}                                  /* bool do_o_ff_or_fc(fmode_t bit) */

/* ******************************** do_print ******************************** */

static bool
do_print(void)
{
  lstlin = ptrpos;                 /* -TO strt curr lin */
  if (!get_opt_lines(&count))
    return false;
  P_V_common();
  return true;                     /* Finished P */
}                                  /* bool do_print(void) */

/* ********************************* do_quit ******************************** */

static bool
do_quit(bool recursing)
{
  char *colonpos = NULL;           /* Pos'n of ":" in q filename */
  bool colontrunc = false;         /* Try truncating ":<line#> */
  bool tried_colontrunc = false;
  bool try_create = false;         /* Try creating a new file this time */
  bool tried_create = false;
  command_state old_cmd_state;

/* q -A and q -V are allowed whether recursing or not, */
/* so get them out of the way. */
/* -A & -V look like file names initially, and they may in fact be... */

  if (!get_file_arg(&nofile))
    ERRRTN("Error in filename");

/* Look for Q command-line option that is enabled while running */
  if (!nofile && strlen(ubuf) == 2 && ubuf[0] == '-' && ( /* Could be option */
    ubuf[1] == 'A' ||              /* Display opcodes */
    ubuf[1] == 'V'))               /* Display version */
  {
    if (access(ubuf, F_OK))        /* Check for file existence (yuck) */
    {
      switch (ubuf[1])
      {
        case 'A':
          display_opcodes();
          break;

        case 'V':
          q_version();
          break;
      }                            /* switch (ubuf[1]) */
      return true;
    }                              /* if (access(ubuf, F_OK)) */
  }                                /* if (strlen(ubuf) == 2 && ... */

/* If recursing, simply return. do_freprompt() cleans up. */
/* Indicate whether Q or FQ though */

  if (recursing)
  {
    fq_from_fr = verb == 'q';
    q_from_fr = true;
    return true;
  }                                /* if (recursing) */

  if (mods &&
    !ysno5a
    ("file modified since last b-backup/s-save, ok to quit (y,n,Cr [n])",
    A5DNO))
  {
    if (previous_argno >= 0)
      argno = previous_argno;
    return true;                   /* J user changed his mind */
  }
  Tcl_DumpActiveMemory("t5mem");
  if (nofile)
  {
/* If in a macro, only action solitary Q if mode says so.
 * Otherwise, convert to ^NU... */
    if (curmac >= 0 && !(fmode & FM_PLUS_Q_BIT) && verb == 'Q')
    {
      macdef(64, (uint8_t *)"", 0, true); /* Macro is ^NU only */
      curmac = 64;
      mcposn = 0;
      return true;
    }
    exit(0);                       /* ACTUALLY EXIT Q */
  }
  rdwr = O_RDONLY;
  q_new_file = false;              /* Q-QUIT into existing file */
  do
  {
    if (colontrunc)
      tried_colontrunc = true;
    if (!do_stat_symlink() || !eolok())
      RESET_ARGNO;
    do
    {
      if (try_create)
        tried_create = true;
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
            if ((colonpos = strchr(ubuf, ':')) &&
              sscanf(colonpos + 1, "%d", &colonline) == 1)
            {
              cmd_state = HAVE_LINE_NUMBER; /* line # in colonline */
              *colonpos = '\0';    /* Truncate filename */
              if (!do_stat_symlink() || !eolok())
              {
                cmd_state = RUNNING;
                RESET_ARGNO;
              }                    /* if (!do_stat_symlink() || !eolok()) */
              colontrunc = true;   /* Try with truncated ubuf */
            }                      /* if((colonpos=strchr(ubuf,':'))&&... */
          }                        /* if (cmd_state == RUNNING) */
/* Just tried truncating at ":" */
          else if (cmd_state == HAVE_LINE_NUMBER)
            *colonpos = ':';       /* Undo truncation */
          if (!colontrunc || tried_colontrunc)
          {
            old_cmd_state = cmd_state;
            cmd_state = RUNNING;

            if (ysno5a("Do you want to create a new file (y,n,Cr [n])", A5DNO))
            {
              cmd_state =
                old_cmd_state == TRY_INITIAL_COMMAND ? old_cmd_state : RUNNING;
              q_new_file = true;   /* Q-QUIT into new file */
              rdwr = O_WRONLY + O_CREAT + O_EXCL; /* File should *not* exist */
              try_create = true;   /* So create file */
            }  /* if (ysno5a("Do you want to create a new file ...)", A5DNO)) */
          }                        /* if (!colontrunc  || tried_colontrunc) */
        }                          /* if (errno == ENOENT && !q_new_file) */
        if ((!colontrunc || tried_colontrunc) && (!try_create || tried_create))
        {
          fprintf(stderr, "%s. %s (open)", strerror(errno), ubuf);
          RESET_ARGNO;
        }
      }                         /* if ((funit = open_buf(rdwr, tmode)) == -1) */
    }
    while (try_create && !tried_create);
  }
  while (colontrunc && !tried_colontrunc);

  if (q_new_file)                  /* Have just created file for Q-QUIT */
  {
    mods = false;                  /* No mods to new file yet */
    my_close(funit);               /* New empty file open in wrong mode */
  }                                /* if(q_new_file) */
  finitl();                        /* Erases old file but keeps segments */
/*
 * Set up new default for S&B
 */
  strcpy(pcnta, ubuf);             /* Filename & length now remembered */
  if (q_new_file)                  /* Q-QUIT into new file */
    return true;                   /* Read next command */
  if (!E_Q_common())
    RESET_ARGNO;
  mods = false;                    /* No mods to new file yet */
  return true;
}                                  /* bool do_quit(void) */

/* ****************************** do_reposition ***************************** */

static bool
do_reposition(void)
{
  repos = true;
  if (!get_c_or_r_args())
    return false;
  if (k4 == j4 - 1)
  {
    fputs("moving a line to before the next line is a no-op", stderr);
    return false;
  }
  if (finish_c_or_r("repositioned"))
    return true;
  return false;
}                                  /* bool do_reposition(void) */

/* ****************************** do_freprompt ****************************** */

static bool
do_freprompt(void)
{
  int fd;
  int rc;
  struct reprompt_frame *r;

  if (curmac < 0)
  {
    fputs("FR only allowed from within a macro", stderr);
    return false;
  }                                /* if (curmac < 0) */
  if (cmsplt)
  {
    fputs("FR not allowed with control-t", stderr);
    return false;
  }                                /* if (cmsplt) */
  if (ttyfd <= 0)
  {
    fputs("FR must have access to a tty", stderr);
    return false;
  }                                /* if (ttyfs <= 0) */
  if (ridx >= NUM_RFRAMES - 1)     /* '>' shouldn't happen */
  {
    fputs("Too many levels of FR", stderr);
    return false;
  }                                /* if (ridx >= NUM_RFRAMES - 1) */
  if (cmd_state != RUNNING)
  {
    fputs("FR not allowed this early", stderr);
    return false;
  }                                /* if (cmd_state != RUNNING) */

/* All checks pass, actually do it */

  r = rframe + ++ridx;
  r->saved_offline = offline;
  offline = false;
  r->saved_piping = piping;
  piping = false;
  for (fd = STDIN5FD; fd <= STDERR5FD; fd++)
  {
    if (isatty(fd))
      r->fds[fd] = -1;
    else
    {
      SYSCALL(r->fds[fd], dup(fd));
      if (r->fds[fd] == -1)
      {
        fprintf(stderr, "%s. (dup(%d))", strerror(errno), fd);
        exit(1);
      }                            /* if r->fds[fd] == -1) */
      SYSCALL(rc, dup2(ttyfd, fd));
      if (rc == -1)
      {
        fprintf(stderr, "%s.(dup2(%d, %d))", strerror(errno), ttyfd, fd);
        exit(1);
      }                            /* if (rc == -1) */
    }                              /* if (isatty(fd)) else */
  }                             /* for (fd = STDIN5FD; fd <= STDOUT5FD; fd++) */
  r->saved_curmac = curmac;
  curmac = -1;
  r->saved_mcposn = mcposn;
  r->saved_immdtum = immdtum;
  immdtum = immnxfr;
  r->saved_mcdtum = mcdtum;
  mcdtum = mcnxfr;
  r->saved_stdbase = stdbase;
  r->saved_fmode = fmode;
  stdbase = stdidx + 1;
  if (r->saved_offline)
    change_attr(ttyfd, &tio5);

  xmain(r->saved_curmac, NULL);    /* *!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*! */

/* Back from xmain, restore previous state */

  if (r->saved_offline)
    change_attr(ttyfd, &tio5save);
  stdbase = r->saved_stdbase;
  fmode = r->saved_fmode;
  mcdtum = r->saved_mcdtum;
  immdtum = r->saved_immdtum;
  mcposn = r->saved_mcposn;
  curmac = r->saved_curmac;
  for (fd = STDIN5FD; fd <= STDERR5FD; fd++)
    if (r->fds[fd] != -1)
    {
      SYSCALL(rc, dup2(r->fds[fd], fd));
      if (rc == -1)
      {
        fprintf(stderr, "%s.(dup2(%d, %d))", strerror(errno), r->fds[fd], fd);
        exit(1);
      }                            /* if (rc == -1) */
      SYSCALL(rc, close(r->fds[fd]));
    }                              /* if (r->fds[fd] != -1) */
  piping = r->saved_piping;
  offline = r->saved_offline;
  --ridx;

  return true;
}                                  /* do_freprompt() */

/* **************************** do_shell_command **************************** */

static bool
do_shell_command(void)
{
  if (do_cmd())
  {
    fputs("bad luck", stderr);
    noRereadIfMacro = true;
    move_cursor_back(0);           /* Error has been reported */
    return false;
  }
  if (cntrlc)
  {
    newlin();
    report_control_c();
  }                                /* if (cntrlc) */
  return true;
}                                  /* bool do_shell_command(void); */

/* ******************************** do_tabset ******************************* */

static bool
do_tabset(void)
{
  return tabset(oldcom);    /* Enter SCREENEDIT subsystem to complete command */
}                                  /* bool do_tabset(void) */

/* ******************************* do_usefile ******************************* */

static bool
do_usefile(void)
{
  if (!get_file_arg(&nofile) || nofile)
    ERRRTN("Error in filename");
  if (!eolok())
    return false;
  duplx5(true);                    /* Assert XOFF recognition */

/* NULLSTDOUT setting is same as parent */
  stdinfo[stdidx + 1].nullstdout =
    stdidx < stdbase ? false : stdinfo[stdidx].nullstdout;

/* Save current stdin */
  stdidx++;
  SYSCALL(stdinfo[stdidx].funit, dup(STDIN5FD));
  if (stdinfo[stdidx].funit == -1)
  {
    stdidx--;
    fprintf(stderr, "%s. (dup(STDIN5FD))", strerror(errno));
    return false;
  }                                /* if (stdinfo[stdidx].funit == -1) */

/* Close stdin */
  SYSCALL(i, close(STDIN5FD));

/* Open new input source */
  i = open_buf(O_RDONLY, STDIN5FD);
  if (i == -1)
  {
    pop_stdin();
    fprintf(stderr, "%s. %s (open)", strerror(errno), ubuf);
    return false;
  }                                /* if (i == -1) */

/* Verify new input opened on STDIN5FD. Try to rectify if not */
  if (i)
  {
    SYSCALL(j, dup2(i, STDIN5FD));
    if (j == -1)
    {
      fprintf(stderr, "%s.(dup2(%d, STDIN5FD))", strerror(errno), i);
      SYSCALL(j, close(i));
      pop_stdin();
      return false;
    }                              /* if (j == -1) */
    SYSCALL(j, close(i));
  }                                /* if (i) */

/* If invoked from a macro, suspend that macro */
  if (curmac >= 0)
  {
    if (!pushmac(true))
      return false;
    curmac = -1;
    stdinfo[stdidx].frommac = true;
  }                                /* if (curmac >= 0) */
  else
    stdinfo[stdidx].frommac = false;

  buf5len = 0;                     /* Flush any input left over */
  return true;
}                                  /* bool do_usefile(void) */

/* ********************************* do_view ******************************** */

static bool
do_view(void)
{
  bool fullv = false;              /* Full VIEW wanted */
  long xcount = 0;                 /* For V-View */

/* Print n lines before current line, then current line, then n lines
 * after. N may be specified as zero - show current line only.
 * If no n, assume enough lines to completely fill the screen.
 * Afterwards, move pointer back to posn of entry. */

  xcount = 0;                      /* Only show requested # */
  lstlin = -1;                     /* Not allowed -TO */
  fullv = false;                   /* Assume not just "V" */
  if ((retcod = get_num(true, &count)) < 0)
    return false;
  if (retcod)                      /* No number given */
  {
    fullv = true;                  /* Try very hard to fill screen */
    count = (row5 / 2) - 1;        /* No count given so assume 1/2 screen */
    xcount = row5 & 1;             /* Extra line if odd screen length */
  }                                /* if(retcod) */

/* Check there are no more args */
  if (!eolok())
    return false;

  if (!lintot && !(deferd && (dfread(1, NULL), lintot))) /* Empty file */
  {
    puts("THE FILE IS EMPTY\r");
    return true;                   /* End V */
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
  P_V_common();                    /* Print them */
  setptr(k4);                      /* Reposition */
  return true;                     /* Finished */
}                                  /* bool do_view(void) */

/* ****************************** do_writefile ****************************** */

static bool
do_writefile(void)
{
  long line_number_saved;

  if (!get_file_arg(&nofile) || nofile)
    ERRRTN("Error in filename");
  if (!getlin(true, false))
    return false;                  /* J line # u/s */

/* Can't do setptr here because that breaks relative line numbering. */
/* But ... must save returned decimal number before calling scrdtk again */

  line_number_saved = oldcom->decval;
  if (!get_opt_lines(&count) || !eolok())
    return false;
  setptr(line_number_saved);       /* Get ready to write */
  wrtnum = count;
  rdwr = O_WRONLY + O_CREAT;
  return s_b_w_common_write();
}                                  /* bool do_writefile(void) */

/* ******************************* do_xistics ******************************* */

static bool
do_xistics(void)
{
  if (!eolok())
    return false;
  puts("Switching off screenedit mode\r");
  xistcs();                        /* So set them */
  puts("Re-enabling screenedit\r");
  return true;                     /* Finished X */
}                                  /* bool do_xistics(void) */

/* ***************************** do_initial_tsks **************************** */

static void
do_initial_tsks(bool *do_rc_p)
{
  dfltmode |= FM_PLUS_A_BIT | FM_PLUS_I_BIT | FM_PLUS_E_BIT | FM_PLUS_M_BIT |
    TAB_READ_BIT | DOS_READ_BIT | FM_PLUS_9_BIT | FM_PLUS_0_BIT;
  end_seq = normal_end_sequence;
  init_alu();
  tmask = umask(0);                /* Get current umask */
  umask(tmask);                    /* Reinstate umask */
  tmode = ~tmask & 0666;           /* Assume no execute on new file */

/* Set up for the command substitution macros */
  macdefw(STDOUT_MACRO_IDX, NULL, 0, true);
  macdefw(STDERR_MACRO_IDX, NULL, 0, true);

/* Pick up any option arguments and set cmd_state if more args follow */
  cmd_state = TRY_INITIAL_COMMAND;
  while ((i = getopt(argc, argv, "AVbdei:k:mnoqtv")) != -1)
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
        dfltmode |= FM_PLUS_F_BIT;
        break;

      case 'd':
        dfltmode ^= DOS_READ_BIT;
        break;

      case 'e':
        dfltmode ^= FM_PLUS_E_BIT;
        break;

      case 'i':
        initial_command = optarg;
        break;

      case 'k':
        if (!open_log_file(optarg))
        {
          fputs("/r/n", stderr);
          exit(1);
        }                          /* if (!open_log_file(optarg)) */
        break;

      case 'm':
        dfltmode ^= FM_PLUS_M_BIT;
        break;

      case 'n':
        *do_rc_p = false;
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
        if (verbose_flag)
        {
          very_verbose_flag = true;
          dfltmode |= FM_PLUS_8_BIT;
        }                          /* if (verbose_flag) */
        else
          verbose_flag = true;
        break;

      case '?':
        errflg = true;
        break;
    }                              /* switch(i) */
  if (errflg)
  {
    fprintf(stderr, "%s",
      "Usage: q [-AVbdemnoqtv] [-i <macro definition>] "
      "[-k <keylog file>] [+<n> file] [file[:<n>]]...\n");
    exit(1);
  }
  if (aluflg)
  {
    display_opcodes();
    if (argc == 2)
      exit(0);
  }                                /* if (aluflg) */
  if (vrsflg)
  {
    q_version();
    if (argc == 2)
      exit(0);
  }                                /* if (vrsflg) */
  if (offline && initial_command == NULL)
  {
    fprintf(stderr, "%s", "Can only use -o with -i\n");
    exit(1);
  }                                /* if (offline && initial_command == NULL) */
  if (verbose_flag && quiet_flag)
  {
    fputs("-q[uiet] and -v[erbose] are mutually exclusive\n", stderr);
    exit(1);
  }                                /* if (verbose_flag && quiet_flag) */
  if (!(sh = getenv("SHELL")))
    sh = "/bin/sh";
  if (!(help_dir = getenv("Q_HELP_DIR")))
    help_dir = HELP_DIR;
  if (!(macro_dir = getenv("Q_MACRO_DIR")))
  {
/* If HELP_DIR and MACRO_DIR were initially the same, */
/* keep them the same now */
    if (strcmp(HELP_DIR, MACRO_DIR))
      macro_dir = MACRO_DIR;
    else
      macro_dir = help_dir;
  }                              /* if (!(macro_dir = getenv("Q_MACRO_DIR"))) */
  if (!(help_cmd = getenv("Q_HELP_CMD")) && !(help_cmd = getenv("PAGER")))
    help_cmd = HELP_CMD;
  if (!(etc_dir = getenv("Q_ETC_DIR")))
    etc_dir = ETC_DIR;
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
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGTERM, &act, NULL);
#ifdef SIGWINCH
  sigaddset(&act.sa_mask, SIGWINCH);
  sigaction(SIGWINCH, &act, NULL);
#endif
}                                  /* static void do_initial_tsks() */

/* ******************************** not_pipe ******************************** */

static void
not_pipe(void)
{
  if (!offline && !(P && Q))       /* P and Q not set if !offline */
  {
    fputs("stdin & stderr must both be a tty unless q -o\n", stderr);
    exit(1);
  }                                /* if (!offline && !(P && Q)) */

/* Implement quiet operation if requested */
  if (quiet_flag)
  {
    if (!offline)
    {
      fputs("-q[uiet] is only available with -o[ffline]\n", stderr);
      exit(1);
    }                              /* if (!offline) */
    dev_null_stdout();
  }                                /* if (quiet_flag) */

/* Ctrl-C just sets a flag (rather than exitting) */
  piping = false;
}                                  /* static void not_pipe() */

/* ******************************* check_pipe ******************************* */

static bool
check_pipe(void)
{
/* Check for running in a pipe (or with redirection), but not if -o */
  if (offline)
    return false;

/* If stdin is not a terminal, assume we are to act as in a pipe. */
/* We don't then care about stdout (c.f. -o) */

  P = isatty(STDIN5FD);
  Q = isatty(STDOUT5FD);
  if (P)
    return false;
  if (initial_command == NULL)
  {
    fputs("You must supply an initial command to run Q in a pipe\n", stderr);
    exit(1);
  }                                /* if (initial_command == NULL) */

  if (argno != -1)
  {
    fputs("You may not give Q a file name when running in a pipe\n", stderr);
    exit(1);
  }                                /* if (argno != -1) */

/* Deal with stdout */
  SYSCALL(saved_pipe_stdout, dup(STDOUT5FD));
  if (saved_pipe_stdout == -1)
  {
    fprintf(stderr, "%s. fd %d (dup)\n", strerror(errno), STDOUT5FD);
    exit(1);
  }                                /* if (saved_pipe_stdout == -1) */

/* If verbose, dup stderr to stdout. Otherwise, stdout is /dev/null */
  if (verbose_flag)
  {
    SYSCALL(i, dup2(STDERR5FD, STDOUT5FD));
    if (i == -1)
    {
      fprintf(stderr, "%s. fd %d to fd %d (dup2)\n", strerror(errno),
        STDERR5FD, STDOUT5FD);
      exit(1);
    }                              /* if (i == -1) */
  }                                /* if (verbose_flag) */
  else
    dev_null_stdout();

/* Deal with stdin */

/* Create & open a temporary file to buffer the entire pipe */
  strcpy(pipe_temp_name, PIPE_NAME);
  atexit(rm_pipe_temp);
  pipe_temp_fd = mkstemp(pipe_temp_name);
  if (pipe_temp_fd == -1)
  {
    fprintf(stderr, "%s. %s (open)\n", strerror(errno), pipe_temp_name);
    exit(1);
  }                                /* if (pipe_temp_fd == -1) */

/* Populate the temporary file */
  while (true)
  {
    ssize_t nc, todo;
    char *write_from;

    SYSCALL(todo, read(STDIN5FD, ubuf, sizeof ubuf));
    if (todo == -1)
    {
      fprintf(stderr, "%s. stdin (read)", strerror(errno));
      exit(1);
    }                              /* if (todo == -1) */
    if (!todo)
      break;                       /* Reached EOF */
    write_from = ubuf;
    while (todo)
    {
      SYSCALL(nc, write(pipe_temp_fd, write_from, todo));
      if (nc == -1)
      {
        fprintf(stderr, "%s. %s (write)\n", strerror(errno), pipe_temp_name);
        exit(1);
      }                            /* if (nc == -1) */
      if (nc == 0)                 /* Is this possible? */
      {
        fprintf(stderr, "Zero characters written. %s (write)\n",
          pipe_temp_name);
        exit(1);
      }                            /* if (nc == 0) */
      write_from += nc;
      todo -= nc;
    }                              /* while(todo) */
  }                                /* while (true) */
  my_close(pipe_temp_fd);
  cmd_state = Q_ARG1;

/* Never ask for input from stdin */
  offline = true;
  return true;
}                                  /* static bool check_pipe(void) */

/* ***************************** compile_regexp ***************************** */

static bool
compile_regexp(char *regex)
{
  int rc;

  regfree(&preg);
  rc = regcomp(&preg, regex, REG_EXTENDED | (CASDEP ? 0 : REG_ICASE));
  if (!rc)
    return true;
  regerror(rc, &preg, ermess, sizeof ermess);
  fputs(ermess, stderr);
  return false;
}                                  /* compile_regexp(char *regex) */

/* ****************************** match_regexp ****************************** */

/* ----------------------------------------------- */
/* Arguments:-                                     */
/*                                                 */
/* A1 - Address of string to search                */
/* A2 - Length of string to search                 */
/* A3 - Offset from string start to start matching */
/* A4 - Returned match position                    */
/* A5 - Returned match length or NULL              */
/* A6 - nmatch arg for regexec()                   */
/* ----------------------------------------------- */

static bool
match_regexp(uint8_t *string, int stringlen, int offset, int *matchpos,
  int *matchlen, size_t nmatch)
{
  int rc;
  uint8_t saved_char;
  bool repeat;
/* Have to search for sub-expression from line start */
  int local_offset = nmatch == 1 ? offset : 0;

/* Make sure pmatch is big enough. If first time through, allocate 8 slots */
  if (nmatch > pmatch_len)
  {
    size_t wanted = nmatch;

    if (!pmatch_len)
    {
      if (wanted < 8)
        wanted = 8;
      pmatch = malloc(wanted * sizeof *pmatch);
    }                              /* if (!pmatch_len && wanted < 8) */
    else
      pmatch = realloc(pmatch, wanted * sizeof *pmatch);
    if (!pmatch)
    {
      fputs(strerror(errno), stderr);
      return false;
    }                              /* if (!pmatch) */
    pmatch_len = wanted;
  }                                /* if (nmatch > pmatch_len) */

/* Unlike lsub5a, caller can't check for a short line */
/* (since minlen is always zero). So do it now */
/* Also unlike lsub5a, it is legal to search an empty string, */
/* so test is for GT rather than GE */
  if (offset > stringlen)
    return false;

/* Inner match loop (required in case substring may only start after offset) */
  do
  {
    repeat = false;

/* Terminate line for regexec */
    saved_char = string[stringlen];
    string[stringlen] = 0;

    rc = regexec(&preg, (char *)string + local_offset, nmatch, pmatch, 0);
    string[stringlen] = saved_char;
    if (rc)
      return false;

/* Match after last allowed column is not a match */
    if (pmatch[nmatch - 1].rm_so + local_offset > lastpos)
      return false;

/* Negative pmatch is not a match */
    if (pmatch[nmatch - 1].rm_so < 0)
    {
      unmatched_substring = true;
      return false;
    }                              /* if (pmatch[nmatch - 1].rm_so < 0) */

/* If matched a substring too early, try again with bigger offset */
    if (pmatch[nmatch - 1].rm_so + local_offset < offset)
    {
      local_offset += pmatch->rm_so + 1;
      repeat = true;
    }                /* if (pmatch[nmatch - 1].rm_so + local_offset < offset) */
  }
  while (repeat);                  /* do */

/* Set found position and length matched */
  *matchpos = pmatch[nmatch - 1].rm_so + local_offset;
  if (matchlen)
    *matchlen = pmatch[nmatch - 1].rm_eo - pmatch[nmatch - 1].rm_so;

  return true;
}                                  /* static bool match_regexp() */

/* ******************************* get_nmatch ******************************* */

static bool
get_nmatch(size_t *nmatch)
/* Get nmatch for this regexp call */
{
  scrdtk(1, NULL, 0, oldcom);
  if (oldcom->toktyp == nortok)
  {
    if (!oldcom->decok)
    {
      fputs("Invalid decimal number of matches", stderr);
      return false;
    }                              /* if (!oldcom->decok) */
    *nmatch = oldcom->decval;
    saved_tokbeg = oldcom->tokbeg;
    if (*nmatch <= 0)
    {
      fputs("Number of matches must be 1 or more", stderr);
      return false;
    }                              /* if (*nmatch <= 0) */
  }                                /* if (oldcom->toktyp == nortok) */
  else
    *nmatch = 1;
  return eolok();
}                                  /* get_nmatch() */
