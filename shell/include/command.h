#ifndef COMMAND_H
#define COMMAND_H

#include "input.h"
#include <sys/types.h>
#include <signal.h>
// Function to execute a command based on tokens
int execute_command(Token *tokens, int token_count);
extern pid_t fg_pid;         // declare, not define
extern char fg_command[1024];
extern char fg_full_command[1024];  // Store full command for fg process

// Set by the SIGTSTP handler when Ctrl-Z hits a foreground job; the wait loops
// read it to park the job in normal (async-signal-safe) context.
extern volatile sig_atomic_t fg_stop_requested;
#endif // COMMAND_H