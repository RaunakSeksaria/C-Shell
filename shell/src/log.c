// ############## LLM Generated Code Begins ##############

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "input.h"
#include "command.h"
#include "shell_state.h"

#define MAX_COMMANDS 15
#define MAX_COMMAND_LENGTH 1024
#define LOG_FILE ".shell_history"

//for debugging purposes only
const char* token_type_to_string2(TokenType type) {
    switch (type) {
        case TOKEN_NAME: return "NAME";
        case TOKEN_PIPE: return "PIPE";
        case TOKEN_AMPERSAND: return "AMPERSAND";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_INPUT_REDIR: return "INPUT_REDIR";
        case TOKEN_OUTPUT_REDIR: return "OUTPUT_REDIR";
        case TOKEN_APPEND_REDIR: return "APPEND_REDIR";
        case TOKEN_END: return "END";
        case TOKEN_INVALID: return "INVALID";
        default: return "UNKNOWN";
    }
}

static char *get_log_file_path(void) {
    if (shell_home_dir[0] == '\0') return NULL;
    
    char *path = malloc(strlen(shell_home_dir) + strlen(LOG_FILE) + 2);
    sprintf(path, "%s/%s", shell_home_dir, LOG_FILE);
    return path;
}

static char **read_log_file(int *count) {
    char *path = get_log_file_path();
    if (!path) return NULL;

    FILE *fp = fopen(path, "r");
    if (!fp) {
        free(path);
        *count = 0;
        return NULL;
    }

    char **commands = malloc(MAX_COMMANDS * sizeof(char*));
    char line[MAX_COMMAND_LENGTH];
    *count = 0;

    while (fgets(line, sizeof(line), fp) && *count < MAX_COMMANDS) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        commands[*count] = strdup(line);
        (*count)++;
    }

    fclose(fp);
    free(path);
    return commands;
}

static void write_log_file(char **commands, int count) {
    char *path = get_log_file_path();
    if (!path) return;

    FILE *fp = fopen(path, "w");
    if (!fp) {
        free(path);
        return;
    }

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%s\n", commands[i]);
    }

    fclose(fp);
    free(path);
}

void store_command(const char *command) {
    // Don't store if command is from log execute
    if (getenv("DO_NOT_STORE_COMMAND")) {
        return;
    }

    // Don't store 'log' commands
    if (strncmp(command, "log", 3) == 0) {
        return;
    }

    int count;
    char **commands = read_log_file(&count);
    
    // Don't store if identical to last command
    if (count > 0 && strcmp(commands[count-1], command) == 0) {
        for (int i = 0; i < count; i++) {
            free(commands[i]);
        }
        free(commands);
        return;
    }

    // Make space if needed by removing oldest command
    if (count == MAX_COMMANDS) {
        free(commands[0]);
        for (int i = 0; i < count - 1; i++) {
            commands[i] = commands[i + 1];
        }
        count--;
    }

    // Add new command
    commands = realloc(commands, (count + 1) * sizeof(char*));
    commands[count] = strdup(command);
    count++;

    write_log_file(commands, count);

    for (int i = 0; i < count; i++) {
        free(commands[i]);
    }
    free(commands);
}

int log_command(int argc, char *argv[]) {
    if (argc > 3) {
        printf("log: Invalid Syntax!\n");
        return -1;
    }

    int count;
    char **commands = read_log_file(&count);

    if (argc == 1) {
        // Print all commands
        for (int i = 0; i < count; i++) {
            printf("%s\n", commands[i]);
        }
    }
    else if (argc == 2 && strcmp(argv[1], "purge") == 0) {
        // Clear history
        char *path = get_log_file_path();
        if (path) {
            unlink(path);
            free(path);
        }
    }
    else if (argc == 3 && strcmp(argv[1], "execute") == 0) {
        // Execute command at index
        char *endptr;
        long index = strtol(argv[2], &endptr, 10);
        
        if (*endptr != '\0' || index < 1 || index > count) {
            printf("log: Please provide a valid index between 1 and %d\n", count);
            for (int i = 0; i < count; i++) {
                free(commands[i]);
            }
            free(commands);
            return -1;
        }

        // Convert to 0-based index (newest = count-1, oldest = 0)
        index = count - index;
        
        // Store command to execute
        char *cmd_to_execute = strdup(commands[index]);

        // Execute the stored command
        int token_count;
        Token *tokens = tokenize(cmd_to_execute, &token_count);
        
        if (!tokens) {
            printf("log: Failed to parse command\n");
            return -1;
        }
        setenv("DO_NOT_STORE_COMMAND", "1", 1);

        int result = execute_command(tokens, token_count);
        unsetenv("DO_NOT_STORE_COMMAND");

        // Clean up commands array
        for (int i = 0; i < count; i++) {
            free(commands[i]);
        }
        free(commands);
        free(cmd_to_execute);
        free(tokens);
        return result;
    }
    else {
        printf("log: Invalid Syntax!\n");
        return -1;
    }

    if (commands) {
        for (int i = 0; i < count; i++) {
            free(commands[i]);
        }
        free(commands);
    }

    return 0;
}
// ############## LLM Generated Code Ends ################