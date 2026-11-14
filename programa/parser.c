#include "parser.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

typedef struct {
    Lexer lexer;
    Token current;
    Token next;
    FILE *diagnostics;
    int errors;
} Parser;

static void advance(Parser *parser)
{
    token_free(&parser->current);
    parser->current = parser->next;
    parser->next = lexer_next_token(&parser->lexer);
}

static void report(Parser *parser, const char *format, ...)
{
    va_list arguments;

    fprintf(parser->diagnostics, "Erro sintatico na linha %d, coluna %d: ",
            parser->current.line, parser->current.column);
    va_start(arguments, format);
    vfprintf(parser->diagnostics, format, arguments);
    va_end(arguments);
    fputc('\n', parser->diagnostics);
    parser->errors++;
}

static void report_lexical_error(Parser *parser)
{
    fprintf(parser->diagnostics,
            "Erro lexico na linha %d, coluna %d: token invalido \"%s\"\n",
            parser->current.line, parser->current.column,
            parser->current.lexeme != NULL ? parser->current.lexeme : "");
    parser->errors++;
}

static int word_is(const Parser *parser, const char *word)
{
    if (parser->current.lexeme == NULL) {
        return 0;
    }

    const unsigned char *left =
        (const unsigned char *)parser->current.lexeme;
    const unsigned char *right = (const unsigned char *)word;

    while (*left != '\0' && *right != '\0' &&
           toupper(*left) == toupper(*right)) {
        left++;
        right++;
    }

    return *left == '\0' && *right == '\0';
}

static int is_type(const Parser *parser, TokenType type)
{
    return parser->current.type == type;
}

static int accept_type(Parser *parser, TokenType type)
{
    if (!is_type(parser, type)) {
        return 0;
    }
    advance(parser);
    return 1;
}

static int accept_word(Parser *parser, const char *word)
{
    if (!word_is(parser, word)) {
        return 0;
    }
    advance(parser);
    return 1;
}

static int expect_type(Parser *parser, TokenType type, const char *expected)
{
    if (accept_type(parser, type)) {
        return 1;
    }
    report(parser, "esperado %s, encontrado %s \"%s\"",
           expected, token_type_to_string(parser->current.type),
           parser->current.lexeme != NULL ? parser->current.lexeme : "");
    return 0;
}

static int is_name(const Parser *parser)
{
    return parser->current.type == TOK_IDENTIFIER ||
           parser->current.type == TOK_KEYWORD ||
           parser->current.type == TOK_UPPER_CASE;
}

static int parse_name(Parser *parser)
{
    if (!is_name(parser)) {
        report(parser, "esperado um identificador");
        return 0;
    }

    advance(parser);
    return 1;
}

static int is_statement_start(const Parser *parser)
{
    switch (parser->current.type) {
        case TOK_DISPLAY:
        case TOK_ACCEPT:
        case TOK_PERFORM:
        case TOK_STOP:
        case TOK_IF:
        case TOK_MOVE:
        case TOK_ADD:
        case TOK_SUBTRACT:
        case TOK_MULTIPLY:
        case TOK_DIVIDE:
        case TOK_COMPUTE:
        case TOK_INSPECT:
        case TOK_SEARCH:
        case TOK_SET:
        case TOK_OPEN:
        case TOK_CLOSE:
        case TOK_READ:
        case TOK_WRITE:
        case TOK_START:
            return 1;
        default:
            return word_is(parser, "EVALUATE");
    }
}

static int is_relation(const Parser *parser)
{
    return parser->current.type == TOK_EQUAL ||
           parser->current.type == TOK_LESS ||
           parser->current.type == TOK_GREATER ||
           parser->current.type == TOK_LESS_EQUAL ||
           parser->current.type == TOK_GREATER_EQUAL ||
           parser->current.type == TOK_NOT_EQUAL ||
           word_is(parser, "IS") || word_is(parser, "EQUAL") ||
           word_is(parser, "GREATER") || word_is(parser, "LESS");
}

static int parse_expression(Parser *parser);

static int parse_data_reference(Parser *parser)
{
    if (!parse_name(parser)) {
        return 0;
    }

    while (word_is(parser, "OF") || word_is(parser, "IN")) {
        advance(parser);
        if (!parse_name(parser)) {
            return 0;
        }
    }

    if (accept_type(parser, TOK_LPAREN)) {
        if (!parse_expression(parser)) {
            return 0;
        }
        while (accept_type(parser, TOK_COMMA)) {
            if (!parse_expression(parser)) {
                return 0;
            }
        }
        if (!expect_type(parser, TOK_RPAREN, "\")\"")) {
            return 0;
        }
    }

    return 1;
}

static int parse_primary(Parser *parser)
{
    if (accept_type(parser, TOK_NUMERIC_LITERAL) ||
        accept_type(parser, TOK_ALPHANUMERIC_LITERAL) ||
        accept_type(parser, TOK_SPACES) ||
        accept_type(parser, TOK_ZERO) ||
        accept_type(parser, TOK_ZEROS) ||
        accept_type(parser, TOK_LOW_VALUES)) {
        return 1;
    }

    if (accept_type(parser, TOK_LPAREN)) {
        if (!parse_expression(parser)) {
            return 0;
        }
        return expect_type(parser, TOK_RPAREN, "\")\"");
    }

    if (accept_word(parser, "FUNCTION")) {
        if (!parse_name(parser)) {
            return 0;
        }
        if (accept_type(parser, TOK_LPAREN)) {
            if (!parse_expression(parser)) {
                return 0;
            }
            return expect_type(parser, TOK_RPAREN, "\")\"");
        }
        return 1;
    }

    return parse_data_reference(parser);
}

static int parse_unary(Parser *parser)
{
    if (accept_type(parser, TOK_PLUS) || accept_type(parser, TOK_MINUS) ||
        accept_type(parser, TOK_NOT)) {
        return parse_unary(parser);
    }
    return parse_primary(parser);
}

static int parse_binary(Parser *parser, int minimum_precedence)
{
    if (!parse_unary(parser)) {
        return 0;
    }

    for (;;) {
        int precedence;
        TokenType type = parser->current.type;

        if (type == TOK_POWER) {
            precedence = 3;
        } else if (type == TOK_ASTERISK || type == TOK_SLASH) {
            precedence = 2;
        } else if (type == TOK_PLUS || type == TOK_MINUS) {
            precedence = 1;
        } else {
            break;
        }

        if (precedence < minimum_precedence) {
            break;
        }

        advance(parser);
        if (!parse_binary(parser, precedence + (type == TOK_POWER ? 0 : 1))) {
            return 0;
        }
    }

    return 1;
}

static int parse_expression(Parser *parser)
{
    return parse_binary(parser, 1);
}

static int parse_condition(Parser *parser, int stop_at_statement)
{
    if (!parse_expression(parser)) {
        return 0;
    }

    if (is_relation(parser)) {
        if (word_is(parser, "IS") || word_is(parser, "EQUAL") ||
            word_is(parser, "GREATER") || word_is(parser, "LESS")) {
            advance(parser);
            if (word_is(parser, "NOT") || word_is(parser, "THAN")) {
                advance(parser);
            }
        } else {
            advance(parser);
        }

        if (word_is(parser, "THAN") || word_is(parser, "TO")) {
            advance(parser);
        }
        if (!parse_expression(parser)) {
            return 0;
        }
    }

    while (is_type(parser, TOK_AND) || is_type(parser, TOK_OR)) {
        advance(parser);
        if (word_is(parser, "NOT")) {
            advance(parser);
        }
        if (!parse_condition(parser, 0)) {
            return 0;
        }
        if (stop_at_statement && is_statement_start(parser)) {
            break;
        }
    }

    return 1;
}

static int parse_operand_list(Parser *parser, int required)
{
    int count = 0;

    while (parser->current.type != TOK_EOF &&
           !is_statement_start(parser) &&
           !is_type(parser, TOK_PERIOD) &&
           !is_type(parser, TOK_ELSE) &&
           !is_type(parser, TOK_END_IF) &&
           !is_type(parser, TOK_END_SEARCH) &&
           !is_type(parser, TOK_END_EVALUATE) &&
           !is_type(parser, TOK_WHEN) &&
           !is_type(parser, TOK_AT) &&
           !is_type(parser, TOK_END) &&
           !is_type(parser, TOK_INVALID) &&
           !word_is(parser, "TO") && !word_is(parser, "FROM") &&
           !word_is(parser, "BY") && !word_is(parser, "GIVING") &&
           !word_is(parser, "REMAINDER") &&
           !word_is(parser, "UNTIL") &&
           !word_is(parser, "INVALID") &&
           !word_is(parser, "NOT") &&
           !word_is(parser, "ON") &&
           !word_is(parser, "WITH")) {
        if (!parse_primary(parser)) {
            return 0;
        }
        count++;
        accept_type(parser, TOK_COMMA);
    }

    if (required && count == 0) {
        report(parser, "esperado ao menos um operando");
        return 0;
    }
    return 1;
}

static int parse_statements(Parser *parser, int stop_else,
                            int stop_end_if, int stop_end_search,
                            int stop_end_evaluate);
static int is_paragraph_header(const Parser *parser);
static int parse_io_handlers(Parser *parser, int enclosing_stop_else);

static int parse_statement(Parser *parser, int enclosing_stop_else)
{
    if (accept_type(parser, TOK_DISPLAY)) {
        if (!parse_operand_list(parser, 1)) {
            return 0;
        }
        if (accept_word(parser, "WITH")) {
            accept_word(parser, "NO");
            accept_word(parser, "ADVANCING");
        }
        return 1;
    }

    if (accept_type(parser, TOK_ACCEPT)) {
        return parse_data_reference(parser);
    }

    if (accept_type(parser, TOK_MOVE)) {
        if (!parse_operand_list(parser, 1) ||
            !expect_type(parser, TOK_TO, "TO") ||
            !parse_operand_list(parser, 1)) {
            return 0;
        }
        return 1;
    }

    if (is_type(parser, TOK_ADD) || is_type(parser, TOK_SUBTRACT) ||
        is_type(parser, TOK_MULTIPLY) || is_type(parser, TOK_DIVIDE)) {
        TokenType operation = parser->current.type;
        advance(parser);

        if (!parse_operand_list(parser, 1)) {
            return 0;
        }

        if (operation == TOK_ADD) {
            if (accept_type(parser, TOK_TO)) {
                if (!parse_operand_list(parser, 1)) {
                    return 0;
                }
            } else if (!word_is(parser, "GIVING")) {
                report(parser, "esperado TO ou GIVING em ADD");
                return 0;
            }
        } else if (operation == TOK_SUBTRACT) {
            if (!expect_type(parser, TOK_FROM, "FROM") ||
                !parse_operand_list(parser, 1)) {
                return 0;
            }
        } else if (!expect_type(parser, TOK_BY, "BY") ||
                   !parse_operand_list(parser, 1)) {
            return 0;
        }

        if (accept_word(parser, "GIVING") &&
            !parse_operand_list(parser, 1)) {
            return 0;
        }
        if (operation == TOK_DIVIDE && accept_word(parser, "REMAINDER") &&
            !parse_operand_list(parser, 1)) {
            return 0;
        }
        if (accept_type(parser, TOK_ON)) {
            accept_word(parser, "SIZE");
            accept_type(parser, TOK_ERROR_WORD);
            if (!parse_statements(parser, enclosing_stop_else, 1, 1, 1)) {
                return 0;
            }
        }
        return 1;
    }

    if (accept_type(parser, TOK_COMPUTE)) {
        if (!parse_data_reference(parser) ||
            !expect_type(parser, TOK_EQUAL, "\"=\"") ||
            !parse_expression(parser)) {
            return 0;
        }
        return 1;
    }

    if (accept_type(parser, TOK_IF)) {
        if (!parse_condition(parser, 1)) {
            return 0;
        }
        if (!parse_statements(parser, 1, 1, 0, 0)) {
            return 0;
        }
        int has_else = accept_type(parser, TOK_ELSE);
        if (has_else &&
            !parse_statements(parser, enclosing_stop_else, 1, 0, 0)) {
            return 0;
        }
        if (!accept_type(parser, TOK_END_IF) &&
            !(is_type(parser, TOK_PERIOD) ||
              (enclosing_stop_else && is_type(parser, TOK_ELSE)) ||
              is_paragraph_header(parser))) {
            report(parser, "esperado END-IF para encerrar IF");
            return 0;
        }
        return 1;
    }

    if (accept_type(parser, TOK_PERFORM)) {
        if (is_name(parser)) {
            parse_name(parser);
        } else if (!parse_statements(parser, enclosing_stop_else, 0, 0, 0)) {
            return 0;
        }
        if (accept_type(parser, TOK_UNTIL) &&
            !parse_condition(parser, 1)) {
            return 0;
        }
        return 1;
    }

    if (accept_type(parser, TOK_STOP)) {
        return expect_type(parser, TOK_RUN, "RUN");
    }

    if (accept_word(parser, "EVALUATE")) {
        if (!parse_expression(parser)) {
            return 0;
        }
        while (accept_type(parser, TOK_WHEN)) {
            if (accept_word(parser, "OTHER")) {
                if (!parse_statements(parser, enclosing_stop_else, 0, 0, 1)) {
                    return 0;
                }
            } else {
                if (!parse_condition(parser, 1) ||
                    !parse_statements(parser, enclosing_stop_else, 0, 0, 1)) {
                    return 0;
                }
            }
        }
        if (!accept_type(parser, TOK_END_EVALUATE)) {
            report(parser, "esperado END-EVALUATE para encerrar EVALUATE");
            return 0;
        }
        return 1;
    }

    if (accept_type(parser, TOK_SEARCH)) {
        if (!parse_data_reference(parser)) {
            return 0;
        }
        while (!is_type(parser, TOK_EOF) &&
               !is_type(parser, TOK_END_SEARCH)) {
            if (accept_type(parser, TOK_AT)) {
                accept_type(parser, TOK_END);
                while (!is_type(parser, TOK_EOF) &&
                       !is_type(parser, TOK_WHEN) &&
                       !is_type(parser, TOK_END_SEARCH) &&
                       !is_type(parser, TOK_PERIOD)) {
                    if (!is_statement_start(parser) ||
                        !parse_statement(parser, enclosing_stop_else)) {
                        return 0;
                    }
                }
            } else if (accept_type(parser, TOK_WHEN)) {
                if (!parse_condition(parser, 1)) {
                    return 0;
                }
                while (!is_type(parser, TOK_EOF) &&
                       !is_type(parser, TOK_WHEN) &&
                       !is_type(parser, TOK_END_SEARCH) &&
                       !is_type(parser, TOK_PERIOD)) {
                    if (!is_statement_start(parser) ||
                        !parse_statement(parser, enclosing_stop_else)) {
                        return 0;
                    }
                }
            } else {
                report(parser, "esperado AT END ou WHEN em SEARCH");
                return 0;
            }
        }
        return expect_type(parser, TOK_END_SEARCH, "END-SEARCH");
    }

    if (accept_type(parser, TOK_SET)) {
        if (!parse_data_reference(parser) ||
            !expect_type(parser, TOK_TO, "TO") ||
            !parse_operand_list(parser, 1)) {
            return 0;
        }
        return 1;
    }

    if (accept_type(parser, TOK_INSPECT)) {
        if (!parse_data_reference(parser)) {
            return 0;
        }
        while (!is_type(parser, TOK_EOF) && !is_type(parser, TOK_PERIOD) &&
               !is_statement_start(parser)) {
            advance(parser);
        }
        return 1;
    }

    if (accept_type(parser, TOK_OPEN)) {
        if (!accept_word(parser, "INPUT") && !accept_word(parser, "OUTPUT") &&
            !accept_word(parser, "I-O")) {
            report(parser, "esperado modo INPUT, OUTPUT ou I-O em OPEN");
            return 0;
        }
        if (!parse_data_reference(parser)) {
            return 0;
        }
        while (is_name(parser) && parse_data_reference(parser)) {
        }
        return 1;
    }

    if (accept_type(parser, TOK_CLOSE)) {
        if (!parse_data_reference(parser)) {
            return 0;
        }
        while (is_name(parser) && parse_data_reference(parser)) {
        }
        return 1;
    }

    if (accept_type(parser, TOK_READ)) {
        if (!parse_name(parser)) {
            return 0;
        }
        while (!is_type(parser, TOK_EOF) && !is_type(parser, TOK_PERIOD) &&
               !is_statement_start(parser) && !is_type(parser, TOK_INVALID) &&
               !word_is(parser, "NOT")) {
            advance(parser);
        }
        if (is_type(parser, TOK_INVALID) || is_type(parser, TOK_NOT)) {
            return parse_io_handlers(parser, enclosing_stop_else);
        }
        return 1;
    }

    if (accept_type(parser, TOK_WRITE)) {
        if (!parse_name(parser)) {
            return 0;
        }
        if (accept_word(parser, "FROM") && !parse_data_reference(parser)) {
            return 0;
        }
        if (is_type(parser, TOK_INVALID) || is_type(parser, TOK_NOT)) {
            return parse_io_handlers(parser, enclosing_stop_else);
        }
        return 1;
    }

    if (accept_type(parser, TOK_START)) {
        if (!parse_name(parser)) {
            return 0;
        }
        while (!is_type(parser, TOK_EOF) && !is_type(parser, TOK_PERIOD) &&
               !is_statement_start(parser)) {
            advance(parser);
        }
        if (accept_type(parser, TOK_INVALID)) {
            accept_type(parser, TOK_KEY);
            return parse_statements(parser, 0, 0, 0, 0);
        }
        return 1;
    }

    if (is_type(parser, TOK_ERROR)) {
        report_lexical_error(parser);
        advance(parser);
        return 1;
    }

    report(parser, "instrucao inesperada %s \"%s\"",
           token_type_to_string(parser->current.type),
           parser->current.lexeme != NULL ? parser->current.lexeme : "");
    return 0;
}

static int parse_io_handlers(Parser *parser, int enclosing_stop_else)
{
    while (is_type(parser, TOK_INVALID) || is_type(parser, TOK_NOT)) {
        int not_invalid = accept_type(parser, TOK_NOT);
        if (not_invalid && !expect_type(parser, TOK_INVALID, "INVALID")) {
            return 0;
        }
        if (!not_invalid) {
            advance(parser);
        }
        if (!expect_type(parser, TOK_KEY, "KEY")) {
            return 0;
        }

        while (!is_type(parser, TOK_EOF) && !is_type(parser, TOK_PERIOD) &&
               !is_type(parser, TOK_INVALID) && !is_type(parser, TOK_NOT) &&
               !(enclosing_stop_else && is_type(parser, TOK_ELSE))) {
            if (!is_statement_start(parser) ||
                !parse_statement(parser, enclosing_stop_else)) {
                return 0;
            }
        }
    }
    return 1;
}

static int is_paragraph_header(const Parser *parser)
{
    return is_name(parser) && parser->next.type == TOK_PERIOD;
}

static int parse_statements(Parser *parser, int stop_else,
                            int stop_end_if, int stop_end_search,
                            int stop_end_evaluate)
{
    while (!is_type(parser, TOK_EOF) && !is_type(parser, TOK_PERIOD) &&
           !(stop_else && is_type(parser, TOK_ELSE)) &&
           !(stop_end_if && is_type(parser, TOK_END_IF)) &&
           !(stop_end_search && is_type(parser, TOK_END_SEARCH)) &&
           !(stop_end_evaluate &&
             (is_type(parser, TOK_END_EVALUATE) ||
              is_type(parser, TOK_WHEN))) &&
           !(is_type(parser, TOK_END) && parser->next.type == TOK_PROGRAM)) {
        if (is_type(parser, TOK_ERROR)) {
            report_lexical_error(parser);
            advance(parser);
            continue;
        }

        if (is_paragraph_header(parser)) {
            break;
        }

        if (!is_statement_start(parser)) {
            report(parser, "instrucao inesperada %s \"%s\"",
                   token_type_to_string(parser->current.type),
                   parser->current.lexeme != NULL ? parser->current.lexeme : "");
            return 0;
        }

        if (!parse_statement(parser, stop_else)) {
            return 0;
        }
        accept_type(parser, TOK_PERIOD);
    }
    return 1;
}

static int parse_sentence(Parser *parser, const char *context)
{
    int depth = 0;

    while (!is_type(parser, TOK_EOF)) {
        if (is_type(parser, TOK_PERIOD) && depth == 0) {
            advance(parser);
            return 1;
        }
        if ((is_type(parser, TOK_DATA) || is_type(parser, TOK_PROCEDURE) ||
             is_type(parser, TOK_ENVIRONMENT)) &&
            parser->next.type == TOK_DIVISION) {
            report(parser, "esperado ponto final antes de %s DIVISION", context);
            return 0;
        }
        if (is_type(parser, TOK_ERROR)) {
            report_lexical_error(parser);
        }
        if (is_type(parser, TOK_LPAREN)) {
            depth++;
        } else if (is_type(parser, TOK_RPAREN) && depth > 0) {
            depth--;
        }
        advance(parser);
    }

    report(parser, "fim inesperado durante %s", context);
    return 0;
}

static int parse_division_header(Parser *parser, TokenType name)
{
    if (!expect_type(parser, name, "nome da divisao") ||
        !expect_type(parser, TOK_DIVISION, "DIVISION") ||
        !expect_type(parser, TOK_PERIOD, "\".\"")) {
        return 0;
    }
    return 1;
}

static int parse_identification(Parser *parser)
{
    if (!parse_division_header(parser, TOK_IDENTIFICATION) ||
        !accept_type(parser, TOK_PROGRAM_ID) ||
        !expect_type(parser, TOK_PERIOD, "\".\"") ||
        !parse_name(parser) ||
        !expect_type(parser, TOK_PERIOD, "\".\"")) {
        if (parser->errors == 0) {
            report(parser, "esperado PROGRAM-ID. nome-do-programa.");
        }
        return 0;
    }

    while (!is_type(parser, TOK_EOF) &&
           !is_type(parser, TOK_ENVIRONMENT) &&
           !is_type(parser, TOK_DATA) &&
           !is_type(parser, TOK_PROCEDURE)) {
        if (is_type(parser, TOK_PERIOD)) {
            advance(parser);
        } else if (!parse_sentence(parser, "paragrafo de identificacao")) {
            return 0;
        }
    }
    return 1;
}

static int parse_environment(Parser *parser)
{
    if (!parse_division_header(parser, TOK_ENVIRONMENT)) {
        return 0;
    }

    while (!is_type(parser, TOK_EOF) &&
           !is_type(parser, TOK_DATA) &&
           !is_type(parser, TOK_PROCEDURE)) {
        if (is_type(parser, TOK_PERIOD)) {
            advance(parser);
        } else if (!parse_sentence(parser, "clausula de ENVIRONMENT")) {
            return 0;
        }
    }
    return 1;
}

static int is_data_section(const Parser *parser)
{
    return is_type(parser, TOK_FILE) ||
           is_type(parser, TOK_WORKING_STORAGE) ||
           is_type(parser, TOK_LINKAGE) ||
           word_is(parser, "SCREEN");
}

static int parse_data(Parser *parser)
{
    if (!parse_division_header(parser, TOK_DATA)) {
        return 0;
    }

    while (!is_type(parser, TOK_EOF) && !is_type(parser, TOK_PROCEDURE)) {
        if (is_type(parser, TOK_PERIOD)) {
            advance(parser);
            continue;
        }

        if (is_data_section(parser)) {
            advance(parser);
            if (accept_type(parser, TOK_SECTION) &&
                !expect_type(parser, TOK_PERIOD, "\".\"")) {
                return 0;
            }
            continue;
        }

        if (accept_type(parser, TOK_FD)) {
            if (!parse_name(parser)) {
                return 0;
            }
            if (!accept_type(parser, TOK_PERIOD) &&
                !parse_sentence(parser, "descritor de arquivo")) {
                return 0;
            }
            continue;
        }

        if (is_type(parser, TOK_NUMERIC_LITERAL)) {
            if (!parse_sentence(parser, "declaracao de dados")) {
                return 0;
            }
            continue;
        }

        report(parser, "declaracao de dados inesperada %s \"%s\"",
               token_type_to_string(parser->current.type),
               parser->current.lexeme != NULL ? parser->current.lexeme : "");
        return 0;
    }
    return 1;
}

static int parse_procedure(Parser *parser)
{
    if (!parse_division_header(parser, TOK_PROCEDURE)) {
        return 0;
    }

    while (!is_type(parser, TOK_EOF) && !is_type(parser, TOK_END_PROGRAM) &&
           !(is_type(parser, TOK_END) &&
             parser->next.type == TOK_PROGRAM)) {
        if (is_type(parser, TOK_PERIOD)) {
            advance(parser);
            continue;
        }
        if (is_paragraph_header(parser)) {
            advance(parser);
            advance(parser);
            continue;
        }
        if (!parse_statements(parser, 0, 0, 0, 0)) {
            return 0;
        }
        accept_type(parser, TOK_PERIOD);
    }
    return 1;
}

int parser_parse(FILE *source, FILE *diagnostics)
{
    Parser parser;
    lexer_init(&parser.lexer, source);
    parser.current = lexer_next_token(&parser.lexer);
    parser.next = lexer_next_token(&parser.lexer);
    parser.diagnostics = diagnostics;
    parser.errors = 0;

    if (!parse_identification(&parser)) {
        goto done;
    }
    if (is_type(&parser, TOK_ENVIRONMENT) && !parse_environment(&parser)) {
        goto done;
    }
    if (is_type(&parser, TOK_DATA) && !parse_data(&parser)) {
        goto done;
    }
    if (!is_type(&parser, TOK_PROCEDURE) || !parse_procedure(&parser)) {
        if (parser.errors == 0) {
            report(&parser, "esperado PROCEDURE DIVISION");
        }
        goto done;
    }

    int has_end_program = accept_type(&parser, TOK_END_PROGRAM);
    if (!has_end_program && accept_type(&parser, TOK_END)) {
        if (!expect_type(&parser, TOK_PROGRAM, "PROGRAM apos END")) {
            goto done;
        }
        has_end_program = 1;
    }
    if (has_end_program) {
        if (!parse_name(&parser) ||
            !expect_type(&parser, TOK_PERIOD, "\".\"")) {
            goto done;
        }
    }
    if (!is_type(&parser, TOK_EOF)) {
        report(&parser, "conteudo inesperado apos o fim do programa");
    }

done:
    token_free(&parser.current);
    token_free(&parser.next);
    return parser.errors == 0;
}
