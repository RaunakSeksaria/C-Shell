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

static int g_suppress_log = 0;

static char *get_log_file_path(void) {
    if (shell_home_dir[0] == '\0') return NULL;

    size_t len = strlen(shell_home_dir) + strlen(LOG_FILE) + 2;
    char *path = malloc(len);
    snprintf(path, len, "%s/%s", shell_home_dir, LOG_FILE);
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
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
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
    if (g_suppress_log) return;
    if (strncmp(command, "log", 3) == 0) return;

    int count;
    char **commands = read_log_file(&count);

    if (count > 0 && strcmp(commands[count-1], command) == 0) {
        for (int i = 0; i < count; i++) free(commands[i]);
        free(commands);
        return;
    }

    if (count == MAX_COMMANDS) {
        free(commands[0]);
        for (int i = 0; i < count - 1; i++) commands[i] = commands[i + 1];
        count--;
    }

    commands = realloc(commands, (count + 1) * sizeof(char*));
    commands[count] = strdup(command);
    count++;

    write_log_file(commands, count);

    for (int i = 0; i < count; i++) free(commands[i]);
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
        for (int i = 0; i < count; i++) printf("%s\n", commands[i]);
    } else if (argc == 2 && strcmp(argv[1], "purge") == 0) {
        char *path = get_log_file_path();
        if (path) {
            unlink(path);
            free(path);
        }
    } else if (argc == 3 && strcmp(argv[1], "execute") == 0) {
        char *endptr;
        long index = strtol(argv[2], &endptr, 10);

        if (*endptr != '\0' || index < 1 || index > count) {
            printf("log: Please provide a valid index between 1 and %d\n", count);
            for (int i = 0; i < count; i++) free(commands[i]);
            free(commands);
            return -1;
        }

        index = count - index;
        char *cmd_to_execute = strdup(commands[index]);

        int token_count;
        Token *tokens = tokenize(cmd_to_execute, &token_count);

        if (!tokens) {
            printf("log: Failed to parse command\n");
            free(cmd_to_execute);
            return -1;
        }

        g_suppress_log = 1;
        int result = execute_command(tokens, token_count);
        g_suppress_log = 0;

        for (int i = 0; i < count; i++) free(commands[i]);
        free(commands);
        free(cmd_to_execute);
        free(tokens);
        return result;
    } else {
        printf("log: Invalid Syntax!\n");
        if (commands) {
            for (int i = 0; i < count; i++) free(commands[i]);
            free(commands);
        }
        return -1;
    }

    if (commands) {
        for (int i = 0; i < count; i++) free(commands[i]);
        free(commands);
    }

    return 0;
}
