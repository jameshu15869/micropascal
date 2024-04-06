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
    if (CurTok != ')') {
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
        case tok_true:
            getNextToken();  // true
            return std::make_unique<ConcreteBoolExprAST>(true);
        case tok_false:
            getNextToken();  // false
            return std::make_unique<ConcreteBoolExprAST>(false);
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

std::unique_ptr<VariableDeclAST> ParseVariableDecl() {
    std::vector<std::string> VarNames;
    if (CurTok != tok_identifier) {
        LogError("Expected identifier in variable decl");
        return nullptr;
    }
    VarNames.push_back(IdentifierStr);
    getNextToken();  // First identifier
    while (CurTok == ',') {
        getNextToken();  // ,
        if (CurTok != tok_identifier) {
            LogError("Expected identifier in variable decl");
            return nullptr;
        }
        VarNames.push_back(IdentifierStr);
        getNextToken();  // identifier
    }

    if (CurTok != ':') {
        LogError("Expected ':' after variable list in variable decl");
        return nullptr;
    }

    getNextToken();  // :
    VarType Type;
    if (!(CurTok == tok_integer || CurTok == tok_boolean)) {
        LogError("Expected type identifier after variable list");
        return nullptr;
    }

    if (IdentifierStr == "integer") {
        Type = TYPE_INTEGER;
    } else if (IdentifierStr == "boolean") {
        Type = TYPE_BOOLEAN;
    } else {
        LogError("Unknown type identifier");
        return nullptr;
    }
    getNextToken();  // type

    if (CurTok != ';') {
        LogError("Expected ';' after variable decl");
        return nullptr;
    }
    getNextToken();  // ;

    return std::make_unique<VariableDeclAST>(std::move(VarNames), Type);
}

std::unique_ptr<DeclarationAST> ParseDeclarations() {
    // assert(false && "NOT IMPLEMENTED: ParseDeclarations");
    std::vector<std::unique_ptr<VariableDeclAST>> VarDecls;

    while (CurTok == tok_var) {
        getNextToken();  // const | var
        while (CurTok == tok_identifier) {
            if (auto D = ParseVariableDecl()) {
                VarDecls.push_back(std::move(D));
            } else {
                LogError("Failed to parse variable decl");
                return nullptr;
            }
        }
    }

    return std::make_unique<DeclarationAST>(std::move(VarDecls));
}

std::unique_ptr<StatementAST> ParseStatement() {
    if (CurTok == tok_identifier) {
        std::string Identifier = IdentifierStr;
        getNextToken();  // eat identifier name

        if (CurTok != '(') {
            // Must be an assignment
            if (CurTok != ':') {
                LogError("Expected ':' in assignment");
                return nullptr;
            }
            getNextToken();  // :
            if (CurTok != '=') {
                LogError("Expected '=' in assignment");
                return nullptr;
            }
            getNextToken();  // =

            auto E = ParseExpression();
            if (!E) {
                LogError("Error while parsing expression in assignment");
                return nullptr;
            }
            return std::make_unique<VariableAssignmentAST>(Identifier,
                                                           std::move(E));
        }

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

    if (CurTok == tok_begin) {
        // We are parsing a nested begin
        if (auto S = ParseCompoundStatement()) {
            return std::move(S);
        }
    }

    return nullptr;
}

std::unique_ptr<CompoundStatementAST> ParseCompoundStatement() {
    if (CurTok != tok_begin) {
        LogError("Expected 'begin'");
        return nullptr;
    }
    getNextToken();  // begin

    std::vector<std::unique_ptr<StatementAST>> Statements;

    if (CurTok != tok_end) {
        while (true) {
            if (auto S = ParseStatement()) {
                Statements.push_back(std::move(S));
            } else {
                LogError("Error while parsing statements in a block");
                return nullptr;
            }

            if (CurTok == tok_end) {
                break;
            }

            if (CurTok != ';') {
                LogError("Expected ';' after statement in block");
                return nullptr;
            }

            getNextToken();  // ;
            // Check for the case ending on a ';'
            if (CurTok == tok_end) {
                break;
            }
        }
    }

    if (CurTok != tok_end) {
        LogError("Expected 'end'");
        return nullptr;
    }

    getNextToken();  // end
    return std::make_unique<CompoundStatementAST>(std::move(Statements));
}

std::unique_ptr<BlockAST> ParseBlock() {
    auto DeclarationAST = ParseDeclarations();
    if (!DeclarationAST) {
        LogError("Failed to parse block declaration");
        return nullptr;
    }
    auto CompoundStatement = ParseCompoundStatement();
    if (!CompoundStatement) {
        LogError("Failed to parse compound statement in block");
        return nullptr;
    }

    return std::make_unique<BlockAST>(std::move(DeclarationAST),
                                      std::move(CompoundStatement));
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
