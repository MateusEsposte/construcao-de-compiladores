#include "lexer.h"
#include <ctype.h>
#include <string.h>


static int is_word_char(int c){
    return isalpha(c) || isdigit(c) || c == '-';
}


static void lexer_advance(Lexer *lexer){
    if (lexer->current_char == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }

    lexer->current_char = fgetc(lexer->file);
}


void lexer_init(Lexer *lexer, FILE *file){
    lexer->file = file;
    lexer->current_char = fgetc(file);
    lexer->line = 1;
    lexer->column = 1;
}


static TokenType keyword_type(char *word){
    for (int i = 0; word[i] != '\0'; i++) {
        word[i] = toupper((unsigned char)word[i]);
    }

    if (strcmp(word, "IDENTIFICATION") == 0) return TOK_IDENTIFICATION;
    if (strcmp(word, "ENVIRONMENT") == 0)    return TOK_ENVIRONMENT;
    if (strcmp(word, "DATA") == 0)           return TOK_DATA;
    if (strcmp(word, "PROCEDURE") == 0)      return TOK_PROCEDURE;
    if (strcmp(word, "DIVISION") == 0)       return TOK_DIVISION;
    if (strcmp(word, "SECTION") == 0)        return TOK_SECTION;
    if (strcmp(word, "DISPLAY") == 0)        return TOK_DISPLAY;
    if (strcmp(word, "ACCEPT") == 0)         return TOK_ACCEPT;
    if (strcmp(word, "PERFORM") == 0)        return TOK_PERFORM;
    if (strcmp(word, "STOP") == 0)           return TOK_STOP;
    if (strcmp(word, "RUN") == 0)            return TOK_RUN;
    if (strcmp(word, "IF") == 0)             return TOK_IF;
    if (strcmp(word, "ELSE") == 0)           return TOK_ELSE;
    if (strcmp(word, "MOVE") == 0)           return TOK_MOVE;

    return TOK_IDENTIFIER;
}


static Token read_word(Lexer *lexer){
    Token token;

    token.line = lexer->line;
    token.column = lexer->column;
    token.type = TOK_IDENTIFIER;
    token.lexeme = NULL;

    char word[31];
    int length = 0;

    while (is_word_char(lexer->current_char)) {
        if (length < 30) {
            word[length] = lexer->current_char;
        }

        length++;
        lexer_advance(lexer);
    }

    if (length > 30) {
        token.type = TOK_ERROR;
        return token;
    }

    word[length] = '\0';

    token.type = keyword_type(word);

    return token;
}


static Token read_number(Lexer *lexer)
{
    Token token;

    token.line = lexer->line;
    token.column = lexer->column;
    token.type = TOK_NUMERIC_LITERAL;
    token.lexeme = NULL;

    if (lexer->current_char == '+' || lexer->current_char == '-') {
        lexer_advance(lexer);
    }

    while (isdigit(lexer->current_char)) {
        lexer_advance(lexer);
    }

    if (lexer->current_char == '.') {
        lexer_advance(lexer);

        while (isdigit(lexer->current_char)) {
            lexer_advance(lexer);
        }
    }

    return token;
}


Token lexer_next_token(Lexer *lexer){
    Token token;

    while (isspace(lexer->current_char)) {
        lexer_advance(lexer);
    }

    token.line = lexer->line;
    token.column = lexer->column;
    token.lexeme = NULL;

    if (lexer->current_char == EOF) {
        token.type = TOK_EOF;
        return token;
    }

    if (isalpha(lexer->current_char)) {
        return read_word(lexer);
    }

    if (isdigit(lexer->current_char)) {
        return read_number(lexer);
    }

    if (lexer->current_char == '+' || lexer->current_char == '-') {
        return read_number(lexer);
    }

    token.type = TOK_ERROR;
    switch (lexer->current_char) {
        case '.':
            token.type = TOK_PERIOD;
            break;

        case ',':
            token.type = TOK_COMMA;
            break;

        case ';':
            token.type = TOK_SEMICOLON;
            break;

        case '(':
            token.type = TOK_LPAREN;
            break;

        case ')':
            token.type = TOK_RPAREN;
            break;

        default:
            token.type = TOK_ERROR;
            break;
    }

    lexer_advance(lexer);
    return token;
}
