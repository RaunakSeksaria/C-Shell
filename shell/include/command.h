#ifndef COMMAND_H
#define COMMAND_H

#include "input.h"
#include <sys/types.h>
// Function to execute a command based on tokens
int execute_command(Token *tokens, int token_count);
extern pid_t fg_pid;         // declare, not define
extern char fg_command[1024];
extern char fg_full_command[1024];  // Store full command for fg process
#endif // COMMAND_H