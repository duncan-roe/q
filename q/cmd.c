/* C M D
 *
 * Obey a shell command returning its exit status
 *
 * Copyright (C) 2002, Duncan Roe
 * Copyright (C) 2012,2015 Duncan Roe
 */
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "alledit.h"
#include "edmast.h"
int
cmd(char *mybuf)
{
  int status;
  int pid;

  pid = fork();
  if (pid == -1)
  {
    perror("fork");
    exit(1);
  }                                /* if(pid==-1) */
  if (pid)
  {
  retry:
    if (waitpid(pid, &status, WUNTRACED) == -1)
    {
      perror("waitpid");
      if (errno == EINTR)
        goto retry;
      status = errno;
    }                              /* if(waitpid(pid,&status,WUNTRACED)==-1) */
    else
    {
      if (WIFSIGNALED(status))
        printf("\"%s\" terminted with signal %d\n", mybuf, WTERMSIG(status));
      else if (WIFEXITED(status))
      {
        if (WEXITSTATUS(status))
          printf("\"%s\" exited with status %d\n", mybuf, WEXITSTATUS(status));
      }
      else if (WIFSTOPPED(status))
      {
        printf("\"%s\" stopped with signal %d\n", mybuf, WSTOPSIG(status));
        goto retry;
      }
      else
        fprintf(stderr, "\"%s\" neither terminted with a signal nor exited"
          " (weird, huh?)\n", mybuf);
    }                              /* if(waitpid(pid,&status,WUNTRACED)==-1) */
  }                                /* if(pid) */

/* Child code */

  else
  {
    execl(sh, sh, "-c", mybuf, (char *)NULL);
    perror("execl");
  }                                /* if(pid) else */

  return status;
}                                  /* int cmd(char*buf) */
