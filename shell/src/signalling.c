#include <signal.h>
#include "command.h"

// The shell catches SIGINT/SIGTSTP and forwards them to the foreground job
// instead of acting on them itself, so Ctrl-C / Ctrl-Z hit the running command
// while the shell survives. The negative pid targets the job's whole process
// group, reaching any children it spawned; with no foreground job the signal
// is simply swallowed.
void sigint_handler(int sig) {
    (void)sig;
    if (fg_pid > 0) {
        kill(-fg_pid, SIGINT);
    }
}

// Only forwards the signal — nothing more. The actual job bookkeeping (parking
// the stopped job, printing the notice) happens back in the waitpid sites in
// normal context, which keeps this handler async-signal-safe: it calls only
// kill(), unlike the printf/strcpy bookkeeping it used to do here.
void sigtstp_handler(int sig) {
    (void)sig;
    if (fg_pid > 0) {
        kill(-fg_pid, SIGTSTP);
        fg_stop_requested = 1;
    }
}
