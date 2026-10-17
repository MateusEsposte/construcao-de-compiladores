#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


static char *copy_lexeme(const char *text, size_t length){
    char *copy = malloc(length + 1);

    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, text, length);
    copy[length] = '\0';

    return copy;
}


static int is_word_char(int c){
    return isalpha(c) || isdigit(c) || c == '-';
}


static void lexer_advance_raw(Lexer *lexer)
{
    if (lexer->current_char == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }

    lexer->current_char = fgetc(lexer->file);
}


static void lexer_skip_fixed_format(Lexer *lexer){
    while (lexer->current_char != EOF) {

        if (lexer->column <= 6) {
            lexer_advance_raw(lexer);
            continue;
        }

        if (lexer->column == 7) {
            lexer->line_indicator = lexer->current_char;

            int indicator = toupper(
                (unsigned char)lexer->current_char
            );

            if (indicator == '*' ||
                indicator == '/' ||
                indicator == 'D') {

                while (lexer->current_char != '\n' &&
                       lexer->current_char != EOF) {
                    lexer_advance_raw(lexer);
                }

                if (lexer->current_char == '\n') {
                    lexer_advance_raw(lexer);
                }

                continue;
            }

            lexer_advance_raw(lexer);
            continue;
        }

        if (lexer->column >= 73) {
            while (lexer->current_char != '\n' &&
                   lexer->current_char != EOF) {
                lexer_advance_raw(lexer);
            }

            if (lexer->current_char == '\n') {
                lexer_advance_raw(lexer);
            }

            continue;
        }

        return;
    }
}


static void lexer_advance(Lexer *lexer)
{
    lexer_advance_raw(lexer);
    lexer_skip_fixed_format(lexer);
}


void lexer_init(Lexer *lexer, FILE *file){
    lexer->file = file;
    lexer->current_char = fgetc(file);
    lexer->line = 1;
    lexer->column = 1;
    lexer->line_indicator = 0;

    lexer_skip_fixed_format(lexer);
}


static TokenType keyword_type(char *word){
    char upper[31];
    int i = 0;

    while (word[i] != '\0' && i < 30) {
        upper[i] = (char)toupper((unsigned char)word[i]);
        i++;
    }

    upper[i] = '\0';

    if (strcmp(upper, "IDENTIFICATION") == 0) return TOK_IDENTIFICATION;
    if (strcmp(upper, "ENVIRONMENT") == 0)    return TOK_ENVIRONMENT;
    if (strcmp(upper, "DATA") == 0)           return TOK_DATA;
    if (strcmp(upper, "PROCEDURE") == 0)      return TOK_PROCEDURE;
    if (strcmp(upper, "DIVISION") == 0)       return TOK_DIVISION;
    if (strcmp(upper, "SECTION") == 0)        return TOK_SECTION;
    if (strcmp(upper, "DISPLAY") == 0)        return TOK_DISPLAY;
    if (strcmp(upper, "ACCEPT") == 0)         return TOK_ACCEPT;
    if (strcmp(upper, "PERFORM") == 0)        return TOK_PERFORM;
    if (strcmp(upper, "STOP") == 0)           return TOK_STOP;
    if (strcmp(upper, "RUN") == 0)            return TOK_RUN;
    if (strcmp(upper, "IF") == 0)             return TOK_IF;
    if (strcmp(upper, "ELSE") == 0)           return TOK_ELSE;
    if (strcmp(upper, "MOVE") == 0)           return TOK_MOVE;

    return TOK_IDENTIFIER;
}


static Token read_word(Lexer *lexer){
    Token token;

    token.line = lexer->line;
    token.column = lexer->column;
    token.type = TOK_IDENTIFIER;
    token.lexeme = NULL;

    char word[256];
    int length = 0;
    int too_long = 0;

    while (is_word_char(lexer->current_char)) {
        if (length < (int)sizeof(word) - 1) {
            word[length] = (char)lexer->current_char;
        } else {
            too_long = 1;
        }

        length++;
        lexer_advance(lexer);
    }

    int stored_length = length < (int)sizeof(word) - 1
                      ? length
                      : (int)sizeof(word) - 1;

    word[stored_length] = '\0';

    token.lexeme = copy_lexeme(word, (size_t)stored_length);

    if (token.lexeme == NULL) {
        token.type = TOK_ERROR;
        return token;
    }

    if (too_long || length > 30 || word[stored_length - 1] == '-') {
        token.type = TOK_ERROR;
        return token;
    }

    token.type = keyword_type(token.lexeme);

    return token;
}


static Token read_number(Lexer *lexer){
    Token token;

    token.line = lexer->line;
    token.column = lexer->column;
    token.type = TOK_NUMERIC_LITERAL;
    token.lexeme = NULL;

    char number[64];
    int length = 0;
    int has_digit = 0;

    if (lexer->current_char == '+' || lexer->current_char == '-') {
        number[length++] = (char)lexer->current_char;
        lexer_advance(lexer);
    }

    while (isdigit(lexer->current_char)) {
        if (length < 63) {
            number[length++] = (char)lexer->current_char;
        }

        has_digit = 1;
        lexer_advance(lexer);
    }

    if (lexer->current_char == '.') {
        int next_char = fgetc(lexer->file);
        ungetc(next_char, lexer->file);

        if (isdigit(next_char)) {
            number[length++] = '.';
            lexer_advance(lexer);

            while (isdigit(lexer->current_char)) {
                if (length < 63) {
                    number[length++] = (char)lexer->current_char;
                }
                lexer_advance(lexer);
            }
        }
    }

    if (!has_digit) {
        token.type = TOK_ERROR;
    }

    number[length] = '\0';
    token.lexeme = copy_lexeme(number, (size_t)length);

    if (token.lexeme == NULL) {
        token.type = TOK_ERROR;
    }

    return token;
}


static Token read_string(Lexer *lexer)
{
    Token token;

    token.line = lexer->line;
    token.column = lexer->column;
    token.type = TOK_ALPHANUMERIC_LITERAL;
    token.lexeme = NULL;

    char quote = (char)lexer->current_char;
    char text[256];
    int length = 0;
    int invalid = 0;

    text[length++] = quote;
    lexer_advance(lexer);

    while (lexer->current_char != EOF) {

        if (lexer->current_char == quote) {
            text[length++] = quote;
            lexer_advance(lexer);
            break;
        }

        if (lexer->current_char == '\n') {
            lexer_advance(lexer);

            if (toupper((unsigned char)lexer->line_indicator) != '-') {
                invalid = 1;
                break;
            }

            while (lexer->current_char != EOF &&
                   lexer->current_char != '\n' &&
                   isspace((unsigned char)lexer->current_char)) {
                lexer_advance(lexer);
            }

            if (lexer->current_char != quote) {
                invalid = 1;
                break;
            }

            lexer_advance(lexer);
            continue;
        }

        if (length >= (int)sizeof(text) - 2) {
            invalid = 1;
            break;
        }

        text[length++] = (char)lexer->current_char;
        lexer_advance(lexer);
    }

    if (lexer->current_char == EOF &&
        (length == 0 || text[length - 1] != quote)) {
        invalid = 1;
    }

    if (length == 1 || text[length - 1] != quote) {
        invalid = 1;
    }

    text[length] = '\0';

    if (invalid) {
        token.type = TOK_ERROR;
    }

    token.lexeme = copy_lexeme(text, (size_t)length);

    if (token.lexeme == NULL) {
        token.type = TOK_ERROR;
    }

    return token;
}


static Token read_invalid_signed_word(Lexer *lexer)
{
    Token token;

    token.line = lexer->line;
    token.column = lexer->column;
    token.type = TOK_ERROR;
    token.lexeme = NULL;

    char text[31];
    int length = 0;

    text[length++] = (char)lexer->current_char;
    lexer_advance(lexer);

    while (is_word_char(lexer->current_char)) {
        if (length < 30) {
            text[length] = (char)lexer->current_char;
        }

        length++;
        lexer_advance(lexer);
    }

    int stored_length = length > 30 ? 30 : length;
    text[stored_length] = '\0';

    token.lexeme = copy_lexeme(text, (size_t)stored_length);

    return token;
}



static int digit_sequence_continues_identifier(Lexer *lexer)
{
    fpos_t position;

    if (fgetpos(lexer->file, &position) != 0) {
        return 0;
    }

    int c;

    do {
        c = fgetc(lexer->file);
    } while (isdigit((unsigned char)c));

    int result = (c == '-' || isalpha((unsigned char)c));

    fsetpos(lexer->file, &position);

    return result;
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

    if (lexer->current_char == '"' ||
        lexer->current_char == '\'') {
        return read_string(lexer);
    }

    if (isalpha(lexer->current_char)) {
        return read_word(lexer);
    }

    if (isdigit(lexer->current_char)) {
        if (digit_sequence_continues_identifier(lexer)) {
            return read_word(lexer);
        }

        return read_number(lexer);
    }

    if (lexer->current_char == '+' || lexer->current_char == '-') {
        int next_char = fgetc(lexer->file);

        if (next_char != EOF) {
            ungetc(next_char, lexer->file);
        }

        if (isalpha(next_char)) {
            return read_invalid_signed_word(lexer);
        }

        return read_number(lexer);
    }

    char punctuation[2] = {
        (char)lexer->current_char,
        '\0'
    };

    token.lexeme = copy_lexeme(punctuation, 1);

    if (token.lexeme == NULL) {
        token.type = TOK_ERROR;
        lexer_advance(lexer);
        return token;
    }

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
