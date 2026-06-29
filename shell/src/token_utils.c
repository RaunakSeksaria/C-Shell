#include "token_utils.h"
#include "util.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Rebuilds a command string from its tokens, joined by single spaces. Since
// tokens are whitespace-stripped, this yields a normalized form of the input
// (collapsed spacing) — that normalized text is what history stores and what
// background-job listings display, not the raw keystrokes.
char* reconstruct_command(Token *tokens, int token_count) {
    char *command = xmalloc(4096);
    int pos = 0;

    for (int i = 0; i < token_count; i++) {
        if (i > 0) command[pos++] = ' ';
        strncpy(command + pos, tokens[i].text, tokens[i].len);
        pos += tokens[i].len;
    }
    command[pos] = '\0';
    return command;
}

// Expands $VAR references against the process environment; an unset variable
// expands to the empty string.
char* expand_env_vars(const char* arg) {
    char* result = xmalloc(4096);
    int pos = 0;

    for (int i = 0; arg[i] != '\0'; i++) {
        if (arg[i] == '$' && arg[i + 1] != '\0') {
            i++;
            char var_name[256] = {0};
            int var_pos = 0;

            while (arg[i] != '\0' && (isalnum(arg[i]) || arg[i] == '_')) {
                var_name[var_pos++] = arg[i++];
            }
            i--;

            char* value = getenv(var_name);
            if (value) {
                strcpy(result + pos, value);
                pos += strlen(value);
            }
        } else {
            result[pos++] = arg[i];
        }
    }

    result[pos] = '\0';
    return result;
}

int split_commands(Token *tokens, int token_count, int *cmd_starts, int *cmd_lens, int max_cmds) {
    int cmd_count = 0;
    int start = 0;
    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == TOKEN_PIPE) {
            if (cmd_count < max_cmds) {
                cmd_starts[cmd_count] = start;
                cmd_lens[cmd_count] = i - start;
                cmd_count++;
                start = i + 1;
            }
        }
    }
    if (cmd_count < max_cmds && start < token_count) {
        cmd_starts[cmd_count] = start;
        cmd_lens[cmd_count] = token_count - start;
        cmd_count++;
    }
    return cmd_count;
}

int split_sequences(Token *tokens, int token_count, int *seq_starts, int *seq_lens, int max_seqs) {
    int seq_count = 0;
    int start = 0;
    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == TOKEN_SEMICOLON) {
            if (seq_count < max_seqs) {
                seq_starts[seq_count] = start;
                seq_lens[seq_count] = i - start;
                seq_count++;
                start = i + 1;
            }
        }
    }
    if (seq_count < max_seqs && start < token_count) {
        seq_starts[seq_count] = start;
        seq_lens[seq_count] = token_count - start;
        seq_count++;
    }
    return seq_count;
}
