#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast/ast.h"

class CodeGen {
   public:
    void Compile(std::unique_ptr<AST>);
};

#endif