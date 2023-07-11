#ifndef clox_scanner_h
#define clox_scanner_h

// define all the supported Token Types:
typedef enum {
    // Single-character Tokens:
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
    
    // 2+ character Tokens:
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,

    // Literals:
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

    // Keywords:
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

    TOKEN_ERROR,    // in clox the scanner produces a synthetic error token for errors like unrecognized chars or unterminated strings. (then just continues)
    TOKEN_EOF,      // we parse the '\0' EndofFile to this token or end it at the end if missing 
    
    /*CUSTOM Tokens added on top of default-lox implementation*/
    TOKEN_MODULO,

} TokenType;

// One Token/Lexeme. Like a keyword 'var' '+' or a identifier like 'nr_of_steps'...
// - in clox token only store the lexeme (jlox did store the whole value)
typedef struct {
    TokenType type;
    const char* start;      // the lexeme (character sequance like "var" or "=" or "12.5")
    int length;
    int line;
} Token;

void initScanner(const char* source);
Token scanToken();

#endif