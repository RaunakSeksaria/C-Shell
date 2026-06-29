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
#include "util.h"

#define MAX_COMMANDS 15
#define MAX_COMMAND_LENGTH 1024
#define LOG_FILE ".shell_history"

// Set only while `log execute` runs a replayed command, so that command (and
// the `log execute` invocation itself) is not appended back into the history.
static int g_suppress_log = 0;

static char *get_log_file_path(void) {
    if (shell_home_dir[0] == '\0') return NULL;

    size_t len = strlen(shell_home_dir) + strlen(LOG_FILE) + 2;
    char *path = xmalloc(len);
    snprintf(path, len, "%s/%s", shell_home_dir, LOG_FILE);
    return path;
}

// Always returns an owned array of *count entries (0 when there is no history
// file yet), never NULL. Treating "no history" as a valid empty array lets
// callers index and free unconditionally instead of relying on a fragile
// "NULL means empty" convention.
static char **read_log_file(int *count) {
    *count = 0;
    char **commands = xmalloc(MAX_COMMANDS * sizeof(char*));

    char *path = get_log_file_path();
    if (!path) return commands;

    FILE *fp = fopen(path, "r");
    free(path);
    if (!fp) return commands;

    char line[MAX_COMMAND_LENGTH];
    while (fgets(line, sizeof(line), fp) && *count < MAX_COMMANDS) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
        commands[*count] = xstrdup(line);
        (*count)++;
    }

    fclose(fp);
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

    // Collapse consecutive repeats: re-running the same command does not grow
    // the history.
    if (count > 0 && strcmp(commands[count-1], command) == 0) {
        for (int i = 0; i < count; i++) free(commands[i]);
        free(commands);
        return;
    }

    // Fixed-size ring of the MAX_COMMANDS most recent entries: once full, drop
    // the oldest to make room.
    if (count == MAX_COMMANDS) {
        free(commands[0]);
        for (int i = 0; i < count - 1; i++) commands[i] = commands[i + 1];
        count--;
    }

    commands = xrealloc(commands, (count + 1) * sizeof(char*));
    commands[count] = xstrdup(command);
    count++;

    write_log_file(commands, count);

    for (int i = 0; i < count; i++) free(commands[i]);
    free(commands);
}

int log_command(int argc, char *argv[]) {
    if (argc > 3) {
        fprintf(stderr, "log: Invalid Syntax!\n");
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
            fprintf(stderr, "log: Please provide a valid index between 1 and %d\n", count);
            for (int i = 0; i < count; i++) free(commands[i]);
            free(commands);
            return -1;
        }

        // Index 1 is the most recent command; the file stores oldest-first, so
        // invert into an array position.
        index = count - index;
        char *cmd_to_execute = xstrdup(commands[index]);

        int token_count;
        Token *tokens = tokenize(cmd_to_execute, &token_count);

        if (!tokens) {
            fprintf(stderr, "log: Failed to parse command\n");
            for (int i = 0; i < count; i++) free(commands[i]);
            free(commands);
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
        fprintf(stderr, "log: Invalid Syntax!\n");
        for (int i = 0; i < count; i++) free(commands[i]);
        free(commands);
        return -1;
    }

    for (int i = 0; i < count; i++) free(commands[i]);
    free(commands);

    return 0;
}
