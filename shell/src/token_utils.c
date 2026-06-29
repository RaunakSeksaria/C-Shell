#include "token_utils.h"
#include "util.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Rebuilds a command string from its tokens, joined by single spaces. Since
// tokens are whitespace-stripped, this yields a normalized form of the input
// (collapsed spacing) — that normalized text is what history stores and what
// background-job listings display, not the raw keystrokes. The buffer is sized
// to the exact length first, so an arbitrarily long line can never overflow it.
char* reconstruct_command(Token *tokens, int token_count) {
    size_t total = 1;  // trailing NUL
    for (int i = 0; i < token_count; i++) {
        total += tokens[i].len + (i > 0 ? 1 : 0);  // token plus its separating space
    }

    char *command = xmalloc(total);
    size_t pos = 0;
    for (int i = 0; i < token_count; i++) {
        if (i > 0) command[pos++] = ' ';
        memcpy(command + pos, tokens[i].text, tokens[i].len);
        pos += tokens[i].len;
    }
    command[pos] = '\0';
    return command;
}

// Appends len bytes to a heap buffer, doubling its capacity as needed and always
// leaving room for a trailing NUL. Returns the (possibly relocated) buffer.
static char *append(char *buf, size_t *cap, size_t *pos, const char *src, size_t len) {
    if (*pos + len + 1 > *cap) {
        while (*pos + len + 1 > *cap) *cap *= 2;
        buf = xrealloc(buf, *cap);
    }
    memcpy(buf + *pos, src, len);
    *pos += len;
    return buf;
}

// Expands $VAR references against the process environment; an unset variable
// expands to the empty string. The result grows on demand, so neither a long
// argument nor a large variable value can overflow it.
char* expand_env_vars(const char* arg) {
    size_t cap = 64, pos = 0;
    char *result = xmalloc(cap);

    for (size_t i = 0; arg[i] != '\0'; i++) {
        if (arg[i] == '$' && arg[i + 1] != '\0') {
            char name[256];
            size_t n = 0;
            i++;
            while (arg[i] != '\0' && (isalnum((unsigned char)arg[i]) || arg[i] == '_')) {
                if (n < sizeof(name) - 1) name[n++] = arg[i];
                i++;
            }
            i--;
            name[n] = '\0';

            const char *value = getenv(name);
            if (value) result = append(result, &cap, &pos, value, strlen(value));
        } else {
            result = append(result, &cap, &pos, &arg[i], 1);
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
