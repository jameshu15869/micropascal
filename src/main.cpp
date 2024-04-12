#include <iostream>

#include "codegen/codegen.h"
#include "kaleidoscopejit/KaleidoscopeJIT.h"
#include "lexer/lexer.h"
#include "llvm/Support/TargetSelect.h"
#include "parser/parser.h"

static std::unique_ptr<llvm::orc::KaleidoscopeJIT> TheJIT;
static llvm::ExitOnError ExitOnErr;

#include <inttypes.h>
#include <stdio.h>

extern "C" void writeln(int64_t v) {
    fprintf(stderr, "%" PRIi64, v);
    fprintf(stderr, "\n");
}

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
        std::cerr << "============================   AST  ============================\n";
        P->PrintAST(0);
        std::cerr << "\n";
        CodeGen CG;
        CG.CompileAndRun(std::move(P), *TheJIT);
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
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    InstantiateBinopPrecendence();

    fprintf(stderr, "ready> ");
    getNextToken();

    TheJIT = ExitOnErr(llvm::orc::KaleidoscopeJIT::Create());

    MainLoop();

    return 0;
}