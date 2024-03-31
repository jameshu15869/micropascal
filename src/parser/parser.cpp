#include "parser/parser.h"

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
    getNextToken(); // FnName
    
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

    getNextToken(); // )

    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

std::unique_ptr<ExprAST> ParseDefinition() {
    getNextToken(); // eat def
    auto Proto = ParsePrototype();

    if (!Proto) {
        return nullptr;
    }

    if (auto E = ParseExpression()) {
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

std::unique_ptr<ExprAST> ParseTopLevelExpr() {
    if (auto E = ParseExpression()) {
        auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}
