#include "lexer/lexer.h"
#include "parser/parser.h"

void HandleDefinition() {
    if (ParseDefinition()) {
        fprintf(stderr, "Parsed a function definition.\n");
    } else {
        getNextToken();
    }
}

void HandleTopLevelExpression() {
    if (ParseTopLevelExpr()) {
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
                getNextToken();
                break;
            case tok_def:
                HandleDefinition();
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