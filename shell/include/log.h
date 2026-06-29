#ifndef LOG_H
#define LOG_H

#include "input.h"

// Handle the `log` builtin: display history, `purge`, or `execute <index>`.
int log_command(int argc, char *argv[]);

// Record one input line in history; called once per line from the shell loop.
void log_record(Token *tokens, int token_count);

#endif // LOG_H
