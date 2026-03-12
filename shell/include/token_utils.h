#pragma once
#include "command.h"

char* reconstruct_command(Token *tokens, int token_count);
char* expand_env_vars(const char* arg);
int split_commands(Token *tokens, int token_count, int *cmd_starts, int *cmd_lens, int max_cmds);
int split_sequences(Token *tokens, int token_count, int *seq_starts, int *seq_lens, int max_seqs);