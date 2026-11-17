#include "parser.h"

#include <ctype.h>
#include <stdarg.h>
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

static int words_equal(const char *text, const char *word)
{
    if (text == NULL) {
        return 0;
    }

    const unsigned char *left =
        (const unsigned char *)text;
    const unsigned char *right = (const unsigned char *)word;

    while (*left != '\0' && *right != '\0' &&
           toupper(*left) == toupper(*right)) {
        left++;
        right++;
    }

    return *left == '\0' && *right == '\0';
}

static int word_is(const Parser *parser, const char *word)
{
    return words_equal(parser->current.lexeme, word);
}

static int next_word_is(const Parser *parser, const char *word)
{
    return words_equal(parser->next.lexeme, word);
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

static int expect_word(Parser *parser, const char *word)
{
    if (accept_word(parser, word)) {
        return 1;
    }
    report(parser, "esperado %s, encontrado %s \"%s\"",
           word, token_type_to_string(parser->current.type),
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
            return word_is(parser, "EVALUATE") ||
                   word_is(parser, "EXIT") ||
                   word_is(parser, "CONTINUE") ||
                   word_is(parser, "INITIALIZE") ||
                   word_is(parser, "GO") ||
                   word_is(parser, "CALL") ||
                   word_is(parser, "GOBACK");
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
           (word_is(parser, "NOT") &&
            (next_word_is(parser, "EQUAL") ||
             next_word_is(parser, "GREATER") ||
             next_word_is(parser, "LESS") ||
             parser->next.type == TOK_EQUAL)) ||
           word_is(parser, "IS") || word_is(parser, "EQUAL") ||
           word_is(parser, "GREATER") || word_is(parser, "LESS");
}

static int parse_expression(Parser *parser);
static int is_procedure_header(const Parser *parser);

static int parse_data_reference(Parser *parser)
{
    if (is_procedure_header(parser)) {
        report(parser, "esperado operando antes do proximo cabecalho");
        return 0;
    }

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
        if (accept_type(parser, TOK_COLON) &&
            !parse_expression(parser)) {
            return 0;
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
    int leading_relation = word_is(parser, "EQUAL") ||
                           word_is(parser, "GREATER") ||
                           word_is(parser, "LESS") ||
                           word_is(parser, "IS") ||
                           (word_is(parser, "NOT") &&
                            (next_word_is(parser, "EQUAL") ||
                             next_word_is(parser, "GREATER") ||
                             next_word_is(parser, "LESS") ||
                             parser->next.type == TOK_EQUAL));

    if (leading_relation) {
        advance(parser);
        if (word_is(parser, "EQUAL") || word_is(parser, "GREATER") ||
            word_is(parser, "LESS")) {
            advance(parser);
        }
        accept_word(parser, "NOT");
        accept_word(parser, "THAN");
        accept_word(parser, "TO");
        if (!parse_expression(parser)) {
            return 0;
        }
    } else if (accept_type(parser, TOK_LPAREN)) {
        if (!parse_condition(parser, 0) ||
            !expect_type(parser, TOK_RPAREN, "\")\"")) {
            return 0;
        }
    } else {
        if (!parse_expression(parser)) {
            return 0;
        }
    }

    if (is_relation(parser)) {
        if (word_is(parser, "NOT")) {
            advance(parser);
            if (!is_type(parser, TOK_EQUAL) &&
                !word_is(parser, "EQUAL") &&
                !word_is(parser, "GREATER") &&
                !word_is(parser, "LESS")) {
                report(parser, "esperado operador relacional apos NOT");
                return 0;
            }
            advance(parser);
        } else if (word_is(parser, "IS") || word_is(parser, "EQUAL") ||
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
           !is_procedure_header(parser) &&
           !word_is(parser, "END-PERFORM") &&
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
    if (accept_word(parser, "EXIT")) {
        return 1;
    }

    if (accept_word(parser, "GOBACK")) {
        return 1;
    }

    if (accept_word(parser, "GO")) {
        return expect_type(parser, TOK_TO, "TO") && parse_name(parser);
    }

    if (accept_word(parser, "CALL")) {
        if (is_type(parser, TOK_ALPHANUMERIC_LITERAL)) {
            advance(parser);
        } else if (!parse_data_reference(parser)) {
            return 0;
        }
        if (accept_word(parser, "USING")) {
            do {
                if (!parse_primary(parser)) {
                    return 0;
                }
            } while (accept_type(parser, TOK_COMMA));
        }
        return 1;
    }

    if (accept_word(parser, "CONTINUE")) {
        return 1;
    }

    if (accept_word(parser, "INITIALIZE")) {
        if (!parse_data_reference(parser)) {
            return 0;
        }
        while (!is_type(parser, TOK_EOF) && !is_type(parser, TOK_PERIOD) &&
               !is_statement_start(parser)) {
            advance(parser);
        }
        return 1;
    }

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
              word_is(parser, "END-PERFORM") ||
              is_paragraph_header(parser))) {
            report(parser, "esperado END-IF para encerrar IF");
            return 0;
        }
        return 1;
    }

    if (accept_type(parser, TOK_PERFORM)) {
        if (accept_word(parser, "VARYING")) {
            if (!parse_data_reference(parser) ||
                !expect_type(parser, TOK_FROM, "FROM") ||
                !parse_expression(parser) ||
                !expect_type(parser, TOK_BY, "BY") ||
                !parse_expression(parser) ||
                !expect_type(parser, TOK_UNTIL, "UNTIL") ||
                !parse_condition(parser, 1)) {
                return 0;
            }
            while (!is_type(parser, TOK_EOF) &&
                   !is_type(parser, TOK_PERIOD) &&
                   !word_is(parser, "END-PERFORM")) {
                if (!is_statement_start(parser) ||
                    !parse_statement(parser, enclosing_stop_else)) {
                    return 0;
                }
            }
            if (!accept_word(parser, "END-PERFORM")) {
                report(parser, "esperado END-PERFORM para encerrar PERFORM");
                return 0;
            }
            return 1;
        } else if (is_name(parser)) {
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
        if (!parse_primary(parser)) {
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
        if (accept_type(parser, TOK_INTO) &&
            !parse_data_reference(parser)) {
            return 0;
        }
        while (!is_type(parser, TOK_EOF) && !is_type(parser, TOK_PERIOD) &&
               !word_is(parser, "END-READ")) {
            if (accept_type(parser, TOK_AT)) {
                if (!expect_type(parser, TOK_END, "END")) {
                    return 0;
                }
            } else if (accept_type(parser, TOK_NOT)) {
                if (!expect_type(parser, TOK_AT, "AT") ||
                    !expect_type(parser, TOK_END, "END")) {
                    return 0;
                }
            } else if (is_statement_start(parser)) {
                if (!parse_statement(parser, enclosing_stop_else)) {
                    return 0;
                }
            } else {
                report(parser, "clausula inesperada em READ: %s \"%s\"",
                       token_type_to_string(parser->current.type),
                       parser->current.lexeme != NULL
                           ? parser->current.lexeme : "");
                return 0;
            }
        }
        accept_word(parser, "END-READ");
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
    return parser->current.column <= 11 &&
           is_name(parser) && parser->next.type == TOK_PERIOD;
}

static int is_section_header(const Parser *parser)
{
    return parser->current.column <= 11 &&
           is_name(parser) && parser->next.type == TOK_SECTION;
}

static int is_procedure_header(const Parser *parser)
{
    return is_paragraph_header(parser) || is_section_header(parser);
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
           !word_is(parser, "END-PERFORM") &&
           !(is_type(parser, TOK_END) && parser->next.type == TOK_PROGRAM) &&
           !is_procedure_header(parser)) {
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

static int parse_clause_value(Parser *parser)
{
    if (is_name(parser) ||
        is_type(parser, TOK_NUMERIC_LITERAL) ||
        is_type(parser, TOK_ALPHANUMERIC_LITERAL) ||
        is_type(parser, TOK_INDEXED) ||
        is_type(parser, TOK_INPUT) ||
        is_type(parser, TOK_OUTPUT)) {
        advance(parser);
        return 1;
    }

    report(parser, "esperado nome ou literal");
    return 0;
}

static int parse_environment_text(Parser *parser, const char *clause,
                                  int required)
{
    int values = 0;

    while (!is_type(parser, TOK_EOF) && !is_type(parser, TOK_PERIOD)) {
        if (is_name(parser) || is_type(parser, TOK_NUMERIC_LITERAL) ||
            is_type(parser, TOK_ALPHANUMERIC_LITERAL) ||
            is_type(parser, TOK_COMMA) || is_type(parser, TOK_LPAREN) ||
            is_type(parser, TOK_RPAREN)) {
            advance(parser);
            values++;
        } else {
            report(parser, "conteudo inesperado em %s: %s \"%s\"",
                   clause, token_type_to_string(parser->current.type),
                   parser->current.lexeme != NULL ? parser->current.lexeme : "");
            return 0;
        }
    }

    if (required && values == 0) {
        report(parser, "esperado conteudo em %s", clause);
        return 0;
    }
    return expect_type(parser, TOK_PERIOD, "\".\"");
}

static int parse_select_clause(Parser *parser)
{
    if (!expect_type(parser, TOK_SELECT, "SELECT") ||
        !parse_name(parser) ||
        !expect_type(parser, TOK_ASSIGN, "ASSIGN") ||
        !expect_type(parser, TOK_TO, "TO") ||
        !parse_clause_value(parser)) {
        return 0;
    }

    while (!is_type(parser, TOK_PERIOD) && !is_type(parser, TOK_EOF)) {
        if (accept_type(parser, TOK_FILE)) {
            if (!expect_type(parser, TOK_STATUS, "STATUS") ||
                !parse_name(parser)) {
                return 0;
            }
        } else if (accept_type(parser, TOK_STATUS)) {
            if (!parse_name(parser)) {
                return 0;
            }
        } else if (accept_type(parser, TOK_ORGANIZATION)) {
            accept_word(parser, "IS");
            if (accept_word(parser, "LINE")) {
                if (!expect_word(parser, "SEQUENTIAL")) {
                    return 0;
                }
            } else if (!parse_clause_value(parser)) {
                return 0;
            }
        } else if (accept_type(parser, TOK_ACCESS)) {
            accept_type(parser, TOK_MODE);
            accept_word(parser, "IS");
            if (!parse_clause_value(parser)) {
                return 0;
            }
        } else if (accept_type(parser, TOK_RECORD)) {
            if (!expect_type(parser, TOK_KEY, "KEY")) {
                return 0;
            }
            accept_word(parser, "IS");
            if (!parse_name(parser)) {
                return 0;
            }
        } else if (accept_word(parser, "ALTERNATE")) {
            if (!expect_type(parser, TOK_RECORD, "RECORD") ||
                !expect_type(parser, TOK_KEY, "KEY")) {
                return 0;
            }
            accept_word(parser, "IS");
            if (!parse_name(parser)) {
                return 0;
            }
        } else if (accept_word(parser, "RELATIVE")) {
            if (!expect_type(parser, TOK_KEY, "KEY")) {
                return 0;
            }
            accept_word(parser, "IS");
            if (!parse_name(parser)) {
                return 0;
            }
        } else if (!accept_word(parser, "OPTIONAL")) {
            report(parser, "clausula inesperada em SELECT: %s \"%s\"",
                   token_type_to_string(parser->current.type),
                   parser->current.lexeme != NULL ? parser->current.lexeme : "");
            return 0;
        }
    }

    return expect_type(parser, TOK_PERIOD, "\".\"");
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

    int configuration_seen = 0;
    int input_output_seen = 0;
    int file_control_seen = 0;

    while (!is_type(parser, TOK_EOF) &&
           !is_type(parser, TOK_DATA) &&
           !is_type(parser, TOK_PROCEDURE)) {
        if (accept_type(parser, TOK_CONFIGURATION)) {
            if (configuration_seen || input_output_seen ||
                !expect_type(parser, TOK_SECTION, "SECTION") ||
                !expect_type(parser, TOK_PERIOD, "\".\"")) {
                if (configuration_seen || input_output_seen) {
                    report(parser, "CONFIGURATION SECTION fora de ordem ou duplicada");
                }
                return 0;
            }
            configuration_seen = 1;
        } else if (accept_type(parser, TOK_INPUT_OUTPUT)) {
            if (input_output_seen ||
                !expect_type(parser, TOK_SECTION, "SECTION") ||
                !expect_type(parser, TOK_PERIOD, "\".\"")) {
                if (input_output_seen) {
                    report(parser, "INPUT-OUTPUT SECTION duplicada");
                }
                return 0;
            }
            input_output_seen = 1;
        } else if (accept_type(parser, TOK_FILE_CONTROL)) {
            if (!input_output_seen || file_control_seen ||
                !expect_type(parser, TOK_PERIOD, "\".\"")) {
                report(parser, "FILE-CONTROL deve aparecer uma vez em INPUT-OUTPUT SECTION");
                return 0;
            }
            file_control_seen = 1;
            if (!is_type(parser, TOK_SELECT)) {
                report(parser, "esperado ao menos um SELECT em FILE-CONTROL");
                return 0;
            }
            do {
                if (!parse_select_clause(parser)) {
                    return 0;
                }
            } while (is_type(parser, TOK_SELECT));
        } else if (word_is(parser, "SOURCE-COMPUTER") ||
                   word_is(parser, "OBJECT-COMPUTER") ||
                   word_is(parser, "SPECIAL-NAMES") ||
                   word_is(parser, "I-O-CONTROL")) {
            char clause[32];
            snprintf(clause, sizeof(clause), "%s", parser->current.lexeme);
            int is_io_control = word_is(parser, "I-O-CONTROL");
            int is_special_names = word_is(parser, "SPECIAL-NAMES");
            if ((is_io_control && !input_output_seen) ||
                (!is_io_control &&
                 (!configuration_seen || input_output_seen))) {
                report(parser, "%s fora da secao correspondente", clause);
                return 0;
            }
            int requires_value = !is_special_names && !is_io_control;
            advance(parser);
            if (!expect_type(parser, TOK_PERIOD, "\".\"") ||
                !parse_environment_text(parser, clause, requires_value)) {
                return 0;
            }
        } else {
            report(parser, "clausula inesperada em ENVIRONMENT DIVISION: %s \"%s\"",
                   token_type_to_string(parser->current.type),
                   parser->current.lexeme != NULL ? parser->current.lexeme : "");
            return 0;
        }
    }
    return 1;
}

static int parse_unsigned_integer(Parser *parser, int *value)
{
    if (!is_type(parser, TOK_NUMERIC_LITERAL) ||
        parser->current.lexeme == NULL ||
        parser->current.lexeme[0] == '\0') {
        report(parser, "esperado literal inteiro positivo");
        return 0;
    }

    int result = 0;
    for (const unsigned char *c =
             (const unsigned char *)parser->current.lexeme;
         *c != '\0'; c++) {
        if (!isdigit(*c) || result > (2147483647 - (*c - '0')) / 10) {
            report(parser, "literal inteiro invalido");
            return 0;
        }
        result = result * 10 + (*c - '0');
    }

    if (result == 0) {
        report(parser, "literal inteiro deve ser maior que zero");
        return 0;
    }
    *value = result;
    advance(parser);
    return 1;
}

static int is_picture_symbol(int c)
{
    return c == '9' || c == 'X' || c == 'A' || c == 'Z' ||
           c == 'S' || c == 'V' || c == 'B' || c == '+' ||
           c == '-' || c == '/' || c == '*' || c == ',' || c == '.';
}

static void append_picture_char(int c, int *symbols, int *data_symbols,
                                int *seen_sign, int *seen_decimal,
                                int *digits_before_decimal,
                                int *digits_after_decimal, int *invalid)
{
    c = toupper((unsigned char)c);
    if (!is_picture_symbol(c)) {
        *invalid = 1;
        return;
    }

    if (c == 'S') {
        if (*symbols != 0 || *seen_sign) {
            *invalid = 1;
        }
        *seen_sign = 1;
    } else if (c == 'V') {
        if (*symbols == 0 || *seen_decimal) {
            *invalid = 1;
        }
        *seen_decimal = 1;
    } else if (c == '9' || c == 'X' || c == 'A' || c == 'Z') {
        (*data_symbols)++;
        if (c == '9') {
            if (*seen_decimal) {
                (*digits_after_decimal)++;
            } else {
                (*digits_before_decimal)++;
            }
        }
    }
    (*symbols)++;
}

static int parse_picture(Parser *parser)
{
    int symbols = 0;
    int data_symbols = 0;
    int seen_sign = 0;
    int seen_decimal = 0;
    int digits_before_decimal = 0;
    int digits_after_decimal = 0;
    int invalid = 0;

    while (!is_type(parser, TOK_EOF)) {
        if (is_type(parser, TOK_IDENTIFIER) ||
            is_type(parser, TOK_NUMERIC_LITERAL)) {
            const unsigned char *text =
                (const unsigned char *)parser->current.lexeme;
            while (text != NULL && *text != '\0') {
                append_picture_char(*text, &symbols, &data_symbols,
                                    &seen_sign, &seen_decimal,
                                    &digits_before_decimal,
                                    &digits_after_decimal, &invalid);
                text++;
            }
            advance(parser);

            if (accept_type(parser, TOK_LPAREN)) {
                int repetitions;
                if (!parse_unsigned_integer(parser, &repetitions) ||
                    !expect_type(parser, TOK_RPAREN, "\")\"")) {
                    return 0;
                }
            }
        } else if (is_type(parser, TOK_MINUS) ||
                   is_type(parser, TOK_PLUS) ||
                   is_type(parser, TOK_SLASH) ||
                   is_type(parser, TOK_ASTERISK) ||
                   is_type(parser, TOK_COMMA)) {
            int symbol = parser->current.type == TOK_MINUS ? '-' :
                         parser->current.type == TOK_PLUS ? '+' :
                         parser->current.type == TOK_SLASH ? '/' :
                         parser->current.type == TOK_ASTERISK ? '*' : ',';
            append_picture_char(symbol, &symbols, &data_symbols, &seen_sign,
                                &seen_decimal, &digits_before_decimal,
                                &digits_after_decimal, &invalid);
            advance(parser);
        } else if (is_type(parser, TOK_PERIOD) &&
                   (parser->next.type == TOK_NUMERIC_LITERAL ||
                    parser->next.type == TOK_IDENTIFIER) &&
                   parser->next.line == parser->current.line &&
                   parser->next.column == parser->current.column + 1) {
            symbols++;
            advance(parser);
        } else {
            break;
        }
    }

    if (symbols == 0 || data_symbols == 0 ||
        (seen_decimal &&
         (digits_before_decimal == 0 || digits_after_decimal == 0)) ||
        invalid) {
        report(parser, "cadeia PIC invalida");
        return 0;
    }
    return 1;
}

typedef struct {
    int level;
    int has_picture;
    int has_children;
} DataEntry;

static int finish_data_entry(Parser *parser, const DataEntry *entry)
{
    if (entry->has_picture && entry->has_children) {
        report(parser, "item de nivel %02d nao pode ter PIC e subcampos",
               entry->level);
        return 0;
    }
    if (!entry->has_picture && !entry->has_children) {
        report(parser, "item elementar de nivel %02d exige PIC",
               entry->level);
        return 0;
    }
    if (entry->level == 77 && entry->has_children) {
        report(parser, "item de nivel 77 nao pode ter subcampos");
        return 0;
    }
    return 1;
}

static int parse_data_entry(Parser *parser, DataEntry *entry)
{
    char level_text[16];
    if (!is_type(parser, TOK_NUMERIC_LITERAL) ||
        parser->current.lexeme == NULL ||
        strlen(parser->current.lexeme) >= sizeof(level_text)) {
        report(parser, "esperado numero de nivel");
        return 0;
    }
    memcpy(level_text, parser->current.lexeme,
           strlen(parser->current.lexeme) + 1);

    int level;
    if (!parse_unsigned_integer(parser, &level)) {
        return 0;
    }
    if (strlen(level_text) != 2) {
        report(parser, "numero de nivel deve conter dois digitos");
        return 0;
    }
    if (!((level >= 1 && level <= 49) || level == 77 || level == 88)) {
        report(parser, "nivel de dados invalido: %d (use 01-49, 77 ou 88)",
               level);
        return 0;
    }
    if (level == 1 && strcmp(level_text, "01") != 0) {
        report(parser, "nivel raiz deve ser escrito como 01");
        return 0;
    }
    if (level == 77 && strcmp(level_text, "77") != 0) {
        report(parser, "nivel independente deve ser 77");
        return 0;
    }
    if (level == 88 && strcmp(level_text, "88") != 0) {
        report(parser, "nivel de condicao deve ser 88");
        return 0;
    }
    entry->level = level;
    entry->has_picture = 0;
    entry->has_children = 0;

    if (!parse_name(parser)) {
        return 0;
    }

    int has_value = 0;
    int has_occurs = 0;
    while (!is_type(parser, TOK_PERIOD) && !is_type(parser, TOK_EOF)) {
        if (accept_type(parser, TOK_PIC)) {
            if (entry->has_picture) {
                report(parser, "PIC duplicado na declaracao");
                return 0;
            }
            accept_word(parser, "IS");
            if (!parse_picture(parser)) {
                return 0;
            }
            entry->has_picture = 1;
        } else if (accept_type(parser, TOK_VALUE) ||
                   accept_type(parser, TOK_VALUES)) {
            accept_word(parser, "IS");
            if (has_value ||
                (!is_type(parser, TOK_NUMERIC_LITERAL) &&
                 !is_type(parser, TOK_ALPHANUMERIC_LITERAL) &&
                 !is_type(parser, TOK_ZERO) &&
                 !is_type(parser, TOK_ZEROS) &&
                 !is_type(parser, TOK_SPACES) &&
                 !is_type(parser, TOK_LOW_VALUES))) {
                report(parser, "literal VALUE invalido ou clausula duplicada");
                return 0;
            }
            has_value = 1;
            advance(parser);
        } else if (accept_type(parser, TOK_OCCURS)) {
            int occurrences;
            if (has_occurs ||
                !parse_unsigned_integer(parser, &occurrences) ||
                !expect_type(parser, TOK_TIMES, "TIMES")) {
                return 0;
            }
            has_occurs = 1;
            if (accept_type(parser, TOK_INDEXED)) {
                if (!expect_type(parser, TOK_BY, "BY") ||
                    !parse_name(parser)) {
                    return 0;
                }
            }
        } else if (accept_word(parser, "REDEFINES")) {
            if (!parse_name(parser)) {
                return 0;
            }
        } else if (accept_word(parser, "COMP")) {
            continue;
        } else {
            report(parser, "clausula inesperada em declaracao de nivel %02d: %s \"%s\"",
                   level, token_type_to_string(parser->current.type),
                   parser->current.lexeme != NULL ? parser->current.lexeme : "");
            return 0;
        }
    }

    if (!expect_type(parser, TOK_PERIOD, "\".\"")) {
        return 0;
    }

    if (level == 88) {
        if (!has_value) {
            report(parser, "item de nivel 88 exige VALUE ou VALUES");
            return 0;
        }
        return 1;
    }
    if (level == 77 && !entry->has_picture) {
        report(parser, "item de nivel 77 exige PIC");
        return 0;
    }
    if (has_value && !entry->has_picture && level != 1) {
        report(parser, "clausula VALUE exige item elementar");
        return 0;
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

static int parse_file_description(Parser *parser)
{
    if (!expect_type(parser, TOK_FD, "FD") || !parse_name(parser)) {
        return 0;
    }

    if (accept_type(parser, TOK_PERIOD)) {
        return 1;
    }

    int block_clause_seen = 0;
    int record_clause_seen = 0;
    while (!is_type(parser, TOK_EOF) && !is_type(parser, TOK_PERIOD)) {
        if (accept_word(parser, "BLOCK")) {
            int count;
            if (block_clause_seen ||
                !expect_word(parser, "CONTAINS") ||
                !parse_unsigned_integer(parser, &count) ||
                !expect_type(parser, TOK_RECORDS, "RECORDS")) {
                return 0;
            }
            block_clause_seen = 1;
        } else if (accept_type(parser, TOK_RECORD)) {
            int count;
            if (record_clause_seen ||
                !expect_word(parser, "CONTAINS") ||
                !parse_unsigned_integer(parser, &count) ||
                !expect_word(parser, "CHARACTERS")) {
                return 0;
            }
            record_clause_seen = 1;
        } else {
            report(parser, "clausula inesperada em FD: %s \"%s\"",
                   token_type_to_string(parser->current.type),
                   parser->current.lexeme != NULL ? parser->current.lexeme : "");
            return 0;
        }
    }
    return expect_type(parser, TOK_PERIOD, "\".\"");
}

static int parse_data_section_header(Parser *parser, int *section)
{
    if (accept_type(parser, TOK_FILE)) {
        *section = 1;
    } else if (accept_type(parser, TOK_WORKING_STORAGE)) {
        *section = 2;
    } else if (accept_type(parser, TOK_LINKAGE)) {
        *section = 3;
    } else if (accept_word(parser, "SCREEN")) {
        *section = 4;
    } else {
        return 0;
    }

    return expect_type(parser, TOK_SECTION, "SECTION") &&
           expect_type(parser, TOK_PERIOD, "\".\"");
}

static int finish_data_stack(Parser *parser, DataEntry *entries, int *depth)
{
    while (*depth > 0) {
        if (!finish_data_entry(parser, &entries[*depth - 1])) {
            return 0;
        }
        (*depth)--;
    }
    return 1;
}

static int parse_data(Parser *parser)
{
    if (!parse_division_header(parser, TOK_DATA)) {
        return 0;
    }

    DataEntry entries[50];
    int depth = 0;
    int section = 0;
    int previous_entry = 0;

    while (!is_type(parser, TOK_EOF) && !is_type(parser, TOK_PROCEDURE)) {
        if (is_data_section(parser)) {
            if (!finish_data_stack(parser, entries, &depth) ||
                !parse_data_section_header(parser, &section)) {
                return 0;
            }
            previous_entry = 0;
            continue;
        }

        if (is_type(parser, TOK_FD)) {
            if (!finish_data_stack(parser, entries, &depth) || section != 1 ||
                !parse_file_description(parser)) {
                if (section != 1) {
                    report(parser, "FD so e permitido em FILE SECTION");
                }
                return 0;
            }
            previous_entry = 0;
            continue;
        }

        if (accept_word(parser, "COPY")) {
            if (!parse_name(parser) ||
                !expect_type(parser, TOK_PERIOD, "\".\"")) {
                return 0;
            }
            continue;
        }

        if (is_type(parser, TOK_NUMERIC_LITERAL)) {
            DataEntry entry;
            if (!parse_data_entry(parser, &entry)) {
                return 0;
            }

            if (entry.level == 88) {
                if (!previous_entry || depth == 0 ||
                    !entries[depth - 1].has_picture) {
                    report(parser, "item de nivel 88 deve seguir um item de dados");
                    return 0;
                }
                continue;
            }

            while (depth > 0 &&
                   entries[depth - 1].level >= entry.level) {
                if (!finish_data_entry(parser, &entries[depth - 1])) {
                    return 0;
                }
                depth--;
            }

            if (entry.level >= 2 && entry.level <= 49) {
                if (depth == 0 || entries[depth - 1].level == 77) {
                    report(parser, "nivel %02d exige um item de grupo anterior",
                           entry.level);
                    return 0;
                }
                if (entries[depth - 1].has_picture) {
                    report(parser, "item de nivel %02d nao pode conter subcampos",
                           entries[depth - 1].level);
                    return 0;
                }
                entries[depth - 1].has_children = 1;
            }
            if (depth == (int)(sizeof(entries) / sizeof(entries[0]))) {
                report(parser, "hierarquia de dados excede o limite suportado");
                return 0;
            }
            entries[depth++] = entry;
            previous_entry = 1;
            continue;
        }

        report(parser, "declaracao de dados inesperada %s \"%s\"",
               token_type_to_string(parser->current.type),
               parser->current.lexeme != NULL ? parser->current.lexeme : "");
        return 0;
    }
    return finish_data_stack(parser, entries, &depth);
}

static int parse_procedure(Parser *parser)
{
    if (!expect_type(parser, TOK_PROCEDURE, "PROCEDURE") ||
        !expect_type(parser, TOK_DIVISION, "DIVISION")) {
        return 0;
    }

    if (accept_word(parser, "USING")) {
        do {
            if (accept_type(parser, TOK_BY) &&
                !accept_word(parser, "REFERENCE") &&
                !accept_word(parser, "CONTENT") &&
                !accept_word(parser, "VALUE")) {
                report(parser, "esperado REFERENCE, CONTENT ou VALUE apos BY");
                return 0;
            }
            if (!parse_data_reference(parser)) {
                return 0;
            }
        } while (accept_type(parser, TOK_COMMA));
    }

    if (!expect_type(parser, TOK_PERIOD, "\".\"")) {
        return 0;
    }

    while (!is_type(parser, TOK_EOF) && !is_type(parser, TOK_END_PROGRAM) &&
           !(is_type(parser, TOK_END) &&
             parser->next.type == TOK_PROGRAM)) {
        if (is_type(parser, TOK_PERIOD)) {
            report(parser, "frase vazia em PROCEDURE DIVISION");
            return 0;
        }
        if (is_section_header(parser)) {
            advance(parser);
            advance(parser);
            if (!expect_type(parser, TOK_PERIOD, "\".\"")) {
                return 0;
            }
            continue;
        }
        if (is_paragraph_header(parser)) {
            advance(parser);
            if (!expect_type(parser, TOK_PERIOD, "\".\"")) {
                return 0;
            }
            continue;
        }
        if (!parse_statements(parser, 0, 0, 0, 0)) {
            return 0;
        }
        if (!expect_type(parser, TOK_PERIOD, "ponto final para encerrar a frase")) {
            return 0;
        }
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
