/* tty;exec disowntty  */
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <signal.h>
static void
end (const char *msg)
{
  perror (msg);
  for (;;)
    pause ();
}
int
main (void)
{
  void (*orig) (int signo);
  setbuf (stdout, NULL);
  orig = signal (SIGHUP, SIG_IGN);
  if (orig != SIG_DFL)
    end ("signal (SIGHUP)");
  /* Verify we are the sole owner of the tty.  */
  if (ioctl (STDIN_FILENO, TIOCSCTTY, 0) != 0)
    end ("TIOCSCTTY");
  /* Disown the tty.  */
  if (ioctl (STDIN_FILENO, TIOCNOTTY) != 0)
    end ("TIOCNOTTY");
  end ("OK, disowned");
  return 1;
}
