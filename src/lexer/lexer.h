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

    // types
    tok_real,
    tok_integer,

    // primary
    tok_identifier,
    tok_number,

    // symbols
    tok_assignment,  // :=
};

extern int CurTok;
extern std::string IdentifierStr;
extern double NumVal;

int gettok();

#endif
