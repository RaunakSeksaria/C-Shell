#include "redirection.h"
#include "util.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Redirections use last-one-wins semantics: in `cmd > a > b` output goes to b,
// matching POSIX shells. Both helpers therefore scan the whole token range and
// keep only the final target before opening anything.

int setup_input_redirection(Token *tokens, int token_count) {
    char *input_file = NULL;

    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == TOKEN_INPUT_REDIR && i + 1 < token_count) {
            char *filename = xmalloc(tokens[i + 1].len + 1);
            strncpy(filename, tokens[i + 1].text, tokens[i + 1].len);
            filename[tokens[i + 1].len] = '\0';

            free(input_file);
            input_file = filename;
        }
    }

    if (input_file) {
        int input_fd = open(input_file, O_RDONLY);
        if (input_fd == -1) {
            fprintf(stderr, "No such file or directory\n");
            free(input_file);
            return -1;
        }

        // dup2 hands the opened file off as the child's stdin; that descriptor
        // is meant to live on through the upcoming execvp, so -fanalyzer's
        // "leak" report on it is a false positive we silence locally.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-fd-leak"
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            perror("dup2");
            close(input_fd);
            free(input_file);
            return -1;
        }
#pragma GCC diagnostic pop

        close(input_fd);
        free(input_file);
    }

    return 0;
}

int setup_output_redirection(Token *tokens, int token_count) {
    char *output_file = NULL;
    int append_mode = 0;

    for (int i = 0; i < token_count; i++) {
        if ((tokens[i].type == TOKEN_OUTPUT_REDIR ||
             tokens[i].type == TOKEN_APPEND_REDIR) && i + 1 < token_count) {
            char *filename = xmalloc(tokens[i + 1].len + 1);
            strncpy(filename, tokens[i + 1].text, tokens[i + 1].len);
            filename[tokens[i + 1].len] = '\0';

            free(output_file);
            output_file = filename;
            append_mode = (tokens[i].type == TOKEN_APPEND_REDIR);
        }
    }

    if (output_file) {
        int flags = O_WRONLY | O_CREAT | (append_mode ? O_APPEND : O_TRUNC);

        int output_fd = open(output_file, flags, 0644);
        if (output_fd == -1) {
            fprintf(stderr, "Unable to create file for writing\n");
            free(output_file);
            return -1;
        }

        // See the note in setup_input_redirection: the dup2'd stdout descriptor
        // intentionally survives into the execvp'd child, so -fanalyzer's leak
        // report is a false positive.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-fd-leak"
        if (dup2(output_fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            close(output_fd);
            free(output_file);
            return -1;
        }
#pragma GCC diagnostic pop

        close(output_fd);
        free(output_file);
    }

    return 0;
}
