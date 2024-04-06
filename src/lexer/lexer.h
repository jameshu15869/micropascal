#ifndef LEXER_H
#define LEXER_H

#include <climits>
#include <string>

enum Token {
    tok_eof = INT_MIN,

    // keywords
    tok_var,
    tok_begin,
    tok_end,
    tok_program,
    tok_const,
    tok_def,
    tok_true,
    tok_false,
    tok_procedure,

    // types
    tok_real,
    tok_integer,
    tok_boolean,

    // primary
    tok_identifier,
    tok_number,

    // symbols
    tok_period,
};

extern int CurTok;
extern std::string IdentifierStr;
extern double NumVal;

int gettok();

int getNextToken();

#endif
