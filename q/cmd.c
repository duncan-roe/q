/* >%---- CODE_STARTS ./cmd.c */
/* C M D
 *
 * Obey a shell command returning its exit status
 *
 * Copyright (C) 2002, Duncan Roe
 * Copyright (C) 2012,2015,2019-2021 Duncan Roe */

/* Headers */

/* >%---- KEEP2HERE ./cmd.c */
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "prototypes.h"
#include "backtick.h"
#include "edmast.h"
#include "macros.h"
/* >%---- CUT_HERE ./cmd.c */

/* Macros */

#define PKT ((struct pkt *)pkt)

/* Typedefs */

struct pkt
{
  int bufcap;
  int buflen;
  int *fds;
  int childfd;                     /* For error messages & debugging */
  uint8_t *buf;
};                                 /* struct pkt */

/* Static Variables */

static char *the_command;

/* ********************************* thrfunc ******************************** */

static void *
thrfunc(void *pkt)
{
  int retcod;
  void *result = NULL;

  SYSCALL(retcod, close(PKT->fds[1]));
  for (;;)
  {
    SYSCALL(retcod, read(PKT->fds[0], PKT->buf + PKT->buflen,
      PKT->bufcap - PKT->buflen));
    if (retcod < 0)
    {
      fprintf(stderr, "%s. fd %d (read)/r/n", strerror(errno), PKT->fds[0]);
      result = stderrbuf;
      break;
    }                              /* if (retcod < 0) */
    if (retcod == 0)
    {
      if (PKT->buf[PKT->buflen - 1] == 012)
      {
        --PKT->buflen;
        if (PKT->buf[PKT->buflen - 1] == 015)
          --PKT->buflen;
      }                            /* if (PKT->buf[PKT->buflen - 1] == 012) */
      break;
    }                              /* if (retcod == 0) */
    PKT->buflen += retcod;
    if (PKT->buflen == PKT->bufcap)
    {
      fprintf(stderr, "Output truncated: \"%s\"\r\n", the_command);
      result = stderrbuf;
      do
      {
        char junkbuf[8192];
        SYSCALL(retcod, read(PKT->fds[0], junkbuf, sizeof junkbuf));
      } while (retcod > 0);        /* do */
      break;
    }                              /* if (PKT->buflen == PKT->bufcap) */
  }                                /* for(;;) */
  PKT->buf[PKT->buflen] = 0;       /* Null-terminate */
  SYSCALL(retcod, close(PKT->fds[0]));
  return result;
}                                  /* thrfunc(void *pkt) */

/* *********************************** cmd ********************************** */

int
cmd(char *mybuf, bool backtick)
{
  int status;
  int pid;
  int retcod;
  int outcod = -1, errcod = -1;
  bool retry;
  int errfds[2], outfds[2];
  struct pkt outpkt, errpkt;
  pthread_t outthr, errthr;
  void *resultptr;                 /* Non-NULL if error */

/* Set up pipes if doing command output macro definition */
  if (backtick)
  {
    the_command = mybuf;
    if (pipe(outfds))
    {
      fprintf(stderr, "%s. outfds (pipe)\r\n", strerror(errno));
      return 1;
    }                              /* if (pipe(outfds)) */
    if (pipe(errfds))
    {
      fprintf(stderr, "%s. errfds (pipe)\r\n", strerror(errno));
      SYSCALL(retcod, close(outfds[0]));
      SYSCALL(retcod, close(outfds[1]));
      return 1;
    }                              /* if (pipe(errfds)) */
    outpkt.bufcap = BUFMAX;        /* Leave room for trlg NUL */
    outpkt.buflen = 0;             /* Nothing written yet */
    outpkt.fds = outfds;
    outpkt.childfd = STDOUT5FD;
    outpkt.buf = stdoutbuf;
    errpkt.bufcap = BUFMAX;        /* Leave room for trlg NUL */
    errpkt.buflen = 0;             /* Nothing written yet */
    errpkt.fds = errfds;
    errpkt.childfd = STDERR5FD;
    errpkt.buf = stderrbuf;
  }                                /* if (backtick) */

  pid = fork();                    /* EINTR not listed  as possible */
  if (pid == -1)
  {
    fprintf(stderr, "%s. (fork)\r\n", strerror(errno));
    return 1;
  }                                /* if(pid==-1) */
  if (pid)
  {                                /* PARENT CODE */
/* Create pipe reading threads if doing command output macro definition */
    if (backtick)
    {
      outcod = pthread_create(&outthr, NULL, thrfunc, &outpkt);
      if (outcod)
        fprintf(stderr, "%s. outfunc (pthread_create)", strerror(outcod));
      else
      {
        errcod = pthread_create(&errthr, NULL, thrfunc, &errpkt);
        if (errcod)
          fprintf(stderr, "%s. errfunc (pthread_create)", strerror(errcod));
      }                            /* if (outcod) else */
    }                              /* if (backtick) */
    do
    {
      retry = false;
      SYSCALL(retcod, (waitpid(pid, &status, WUNTRACED) == -1));
      if (retcod == -1)
      {
        fprintf(stderr, "%s. (waitpid)\r\n", strerror(errno));
        status = errno;
      }                            /* if (retcod == -1) */
      else
      {
        if (WIFSIGNALED(status) && !backtick)
          fprintf(stderr, "\"%s\" terminted with signal %d\r\n", mybuf,
            WTERMSIG(status));
        else if (WIFEXITED(status))
        {
          if (WEXITSTATUS(status) && !backtick)
            fprintf(stderr, "\"%s\" exited with status %d\r\n", mybuf,
              WEXITSTATUS(status));
        }
        else if (WIFSTOPPED(status))
        {
          if (!backtick)
            fprintf(stderr, "\"%s\" stopped with signal %d\r\n", mybuf,
              WSTOPSIG(status));
          retry = true;
        }
        else
          fprintf(stderr, "\"%s\" neither terminted with a signal nor exited"
            " (weird, huh?)\r\n", mybuf);
      }                            /* if (retcod == -1) else */
    }                              /* do */
    while (retry);

/* Wait for threads to finish */
    if (!outcod)
    {
      outcod = pthread_join(outthr, &resultptr);
      if (outcod)
      {
        fprintf(stderr, "%s. outfunc (pthread_join)", strerror(outcod));
        if (!status)
          status = 1;
      }                            /* if (outcod) */
      else
      {
        if (resultptr && !status)
          status = 1;
      }                            /* if (outcod) else */
    }                              /* if (!outcod) */
    if (!errcod)
    {
      errcod = pthread_join(errthr, &resultptr);
      if (errcod)
      {
        fprintf(stderr, "%s. errfunc (pthread_join)", strerror(errcod));
        if (!status)
          status = 1;
      }                            /* if (errcod) */
      else
      {
        if (resultptr && !status)
          status = 1;
      }                            /* if (errcod) else */
    }                              /* if (!errcod) */
    if (backtick)
    {
      int i, j, k;
      bool e = false;

/* Shovel stdout straight into its macro */
      macdef(STDOUT_MACRO_IDX, stdoutbuf, outpkt.buflen, true);

/* Massage stderr so any control chars are escaped */
      for (i = BUFMAX, j = 0, k = 0; i > 0; i--)
      {
        if (!stderrbuf[j])
          break;
        if (stderrbuf[j] < 32)
        {
          if (e)
          {
            e = false;
            ubuf[k++] = stderrbuf[j++];
          }                        /* if (e) */
          else
          {
            e = true;
            ubuf[k++] = 020;       /* ^P */
          }                        /* if (e) else */
        }                          /* if (stderrbuf[j] < 32) */
        else
          ubuf[k++] = stderrbuf[j++];
      }                            /* for (...) */
      ubuf[BUFMAX] = 0;            /* Backstop */
      macdef(STDERR_MACRO_IDX, (uint8_t *)ubuf, k, true);
    }                              /* if (backtick) */
  }                                /* if(pid) */
  else
  {                                /* CHILD CODE */
/* Connect pipes if doing command output macro definition */
    if (backtick)
    {
      int i;
      SYSCALL(i, close(outfds[0]));
      SYSCALL(i, dup2(outfds[1], STDOUT5FD));
      if (i != STDOUT5FD)
      {
        if (i == -1)
          fprintf(stderr, "%s. STDOUT5FD (dup2)\r\n", strerror(errno));
        else
          fprintf(stderr, "dup2 returned %d when %d wanted\r\n", i, STDOUT5FD);
        return 1;
      }                            /* if (i != STDOUT5FD) */
      SYSCALL(i, close(outfds[1]));
      SYSCALL(i, close(errfds[0]));
      SYSCALL(i, dup2(errfds[1], STDERR5FD));
      if (i != STDERR5FD)
      {
        if (i == -1)
          fprintf(stderr, "%s. STDERR5FD (dup2)\r\n", strerror(errno));
        else
          fprintf(stderr, "dup2 returned %d when %d wanted\r\n", i, STDERR5FD);
        return 1;
      }                            /* if (i != STDERR5FD) */
      SYSCALL(i, close(errfds[1]));
    }                              /* if (backtick) */
    execl(sh, sh, "-c", mybuf, NULL);
    fprintf(stderr, "%s. %s (execl)\r\n", strerror(errno), sh);
    return 1;
  }                                /* if(pid) else */

  return status;
}                                  /* int cmd(char*buf) */
