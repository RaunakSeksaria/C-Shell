// ############## LLM Generated Code Begins ##############

#include <unistd.h>
#include <limits.h> // For LOGIN_NAME_MAX
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell_state.h"

void print_start_prompt(){

    // char total[LOGIN_NAME_MAX+HOST_NAME_MAX+PATH_MAX];

    // // ############## LLM Generated Code Begins ##############
    char username[LOGIN_NAME_MAX];
    int result = getlogin_r(username, sizeof(username));

    if (result != 0) {
        perror("getlogin_r");
        // Handle error, perhaps use a fallback
    } else {
        printf("<%s@", username);
    }
    char hostname[HOST_NAME_MAX];
    result = gethostname(hostname, sizeof(hostname));

    if (result != 0) {
        perror("gethostname");
        // Handle error
    } else {
        printf("%s:", hostname);
    }
    char *cwd = getcwd(NULL, 0);

    if (cwd == NULL) {
        perror("getcwd");
        // Handle error
    } else {
        // printf("%s>", cwd);
        // free(cwd); // IMPORTANT: Free the memory allocated by getcwd
    }
    // // ############## LLM Generated Code Ends ################
    // Use shell home directory instead of $HOME environment variable
    if(shell_home_dir[0] == '\0'){
        // If shell home directory is not set, use absolute path
        printf("%s> ", cwd);
    }
    else{
        //if matches, then i give ~ otherwise will keep original path
        size_t trial_n = strlen(shell_home_dir);
        if(!strncmp(cwd, shell_home_dir, trial_n)){
            //match
            if(strlen(cwd) == trial_n) {
                // Exact match - we're in the home directory
                printf("~> ");
            } else {
                // We're in a subdirectory of home
                printf("~%s> ", cwd + trial_n);
            }
        }
        else{
            printf("%s> ", cwd);
        }
    }
    free(cwd); // IMPORTANT: Free the memory allocated by getcwd
    return;
}
// ############## LLM Generated Code Ends ################