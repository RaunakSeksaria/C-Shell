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
#include "util.h"

#define MAX_SEQS 64   // max ';'-separated commands in one line
#define MAX_CMDS 32   // max '|'-separated stages in one pipeline
#define MAX_ARGS 256  // max arguments (incl. command name) per command

pid_t fg_pid = -1;
char fg_command[1024] = "";
char fg_full_command[1024] = "";

static int execute_external_command(char **args, Token *tokens, int token_count, int is_background) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        // Background jobs read from /dev/null so they can never steal keystrokes
        // from the terminal the interactive shell is still reading.
        if (is_background) {
            int devnull = open("/dev/null", O_RDONLY);
            if (devnull != -1) {
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            }
        }

        if (setup_input_redirection(tokens, token_count) == -1) {
            exit(1);
        }
        if (setup_output_redirection(tokens, token_count) == -1) {
            exit(1);
        }

        execvp(args[0], args);
        // execvp only returns on failure, so reaching here means the lookup failed.
        fprintf(stderr, "Command not found!\n");
        exit(1);
    } else {
        if (is_background) {
            if (bg_job_count < MAX_BG_JOBS) {
                bg_jobs[bg_job_count].job_number = next_job_number;
                bg_jobs[bg_job_count].pid = pid;
                strncpy(bg_jobs[bg_job_count].command_name, args[0], 255);
                bg_jobs[bg_job_count].command_name[255] = '\0';

                char *full_cmd = reconstruct_command(tokens, token_count);
                strncpy(bg_jobs[bg_job_count].full_command, full_cmd, 1023);
                bg_jobs[bg_job_count].full_command[1023] = '\0';
                free(full_cmd);

                strcpy(bg_jobs[bg_job_count].state, "Running");
                bg_job_count++;
            }
            printf("[%d] %d\n", next_job_number, pid);
            next_job_number++;
            return 0;
        } else {
            // Publish the foreground child so the SIGINT/SIGTSTP handlers can
            // forward terminal signals to it (see signalling.c).
            fg_pid = pid;
            strncpy(fg_command, args[0], sizeof(fg_command)-1);
            fg_command[sizeof(fg_command)-1] = '\0';

            char *full_cmd = reconstruct_command(tokens, token_count);
            strncpy(fg_full_command, full_cmd, sizeof(fg_full_command)-1);
            fg_full_command[sizeof(fg_full_command)-1] = '\0';
            free(full_cmd);

            int status;
            waitpid(pid, &status, WUNTRACED);  // WUNTRACED: return on Ctrl-Z stop, not only on exit
            fg_pid = -1;
            return WEXITSTATUS(status);
        }
    }
}

// Dispatches one parsed line. A trailing '&' is peeled off first, then the
// separators are handled in precedence order: ';' sequences recurse back into
// this function, '|' pipelines fork a child per stage, and a bare command runs
// directly. Builtins are matched before exec so commands like hop and log can
// mutate the shell's own state (cwd, history) instead of a throwaway child's.
int execute_command(Token *tokens, int token_count) {
    if (token_count == 0) return 0;

    int is_background = 0;
    if (token_count > 0 && tokens[token_count - 1].type == TOKEN_AMPERSAND) {
        is_background = 1;
        token_count--;
    }

    int seq_count = 0;
    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == TOKEN_SEMICOLON) seq_count++;
    }
    if (seq_count > 0) {
        int seq_starts[MAX_SEQS], seq_lens[MAX_SEQS];
        int nseqs = split_sequences(tokens, token_count, seq_starts, seq_lens, MAX_SEQS);
        int last_status = 0;
        for (int i = 0; i < nseqs; i++) {
            // A failing segment never aborts the rest: `a ; b` always runs b.
            last_status = execute_command(&tokens[seq_starts[i]], seq_lens[i]);
        }
        return last_status;
    }

    int pipe_count = 0;
    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == TOKEN_PIPE) pipe_count++;
    }
    if (pipe_count == 0) {
        char *args[MAX_ARGS];
        int arg_count = 0;

        // Build argv from NAME tokens, copying each out of the input buffer and
        // expanding $VARs. Redirection targets are skipped here; redirection.c
        // consumes them separately straight from the token stream.
        for (int i = 0; i < token_count && arg_count < MAX_ARGS - 1; i++) {
            if (tokens[i].type == TOKEN_NAME) {
                if (i > 0 && (tokens[i-1].type == TOKEN_INPUT_REDIR ||
                             tokens[i-1].type == TOKEN_OUTPUT_REDIR ||
                             tokens[i-1].type == TOKEN_APPEND_REDIR)) {
                    continue;
                }

                char* temp = xmalloc(tokens[i].len + 1);
                strncpy(temp, tokens[i].text, tokens[i].len);
                temp[tokens[i].len] = '\0';

                args[arg_count] = expand_env_vars(temp);
                free(temp);
                arg_count++;
            }
        }
        args[arg_count] = NULL;

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
            else if (strcmp(args[0], "ping") == 0) {
                result = ping_command(arg_count, args);
            }
            else if (strcmp(args[0], "fg") == 0) {
                result = fg_builtin(arg_count, args);
            }
            else if (strcmp(args[0], "bg") == 0) {
                result = bg_builtin(arg_count, args);
            }
            else {
                result = execute_external_command(args, tokens, token_count, is_background);
            }
        }

        for (int i = 0; i < arg_count; i++) {
            free(args[i]);
        }

        return result;
    }

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
            // Only the ends touch files: the first stage may read a file, the
            // last may redirect out; interior stages are wired pipe-to-pipe.
            if (i == 0) {
                setup_input_redirection(&tokens[cmd_starts[i]], cmd_lens[i]);
            }
            if (i == cmd_count - 1) {
                setup_output_redirection(&tokens[cmd_starts[i]], cmd_lens[i]);
            }

            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            if (i < cmd_count - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // After dup2 every child closes all pipe fds: a single write end
            // left open anywhere prevents a downstream reader from seeing EOF.
            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

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
                    char* temp = xmalloc(t->len + 1);
                    strncpy(temp, t->text, t->len);
                    temp[t->len] = '\0';
                    args[arg_count] = expand_env_vars(temp);
                    free(temp);
                    arg_count++;
                }
            }
            args[arg_count] = NULL;

            execvp(args[0], args);
            fprintf(stderr, "Command not found!\n");
            exit(1);
        }
        pids[i] = pid;
    }

    // The parent owns no pipe end it needs; closing them all is what lets each
    // stage see EOF once its upstream writer exits.
    for (int i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    int status = 0;
    for (int i = 0; i < cmd_count; i++) {
        int s;
        if (!is_background) waitpid(pids[i], &s, 0);
        // Pipeline status is that of its last stage, matching POSIX shells.
        if (i == cmd_count - 1) status = WEXITSTATUS(s);
    }
    return status;
}
