#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <memory>
#include "ast/ast.h"

std::unique_ptr<ExprAST> ParseNumberExpr();
std::unique_ptr<ExprAST> ParseParenExpr();
std::unique_ptr<ExprAST> ParseIdentifierExpr();
std::unique_ptr<ExprAST> ParsePrimary();
std::unique_ptr<ExprAST> ParseExpression();
std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS);
std::unique_ptr<PrototypeAST> ParsePrototype();
std::unique_ptr<FunctionAST> ParseDefinition();
std::unique_ptr<StatementAST> ParseStatement();
std::unique_ptr<DeclarationAST> ParseDeclarations();
std::unique_ptr<BlockAST> ParseBlock();
std::unique_ptr<ProgramAST> ParseProgram();
std::unique_ptr<FunctionAST> ParseTopLevelExpr();

extern std::map<char, int> BinopPrecedence;
int GetTokPrecedence();
void InstantiateBinopPrecendence();

#endif
