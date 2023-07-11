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

// helper for scanToken - check for alphabethical Char (begin of identifier or Keyword)
static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// helper for scanToken() - true if char between 0 and 9
static bool isDigit(char c) {
    return c >= '0' && c <= '9';
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
    Token token;
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

// helper for identifierType() - tries to match for keywords after hitting the 'start'-character
// - after previously finding the prefix, we check if the rest of the Token chars follow 
//      - (ex after a 'c' we check if the following chars are 'lass' if so we know we hit a 'class'-Token)
static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length && 
            memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

// helper for identifierToken() - this checks Type of the current Token -> could be Keyword or Literal etc...
// - we just check by hand for all reserved keywords:
//      ( we know only and starts with 'a' or this and true for 't' so we only check that)
// - this mean we can build a Trie/digital-tree to check for valid character combinations
static TokenType identifierType() {
    switch(scanner.start[0]) {
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        case 'f':
            // since f cound lead to false || for or fun -> we branch for those:
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

// helper for scanToken() - creates an Identifier Token (can be a Keyword or Literal etc...)
static Token identifierToken() {
    while (isAlpha(peek()) || isDigit(peek())) advance();       // after first digit we allow Alphanumerical
    return makeToken(identifierType());                         // here let identifierType() identify the type
}

// helper for scanToken() - keeps consuming Digits (and one . if next Digit again) to get a Float-Token
static Token numberToken() {
    while (isDigit(peek())) advance();

    if (peek() == '.' && isDigit(peekNext())) {
        advance();                  // consume the '.'
        while (isDigit(peek())) advance();
    }
    return makeToken(TOKEN_NUMBER);
}

// helper for scanToken - (after opening ") we keep reading the literal till we hit a closing one or Error
static Token stringToken() {
    // keep going till we find '"' to terminate the string:
    while(peek() != '"' && !isAtEnd()) {
        if(peek() == '\n') scanner.line++;
        advance();
    }
    if (isAtEnd()) return errorToken("Unterminated string.");

    advance();
    return makeToken(TOKEN_STRING);
}

// each call to this function scans a complete token.
// - and by doing this sets the new current to after then end of the 'consumed' token
Token scanToken() {
    skipWhitespace();                               // ignore leading-whitespace before we start checking for a LEXEME
    scanner.start = scanner.current;                // we know our last call to scanToken() ended the current-pointer 'above' the end of the last
    if (isAtEnd()) return makeToken(TOKEN_EOF);     // We must add a EOF-Token at the end. The compiler needs this or it will keep going

    // advance a character
    char c = advance();

    if (isAlpha(c)) return identifierToken();
    if (isDigit(c)) return numberToken();  	            // instead of a switch for 0-9 digits we do this

    switch (c) {
        // Map single-character TokenTypes to the char we just read:
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case '[': return makeToken(TOKEN_LEFT_BRACKET);
        case ']': return makeToken(TOKEN_RIGHT_BRACKET);
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
        
        // Literal Tokens (enclosed by "")
        case '"': return stringToken();
    }
    return errorToken("Unexpected character.");     // couldnt parse Token -> do the Error-Token 
}
