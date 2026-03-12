// ############## LLM Generated Code Begins ##############

#include "input.h"
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

ssize_t take_input(char **input, size_t *bufsize) {
    // Use getline to read a line of input from stdin, automatically managing buffer allocation.
    return getline(input, bufsize, stdin);
}

// // ############## LLM Generated Code Begins ##############

// Helper function to check for whitespace characters as defined by the grammar.
static int is_shell_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c=='\f' || c=='\v';
}

Token* tokenize(const char *input, int *token_count) {
    int capacity = 16; // Start with an initial capacity for the tokens array.
    Token *tokens = malloc(sizeof(Token) * capacity); // Allocate memory for the tokens.
    int count = 0; // Keep track of the number of tokens found.
    const char *p = input; // Use a pointer 'p' to traverse the input string.

    while (*p != '\0') { // Loop until the end of the input string is reached.
        if (is_shell_whitespace(*p)) { // Check if the current character is whitespace.
            p++; // If it is, skip it and continue to the next character.
            continue;
        }

        if (count >= capacity - 1) { // If the token array is full.
            capacity *= 2; // Double the capacity.
            tokens = realloc(tokens, sizeof(Token) * capacity); // And reallocate the memory.
        }

        Token token; // Create a new token.
        token.text = (char*)p; // The token's text starts at the current position of 'p'.

        if (*p == '|') { // Check for the pipe character.
            token.type = TOKEN_PIPE;
            token.len = 1;
            p++;
        } else if (*p == '&') { // Check for the ampersand character.
            token.type = TOKEN_AMPERSAND;
            token.len = 1;
            p++;
        } else if (*p == ';') { // Check for the semicolon character.
            token.type = TOKEN_SEMICOLON;
            token.len = 1;
            p++;
        } else if (*p == '<') { // Check for the input redirection character.
            token.type = TOKEN_INPUT_REDIR;
            token.len = 1;
            p++;
            // Handle case where name immediately follows '<'
            if (*p != '\0' && !is_shell_whitespace(*p) && !strchr("|&;<>", *p)) {
                if (count >= capacity - 1) {
                    capacity *= 2;
                    tokens = realloc(tokens, sizeof(Token) * capacity);
                }
                tokens[count++] = token;
                
                // Create name token
                token.text = (char*)p;
                token.type = TOKEN_NAME;
                while (*p != '\0' && !is_shell_whitespace(*p) && !strchr("|&;<>", *p)) {
                    p++;
                }
                token.len = p - token.text;
            }
        } else if (*p == '>') {
            if (*(p + 1) == '>') {
                token.type = TOKEN_APPEND_REDIR;
                token.len = 2;
                p += 2;
                // Handle case where name immediately follows '>>'
                if (*p != '\0' && !is_shell_whitespace(*p) && !strchr("|&;<>", *p)) {
                    if (count >= capacity - 1) {
                        capacity *= 2;
                        tokens = realloc(tokens, sizeof(Token) * capacity);
                    }
                    tokens[count++] = token;
                    
                    // Create name token
                    token.text = (char*)p;
                    token.type = TOKEN_NAME;
                    while (*p != '\0' && !is_shell_whitespace(*p) && !strchr("|&;<>", *p)) {
                        p++;
                    }
                    token.len = p - token.text;
                }
            } else {
                token.type = TOKEN_OUTPUT_REDIR;
                token.len = 1;
                p++;
                // Handle case where name immediately follows '>'
                if (*p != '\0' && !is_shell_whitespace(*p) && !strchr("|&;<>", *p)) {
                    if (count >= capacity - 1) {
                        capacity *= 2;
                        tokens = realloc(tokens, sizeof(Token) * capacity);
                    }
                    tokens[count++] = token;
                    
                    // Create name token
                    token.text = (char*)p;
                    token.type = TOKEN_NAME;
                    while (*p != '\0' && !is_shell_whitespace(*p) && !strchr("|&;<>", *p)) {
                        p++;
                    }
                    token.len = p - token.text;
                }
            }
        } else {
            token.type = TOKEN_NAME;
            const char *start = p; // Mark the starting position of the name.
            // A NAME continues until a special character, whitespace, or the end of the string is found.
            while (*p != '\0' && !is_shell_whitespace(*p) && !strchr("|&;<>", *p)) {
                p++;
            }
            token.len = p - start; // The length is the difference between the end and start pointers.
        }
        tokens[count++] = token; // Add the newly created token to the array and increment the count.
    }

    tokens[count].type = TOKEN_END; // Add a final TOKEN_END to mark the end of the stream.
    tokens[count].text = NULL;
    tokens[count].len = 0;
    *token_count = count; // Set the output parameter for the total number of tokens.
    return tokens; // Return the array of tokens.
}

// --- Parser Implementation ---

// Forward declarations for the recursive descent parser functions.
static int parse_shell_cmd(Token **current);
static int parse_cmd_group(Token **current);
static int parse_atomic(Token **current);

// Helper function to look at the current token's type without consuming it.
static TokenType peek(Token *current) {
    return current->type;
}

// Helper function to consume the current token and move to the next one.
static void advance(Token **current) {
    (*current)++;
}

// Helper function to check if current token is a redirection token
// static int is_redirection(TokenType type) {
//     return type == TOKEN_INPUT_REDIR || 
//            type == TOKEN_OUTPUT_REDIR || 
//            type == TOKEN_APPEND_REDIR;
// }

// Parses input redirection: input -> < name | <name
static int parse_input_redir(Token **current) {
    if (peek(*current) != TOKEN_INPUT_REDIR) return 0;
    advance(current);  // consume '<'
    
    if (peek(*current) != TOKEN_NAME) return 0;
    advance(current);  // consume filename
    return 1;
}

// Parses output redirection: output -> > name | >name | >> name | >>name
static int parse_output_redir(Token **current) {
    TokenType type = peek(*current);
    if (type != TOKEN_OUTPUT_REDIR && type != TOKEN_APPEND_REDIR) return 0;
    advance(current);  // consume '>' or '>>'
    
    if (peek(*current) != TOKEN_NAME) return 0;
    advance(current);  // consume filename
    return 1;
}

// Parses the 'atomic' grammar rule: atomic -> name (name | input | output)*
static int parse_atomic(Token **current) {
    if (peek(*current) != TOKEN_NAME) return 0;
    advance(current);

    while (1) {
        TokenType type = peek(*current);
        
        if (type == TOKEN_NAME) {
            advance(current);
        }
        else if (type == TOKEN_INPUT_REDIR) {
            if (!parse_input_redir(current)) return 0;
        }
        else if (type == TOKEN_OUTPUT_REDIR || type == TOKEN_APPEND_REDIR) {
            if (!parse_output_redir(current)) return 0;
        }
        else {
            break;
        }
    }
    return 1;
}

// Parses the 'cmd_group' grammar rule: cmd_group -> atomic (\| atomic)*
static int parse_cmd_group(Token **current) {
    if (!parse_atomic(current)) return 0; // A command group must start with an atomic command.

    while (peek(*current) == TOKEN_PIPE) { // It can be followed by one or more pipes.
        advance(current); // Consume the '|' token.
        if (!parse_atomic(current)) return 0; // A pipe must be followed by another atomic command.
    }
    return 1; // Return 1 (success).
}

// Parses the 'shell_cmd' grammar rule: shell_cmd -> cmd_group ((& | ;) cmd_group)* &?
static int parse_shell_cmd(Token **current) {
    if (!parse_cmd_group(current)) return 0; // A shell command must start with a command group.

    while (peek(*current) == TOKEN_SEMICOLON || peek(*current) == TOKEN_AMPERSAND) { // It can be followed by separators.
        TokenType separator = peek(*current);
        advance(current); // Consume the separator (';' or '&').
        
        if (peek(*current) == TOKEN_END) { // Check if the command ends right after the separator.
            return separator == TOKEN_AMPERSAND; // This is only valid if the separator was a trailing '&'.
        }
        if (peek(*current) == TOKEN_SEMICOLON || peek(*current) == TOKEN_AMPERSAND) { // Check for invalid sequences like "cmd ; ;".
            return 0; // This is a syntax error.
        }

        if (!parse_cmd_group(current)) return 0; // The separator must be followed by another command group.
    }
    return 1; // Return 1 (success).
}

int parse_input(Token *tokens, int token_count) {
    if (token_count == 0) { // An empty input line is considered valid.
        return 1;
    }
    Token *current = tokens; // Create a pointer to the current token.
    if (!parse_shell_cmd(&current)) { // Start parsing from the top-level rule.
        return 0; // If it fails at any point, the syntax is invalid.
    }

    // After a successful parse, we must be at the end of the token stream.
    return peek(current) == TOKEN_END; // Check if the current token is the END
}

// // ############## LLM Generated Code Ends ################