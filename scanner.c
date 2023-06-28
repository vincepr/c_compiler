#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

/*
    Scanner/Lexer chews trough the source code
    - tracks how far it already has gone
    - one it reaches EoF '\0' its done

    At any point we know we only need 1 token look ahead (of current) to fulfill all our grammar
    So it will be enough to just 
*/

typedef struct {
    const char* start;      // beginning of the current Lexeme that is beeing parse
    const char* current;    // current char were scanning
    int line;               // line in source code we need to pass on for error reporting
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;       // first line is a 1 because thats how us humans roll
}

// helper for scanToken() - checks if were at the end of the string (we appended '\0' at the end of the string when reading it form the file)
static bool isAtEnd() {
    return *scanner.current == '\0';
}

// helper for scanToken() - read out the next char - consumes by incrementing the current-pointer
static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

// helper - peek into upcoming Char WITHOUT consuming it/incrementing current-pointer;
static char peek() {
    return *scanner.current;
}

// helper - peek 2 characters forward WITHOUT consuming
static char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

// helper for scanToken() - peaks into next char 
// ONLY incrementing the current-pointer IF we consume next char (like with '!=' or '>=')
static bool match(char expected) {
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

// helper for scanToken() - constructor like for Token: 
// - creates a token from the current start-pointer to current-pointer
static Token makeToken(TokenType type) {
    Token type;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

// helper for scanToken() - constructs a error Token - with length of the error-message etc.
static Token errorToken(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

// helper for scanToken() 
// - removes all leading whitespace(and newlines)
// - also remove everything commented out. after '//' till newline
static void skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;     // here we aditionally need to increment current line
                advance();
                break;
            case '/':
                if (peekNext() == '/')  {
                    while (peek() != '\n' && !isAtEnd()) {
                        advance();  // keep advancing till newline reached (== end of commented-line)
                        // since we did NOT consume the '\0' next skipWhitespace() loop will hit it and increment the linecount there
                    }
                } else {
                    return;         // only single / detected -> not whitespace
                }
                break;
            default:
                return;
        }
    }
}

// each call to this function scans a complete token.
// - and by doing this sets the new current to after then end of the 'consumed' token
// - 
Token scanToken() {
    skipWhitespace();                               // ignore leading-whitespace before we start checking for a LEXEME
    scanner.start = scaner.current;                 // we know our last call to scanToken() ended the current-pointer 'above' the end of the last

    if (isAtEnd()) return makeToken(TOKEN_EOF);     // We must add a EOF-Token at the end. The compiler needs this or it will keep going

    // advance a character
    char c = advance();
    switch (c) {
        // Map single-character TokenTypes to the char we just read:
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);

        // Map two-character TokenTypes (like '!' can be ! for NOT or != for UNEQUALS)
        case '!':
            return makeToken(
                match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(
                match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(
                match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(
                match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    }

    return errorToken("Unexpected character.");     // couldnt parse Token -> do the Error-Token 
}
