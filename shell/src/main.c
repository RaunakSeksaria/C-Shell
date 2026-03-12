// ############## LLM Generated Code Begins ##############

#include "shell_prompt.h"
#include "input.h"
#include "command.h"
#include "background.h"
#include "signalling.h"
#include "shell_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>

// prototypes for your handlers
void sigint_handler(int sig);
void sigtstp_handler(int sig);

// Helper function to convert token types to strings for printing
const char* token_type_to_string(TokenType type) {
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

void shell_loop(void) {
    char *input = NULL;
    size_t bufsize = 0;

    while(1) {
        print_start_prompt();
        
        ssize_t characters = take_input(&input, &bufsize);

        if (characters == -1) {
            // Ctrl-D pressed, EOF received
            // kill all background jobs
            for (int i = 0; i < bg_job_count; i++) {
                kill(bg_jobs[i].pid, SIGKILL);
            }

            printf("logout\n");
            exit(0);  // clean exit
        }

        if (characters > 0 && input[characters - 1] == '\n') {
            input[characters - 1] = '\0';
        }
        // check for completed background jobs before parsing this input
        check_background_jobs();
        int token_count;
        Token *tokens = tokenize(input, &token_count);

        if (!parse_input(tokens, token_count)) {
            printf("Invalid Syntax!\n");
        } else {
            execute_command(tokens, token_count);
        }

        free(tokens);
    }

    free(input);
}

int main(){
    // install the handlers
    signal(SIGINT, sigint_handler);   // Ctrl-C
    signal(SIGTSTP, sigtstp_handler); // Ctrl-Z

    // Initialize shell home directory to current working directory
    char *cwd = getcwd(NULL, 0);
    if (cwd) {
        strncpy(shell_home_dir, cwd, PATH_MAX - 1);
        shell_home_dir[PATH_MAX - 1] = '\0';
        free(cwd);
    }

    shell_loop();
    return 0;
}
// ############## LLM Generated Code Ends ################