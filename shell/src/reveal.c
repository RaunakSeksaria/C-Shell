#include "reveal.h"
#include "shell_state.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#define MAX_FILES 1024
#define MAX_PATH 4096

static int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

static void process_flags(char *arg, int *show_hidden, int *line_by_line) {
    if (arg[0] != '-') return;
    for (int i = 1; arg[i]; i++) {
        if (arg[i] == 'a') *show_hidden = 1;
        else if (arg[i] == 'l') *line_by_line = 1;
    }
}

static int list_directory(const char *path, int show_hidden, int line_by_line) {
    DIR *dir;
    struct dirent *entry;
    char *files[MAX_FILES];
    int file_count = 0;

    dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "No such directory!\n");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        if (!show_hidden && entry->d_name[0] == '.') continue;
        files[file_count] = xstrdup(entry->d_name);
        file_count++;
    }
    closedir(dir);

    qsort(files, file_count, sizeof(char*), compare_strings);

    for (int i = 0; i < file_count; i++) {
        if (line_by_line) {
            printf("%s\n", files[i]);
        } else {
            printf("%s%s", files[i], (i < file_count - 1) ? " " : "");
        }
        free(files[i]);
    }

    if (!line_by_line && file_count > 0) printf("\n");

    return 0;
}

int reveal_command(int argc, char *argv[]) {
    int show_hidden = 0;
    int line_by_line = 0;
    char target_path[MAX_PATH];

    if (shell_home_dir[0] == '\0') {
        fprintf(stderr, "Shell home directory not set!\n");
        return -1;
    }

    // Flags may appear anywhere and any number of times (their letters merge,
    // so `-a -l` == `-al`); reveal then accepts at most one path operand — the
    // lone non-flag token following the flags.
    int last_flag_index = 0;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            process_flags(argv[i], &show_hidden, &line_by_line);
            last_flag_index = i;
        }
    }

    if (last_flag_index + 2 < argc) {
        fprintf(stderr, "reveal: Invalid Syntax!\n");
        return -1;
    }

    if (last_flag_index + 1 >= argc) {
        if (getcwd(target_path, sizeof(target_path)) == NULL) {
            perror("getcwd");
            return -1;
        }
    } else {
        const char *path_arg = argv[last_flag_index + 1];

        if (strcmp(path_arg, "~") == 0) {
            snprintf(target_path, sizeof(target_path), "%s", shell_home_dir);
        } else if (strcmp(path_arg, ".") == 0) {
            if (getcwd(target_path, sizeof(target_path)) == NULL) {
                perror("getcwd");
                return -1;
            }
        } else if (strcmp(path_arg, "..") == 0) {
            if (getcwd(target_path, sizeof(target_path)) == NULL) {
                perror("getcwd");
                return -1;
            }
            strncat(target_path, "/..", sizeof(target_path) - strlen(target_path) - 1);
        } else if (strcmp(path_arg, "-") == 0) {
            if (!has_previous) {
                fprintf(stderr, "No such directory!\n");
                return -1;
            }
            snprintf(target_path, sizeof(target_path), "%s", previous_dir);
        } else {
            if (path_arg[0] == '~' && (path_arg[1] == '/' || path_arg[1] == '\0')) {
                if (path_arg[1] == '\0') {
                    snprintf(target_path, sizeof(target_path), "%s", shell_home_dir);
                } else {
                    snprintf(target_path, sizeof(target_path), "%s%s", shell_home_dir, path_arg + 1);
                }
                target_path[sizeof(target_path) - 1] = '\0';
            } else {
                snprintf(target_path, sizeof(target_path), "%s", path_arg);
            }
        }
    }

    return list_directory(target_path, show_hidden, line_by_line);
}
