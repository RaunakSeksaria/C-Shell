#include "hop.h"
#include "shell_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>

static int change_directory(const char *path) {
    char current[PATH_MAX];

    if (getcwd(current, sizeof(current)) == NULL) {
        perror("getcwd");
        return -1;
    }

    if (chdir(path) == -1) {
        printf("No such directory!\n");
        return -1;
    }

    strncpy(previous_dir, current, PATH_MAX);
    has_previous = 1;
    return 0;
}

int hop_command(int argc, char *argv[]) {
    if (shell_home_dir[0] == '\0') {
        printf("Shell home directory not set!\n");
        return -1;
    }

    if (argc == 1) {
        return change_directory(shell_home_dir);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "~") == 0) {
            if (change_directory(shell_home_dir) == -1) return -1;
        } else if (strcmp(argv[i], ".") == 0) {
            continue;
        } else if (strcmp(argv[i], "..") == 0) {
            if (change_directory("..") == -1) return -1;
        } else if (strcmp(argv[i], "-") == 0) {
            if (!has_previous) continue;
            if (change_directory(previous_dir) == -1) return -1;
        } else {
            char expanded_path[PATH_MAX];
            if (argv[i][0] == '~' && (argv[i][1] == '/' || argv[i][1] == '\0')) {
                if (argv[i][1] == '\0') {
                    strncpy(expanded_path, shell_home_dir, PATH_MAX - 1);
                } else {
                    snprintf(expanded_path, PATH_MAX, "%s%s", shell_home_dir, argv[i] + 1);
                }
                expanded_path[PATH_MAX - 1] = '\0';
                if (change_directory(expanded_path) == -1) return -1;
            } else {
                if (change_directory(argv[i]) == -1) return -1;
            }
        }
    }

    return 0;
}
