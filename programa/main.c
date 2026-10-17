#include <stdio.h>
#include "lexer.h"

int main(int argc, char *argv[])
{
    if (argc > 2) {
        fprintf(stderr, "Uso: %s [arquivo.cob]\n", argv[0]);
        return 2;
    }

    const char *filename = argc == 2 ? argv[1] : "teste.cob";
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        perror(filename);
        return 1;
    }

    Lexer lexer;
    lexer_init(&lexer, file);

    int had_errors = 0;
    Token token;

    do {
        token = lexer_next_token(&lexer);

        if (token.type == TOK_ERROR) {
            had_errors = 1;
        }

        printf(
            "%-25s | %-20s | %5d | %6d\n",
            token_type_to_string(token.type),
            token.lexeme != NULL ? token.lexeme : "(vazio)",
            token.line,
            token.column
        );

        token_free(&token);

    } while (token.type != TOK_EOF);

    fclose(file);

    return had_errors ? 1 : 0;
}