#include <stdio.h>
#include <string.h>

#include "parser.h"

static int parse_source(const char *source, int should_succeed)
{
    FILE *input = tmpfile();
    FILE *diagnostics = tmpfile();

    if (input == NULL || diagnostics == NULL) {
        perror("tmpfile");
        if (input != NULL) {
            fclose(input);
        }
        if (diagnostics != NULL) {
            fclose(diagnostics);
        }
        return 1;
    }

    if (fputs(source, input) == EOF || fseek(input, 0, SEEK_SET) != 0) {
        perror("preparar entrada do teste");
        fclose(input);
        fclose(diagnostics);
        return 1;
    }

    int succeeded = parser_parse(input, diagnostics);
    int failed = succeeded != should_succeed;

    if (failed) {
        rewind(diagnostics);
        fprintf(stderr, "resultado inesperado ao analisar:\n%s\n", source);
        int c;
        while ((c = fgetc(diagnostics)) != EOF) {
            fputc(c, stderr);
        }
    }

    fclose(input);
    fclose(diagnostics);
    return failed;
}

int main(void)
{
    int failures = 0;

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. TEST-PROGRAM.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "           77 COUNTER PIC 9(2) VALUE 0.\n"
        "       PROCEDURE DIVISION.\n"
        "       MAIN-PROCEDURE.\n"
        "           MOVE 0 TO COUNTER.\n"
        "           IF COUNTER = 0\n"
        "              DISPLAY \"zero\"\n"
        "           ELSE\n"
        "              DISPLAY \"nonzero\"\n"
        "           END-IF.\n"
        "           PERFORM MAIN-PROCEDURE UNTIL COUNTER > 2.\n"
        "           STOP RUN.\n"
        "       END PROGRAM TEST-PROGRAM.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. TEST.\n"
        "       PROCEDURE DIVISION.\n"
        "           MOVE 1 VALUE.\n",
        0);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. TEST.\n"
        "       PROCEDURE DIVISION.\n"
        "           IF 1 = 1 DISPLAY \"missing end if\"\n",
        0);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. TEST.\n"
        "       PROCEDURE DIVISION.\n"
        "           @\n",
        0);

    if (failures != 0) {
        fprintf(stderr, "%d teste(s) do parser falharam.\n", failures);
        return 1;
    }

    puts("Todos os testes do parser passaram.");
    return 0;
}
