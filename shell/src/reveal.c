// ############## LLM Generated Code Begins ##############

#include "reveal.h"
#include "shell_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#define MAX_FILES 1024
#define MAX_PATH 4096

// Compare function for qsort
static int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

// Helper function to process flags
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

    // Try to open directory
    dir = opendir(path);
    if (!dir) {
        printf("No such directory!\n");
        return -1;
    }

    // Read all entries
    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        // Skip hidden files unless show_hidden is true
        if (!show_hidden && entry->d_name[0] == '.') {
            continue;
        }
        
        files[file_count] = strdup(entry->d_name);
        file_count++;
    }
    closedir(dir);

    // Sort files lexicographically
    qsort(files, file_count, sizeof(char*), compare_strings);

    // Print files
    for (int i = 0; i < file_count; i++) {
        if (line_by_line) {
            printf("%s\n", files[i]);
        } else {
            printf("%s%s", files[i], (i < file_count - 1) ? " " : "");
        }
        free(files[i]);
    }
    
    if (!line_by_line && file_count > 0) {
        printf("\n");
    }

    return 0;
}

int reveal_command(int argc, char *argv[]) {
    int show_hidden = 0;
    int line_by_line = 0;
    char target_path[MAX_PATH];

    if (shell_home_dir[0] == '\0') {
        printf("Shell home directory not set!\n");
        return -1;
    }

    // Process all flags first
    int last_flag_index = 0;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            process_flags(argv[i], &show_hidden, &line_by_line);
            last_flag_index = i;
        }
    }

    // Check if there's a path argument after the flags
    if (last_flag_index + 2 < argc) {
        printf("reveal: Invalid Syntax!\n");
        return -1;
    }

    // Get the target path
    if (last_flag_index + 1 >= argc) {
        // No path argument, use current directory
        if (getcwd(target_path, sizeof(target_path)) == NULL) {
            perror("getcwd");
            return -1;
        }
    } else {
        const char *path_arg = argv[last_flag_index + 1];
        
        if (strcmp(path_arg, "~") == 0) {
            strncpy(target_path, shell_home_dir, sizeof(target_path));
        }
        else if (strcmp(path_arg, ".") == 0) {
            if (getcwd(target_path, sizeof(target_path)) == NULL) {
                perror("getcwd");
                return -1;
            }
        }
        else if (strcmp(path_arg, "..") == 0) {
            if (getcwd(target_path, sizeof(target_path)) == NULL) {
                perror("getcwd");
                return -1;
            }
            strcat(target_path, "/..");
        }
        else if (strcmp(path_arg, "-") == 0) {
            if (!has_previous) {
                printf("No such directory!\n");
                return -1;
            }
            strncpy(target_path, previous_dir, sizeof(target_path));
        }
        else {
            // Handle tilde expansion for paths like ~/Downloads
            if (path_arg[0] == '~' && (path_arg[1] == '/' || path_arg[1] == '\0')) {
                // Expand ~ to shell home directory
                if (path_arg[1] == '\0') {
                    // Just ~ - already handled above, but for safety
                    strncpy(target_path, shell_home_dir, sizeof(target_path));
                } else {
                    // ~/path format
                    snprintf(target_path, sizeof(target_path), "%s%s", shell_home_dir, path_arg + 1);
                }
                target_path[sizeof(target_path) - 1] = '\0';
            } else {
                // Use path as-is
                strncpy(target_path, path_arg, sizeof(target_path));
            }
        }
    }

    return list_directory(target_path, show_hidden, line_by_line);
}
// ############## LLM Generated Code Ends ################