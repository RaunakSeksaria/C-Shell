#include "shell_prompt.h"
#include "input.h"
#include "command.h"
#include "background.h"
#include "signalling.h"
#include "shell_state.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>

void sigint_handler(int sig);
void sigtstp_handler(int sig);

void shell_loop(void) {
    char *input = NULL;
    size_t bufsize = 0;

    while(1) {
        print_start_prompt();
        
        ssize_t characters = take_input(&input, &bufsize);

        if (characters == -1) {
            // Ctrl-D (EOF): tear down background jobs so none are left orphaned,
            // then exit the way a login shell does.
            for (int i = 0; i < bg_job_count; i++) {
                kill(bg_jobs[i].pid, SIGKILL);
            }

            printf("logout\n");
            exit(0);
        }

        if (characters > 0 && input[characters - 1] == '\n') {
            input[characters - 1] = '\0';
        }
        // Reap and report finished jobs before the next prompt, so completion
        // notices interleave with the prompt the way an interactive shell expects.
        check_background_jobs();
        int token_count;
        Token *tokens = tokenize(input, &token_count);

        if (!parse_input(tokens, token_count)) {
            fprintf(stderr, "Invalid Syntax!\n");
        } else {
            log_record(tokens, token_count);
            execute_command(tokens, token_count);
        }

        free(tokens);
    }

    free(input);
}

int main(){
    signal(SIGINT, sigint_handler);   // Ctrl-C
    signal(SIGTSTP, sigtstp_handler); // Ctrl-Z

    // The launch directory becomes the shell's "home": '~' in the prompt and
    // `hop` with no argument resolve to it, independent of $HOME.
    char *cwd = getcwd(NULL, 0);
    if (cwd) {
        strncpy(shell_home_dir, cwd, PATH_MAX - 1);
        shell_home_dir[PATH_MAX - 1] = '\0';
        free(cwd);
    }

    shell_loop();
    return 0;
}