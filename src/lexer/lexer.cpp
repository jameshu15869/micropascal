#include "lexer.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

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

        return tok_identifier;
    }

    if (std::isdigit(LastChar) || LastChar == '.') {
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
        } while (LastChar != EOF && LastChar != '\n' &&
                 LastChar != '\r');

        if (LastChar != EOF) {
            return gettok();
        }
    }

    int ThisChar = LastChar;
    LastChar = getchar();  // We must go to the next token
    return ThisChar;
}
