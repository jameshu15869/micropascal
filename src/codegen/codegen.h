#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast/ast.h"
#include "kaleidoscopejit/KaleidoscopeJIT.h"

class CodeGen {
    std::unique_ptr<llvm::Module> M;

   public:
    void CompileAndRun(std::unique_ptr<AST>, llvm::orc::KaleidoscopeJIT &);
};

#endif