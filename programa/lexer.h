#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "token.h"

typedef struct {
    FILE *file;
    int current_char;
    int line;
    int column;
    int line_indicator;
    int can_start_signed_number;
} Lexer;

void lexer_init(Lexer *lexer, FILE *file);

Token lexer_next_token(Lexer *lexer);

#endif
