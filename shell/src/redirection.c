// ############## LLM Generated Code Begins ##############

#include "redirection.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int setup_input_redirection(Token *tokens, int token_count) {
    int input_fd = STDIN_FILENO;  // Default to standard input
    char *input_file = NULL;

    // Find the last input redirection
    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == TOKEN_INPUT_REDIR && i + 1 < token_count) {
            // Get the filename
            char *filename = malloc(tokens[i + 1].len + 1);
            strncpy(filename, tokens[i + 1].text, tokens[i + 1].len);
            filename[tokens[i + 1].len] = '\0';
            
            // Free previous filename if it exists
            if (input_file) {
                free(input_file);
            }
            input_file = filename;
        }
    }

    // If we found an input file, set up the redirection
    if (input_file) {
        input_fd = open(input_file, O_RDONLY);
        if (input_fd == -1) {
            fprintf(stderr, "No such file or directory\n");
            free(input_file);
            return -1;
        }
        
        // Redirect standard input to the file
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            perror("dup2");
            close(input_fd);
            free(input_file);
            return -1;
        }
        
        // Close the original file descriptor
        close(input_fd);
        free(input_file);
    }

    return 0;
}

// Add these helper functions
int setup_output_redirection(Token *tokens, int token_count) {
    int output_fd = STDOUT_FILENO;  // Default to standard output
    char *output_file = NULL;
    int append_mode = 0;

    // Find the last output redirection
    for (int i = 0; i < token_count; i++) {
        if ((tokens[i].type == TOKEN_OUTPUT_REDIR || 
             tokens[i].type == TOKEN_APPEND_REDIR) && i + 1 < token_count) {
            // Get the filename
            char *filename = malloc(tokens[i + 1].len + 1);
            strncpy(filename, tokens[i + 1].text, tokens[i + 1].len);
            filename[tokens[i + 1].len] = '\0';
            
            // Free previous filename if it exists
            if (output_file) {
                free(output_file);
            }
            output_file = filename;
            append_mode = (tokens[i].type == TOKEN_APPEND_REDIR);
        }
    }

    // If we found an output file, set up the redirection
    if (output_file) {
        int flags = O_WRONLY | O_CREAT;
        flags |= (append_mode ? O_APPEND : O_TRUNC);
        
        output_fd = open(output_file, flags, 0644);
        if (output_fd == -1) {
            fprintf(stderr, "Unable to create file for writing\n");
            free(output_file);
            return -1;
        }
        
        // Redirect standard output to the file
        if (dup2(output_fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            close(output_fd);
            free(output_file);
            return -1;
        }
        
        // Close the original file descriptor
        close(output_fd);
        free(output_file);
    }

    return 0;
}
// ############## LLM Generated Code Ends ################