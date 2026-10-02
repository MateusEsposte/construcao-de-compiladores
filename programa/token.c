#include "token.h"

const char *token_type_to_string(TokenType type)
{
    switch (type) {
        case TOK_IDENTIFICATION:         return "TOK_IDENTIFICATION";
        case TOK_ENVIRONMENT:            return "TOK_ENVIRONMENT";
        case TOK_DATA:                   return "TOK_DATA";
        case TOK_PROCEDURE:              return "TOK_PROCEDURE";
        case TOK_DIVISION:               return "TOK_DIVISION";
        case TOK_SECTION:                return "TOK_SECTION";
        case TOK_DISPLAY:                return "TOK_DISPLAY";
        case TOK_ACCEPT:                 return "TOK_ACCEPT";
        case TOK_PERFORM:                return "TOK_PERFORM";
        case TOK_STOP:                   return "TOK_STOP";
        case TOK_RUN:                    return "TOK_RUN";
        case TOK_IF:                     return "TOK_IF";
        case TOK_ELSE:                   return "TOK_ELSE";
        case TOK_MOVE:                   return "TOK_MOVE";

        case TOK_IDENTIFIER:             return "TOK_IDENTIFIER";
        case TOK_NUMERIC_LITERAL:        return "TOK_NUMERIC_LITERAL";
        case TOK_ALPHANUMERIC_LITERAL:   return "TOK_ALPHANUMERIC_LITERAL";

        case TOK_PERIOD:                 return "TOK_PERIOD";
        case TOK_COMMA:                  return "TOK_COMMA";
        case TOK_SEMICOLON:              return "TOK_SEMICOLON";
        case TOK_LPAREN:                 return "TOK_LPAREN";
        case TOK_RPAREN:                 return "TOK_RPAREN";

        case TOK_EOF:                    return "TOK_EOF";
        case TOK_ERROR:                  return "TOK_ERROR";

        default:                         return "TOK_UNKNOWN";
    }
}
