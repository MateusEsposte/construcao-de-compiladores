#include <stdio.h>
#include "lexer.h"

int main(void)
{
    FILE *file = fopen("teste.cob", "r");

    if (file == NULL) {
        printf("Erro ao abrir o arquivo.\n");
        return 1;
    }

    Lexer lexer;
    lexer_init(&lexer, file);

    Token token;

    do {
        token = lexer_next_token(&lexer);

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

    return 0;
}