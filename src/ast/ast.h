#ifndef AST_H
#define AST_H

#include <iostream>
#include <memory>
#include <string>
#include <vector>

class AST {
   public:
    virtual ~AST() = default;
    virtual void PrintAST(int numIndents) {};
    void PrintIndents(int numIndents) {
        std::cerr << std::string(numIndents, ' ');
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
    void PrintAST(int numIndents) {
        PrintIndents(numIndents);
        std::cerr << Val << '\n';
    }
};

class VariableExprAST : public ExprAST {
    std::string Name;

   public:
    VariableExprAST(const std::string &Name) : Name(Name) {}
    void PrintAST(int numIndents) {
        PrintIndents(numIndents);
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
    void PrintAST(int numIndents) {
        PrintIndents(numIndents);
        std::cerr << Op << '\n';
        LHS->PrintAST(numIndents + 1);
        RHS->PrintAST(numIndents + 1);
    }
};

class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

   public:
    CallExprAST(std::string &Callee, std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}
    void PrintAST(int numIndents) {
        PrintIndents(numIndents);
        std::cerr << "Called: " << Callee << '\n';
        for (auto &Arg : Args) {
            Arg->PrintAST(numIndents + 1);
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
    void PrintAST(int numIndents) {
        PrintIndents(numIndents);
        std::cerr << "Statement Call: " << Callee << "\n";
        for (auto &Arg : Args) {
            Arg->PrintAST(numIndents + 1);
        }
    }
};

class PrototypeAST : public AST {
    std::string Name;
    std::vector<std::string> Args;

   public:
    PrototypeAST(const std::string &Name, std::vector<std::string> Args)
        : Name(Name), Args(std::move(Args)) {}

    const std::string &getName() const { return Name; }

    void PrintAST(int numIndents) {
        PrintIndents(numIndents);
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
    void PrintAST(int numIndents) {
        PrintIndents(numIndents);
        Proto->PrintAST(numIndents + 1);
        Body->PrintAST(numIndents + 1);
    }
};

class VariableDeclAST : public AST {};

class DeclarationAST : public AST {
    std::vector<std::unique_ptr<VariableDeclAST>> Declarations;

   public:
    DeclarationAST(std::vector<std::unique_ptr<VariableDeclAST>> Declarations)
        : Declarations(std::move(Declarations)) {}
};

class BlockAST : public AST {
    std::unique_ptr<DeclarationAST> Declaration;
    std::vector<std::unique_ptr<StatementAST>> Statements;

   public:
    BlockAST(std::unique_ptr<DeclarationAST> Declaration,
             std::vector<std::unique_ptr<StatementAST>> Statements)
        : Declaration(std::move(Declaration)),
          Statements(std::move(Statements)) {}
    void PrintAST(int numIndents) {
        PrintIndents(numIndents);
        std::cerr << "Block\n";
        Declaration->PrintAST(numIndents + 1);
        for (auto &Statement : Statements) {
            Statement->PrintAST(numIndents + 1);
        }
        std::cerr << "End block\n";
    }
};

class ProgramAST : public AST {
    std::string Name;
    std::unique_ptr<BlockAST> Block;

   public:
    ProgramAST(std::string &Name, std::unique_ptr<BlockAST> Block)
        : Name(Name), Block(std::move(Block)) {}
    void PrintAST(int numIndents) {
        PrintIndents(numIndents);
        std::cerr << "Program: " << Name << "\n";
        Block->PrintAST(numIndents + 1);
        std::cerr << "\n";
    }
};

#endif
