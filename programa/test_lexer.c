#include <stdio.h>
#include <string.h>

#include "lexer.h"

typedef struct {
    TokenType type;
    const char *lexeme;
} ExpectedToken;

static int check_tokens(const char *test_name, FILE *file,
                        const ExpectedToken *expected, size_t count)
{
    Lexer lexer;
    lexer_init(&lexer, file);

    int failed = 0;

    for (size_t i = 0; i < count; i++) {
        Token token = lexer_next_token(&lexer);

        if (token.type != expected[i].type ||
            token.lexeme == NULL ||
            strcmp(token.lexeme, expected[i].lexeme) != 0) {
            fprintf(stderr,
                    "%s: token %lu: esperado %s \"%s\", recebido %s \"%s\"\n",
                    test_name,
                    (unsigned long)i,
                    token_type_to_string(expected[i].type),
                    expected[i].lexeme,
                    token_type_to_string(token.type),
                    token.lexeme != NULL ? token.lexeme : "(vazio)");
            failed = 1;
        }

        token_free(&token);
    }

    Token eof = lexer_next_token(&lexer);
    if (eof.type != TOK_EOF) {
        fprintf(stderr, "%s: esperado TOK_EOF, recebido %s\n",
                test_name, token_type_to_string(eof.type));
        failed = 1;
    }
    token_free(&eof);

    fclose(file);
    return failed;
}


static int test_source(const char *test_name, const char *source,
                       const ExpectedToken *expected, size_t count)
{
    FILE *file = tmpfile();

    if (file == NULL) {
        perror("tmpfile");
        return 1;
    }

    if (fputs(source, file) == EOF || fseek(file, 0, SEEK_SET) != 0) {
        perror("preparar entrada de teste");
        fclose(file);
        return 1;
    }

    return check_tokens(test_name, file, expected, count);
}


int main(void)
{
    int failures = 0;

    static const ExpectedToken keywords[] = {
        {TOK_IDENTIFICATION, "identification"},
        {TOK_DIVISION, "division"},
        {TOK_PERIOD, "."},
        {TOK_PROGRAM_ID, "program-id"},
        {TOK_PERIOD, "."},
        {TOK_IDENTIFIER, "PROG-1"},
        {TOK_PERIOD, "."}
    };
    failures += test_source(
        "palavras-chave e identificadores",
        "       identification division.\n"
        "       program-id. PROG-1.\n",
        keywords, sizeof(keywords) / sizeof(keywords[0]));

    static const ExpectedToken operators[] = {
        {TOK_COMPUTE, "COMPUTE"},
        {TOK_IDENTIFIER, "RESULT"},
        {TOK_EQUAL, "="},
        {TOK_IDENTIFIER, "A"},
        {TOK_PLUS, "+"},
        {TOK_NUMERIC_LITERAL, "2"},
        {TOK_POWER, "**"},
        {TOK_NUMERIC_LITERAL, "3"},
        {TOK_MINUS, "-"},
        {TOK_NUMERIC_LITERAL, "1"},
        {TOK_SLASH, "/"},
        {TOK_NUMERIC_LITERAL, "4"},
        {TOK_PERIOD, "."}
    };
    failures += test_source(
        "operadores aritmeticos e de atribuicao",
        "       COMPUTE RESULT = A + 2 ** 3 - 1 / 4.\n",
        operators, sizeof(operators) / sizeof(operators[0]));

    static const ExpectedToken signed_number[] = {
        {TOK_MOVE, "MOVE"},
        {TOK_NUMERIC_LITERAL, "-12.50"},
        {TOK_TO, "TO"},
        {TOK_IDENTIFIER, "RESULT"},
        {TOK_PERIOD, "."}
    };
    failures += test_source(
        "literal numerico com sinal",
        "       MOVE -12.50 TO RESULT.\n",
        signed_number, sizeof(signed_number) / sizeof(signed_number[0]));

    static const ExpectedToken relational[] = {
        {TOK_IF, "IF"},
        {TOK_IDENTIFIER, "A"},
        {TOK_LESS_EQUAL, "<="},
        {TOK_NUMERIC_LITERAL, "2"},
        {TOK_OR, "OR"},
        {TOK_IDENTIFIER, "B"},
        {TOK_NOT_EQUAL, "<>"},
        {TOK_NUMERIC_LITERAL, "0"},
        {TOK_END_IF, "END-IF"},
        {TOK_PERIOD, "."}
    };
    failures += test_source(
        "operadores relacionais",
        "       IF A <= 2 OR B <> 0 END-IF.\n",
        relational, sizeof(relational) / sizeof(relational[0]));

    static const ExpectedToken in_keyword[] = {
        {TOK_IF, "IF"},
        {TOK_IDENTIFIER, "A"},
        {TOK_IN, "IN"},
        {TOK_IDENTIFIER, "B"},
        {TOK_PERIOD, "."}
    };
    failures += test_source(
        "palavra reservada IN",
        "       IF A IN B.\n",
        in_keyword, sizeof(in_keyword) / sizeof(in_keyword[0]));

    static const ExpectedToken literals[] = {
        {TOK_DISPLAY, "DISPLAY"},
        {TOK_ALPHANUMERIC_LITERAL, "\"a\"\"b\""},
        {TOK_ALPHANUMERIC_LITERAL, "'c''d'"},
        {TOK_PERIOD, "."}
    };
    failures += test_source(
        "literais delimitados e aspas duplicadas",
        "       DISPLAY \"a\"\"b\" 'c''d'.\n",
        literals, sizeof(literals) / sizeof(literals[0]));

    static const ExpectedToken punctuation[] = {
        {TOK_IDENTIFIER, "A"},
        {TOK_COLON, ":"},
        {TOK_IDENTIFIER, "B"},
        {TOK_COMMA, ","},
        {TOK_IDENTIFIER, "C"},
        {TOK_SEMICOLON, ";"},
        {TOK_LPAREN, "("},
        {TOK_NUMERIC_LITERAL, "1"},
        {TOK_RPAREN, ")"}
    };
    failures += test_source(
        "pontuacao",
        "       A:B,C;(1)\n",
        punctuation, sizeof(punctuation) / sizeof(punctuation[0]));

    static const ExpectedToken comments[] = {
        {TOK_DISPLAY, "DISPLAY"},
        {TOK_ALPHANUMERIC_LITERAL, "\"ok\""},
        {TOK_PERIOD, "."}
    };
    failures += test_source(
        "comentario de linha em formato fixo",
        "      * comentario ignorado\n"
        "       DISPLAY \"ok\".\n",
        comments, sizeof(comments) / sizeof(comments[0]));

    static const ExpectedToken identifier_extensions[] = {
        {TOK_ERROR, "_"},
        {TOK_KEYWORD, "STRING"},
        {TOK_IDENTIFIER, "PROG005_strings"},
        {TOK_ERROR, "A_"}
    };
    failures += test_source(
        "extensao de sublinhado do corpus e limites do identificador",
        "       _STRING PROG005_strings A_\n",
        identifier_extensions,
        sizeof(identifier_extensions) / sizeof(identifier_extensions[0]));

    static const ExpectedToken identifier_length[] = {
        {TOK_IDENTIFIER, "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234"},
        {TOK_ERROR, "ABCDEFGHIJKLMNOPQRSTUVWXYZ12345"}
    };
    failures += test_source(
        "limite de 30 caracteres do identificador",
        "       ABCDEFGHIJKLMNOPQRSTUVWXYZ1234\n"
        "       ABCDEFGHIJKLMNOPQRSTUVWXYZ12345\n",
        identifier_length,
        sizeof(identifier_length) / sizeof(identifier_length[0]));

    char long_number_source[74];
    char long_number_expected[66];
    long_number_expected[0] = '+';
    memset(long_number_expected + 1, '7', 64);
    long_number_expected[65] = '\0';
    memset(long_number_source, ' ', 7);
    long_number_source[7] = '+';
    memset(long_number_source + 8, '7', 64);
    long_number_source[72] = '\n';
    long_number_source[73] = '\0';

    ExpectedToken long_number[] = {
        {TOK_NUMERIC_LITERAL, long_number_expected}
    };
    failures += test_source(
        "literal numerico maior que o antigo buffer",
        long_number_source, long_number,
        sizeof(long_number) / sizeof(long_number[0]));

    if (failures == 0) {
        puts("Todos os testes do lexer passaram.");
        return 0;
    }

    fprintf(stderr, "%d teste(s) falharam.\n", failures);
    return 1;
}
