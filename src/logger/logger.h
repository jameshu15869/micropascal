#ifndef LOGGER_H
#define LOGGER_H

#include <memory>
#include "ast/ast.h"

std::unique_ptr<ExprAST> LogError(const char *Str);
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str);
#endif
