#include "input.h"
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

ssize_t take_input(char **input, size_t *bufsize) {
    return getline(input, bufsize, stdin);
}

static int is_shell_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c=='\f' || c=='\v';
}

// Lexes input into a Token array. Tokens are zero-copy: each Token.text points
// into the caller's input buffer instead of owning a copy, so the returned
// array is valid only while that buffer lives. An operator glued to its operand
// (e.g. ">out.txt") is split into two tokens, letting the parser and executor
// treat operands uniformly whether or not a space separates them.
Token* tokenize(const char *input, int *token_count) {
    int capacity = 16;
    Token *tokens = malloc(sizeof(Token) * capacity);
    int count = 0;
    const char *p = input;

    while (*p != '\0') {
        if (is_shell_whitespace(*p)) {
            p++;
            continue;
        }

        if (count >= capacity - 1) {
            capacity *= 2;
            tokens = realloc(tokens, sizeof(Token) * capacity);
        }

        Token token;
        token.text = (char*)p;

        if (*p == '|') {
            token.type = TOKEN_PIPE;
            token.len = 1;
            p++;
        } else if (*p == '&') {
            token.type = TOKEN_AMPERSAND;
            token.len = 1;
            p++;
        } else if (*p == ';') {
            token.type = TOKEN_SEMICOLON;
            token.len = 1;
            p++;
        } else if (*p == '<') {
            token.type = TOKEN_INPUT_REDIR;
            token.len = 1;
            p++;
            if (*p != '\0' && !is_shell_whitespace(*p) && !strchr("|&;<>", *p)) {
                if (count >= capacity - 1) {
                    capacity *= 2;
                    tokens = realloc(tokens, sizeof(Token) * capacity);
                }
                tokens[count++] = token;
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
                if (*p != '\0' && !is_shell_whitespace(*p) && !strchr("|&;<>", *p)) {
                    if (count >= capacity - 1) {
                        capacity *= 2;
                        tokens = realloc(tokens, sizeof(Token) * capacity);
                    }
                    tokens[count++] = token;
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
                if (*p != '\0' && !is_shell_whitespace(*p) && !strchr("|&;<>", *p)) {
                    if (count >= capacity - 1) {
                        capacity *= 2;
                        tokens = realloc(tokens, sizeof(Token) * capacity);
                    }
                    tokens[count++] = token;
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
            const char *start = p;
            while (*p != '\0' && !is_shell_whitespace(*p) && !strchr("|&;<>", *p)) {
                p++;
            }
            token.len = p - start;
        }
        tokens[count++] = token;
    }

    tokens[count].type = TOKEN_END;
    tokens[count].text = NULL;
    tokens[count].len = 0;
    *token_count = count;
    return tokens;
}

// Recursive-descent syntax validator for one input line. Shape:
//   line   := group ( sep group )*   with an optional trailing '&'
//   group  := atomic ( '|' atomic )*
//   atomic := NAME ( NAME | ('<' | '>' | '>>') NAME )*
//   sep    := ';' | '&'
// A trailing '&' is allowed (it backgrounds the last group); a trailing ';' and
// empty groups between two separators are errors. This only checks shape —
// execute.c re-walks the same tokens to actually run them.
static int parse_shell_cmd(Token **current);
static int parse_cmd_group(Token **current);
static int parse_atomic(Token **current);

static TokenType peek(Token *current) {
    return current->type;
}

static void advance(Token **current) {
    (*current)++;
}

static int parse_input_redir(Token **current) {
    if (peek(*current) != TOKEN_INPUT_REDIR) return 0;
    advance(current);
    if (peek(*current) != TOKEN_NAME) return 0;
    advance(current);
    return 1;
}

static int parse_output_redir(Token **current) {
    TokenType type = peek(*current);
    if (type != TOKEN_OUTPUT_REDIR && type != TOKEN_APPEND_REDIR) return 0;
    advance(current);
    if (peek(*current) != TOKEN_NAME) return 0;
    advance(current);
    return 1;
}

static int parse_atomic(Token **current) {
    if (peek(*current) != TOKEN_NAME) return 0;
    advance(current);

    while (1) {
        TokenType type = peek(*current);
        if (type == TOKEN_NAME) {
            advance(current);
        } else if (type == TOKEN_INPUT_REDIR) {
            if (!parse_input_redir(current)) return 0;
        } else if (type == TOKEN_OUTPUT_REDIR || type == TOKEN_APPEND_REDIR) {
            if (!parse_output_redir(current)) return 0;
        } else {
            break;
        }
    }
    return 1;
}

static int parse_cmd_group(Token **current) {
    if (!parse_atomic(current)) return 0;

    while (peek(*current) == TOKEN_PIPE) {
        advance(current);
        if (!parse_atomic(current)) return 0;
    }
    return 1;
}

static int parse_shell_cmd(Token **current) {
    if (!parse_cmd_group(current)) return 0;

    while (peek(*current) == TOKEN_SEMICOLON || peek(*current) == TOKEN_AMPERSAND) {
        TokenType separator = peek(*current);
        advance(current);

        if (peek(*current) == TOKEN_END) {
            return separator == TOKEN_AMPERSAND;
        }
        if (peek(*current) == TOKEN_SEMICOLON || peek(*current) == TOKEN_AMPERSAND) {
            return 0;
        }

        if (!parse_cmd_group(current)) return 0;
    }
    return 1;
}

int parse_input(Token *tokens, int token_count) {
    if (token_count == 0) return 1;
    Token *current = tokens;
    if (!parse_shell_cmd(&current)) return 0;
    return peek(current) == TOKEN_END;
}
