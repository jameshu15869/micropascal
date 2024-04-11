#include <iostream>

#include "codegen/codegen.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

void HandleDefinition() {
    if (ParseDefinition()) {
        fprintf(stderr, "Parsed a function definition.\n");
    } else {
        getNextToken();
    }
}

void HandleProgram() {
    if (auto P = ParseProgram()) {
        fprintf(stderr, "Parsed a program.\n");
        P->PrintAST(0);
        std::cerr << "\n";
        CodeGen CG;
        CG.Compile(std::move(P));
    } else {
        getNextToken();
    }
}

void HandleTopLevelExpression() {
    if (auto E = ParseTopLevelExpr()) {
        E->PrintAST(0);
        std::cerr << "\n";
        fprintf(stderr, "Parsed a top-level expr\n");
    } else {
        getNextToken();
    }
}

void MainLoop() {
    while (true) {
        fprintf(stderr, "ready> ");
        switch (CurTok) {
            case tok_eof:
                return;
            case ';':
            case tok_period:  // top level period
                getNextToken();
                break;
            case tok_def:
                HandleDefinition();
                break;
            case tok_program:
                HandleProgram();
                break;
            default:
                HandleTopLevelExpression();
        }
    }
}

int main() {
    InstantiateBinopPrecendence();

    fprintf(stderr, "ready> ");
    getNextToken();

    MainLoop();

    return 0;
}