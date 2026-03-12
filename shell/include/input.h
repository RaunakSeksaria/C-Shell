#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>
#include <stdlib.h>

ssize_t take_input(char **input, size_t *bufsize);

// Tokenizer and Parser definitions
typedef enum {
    TOKEN_NAME,
    TOKEN_PIPE,          // |
    TOKEN_AMPERSAND,     // &
    TOKEN_SEMICOLON,     // ;
    TOKEN_INPUT_REDIR,   // <
    TOKEN_OUTPUT_REDIR,  // >
    TOKEN_APPEND_REDIR,  // >>
    TOKEN_END,           // End of input
    TOKEN_INVALID        // Unrecognized token
} TokenType;

typedef struct {
    TokenType type;
    char *text;
    int len;
} Token;

Token* tokenize(const char *input, int *token_count);
int parse_input(Token *tokens, int token_count);


#endif // INPUT_H