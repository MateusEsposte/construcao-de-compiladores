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

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. ENV-TEST.\n"
        "       ENVIRONMENT DIVISION.\n"
        "       CONFIGURATION SECTION.\n"
        "       SOURCE-COMPUTER. LOCAL-MACHINE.\n"
        "       OBJECT-COMPUTER. LOCAL-MACHINE.\n"
        "       INPUT-OUTPUT SECTION.\n"
        "       FILE-CONTROL.\n"
        "           SELECT INPUT-FILE ASSIGN TO \"INPUT.DAT\"\n"
        "               ORGANIZATION IS SEQUENTIAL\n"
        "               ACCESS MODE IS DYNAMIC\n"
        "               FILE STATUS FILE-STATUS.\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. LINE-SEQUENTIAL-TEST.\n"
        "       ENVIRONMENT DIVISION.\n"
        "       INPUT-OUTPUT SECTION.\n"
        "       FILE-CONTROL.\n"
        "           SELECT INPUT-FILE ASSIGN TO \"INPUT.DAT\"\n"
        "               ORGANIZATION IS LINE SEQUENTIAL.\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. ENV-TEST.\n"
        "       ENVIRONMENT DIVISION.\n"
        "       BLAH BLAH FOO BAR.\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n",
        0);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. ENV-TEST.\n"
        "       ENVIRONMENT DIVISION.\n"
        "       INPUT-OUTPUT SECTION.\n"
        "       FILE-CONTROL.\n"
        "           SELECT INPUT-FILE ASSIGN \"INPUT.DAT\".\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n",
        0);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. DATA-TEST.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       01 GROUP-A.\n"
        "          05 FIELD-X PIC X(2).\n"
        "          05 FIELD-N PIC S9(3)V99 VALUE 0.\n"
        "       77 SEPARATE PIC 9.\n"
        "          88 IS-READY VALUE 1.\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. DATA-TEST.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       01 ELEMENTARY-ITEM.\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n",
        0);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. COPY-TEST.\n"
        "       DATA DIVISION.\n"
        "       LINKAGE SECTION.\n"
        "       COPY COPYBOOK.\n"
        "       PROCEDURE DIVISION USING COPYBOOK-RECORD.\n"
        "       MAIN-PARAGRAPH.\n"
        "           EXIT.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. PICTURE-TEST.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       01 FORMATTED-AMOUNT PIC ZZZ.ZZZ.ZZ,ZZ.\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. IO-TEST.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       01 RECORD-AREA PIC X(10).\n"
        "       PROCEDURE DIVISION.\n"
        "           READ INPUT-FILE INTO RECORD-AREA\n"
        "               AT END CONTINUE\n"
        "               NOT AT END DISPLAY \"FOUND\"\n"
        "           END-READ.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. READ-ELSE-TEST.\n"
        "       PROCEDURE DIVISION.\n"
        "           IF STATUS-FLAG = 0\n"
        "               READ INPUT-FILE INTO RECORD-AREA\n"
        "           ELSE\n"
        "               IF STATUS-FLAG = 35\n"
        "                   IF STATUS-FLAG = 0\n"
        "                       CONTINUE\n"
        "                   ELSE\n"
        "                       CONTINUE\n"
        "               ELSE\n"
        "                   CONTINUE.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. READ-KEY-TEST.\n"
        "       PROCEDURE DIVISION.\n"
        "           READ INPUT-FILE RECORD KEY RECORD-KEY\n"
        "               INVALID KEY DISPLAY \"NOT FOUND\"\n"
        "               NOT INVALID KEY DISPLAY \"FOUND\".\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. INITIALIZE-TEST.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       01 GROUP-A.\n"
        "          05 TEXT-FIELD PIC X(2).\n"
        "       PROCEDURE DIVISION.\n"
        "           INITIALIZE GROUP-A REPLACING ALPHANUMERIC BY SPACES.\n"
        "           CONTINUE.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. PERFORM-TEST.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       01 INDEX-VALUE PIC 9.\n"
        "       01 TEXT-VALUE PIC X(10).\n"
        "       PROCEDURE DIVISION.\n"
        "           PERFORM VARYING INDEX-VALUE FROM 1 BY 1 UNTIL\n"
        "               (INDEX-VALUE > LENGTH OF TEXT-VALUE) OR\n"
        "               (INDEX-VALUE EQUAL 1)\n"
        "               IF INDEX-VALUE > 1\n"
        "                   DISPLAY TEXT-VALUE(INDEX-VALUE:1)\n"
        "               ELSE CONTINUE\n"
        "           END-PERFORM.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. GO-TO-TEST.\n"
        "       PROCEDURE DIVISION.\n"
        "           GO TO FINISH.\n"
        "       FINISH.\n"
        "           STOP RUN.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. EVALUATE-TEST.\n"
        "       PROCEDURE DIVISION.\n"
        "           EVALUATE FIELD-A\n"
        "               WHEN IS NOT NUMERIC DISPLAY \"INVALID\"\n"
        "               WHEN OTHER CONTINUE\n"
        "           END-EVALUATE.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. CALL-TEST.\n"
        "       PROCEDURE DIVISION.\n"
        "           CALL \"SUBPROGRAM\" USING ARGUMENT.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. NOT-EQUAL-TEST.\n"
        "       PROCEDURE DIVISION.\n"
        "           IF FIELD-A NOT = 0\n"
        "               GOBACK\n"
        "           END-IF.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. COMP-TEST.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       01 RETURN-CODE PIC S9(4) COMP VALUE ZERO.\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. FUNCTION-TEST.\n"
        "       PROCEDURE DIVISION.\n"
        "           COMPUTE REMAINDER-VALUE = FUNCTION MOD(TOTAL, 11).\n"
        "           STOP RUN.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. DATA-TEST.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       01 GROUP-A.\n"
        "          88 IS-READY VALUE 1.\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n",
        0);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. DATA-TEST.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       99 FIELD-A PIC 9.\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n",
        0);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. DATA-TEST.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       01 FIELD-A PIC Q(2).\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n",
        0);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. DATA-TEST.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       50 FIELD-A PIC 9.\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n",
        0);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. DATA-TEST.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       02 FIELD-A PIC 9.\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n",
        0);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. PROCEDURE-TEST.\n"
        "       PROCEDURE DIVISION.\n"
        "       MAIN-SECTION SECTION.\n"
        "       MAIN-PARAGRAPH.\n"
        "           DISPLAY \"A\"\n"
        "           DISPLAY \"B\".\n"
        "           STOP RUN.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. PROCEDURE-TEST.\n"
        "       PROCEDURE DIVISION.\n"
        "       MAIN-PARAGRAPH.\n"
        "           DISPLAY \"A\"\n"
        "           DISPLAY \"B\"\n"
        "           STOP RUN\n",
        0);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. PROCEDURE-TEST.\n"
        "       PROCEDURE DIVISION.\n"
        "       FIRST-PARAGRAPH.\n"
        "           DISPLAY \"missing period\"\n"
        "       SECOND-PARAGRAPH.\n"
        "           STOP RUN.\n",
        0);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. PROCEDURE-FEATURES.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       01 INDEX-VALUE PIC 9.\n"
        "       01 VALUE-A PIC 9(3).\n"
        "       01 TEXT-VALUE PIC X(10).\n"
        "       01 ARG-VALUE PIC 9.\n"
        "       PROCEDURE DIVISION.\n"
        "           ACCEPT DATE-FIELD FROM DATE YYYYMMDD.\n"
        "           DIVIDE 2 INTO VALUE-A.\n"
        "           COMPUTE VALUE-A ROUNDED = VALUE-A + 1.\n"
        "           IF VALUE-A > 0 THEN DISPLAY \"OK\" END-IF.\n"
        "           PERFORM UNTIL VALUE-A > 2\n"
        "               EXIT PERFORM\n"
        "           END-PERFORM.\n"
        "           CALL \"SUB\" USING BY VALUE 1 BY REFERENCE ARG-VALUE.\n"
        "           OPEN INPUT INPUT-A INPUT INPUT-B OUTPUT OUTPUT-A.\n"
        "           SORT SORT-FILE ON ASCENDING KEY VALUE-A\n"
        "               USING INPUT-A GIVING OUTPUT-A.\n"
        "           UNSTRING TEXT-VALUE DELIMITED BY ALL \"|\"\n"
        "               INTO ARG-VALUE ARG-VALUE END-UNSTRING.\n"
        "           WRITE RECORD-A INVALID KEY DISPLAY \"NO\"\n"
        "               NOT INVALID KEY DISPLAY \"YES\" END-WRITE.\n"
        "           STOP RUN.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. DATA-FEATURES.\n"
        "       ENVIRONMENT DIVISION.\n"
        "       CONFIGURATION SECTION.\n"
        "       SPECIAL-NAMES.\n"
        "           DECIMAL-POINT IS COMMA.\n"
        "       INPUT-OUTPUT SECTION.\n"
        "       FILE-CONTROL.\n"
        "           SELECT INPUT-A ASSIGN TO \"INPUT.DAT\"\n"
        "               FILE STATUS IS FILE-STATUS.\n"
        "       DATA DIVISION.\n"
        "       FILE SECTION.\n"
        "       FD INPUT-A DATA RECORD IS INPUT-RECORD.\n"
        "       01 INPUT-RECORD PIC X(10).\n"
        "       SD SORT-FILE.\n"
        "       01 SORT-RECORD PIC X(10).\n"
        "       WORKING-STORAGE SECTION.\n"
        "       01 RATE PIC V999 VALUE 0,25.\n"
        "       01 DASHES PIC X(5) VALUE ALL \"-\".\n"
        "       SCREEN SECTION.\n"
        "       01 TEST-SCREEN.\n"
        "          05 BLANK SCREEN.\n"
        "          05 LINE 02 COLUMN 10 VALUE \"TITLE\".\n"
        "          05 LINE 04 COLUMN 10 PIC X(10) USING TEXT-VALUE.\n"
        "       PROCEDURE DIVISION.\n"
        "           DISPLAY \"Name\" AT LINE 5 COLUMN 10 TEXT-VALUE.\n"
        "           STOP RUN.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. EMBEDDED-CODE.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       EXEC SQL INCLUDE SQLCA END-EXEC.\n"
        "       01 FIELD-A PIC X.\n"
        "       PROCEDURE DIVISION.\n"
        "           EXEC CICS RETURN END-EXEC.\n"
        "           STOP RUN.\n",
        1);

    failures += parse_source(
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. UNSTRING-ERROR.\n"
        "       PROCEDURE DIVISION.\n"
        "           UNSTRING SOURCE-VALUE DELIMITED BY '-'\n"
        "               INTO TARGET-A, END-UNSTRING.\n",
        0);

    if (failures != 0) {
        fprintf(stderr, "%d teste(s) do parser falharam.\n", failures);
        return 1;
    }

    puts("Todos os testes do parser passaram.");
    return 0;
}
