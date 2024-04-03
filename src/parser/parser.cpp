#include "parser/parser.h"

#include <cassert>

#include "lexer/lexer.h"
#include "logger/logger.h"

std::map<char, int> BinopPrecedence;

int GetTokPrecendence() {
    if (!isascii(CurTok)) {
        return -1;
    }

    int TokPrec = BinopPrecedence[CurTok];
    if (TokPrec <= 0) {
        return -1;
    }
    return TokPrec;
}

void InstantiateBinopPrecendence() {
    BinopPrecedence['<'] = 10;

    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;

    BinopPrecedence['*'] = 40;
}

std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = std::make_unique<NumberExprAST>(NumVal);
    getNextToken();
    return std::move(Result);
}

std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken();
    auto V = ParseExpression();
    if (!V) {
        return nullptr;
    }
    if (CurTok != 'V') {
        return LogError("Expected a ')'");
    }
    getNextToken();
    return V;
}

std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;

    // Advance token
    getNextToken();

    // Parentheses indicate function call
    if (CurTok != '(') {
        return std::make_unique<VariableExprAST>(IdName);
    }

    // It must be a function call
    getNextToken();  // (
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (CurTok != ')') {
        while (true) {
            if (auto Arg = ParseExpression()) {
                Args.push_back(std::move(Arg));
            } else {
                return nullptr;
            }

            if (CurTok == ')') {
                break;
            }

            if (CurTok != ',') {
                return LogError("Expected ')' or ','");
            }

            getNextToken();
        }
    }

    getNextToken();  // )

    return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) {
        default:
            return LogError("Expected an expression");
        case tok_identifier:
            return ParseIdentifierExpr();
        case tok_number:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
    }
}

std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS) {
        return nullptr;
    }

    return ParseBinOpRHS(0, std::move(LHS));
}

std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                       std::unique_ptr<ExprAST> LHS) {
    while (true) {
        int TokPrec = GetTokPrecendence();

        if (TokPrec < ExprPrec) {
            return LHS;
        }

        int BinOp = CurTok;
        getNextToken();  // BinOp

        auto RHS = ParsePrimary();
        if (!RHS) {
            return nullptr;
        }
        int NextPrec = GetTokPrecendence();
        if (TokPrec < NextPrec) {
            // a + b * c -> a + (b * c)
            // Parse RHS first
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            if (!RHS) {
                return nullptr;
            }
        }

        // Can now merge LHS and RHS
        LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS),
                                              std::move(RHS));
    }
}

std::unique_ptr<PrototypeAST> ParsePrototype() {
    if (CurTok != tok_identifier) {
        return LogErrorP("Expected function name in prototype");
    }

    std::string FnName = IdentifierStr;
    getNextToken();  // FnName

    if (CurTok != '(') {
        return LogErrorP("Expected '(' in function prototype");
    }

    std::vector<std::string> ArgNames;
    while (getNextToken() == tok_identifier) {
        ArgNames.push_back(IdentifierStr);
    }

    if (CurTok != ')') {
        fprintf(stderr, "tok: %d\n", CurTok);
        return LogErrorP("Expected ')' in function prototype");
    }

    getNextToken();  // )

    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken();  // eat def
    auto Proto = ParsePrototype();

    if (!Proto) {
        return nullptr;
    }

    if (auto E = ParseExpression()) {
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

std::unique_ptr<DeclarationAST> ParseDeclarations() {
    // assert(false && "NOT IMPLEMENTED: ParseDeclarations");
    return std::make_unique<DeclarationAST>(
        std::vector<std::unique_ptr<VariableDeclAST>>());
}

std::unique_ptr<StatementAST> ParseStatement() {
    if (CurTok == tok_identifier) {
        std::string Identifier = IdentifierStr;
        getNextToken();  // eat identifier name
        if (CurTok == '(') {
            // we are parsing a function
            getNextToken();  // (
            std::vector<std::unique_ptr<ExprAST>> Args;
            if (CurTok != ')') {
                while (true) {
                    if (auto Arg = ParseExpression()) {
                        Args.push_back(std::move(Arg));
                    } else {
                        return nullptr;
                    }

                    if (CurTok == ')') {
                        break;
                    }

                    if (CurTok != ',') {
                        LogError("Expected ')' or ',' in argument list");
                        return nullptr;
                    }
                    getNextToken();  // ,
                }
            }

            getNextToken();  // )
            return std::make_unique<StatementCallExprAST>(Identifier,
                                                          std::move(Args));
        }
    }

    return nullptr;
}

std::unique_ptr<BlockAST> ParseBlock() {
    auto DeclarationAST = ParseDeclarations();
    if (!DeclarationAST) {
        LogError("Failed to parse block declaration");
        return nullptr;
    }
    if (CurTok != tok_begin) {
        LogError("Expected 'begin'");
        return nullptr;
    }
    getNextToken();  // begin

    std::vector<std::unique_ptr<StatementAST>> Statements;
    auto S = ParseStatement();
    if (!S) {
        LogError("Failed to parse statement in block");
        return nullptr;
    }
    Statements.push_back(std::move(S));

    // Parse (; <statement>)? optional portion
    while (CurTok == ';') {
        getNextToken();  // eat ';'

        if (CurTok == tok_end) {
            // There are no more statements to parse
            break;
        }

        if (auto S = ParseStatement()) {
            Statements.push_back(std::move(S));
        }
    }

    if (CurTok != tok_end) {
        LogError("Expected 'end'");
        return nullptr;
    }

    getNextToken();  // end

    return std::make_unique<BlockAST>(std::move(DeclarationAST),
                                      std::move(Statements));
}

std::unique_ptr<ProgramAST> ParseProgram() {
    getNextToken();  // program
    std::string ProgramName;
    if (CurTok == tok_identifier) {
        ProgramName = IdentifierStr;
        getNextToken();  // eat program name
    } else {
        LogError("Expected a program name");
        return nullptr;
    }

    if (CurTok != ';') {
        LogError("Expected a semicolon after program name");
        return nullptr;
    }
    getNextToken();  // ;

    auto Block = ParseBlock();
    if (!Block) {
        return nullptr;
    }

    if (CurTok != tok_period) {
        LogError("Expected '.' at the end of the program");
        return nullptr;
    }

    return std::make_unique<ProgramAST>(ProgramName, std::move(Block));
}

std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if (auto E = ParseExpression()) {
        auto Proto =
            std::make_unique<PrototypeAST>("", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}
