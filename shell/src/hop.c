// ############## LLM Generated Code Begins ##############

#include "hop.h"
#include "shell_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <limits.h>

// Define the variables globally (no static)
char previous_dir[PATH_MAX] = "";  // Store the previous directory
int has_previous = 0;              // Flag to track if we have a previous directory
char shell_home_dir[PATH_MAX] = "";  // Store the shell home directory

// Helper function to change directory and handle errors
static int change_directory(const char *path) {
    char current[PATH_MAX];
    
    // Save current directory before changing
    if (getcwd(current, sizeof(current)) == NULL) {
        perror("getcwd");
        return -1;
    }

    if (chdir(path) == -1) {
        printf("No such directory!\n");
        return -1;
    }

    // Only update previous directory if change was successful
    strncpy(previous_dir, current, PATH_MAX);
    has_previous = 1;
    return 0;
}

int hop_command(int argc, char *argv[]) {
    if (shell_home_dir[0] == '\0') {
        printf("Shell home directory not set!\n");
        return -1;
    }

    // If no arguments, go to home directory
    if (argc == 1) {
        return change_directory(shell_home_dir);
    }

    // Process each argument sequentially
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "~") == 0) {
            // Change to home directory
            if (change_directory(shell_home_dir) == -1) {
                return -1;
            }
        }
        else if (strcmp(argv[i], ".") == 0) {
            // Do nothing for current directory
            continue;
        }
        else if (strcmp(argv[i], "..") == 0) {
            // Change to parent directory
            if (change_directory("..") == -1) {
                return -1;
            }
        }
        else if (strcmp(argv[i], "-") == 0) {
            // Change to previous directory if it exists
            if (!has_previous) {
                continue;  // Do nothing if no previous directory
            }
            if (change_directory(previous_dir) == -1) {
                return -1;
            }
        }
        else {
            // Handle tilde expansion for paths like ~/Downloads
            char expanded_path[PATH_MAX];
            if (argv[i][0] == '~' && (argv[i][1] == '/' || argv[i][1] == '\0')) {
                // Expand ~ to shell home directory
                if (argv[i][1] == '\0') {
                    // Just ~ - already handled above, but for safety
                    strncpy(expanded_path, shell_home_dir, PATH_MAX - 1);
                } else {
                    // ~/path format
                    snprintf(expanded_path, PATH_MAX, "%s%s", shell_home_dir, argv[i] + 1);
                }
                expanded_path[PATH_MAX - 1] = '\0';
                if (change_directory(expanded_path) == -1) {
                    return -1;
                }
            } else {
                // Change to specified path as-is
                if (change_directory(argv[i]) == -1) {
                    return -1;
                }
            }
        }
    }

    return 0;
}
// ############## LLM Generated Code Ends ################
