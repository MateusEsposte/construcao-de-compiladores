#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

static int print_tokens(FILE *file)
{
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

    return had_errors ? 1 : 0;
}

int main(int argc, char *argv[])
{
    int tokens_only = argc > 1 && strcmp(argv[1], "--tokens") == 0;
    if ((!tokens_only && argc > 2) || (tokens_only && argc > 3)) {
        fprintf(stderr, "Uso: %s [--tokens] [arquivo.cob]\n", argv[0]);
        return 2;
    }

    const char *filename = tokens_only
                         ? (argc == 3 ? argv[2] : "teste.cob")
                         : (argc == 2 ? argv[1] : "teste.cob");
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        perror(filename);
        return 1;
    }

    int result = tokens_only
               ? print_tokens(file)
               : (parser_parse(file, stderr) ? 0 : 1);
    fclose(file);

    if (!tokens_only && result == 0) {
        puts("Analise sintatica concluida sem erros.");
    }
    return result;
}