#include "lexer/lexer.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

int CurTok;
std::string IdentifierStr;
double NumVal;

int gettok() {
    static int LastChar = ' ';

    while (std::isspace(LastChar)) {
        LastChar = std::getchar();
    }

    if (std::isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while (std::isalnum(LastChar = getchar())) {
            IdentifierStr += LastChar;
        }

        if (IdentifierStr == "def") {
            return tok_def;
        }
        if (IdentifierStr == "var") {
            return tok_var;
        }
        if (IdentifierStr == "const") {
            return tok_const;
        }
        if (IdentifierStr == "begin") {
            return tok_begin;
        }
        if (IdentifierStr == "end") {
            return tok_end;
        }
        if (IdentifierStr == "program") {
            return tok_program;
        }
        if (IdentifierStr == "integer") {
            return tok_integer;
        }
        if (IdentifierStr == "real") {
            return tok_real;
        }
        if (IdentifierStr == "boolean") {
            return tok_boolean;
        }
        if (IdentifierStr == "true") {
            return tok_true;
        }
        if (IdentifierStr == "false") {
            return tok_false;
        }
        if (IdentifierStr == "procedure") {
            return tok_procedure;
        }
        if (IdentifierStr == "if") {
            return tok_if;
        }
        if (IdentifierStr == "then") {
            return tok_then;
        }
        if (IdentifierStr == "else") {
            return tok_else;
        }

        return tok_identifier;
    }

    // Check for lone period, since otherwise it gets parsed as a number
    if (LastChar == '.') {
        LastChar = getchar();  // need to skip this token?
        return tok_period;
    }

    if (std::isdigit(LastChar)) {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = std::getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = std::strtod(NumStr.c_str(), 0);
        return tok_number;
    }

    if (LastChar == '#') {
        do {
            LastChar = std::getchar();
        } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF) {
            return gettok();
        }
    }

    if (LastChar == EOF) {
        return tok_eof;
    }

    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;  // Return as ASCII
}

int getNextToken() { return CurTok = gettok(); }
