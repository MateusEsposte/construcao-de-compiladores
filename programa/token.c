#include "token.h"
#include <stdlib.h>

const char *token_type_to_string(TokenType type){
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
        case TOK_ADD:                    return "TOK_ADD";
        case TOK_SUBTRACT:               return "TOK_SUBTRACT";
        case TOK_MULTIPLY:               return "TOK_MULTIPLY";
        case TOK_DIVIDE:                 return "TOK_DIVIDE";
        case TOK_COMPUTE:                return "TOK_COMPUTE";
        case TOK_PROGRAM:                return "TOK_PROGRAM";
        case TOK_PROGRAM_ID:             return "TOK_PROGRAM_ID";
        case TOK_FILE:                   return "TOK_FILE";
        case TOK_FILE_CONTROL:           return "TOK_FILE_CONTROL";
        case TOK_SELECT:                 return "TOK_SELECT";
        case TOK_FD:                     return "TOK_FD";
        case TOK_PIC:                    return "TOK_PIC";
        case TOK_VALUE:                  return "TOK_VALUE";
        case TOK_VALUES:                 return "TOK_VALUES";
        case TOK_OCCURS:                 return "TOK_OCCURS";
        case TOK_TIMES:                  return "TOK_TIMES";
        case TOK_INDEXED:                return "TOK_INDEXED";
        case TOK_BY:                     return "TOK_BY";
        case TOK_OF:                     return "TOK_OF";
        case TOK_INTO:                   return "TOK_INTO";
        case TOK_TO:                     return "TOK_TO";
        case TOK_FROM:                   return "TOK_FROM";
        case TOK_GIVING:                 return "TOK_GIVING";
        case TOK_REMAINDER:              return "TOK_REMAINDER";
        case TOK_AND:                    return "TOK_AND";
        case TOK_OR:                     return "TOK_OR";
        case TOK_NOT:                    return "TOK_NOT";
        case TOK_IN:                     return "TOK_IN";
        case TOK_OPEN:                   return "TOK_OPEN";
        case TOK_CLOSE:                  return "TOK_CLOSE";
        case TOK_READ:                   return "TOK_READ";
        case TOK_WRITE:                  return "TOK_WRITE";
        case TOK_RECORD:                 return "TOK_RECORD";
        case TOK_RECORDS:                return "TOK_RECORDS";
        case TOK_ASSIGN:                 return "TOK_ASSIGN";
        case TOK_INPUT:                  return "TOK_INPUT";
        case TOK_OUTPUT:                 return "TOK_OUTPUT";
        case TOK_INPUT_OUTPUT:           return "TOK_INPUT_OUTPUT";
        case TOK_ORGANIZATION:           return "TOK_ORGANIZATION";
        case TOK_ACCESS:                 return "TOK_ACCESS";
        case TOK_MODE:                   return "TOK_MODE";
        case TOK_STATUS:                 return "TOK_STATUS";
        case TOK_START:                  return "TOK_START";
        case TOK_DELETE:                 return "TOK_DELETE";
        case TOK_UPDATE:                 return "TOK_UPDATE";
        case TOK_INVALID:                return "TOK_INVALID";
        case TOK_KEY:                    return "TOK_KEY";
        case TOK_END_IF:                 return "TOK_END_IF";
        case TOK_END_SEARCH:             return "TOK_END_SEARCH";
        case TOK_END_EVALUATE:           return "TOK_END_EVALUATE";
        case TOK_END_PROGRAM:            return "TOK_END_PROGRAM";
        case TOK_SEARCH:                 return "TOK_SEARCH";
        case TOK_SET:                    return "TOK_SET";
        case TOK_WHEN:                   return "TOK_WHEN";
        case TOK_AT:                     return "TOK_AT";
        case TOK_END:                    return "TOK_END";
        case TOK_ON:                     return "TOK_ON";
        case TOK_SIZE:                   return "TOK_SIZE";
        case TOK_ERROR_WORD:             return "TOK_ERROR_WORD";
        case TOK_UNTIL:                  return "TOK_UNTIL";
        case TOK_FUNCTION:               return "TOK_FUNCTION";
        case TOK_UPPER_CASE:             return "TOK_UPPER_CASE";
        case TOK_INSPECT:                return "TOK_INSPECT";
        case TOK_TALLYING:               return "TOK_TALLYING";
        case TOK_FOR:                    return "TOK_FOR";
        case TOK_ALL:                    return "TOK_ALL";
        case TOK_CHARACTERS:             return "TOK_CHARACTERS";
        case TOK_BEFORE:                 return "TOK_BEFORE";
        case TOK_AFTER:                  return "TOK_AFTER";
        case TOK_INITIAL:                return "TOK_INITIAL";
        case TOK_LEADING:                return "TOK_LEADING";
        case TOK_REPLACING:              return "TOK_REPLACING";
        case TOK_FIRST:                  return "TOK_FIRST";
        case TOK_CONVERTING:             return "TOK_CONVERTING";
        case TOK_SPACES:                 return "TOK_SPACES";
        case TOK_ZERO:                   return "TOK_ZERO";
        case TOK_ZEROS:                  return "TOK_ZEROS";
        case TOK_LOW_VALUES:             return "TOK_LOW_VALUES";
        case TOK_CONFIGURATION:          return "TOK_CONFIGURATION";
        case TOK_LINKAGE:                return "TOK_LINKAGE";
        case TOK_WORKING_STORAGE:        return "TOK_WORKING_STORAGE";
        case TOK_KEYWORD:                return "TOK_KEYWORD";

        case TOK_IDENTIFIER:             return "TOK_IDENTIFIER";
        case TOK_NUMERIC_LITERAL:        return "TOK_NUMERIC_LITERAL";
        case TOK_ALPHANUMERIC_LITERAL:   return "TOK_ALPHANUMERIC_LITERAL";

        case TOK_PERIOD:                 return "TOK_PERIOD";
        case TOK_COMMA:                  return "TOK_COMMA";
        case TOK_SEMICOLON:              return "TOK_SEMICOLON";
        case TOK_COLON:                  return "TOK_COLON";
        case TOK_LPAREN:                 return "TOK_LPAREN";
        case TOK_RPAREN:                 return "TOK_RPAREN";
        case TOK_PLUS:                   return "TOK_PLUS";
        case TOK_MINUS:                  return "TOK_MINUS";
        case TOK_ASTERISK:               return "TOK_ASTERISK";
        case TOK_SLASH:                  return "TOK_SLASH";
        case TOK_POWER:                  return "TOK_POWER";
        case TOK_EQUAL:                  return "TOK_EQUAL";
        case TOK_LESS:                   return "TOK_LESS";
        case TOK_GREATER:                return "TOK_GREATER";
        case TOK_LESS_EQUAL:             return "TOK_LESS_EQUAL";
        case TOK_GREATER_EQUAL:          return "TOK_GREATER_EQUAL";
        case TOK_NOT_EQUAL:              return "TOK_NOT_EQUAL";

        case TOK_EOF:                    return "TOK_EOF";
        case TOK_ERROR:                  return "TOK_ERROR";

        default:                         return "TOK_UNKNOWN";
    }
}


void token_free(Token *token){
    free(token->lexeme);
    token->lexeme = NULL;
}
