#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell_state.h"

void print_start_prompt(void) {
    char username[LOGIN_NAME_MAX];
    if (getlogin_r(username, sizeof(username)) != 0) {
        perror("getlogin_r");
    } else {
        printf("<%s@", username);
    }

    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname");
    } else {
        printf("%s:", hostname);
    }

    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        perror("getcwd");
        return;
    }

    if (shell_home_dir[0] == '\0') {
        printf("%s> ", cwd);
    } else {
        size_t home_len = strlen(shell_home_dir);
        if (strncmp(cwd, shell_home_dir, home_len) == 0) {
            if (strlen(cwd) == home_len) {
                printf("~> ");
            } else {
                printf("~%s> ", cwd + home_len);
            }
        } else {
            printf("%s> ", cwd);
        }
    }

    free(cwd);
}
