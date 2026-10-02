#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    /* Palavras-chave */
    TOK_IDENTIFICATION,
    TOK_ENVIRONMENT,
    TOK_DATA,
    TOK_PROCEDURE,
    TOK_DIVISION,
    TOK_SECTION,
    TOK_DISPLAY,
    TOK_ACCEPT,
    TOK_PERFORM,
    TOK_STOP,
    TOK_RUN,
    TOK_IF,
    TOK_ELSE,
    TOK_MOVE,

    /* Categorias léxicas */
    TOK_IDENTIFIER,
    TOK_NUMERIC_LITERAL,
    TOK_ALPHANUMERIC_LITERAL,

    /* Pontuação */
    TOK_PERIOD,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_LPAREN,
    TOK_RPAREN,

    /* Controle */
    TOK_EOF,
    TOK_ERROR

} TokenType;

typedef struct {
    TokenType type;
    char *lexeme;
    int line;
    int column;
} Token;

const char *token_type_to_string(TokenType type);

#endif
