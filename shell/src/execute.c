#include "execute.h"
#include "command.h"
#include "token_utils.h"
#include "redirection.h"
#include "background.h"
#include "hop.h"
#include "reveal.h"
#include "log.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "ping.h"

#define MAX_SEQS 64   // max ';'-separated commands in one line
#define MAX_CMDS 32   // max '|'-separated stages in one pipeline
#define MAX_ARGS 256  // max arguments (incl. command name) per command

pid_t fg_pid = -1;
char fg_command[1024] = "";
char fg_full_command[1024] = "";

// Update execute_external_command to handle background jobs
static int execute_external_command(char **args, Token *tokens, int token_count, int is_background) {
    pid_t pid = fork();

    if (pid == -1) {
        // Fork failed
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        // Child process

        // Redirect stdin to /dev/null for background jobs
        if (is_background) {
            int devnull = open("/dev/null", O_RDONLY);
            if (devnull != -1) {
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            }
        }

        // Set up input redirection
        if (setup_input_redirection(tokens, token_count) == -1) {
            exit(1);
        }

        // Set up output redirection
        if (setup_output_redirection(tokens, token_count) == -1) {
            exit(1);
        }

        execvp(args[0], args);
        // If execvp returns, it means it failed
        fprintf(stderr, "Command not found!\n");
        exit(1);
    } else {
        // Parent process
        if (is_background) {
            // Add job to background job list
            if (bg_job_count < MAX_BG_JOBS) {
                bg_jobs[bg_job_count].job_number = next_job_number;
                bg_jobs[bg_job_count].pid = pid;
                strncpy(bg_jobs[bg_job_count].command_name, args[0], 255);
                bg_jobs[bg_job_count].command_name[255] = '\0';
                
                // Store full command
                char *full_cmd = reconstruct_command(tokens, token_count);
                strncpy(bg_jobs[bg_job_count].full_command, full_cmd, 1023);
                bg_jobs[bg_job_count].full_command[1023] = '\0';
                free(full_cmd);
                
                // Set initial state as running
                strcpy(bg_jobs[bg_job_count].state, "Running");
                
                bg_job_count++;
            }
            printf("[%d] %d\n", next_job_number, pid);
            next_job_number++;
            // Do not wait for child, return immediately
            return 0;
        } else {
            fg_pid = pid;
            strncpy(fg_command, args[0], sizeof(fg_command)-1);
            fg_command[sizeof(fg_command)-1] = '\0';
            
            // Store full command for fg process
            char *full_cmd = reconstruct_command(tokens, token_count);
            strncpy(fg_full_command, full_cmd, sizeof(fg_full_command)-1);
            fg_full_command[sizeof(fg_full_command)-1] = '\0';
            free(full_cmd);
            
            int status;
            waitpid(pid, &status, WUNTRACED);  // allow detecting stop signals
            fg_pid = -1;  // reset after process finishes/stops
            return WEXITSTATUS(status);
        }
    }
}



int execute_command(Token *tokens, int token_count) {
    if (token_count == 0) return 0;

    // Check for background execution
    int is_background = 0;
    if (token_count > 0 && tokens[token_count - 1].type == TOKEN_AMPERSAND) {
        is_background = 1;
        token_count--; // Remove '&' from tokens
    }

    // Check for sequential execution
    int seq_count = 0;
    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == TOKEN_SEMICOLON) seq_count++;
    }
    if (seq_count > 0) {
        int seq_starts[MAX_SEQS], seq_lens[MAX_SEQS];
        int nseqs = split_sequences(tokens, token_count, seq_starts, seq_lens, MAX_SEQS);
        int last_status = 0;
        for (int i = 0; i < nseqs; i++) {
            last_status = execute_command(&tokens[seq_starts[i]], seq_lens[i]);
            // Continue even if a command fails
        }
        return last_status;
    }

    // Detect if there are pipes
    int pipe_count = 0;
    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == TOKEN_PIPE) pipe_count++;
    }
    if (pipe_count == 0) {
        char *args[MAX_ARGS];
        int arg_count = 0;

        // Convert tokens to arguments, skipping redirection operators and their filenames
        for (int i = 0; i < token_count && arg_count < MAX_ARGS - 1; i++) {
            if (tokens[i].type == TOKEN_NAME) {
                // Skip if this is a filename following a redirection operator
                if (i > 0 && (tokens[i-1].type == TOKEN_INPUT_REDIR ||
                             tokens[i-1].type == TOKEN_OUTPUT_REDIR ||
                             tokens[i-1].type == TOKEN_APPEND_REDIR)) {
                    continue;
                }
                
                char* temp = malloc(tokens[i].len + 1);
                strncpy(temp, tokens[i].text, tokens[i].len);
                temp[tokens[i].len] = '\0';
                
                // Expand environment variables
                args[arg_count] = expand_env_vars(temp);
                free(temp);
                arg_count++;
            }
        }
        args[arg_count] = NULL;  // NULL terminate the argument list

        // Check for built-in commands first
        int result = -1;
        if (arg_count > 0) {
            if (strcmp(args[0], "hop") == 0) {
                result = hop_command(arg_count, args);
            }
            else if (strcmp(args[0], "reveal") == 0) {
                result = reveal_command(arg_count, args);
            }
            else if (strcmp(args[0], "log") == 0) {
                result = log_command(arg_count, args);
            }
            else if (strcmp(args[0], "activities") == 0) {
                result = activities_command();
            }
            else if(strcmp(args[0], "ping") == 0){
                result = ping_command(arg_count,args);
            }
            else if (strcmp(args[0], "fg") == 0) {
                result = fg_builtin(arg_count, args);
            }
            else if (strcmp(args[0], "bg") == 0) {
                result = bg_builtin(arg_count, args);
            }
            else {
                // Not a built-in command, try to execute it as an external command
                result = execute_external_command(args, tokens, token_count,is_background);
            }
            
            // Store command in log if it's not a log command
            if (strcmp(args[0], "log") != 0 && result != -1) {
                char *command = reconstruct_command(tokens, token_count);
                store_command(command);
                free(command);
            }
        }

        // Free allocated memory
        for (int i = 0; i < arg_count; i++) {
            free(args[i]);
        }

        return result;
    }

    // Handle piped commands
    int cmd_starts[MAX_CMDS], cmd_lens[MAX_CMDS];
    int cmd_count = split_commands(tokens, token_count, cmd_starts, cmd_lens, MAX_CMDS);

    int pipes[MAX_CMDS-1][2];
    for (int i = 0; i < cmd_count - 1; i++) {
        pipe(pipes[i]);
    }

    pid_t pids[MAX_CMDS];
    for (int i = 0; i < cmd_count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process

            // Input redirection for first command
            if (i == 0) {
                setup_input_redirection(&tokens[cmd_starts[i]], cmd_lens[i]);
            }
            // Output redirection for last command
            if (i == cmd_count - 1) {
                setup_output_redirection(&tokens[cmd_starts[i]], cmd_lens[i]);
            }

            // If not first, set stdin to previous pipe
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            // If not last, set stdout to next pipe
            if (i < cmd_count - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Close all pipe fds in child
            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Build args for this command
            char *args[MAX_ARGS];
            int arg_count = 0;
            for (int k = 0; k < cmd_lens[i] && arg_count < MAX_ARGS - 1; k++) {
                Token *t = &tokens[cmd_starts[i] + k];
                if (t->type == TOKEN_NAME) {
                    if (k > 0 && (tokens[cmd_starts[i] + k - 1].type == TOKEN_INPUT_REDIR ||
                                  tokens[cmd_starts[i] + k - 1].type == TOKEN_OUTPUT_REDIR ||
                                  tokens[cmd_starts[i] + k - 1].type == TOKEN_APPEND_REDIR)) {
                        continue;
                    }
                    char* temp = malloc(t->len + 1);
                    strncpy(temp, t->text, t->len);
                    temp[t->len] = '\0';
                    args[arg_count] = expand_env_vars(temp);
                    free(temp);
                    arg_count++;
                }
            }
            args[arg_count] = NULL;

            // Execute command
            execvp(args[0], args);
            fprintf(stderr, "Command not found!\n");
            exit(1);
        }
        pids[i] = pid;
    }

    // Parent closes all pipe fds
    for (int i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all children
    int status = 0;
    for (int i = 0; i < cmd_count; i++) {
        int s;
        if(!is_background) waitpid(pids[i], &s, 0);
        if (i == cmd_count - 1) status = WEXITSTATUS(s);
    }
    return status;
}
