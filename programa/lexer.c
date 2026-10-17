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

static int is_alpha(int c){
    return c != EOF && isalpha((unsigned char)c);
}


static int is_digit(int c){
    return c != EOF && isdigit((unsigned char)c);
}


static int append_char(char **text, size_t *length, size_t *capacity, int c){
    if (*length + 1 >= *capacity) {
        size_t new_capacity = *capacity == 0 ? 64 : *capacity * 2;

        if (new_capacity <= *capacity) {
            return 0;
        }

        char *new_text = realloc(*text, new_capacity);

        if (new_text == NULL) {
            return 0;
        }

        *text = new_text;
        *capacity = new_capacity;
    }

    (*text)[(*length)++] = (char)c;
    (*text)[*length] = '\0';

    return 1;
}


static int is_word_char(int c){
    return is_alpha(c) || is_digit(c) || c == '-' || c == '_';
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
    lexer->can_start_signed_number = 1;

    lexer_skip_fixed_format(lexer);
}


static TokenType keyword_type(const char *word){
    typedef struct {
        const char *word;
        TokenType type;
    } Keyword;

    static const Keyword keywords[] = {
        {"IDENTIFICATION", TOK_IDENTIFICATION},
        {"ENVIRONMENT", TOK_ENVIRONMENT},
        {"DATA", TOK_DATA},
        {"PROCEDURE", TOK_PROCEDURE},
        {"DIVISION", TOK_DIVISION},
        {"SECTION", TOK_SECTION},
        {"DISPLAY", TOK_DISPLAY},
        {"ACCEPT", TOK_ACCEPT},
        {"PERFORM", TOK_PERFORM},
        {"STOP", TOK_STOP},
        {"RUN", TOK_RUN},
        {"IF", TOK_IF},
        {"ELSE", TOK_ELSE},
        {"MOVE", TOK_MOVE},
        {"ADD", TOK_ADD},
        {"SUBTRACT", TOK_SUBTRACT},
        {"MULTIPLY", TOK_MULTIPLY},
        {"DIVIDE", TOK_DIVIDE},
        {"COMPUTE", TOK_COMPUTE},
        {"PROGRAM", TOK_PROGRAM},
        {"PROGRAM-ID", TOK_PROGRAM_ID},
        {"FILE", TOK_FILE},
        {"FILE-CONTROL", TOK_FILE_CONTROL},
        {"SELECT", TOK_SELECT},
        {"FD", TOK_FD},
        {"PIC", TOK_PIC},
        {"VALUE", TOK_VALUE},
        {"VALUES", TOK_VALUES},
        {"OCCURS", TOK_OCCURS},
        {"TIMES", TOK_TIMES},
        {"INDEXED", TOK_INDEXED},
        {"BY", TOK_BY},
        {"OF", TOK_OF},
        {"INTO", TOK_INTO},
        {"TO", TOK_TO},
        {"FROM", TOK_FROM},
        {"GIVING", TOK_GIVING},
        {"REMAINDER", TOK_REMAINDER},
        {"AND", TOK_AND},
        {"OR", TOK_OR},
        {"NOT", TOK_NOT},
        {"IN", TOK_IN},
        {"OPEN", TOK_OPEN},
        {"CLOSE", TOK_CLOSE},
        {"READ", TOK_READ},
        {"WRITE", TOK_WRITE},
        {"RECORD", TOK_RECORD},
        {"RECORDS", TOK_RECORDS},
        {"ASSIGN", TOK_ASSIGN},
        {"INPUT", TOK_INPUT},
        {"OUTPUT", TOK_OUTPUT},
        {"INPUT-OUTPUT", TOK_INPUT_OUTPUT},
        {"ORGANIZATION", TOK_ORGANIZATION},
        {"ACCESS", TOK_ACCESS},
        {"MODE", TOK_MODE},
        {"STATUS", TOK_STATUS},
        {"START", TOK_START},
        {"DELETE", TOK_DELETE},
        {"UPDATE", TOK_UPDATE},
        {"INVALID", TOK_INVALID},
        {"KEY", TOK_KEY},
        {"END-IF", TOK_END_IF},
        {"END-SEARCH", TOK_END_SEARCH},
        {"END-EVALUATE", TOK_END_EVALUATE},
        {"END-PROGRAM", TOK_END_PROGRAM},
        {"SEARCH", TOK_SEARCH},
        {"SET", TOK_SET},
        {"WHEN", TOK_WHEN},
        {"AT", TOK_AT},
        {"END", TOK_END},
        {"ON", TOK_ON},
        {"SIZE", TOK_SIZE},
        {"ERROR", TOK_ERROR_WORD},
        {"UNTIL", TOK_UNTIL},
        {"FUNCTION", TOK_FUNCTION},
        {"UPPER-CASE", TOK_UPPER_CASE},
        {"INSPECT", TOK_INSPECT},
        {"TALLYING", TOK_TALLYING},
        {"FOR", TOK_FOR},
        {"ALL", TOK_ALL},
        {"CHARACTERS", TOK_CHARACTERS},
        {"BEFORE", TOK_BEFORE},
        {"AFTER", TOK_AFTER},
        {"INITIAL", TOK_INITIAL},
        {"LEADING", TOK_LEADING},
        {"REPLACING", TOK_REPLACING},
        {"FIRST", TOK_FIRST},
        {"CONVERTING", TOK_CONVERTING},
        {"SPACES", TOK_SPACES},
        {"ZERO", TOK_ZERO},
        {"ZEROS", TOK_ZEROS},
        {"LOW-VALUES", TOK_LOW_VALUES},
        {"CONFIGURATION", TOK_CONFIGURATION},
        {"LINKAGE", TOK_LINKAGE},
        {"WORKING-STORAGE", TOK_WORKING_STORAGE},
        {"OTHER", TOK_KEYWORD},
        {"ADVANCING", TOK_KEYWORD},
        {"ALTERNATE", TOK_KEYWORD},
        {"BLOCK", TOK_KEYWORD},
        {"CONTAINS", TOK_KEYWORD},
        {"DYNAMIC", TOK_KEYWORD},
        {"EQUAL", TOK_KEYWORD},
        {"EVALUATE", TOK_KEYWORD},
        {"IS", TOK_KEYWORD},
        {"WITH", TOK_KEYWORD},
        {"I-O", TOK_KEYWORD},
        {"LINE", TOK_KEYWORD},
        {"SEQUENTIAL", TOK_KEYWORD},
        {"AUTHOR", TOK_KEYWORD},
        {"INSTALLATION", TOK_KEYWORD},
        {"DATE-WRITTEN", TOK_KEYWORD},
        {"DATE-COMPILED", TOK_KEYWORD},
        {"SECURITY", TOK_KEYWORD},
        {"REMARKS", TOK_KEYWORD},
        {"SOURCE-COMPUTER", TOK_KEYWORD},
        {"OBJECT-COMPUTER", TOK_KEYWORD},
        {"SPECIAL-NAMES", TOK_KEYWORD},
        {"I-O-CONTROL", TOK_KEYWORD},
        {"SCREEN", TOK_KEYWORD},
        {"STRING", TOK_KEYWORD}
    };

    char upper[31];
    int i = 0;

    while (word[i] != '\0' && i < 30) {
        upper[i] = (char)toupper((unsigned char)word[i]);
        i++;
    }

    upper[i] = '\0';

    for (size_t keyword = 0;
         keyword < sizeof(keywords) / sizeof(keywords[0]);
         keyword++) {
        if (strcmp(upper, keywords[keyword].word) == 0) {
            return keywords[keyword].type;
        }
    }

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

    if (too_long || length > 30 ||
        word[stored_length - 1] == '-' ||
        word[stored_length - 1] == '_') {
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

    char *number = NULL;
    size_t length = 0;
    size_t capacity = 0;
    int has_digit = 0;
    int allocation_failed = 0;

    if (lexer->current_char == '+' || lexer->current_char == '-') {
        if (!append_char(&number, &length, &capacity, lexer->current_char)) {
            allocation_failed = 1;
        }
        lexer_advance(lexer);
    }

    while (is_digit(lexer->current_char)) {
        if (!allocation_failed &&
            !append_char(&number, &length, &capacity, lexer->current_char)) {
            allocation_failed = 1;
        }

        has_digit = 1;
        lexer_advance(lexer);
    }

    if (lexer->current_char == '.') {
        int next_char = fgetc(lexer->file);

        if (next_char != EOF) {
            ungetc(next_char, lexer->file);
        }

        if (lexer->column < 72 && is_digit(next_char)) {
            if (!allocation_failed &&
                !append_char(&number, &length, &capacity, '.')) {
                allocation_failed = 1;
            }
            lexer_advance(lexer);

            while (is_digit(lexer->current_char)) {
                if (!allocation_failed &&
                    !append_char(&number, &length, &capacity,
                                 lexer->current_char)) {
                    allocation_failed = 1;
                }
                lexer_advance(lexer);
            }
        }
    }

    if (!has_digit || allocation_failed) {
        token.type = TOK_ERROR;
    }

    if (allocation_failed) {
        free(number);
    } else {
        token.lexeme = number;
    }

    if (!has_digit && token.lexeme == NULL) {
        token.lexeme = copy_lexeme("", 0);
    }

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
    char *text = NULL;
    size_t length = 0;
    size_t capacity = 0;
    int invalid = 0;
    int allocation_failed = 0;

    if (!append_char(&text, &length, &capacity, quote)) {
        allocation_failed = 1;
    }
    lexer_advance(lexer);

    while (lexer->current_char != EOF) {

        if (lexer->current_char == quote) {
            int next_char = lexer->column < 72
                          ? fgetc(lexer->file)
                          : EOF;

            if (next_char != EOF) {
                ungetc(next_char, lexer->file);
            }

            if (next_char == quote) {
                if (!allocation_failed &&
                    (!append_char(&text, &length, &capacity, quote) ||
                     !append_char(&text, &length, &capacity, quote))) {
                    allocation_failed = 1;
                }
                lexer_advance(lexer);
                lexer_advance(lexer);
                continue;
            }

            if (!allocation_failed &&
                !append_char(&text, &length, &capacity, quote)) {
                allocation_failed = 1;
            }
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

        if (!allocation_failed &&
            !append_char(&text, &length, &capacity, lexer->current_char)) {
            allocation_failed = 1;
        }

        lexer_advance(lexer);
    }

    if (!allocation_failed && lexer->current_char == EOF &&
        (length == 0 || text[length - 1] != quote)) {
        invalid = 1;
    }

    if (allocation_failed || length < 2 || text[length - 1] != quote) {
        invalid = 1;
    }

    if (invalid || allocation_failed) {
        token.type = TOK_ERROR;
    }

    if (allocation_failed) {
        free(text);
    } else {
        token.lexeme = text;
    }

    if (token.lexeme == NULL) {
        token.type = TOK_ERROR;
    }

    return token;
}


static int digit_sequence_continues_identifier(Lexer *lexer)
{
    fpos_t position;

    if (fgetpos(lexer->file, &position) != 0) {
        return 0;
    }

    int column = lexer->column + 1;
    int c = fgetc(lexer->file);

    while (is_digit(c) && column < 72) {
        c = fgetc(lexer->file);
        column++;
    }

    int result = column <= 72 && c == '-';

    if (fsetpos(lexer->file, &position) != 0) {
        return 0;
    }

    return result;
}

static int peek_next_char(Lexer *lexer){
    if (lexer->column >= 72) {
        return EOF;
    }

    int next_char = fgetc(lexer->file);

    if (next_char != EOF) {
        ungetc(next_char, lexer->file);
    }

    return next_char;
}


static Token finish_token(Lexer *lexer, Token token){
    switch (token.type) {
        case TOK_IDENTIFIER:
        case TOK_NUMERIC_LITERAL:
        case TOK_ALPHANUMERIC_LITERAL:
        case TOK_RPAREN:
            lexer->can_start_signed_number = 0;
            break;

        case TOK_EOF:
            break;

        default:
            lexer->can_start_signed_number = 1;
            break;
    }

    return token;
}


static Token read_symbol(Lexer *lexer, TokenType type, size_t length){
    Token token;
    char symbol[3];

    token.type = type;
    token.line = lexer->line;
    token.column = lexer->column;
    token.lexeme = NULL;

    for (size_t i = 0; i < length; i++) {
        symbol[i] = (char)lexer->current_char;
        lexer_advance(lexer);
    }
    symbol[length] = '\0';

    token.lexeme = copy_lexeme(symbol, length);

    if (token.lexeme == NULL) {
        token.type = TOK_ERROR;
    }

    return finish_token(lexer, token);
}


Token lexer_next_token(Lexer *lexer){
    Token token;

    while (lexer->current_char != EOF &&
           isspace((unsigned char)lexer->current_char)) {
        lexer_advance(lexer);
    }

    token.line = lexer->line;
    token.column = lexer->column;
    token.lexeme = NULL;

    if (lexer->current_char == EOF) {
        token.type = TOK_EOF;
        return finish_token(lexer, token);
    }

    if (lexer->current_char == '"' ||
        lexer->current_char == '\'') {
        return finish_token(lexer, read_string(lexer));
    }

    if (is_alpha(lexer->current_char)) {
        return finish_token(lexer, read_word(lexer));
    }

    if (is_digit(lexer->current_char)) {
        if (digit_sequence_continues_identifier(lexer)) {
            return finish_token(lexer, read_word(lexer));
        }

        return finish_token(lexer, read_number(lexer));
    }

    if (lexer->current_char == '+' || lexer->current_char == '-') {
        int next_char = peek_next_char(lexer);

        if (is_digit(next_char) && lexer->can_start_signed_number) {
            return finish_token(lexer, read_number(lexer));
        }
    }

    switch (lexer->current_char) {
        case '.':
            return read_symbol(lexer, TOK_PERIOD, 1);

        case ',':
            return read_symbol(lexer, TOK_COMMA, 1);

        case ';':
            return read_symbol(lexer, TOK_SEMICOLON, 1);

        case ':':
            return read_symbol(lexer, TOK_COLON, 1);

        case '(':
            return read_symbol(lexer, TOK_LPAREN, 1);

        case ')':
            return read_symbol(lexer, TOK_RPAREN, 1);

        case '+':
            return read_symbol(lexer, TOK_PLUS, 1);

        case '-':
            return read_symbol(lexer, TOK_MINUS, 1);

        case '*':
            if (peek_next_char(lexer) == '*') {
                return read_symbol(lexer, TOK_POWER, 2);
            }
            return read_symbol(lexer, TOK_ASTERISK, 1);

        case '/':
            return read_symbol(lexer, TOK_SLASH, 1);

        case '=':
            return read_symbol(lexer, TOK_EQUAL, 1);

        case '<':
            if (peek_next_char(lexer) == '=') {
                return read_symbol(lexer, TOK_LESS_EQUAL, 2);
            }
            if (peek_next_char(lexer) == '>') {
                return read_symbol(lexer, TOK_NOT_EQUAL, 2);
            }
            return read_symbol(lexer, TOK_LESS, 1);

        case '>':
            if (peek_next_char(lexer) == '=') {
                return read_symbol(lexer, TOK_GREATER_EQUAL, 2);
            }
            return read_symbol(lexer, TOK_GREATER, 1);

        default:
            return read_symbol(lexer, TOK_ERROR, 1);
    }
}
