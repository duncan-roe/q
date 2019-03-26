/* C M D
 *
 * Obey a shell command returning its exit status
 *
 * Copyright (C) 2002, Duncan Roe
 * Copyright (C) 2012,2015,2019 Duncan Roe
 */
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "prototypes.h"
#include "edmast.h"
int
cmd(char *mybuf, bool backtick)
{
  int status;
  int pid;
  int retcod;
  bool retry;

  pid = fork();                    /* EINTR not listed  as possible */
  if (pid == -1)
  {
    fprintf(stderr, "%s. (fork)\r\n", strerror(errno));
    return 1;
  }                                /* if(pid==-1) */
  if (pid)
  {
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
        if (WIFSIGNALED(status))
          fprintf(stderr, "\"%s\" terminted with signal %d\r\n", mybuf,
            WTERMSIG(status));
        else if (WIFEXITED(status))
        {
          if (WEXITSTATUS(status))
            fprintf(stderr, "\"%s\" exited with status %d\r\n", mybuf,
              WEXITSTATUS(status));
        }
        else if (WIFSTOPPED(status))
        {
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
  }                                /* if(pid) */
  else
  {                                /* Child code */
    execl(sh, sh, "-c", mybuf, (char *)NULL);
    fprintf(stderr, "%s. (execl)\r\n", strerror(errno));
    return 1;
  }                                /* if(pid) else */

  return status;
}                                  /* int cmd(char*buf) */
