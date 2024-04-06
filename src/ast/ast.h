#ifndef AST_H
#define AST_H

#include <iostream>
#include <memory>
#include <string>
#include <vector>

enum VarType {
    TYPE_INTEGER,
    TYPE_BOOLEAN,
};

class AST {
   public:
    virtual ~AST() = default;
    virtual void PrintAST(int NumIndents) {};
    void PrintIndents(int NumIndents) {
        std::cerr << std::string(NumIndents, ' ');
    }
};

class ExprAST : public AST {
   public:
    virtual ~ExprAST() = default;
};

class NumberExprAST : public ExprAST {
    double Val;

   public:
    NumberExprAST(double Val) : Val(Val) {}
    void PrintAST(int NumIndents) {
        PrintIndents(NumIndents);
        std::cerr << Val << '\n';
    }
};

/**
 * should only be true or false
 */
class ConcreteBoolExprAST : public ExprAST {
    bool Val;

   public:
    ConcreteBoolExprAST(bool Val) : Val(Val) {}
    void PrintAST(int NumIndents) {
        PrintIndents(NumIndents);
        std::cerr << Val << '\n';
    }
};

class VariableExprAST : public ExprAST {
    std::string Name;

   public:
    VariableExprAST(const std::string &Name) : Name(Name) {}
    void PrintAST(int NumIndents) {
        PrintIndents(NumIndents);
        std::cerr << Name << '\n';
    }
};

class BinaryExprAST : public ExprAST {
    char Op;

    std::unique_ptr<ExprAST> LHS, RHS;

   public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
    void PrintAST(int NumIndents) {
        PrintIndents(NumIndents);
        std::cerr << Op << '\n';
        LHS->PrintAST(NumIndents + 1);
        RHS->PrintAST(NumIndents + 1);
    }
};

class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

   public:
    CallExprAST(std::string &Callee, std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}
    void PrintAST(int NumIndents) {
        PrintIndents(NumIndents);
        std::cerr << "Called: " << Callee << '\n';
        for (auto &Arg : Args) {
            Arg->PrintAST(NumIndents + 1);
        }
    }
};

class StatementAST : public AST {};

/**
Represents a void call at the top level of a statement
*/
class StatementCallExprAST : public StatementAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

   public:
    StatementCallExprAST(std::string &Callee,
                         std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}
    void PrintAST(int NumIndents) {
        PrintIndents(NumIndents);
        std::cerr << "Statement Call: " << Callee << "\n";
        for (auto &Arg : Args) {
            Arg->PrintAST(NumIndents + 1);
        }
    }
};

class VariableAssignmentAST : public StatementAST {
    std::string VarName;
    std::unique_ptr<ExprAST> Value;

   public:
    VariableAssignmentAST(std::string &VarName, std::unique_ptr<ExprAST> Value)
        : VarName(VarName), Value(std::move(Value)) {}

    void PrintAST(int NumIndents) {
        PrintIndents(NumIndents);
        std::cerr << "Assignment: " << VarName << '\n';
        Value->PrintAST(NumIndents + 1);
        PrintIndents(NumIndents);
        std::cerr << "End Assignment: " << VarName << '\n';
    }
};

class PrototypeAST : public AST {
    std::string Name;
    std::vector<std::string> Args;

   public:
    PrototypeAST(const std::string &Name, std::vector<std::string> Args)
        : Name(Name), Args(std::move(Args)) {}

    const std::string &getName() const { return Name; }

    void PrintAST(int NumIndents) {
        PrintIndents(NumIndents);
        std::cerr << "Fn: " << Name << " ";
    }
};

class FunctionAST : public AST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

   public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::unique_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}
    void PrintAST(int NumIndents) {
        PrintIndents(NumIndents);
        Proto->PrintAST(NumIndents + 1);
        Body->PrintAST(NumIndents + 1);
    }
};

class VariableDeclAST : public AST {
    std::vector<std::string> VarNames;
    VarType Type;

   public:
    VariableDeclAST(std::vector<std::string> VarNames, VarType Type)
        : VarNames(std::move(VarNames)), Type(Type) {}
    void PrintAST(int NumIndents) {
        PrintIndents(NumIndents);
        std::cerr << "Variable Declaration Block: " << Type << '\n';
        for (auto &Name : VarNames) {
            PrintIndents(NumIndents + 1);
            std::cerr << Name << " " << Type << '\n';
        }
    }
};

class DeclarationAST : public AST {
    std::vector<std::unique_ptr<VariableDeclAST>> VarDeclarations;

   public:
    DeclarationAST(
        std::vector<std::unique_ptr<VariableDeclAST>> VarDeclarations)
        : VarDeclarations(std::move(VarDeclarations)) {}
    void PrintAST(int NumIndents) {
        PrintIndents(NumIndents);
        std::cerr << "Variable declarations:\n";
        for (auto &VarDecl : VarDeclarations) {
            VarDecl->PrintAST(NumIndents + 1);
        }
    }
};

class CompoundStatementAST : public StatementAST {
    std::vector<std::unique_ptr<StatementAST>> Statements;

   public:
    CompoundStatementAST(std::vector<std::unique_ptr<StatementAST>> Statements)
        : Statements(std::move(Statements)) {}

    void PrintAST(int NumIndents) {
        PrintIndents(NumIndents);
        std::cerr << "Statements\n";
        for (auto &Statement : Statements) {
            Statement->PrintAST(NumIndents + 1);
        }
        PrintIndents(NumIndents);
        std::cerr << "End Statements\n";
    }
};

class BlockAST : public AST {
    std::unique_ptr<DeclarationAST> Declaration;
    std::unique_ptr<CompoundStatementAST> CompoundStatement;

   public:
    BlockAST(std::unique_ptr<DeclarationAST> Declaration,
             std::unique_ptr<CompoundStatementAST> CompoundStatement)
        : Declaration(std::move(Declaration)),
          CompoundStatement(std::move(CompoundStatement)) {}
    void PrintAST(int NumIndents) {
        PrintIndents(NumIndents);
        std::cerr << "Block\n";
        Declaration->PrintAST(NumIndents + 1);
        CompoundStatement->PrintAST(NumIndents + 1);
        PrintIndents(NumIndents);
        std::cerr << "End block\n";
    }
};

class ProgramAST : public AST {
    std::string Name;
    std::unique_ptr<BlockAST> Block;

   public:
    ProgramAST(std::string &Name, std::unique_ptr<BlockAST> Block)
        : Name(Name), Block(std::move(Block)) {}
    void PrintAST(int NumIndents) {
        PrintIndents(NumIndents);
        std::cerr << "Program: " << Name << "\n";
        Block->PrintAST(NumIndents + 1);
        PrintIndents(NumIndents);
        std::cerr << "End Program: " << Name << "\n";
    }
};

#endif
