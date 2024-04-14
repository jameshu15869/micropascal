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

void HandleProgram() {
    if (auto P = ParseProgram()) {
        CodeGen CG;
        CG.CompileAndRun(std::move(P), *TheJIT);
    } else {
        getNextToken();
    }
}

void MainLoop() {
    while (true) {
        switch (CurTok) {
            case tok_eof:
                return;
            case ';':
            case tok_period:  // top level period
                getNextToken();
                break;
            case tok_program:
                HandleProgram();
                break;
            default:
                break;
        }
        fprintf(stderr, "ready> ");
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