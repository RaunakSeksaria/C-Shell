// ############## LLM Generated Code Begins ##############

#include "token_utils.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Helper function to reconstruct original command
char* reconstruct_command(Token *tokens, int token_count) {
    char *command = malloc(4096);  // Reasonable buffer size
    int pos = 0;
    
    for (int i = 0; i < token_count; i++) {
        if (i > 0) command[pos++] = ' ';
        strncpy(command + pos, tokens[i].text, tokens[i].len);
        pos += tokens[i].len;
    }
    command[pos] = '\0';
    return command;
}

// Helper function to expand environment variables in a string
char* expand_env_vars(const char* arg) {
    char* result = malloc(4096);  // Reasonable buffer size
    int pos = 0;
    
    for (int i = 0; arg[i] != '\0'; i++) {
        if (arg[i] == '$' && arg[i + 1] != '\0') {
            // Found a potential environment variable
            i++;  // Skip the $
            char var_name[256] = {0};
            int var_pos = 0;
            
            // Collect the variable name
            while (arg[i] != '\0' && (isalnum(arg[i]) || arg[i] == '_')) {
                var_name[var_pos++] = arg[i++];
            }
            i--;  // Move back one as the loop will increment
            
            // Look up the environment variable
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

// Helper to find pipe positions and split tokens into commands
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
    // Add last command
    if (cmd_count < max_cmds && start < token_count) {
        cmd_starts[cmd_count] = start;
        cmd_lens[cmd_count] = token_count - start;
        cmd_count++;
    }
    return cmd_count;
}

// Helper to split tokens by semicolon
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
    // Add last sequence
    if (seq_count < max_seqs && start < token_count) {
        seq_starts[seq_count] = start;
        seq_lens[seq_count] = token_count - start;
        seq_count++;
    }
    return seq_count;
}
// ############## LLM Generated Code Ends ################
