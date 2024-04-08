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
    void PrintIndents(int NumIndents);
};

class ExprAST : public AST {
   public:
    virtual ~ExprAST() = default;
};

class NumberExprAST : public ExprAST {
    double Val;

   public:
    NumberExprAST(double Val) : Val(Val) {}
    void PrintAST(int NumIndents) override;
};

/**
 * should only be true or false
 */
class ConcreteBoolExprAST : public ExprAST {
    bool Val;

   public:
    ConcreteBoolExprAST(bool Val) : Val(Val) {}
    void PrintAST(int NumIndents) override;
};

class VariableExprAST : public ExprAST {
    std::string Name;

   public:
    VariableExprAST(const std::string &Name) : Name(Name) {}
    void PrintAST(int NumIndents) override;
};

class BinaryExprAST : public ExprAST {
    char Op;

    std::unique_ptr<ExprAST> LHS, RHS;

   public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
    void PrintAST(int NumIndents) override;
};

class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

   public:
    CallExprAST(std::string &Callee, std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}
    void PrintAST(int NumIndents) override;
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
    void PrintAST(int NumIndents) override;
};

class IfStatementAST : public StatementAST {
    std::unique_ptr<ExprAST> Cond;
    std::unique_ptr<StatementAST> Then, Else;

   public:
    IfStatementAST(std::unique_ptr<ExprAST> Cond,
                   std::unique_ptr<StatementAST> Then,
                   std::unique_ptr<StatementAST> Else)
        : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}
    void PrintAST(int NumIndents) override;
};

class VariableAssignmentAST : public StatementAST {
    std::string VarName;
    std::unique_ptr<ExprAST> Value;

   public:
    VariableAssignmentAST(std::string &VarName, std::unique_ptr<ExprAST> Value)
        : VarName(VarName), Value(std::move(Value)) {}

    void PrintAST(int NumIndents) override;
};

class VariableDeclAST : public AST {
    std::vector<std::string> VarNames;
    VarType Type;

   public:
    VariableDeclAST(std::vector<std::string> VarNames, VarType Type)
        : VarNames(std::move(VarNames)), Type(Type) {}
    void PrintAST(int NumIndents) override;
};

class PrototypeAST : public AST {
    std::string Name;
    std::vector<std::unique_ptr<VariableDeclAST>> Parameters;

   public:
    PrototypeAST(const std::string &Name,
                 std::vector<std::unique_ptr<VariableDeclAST>> Parameters)
        : Name(Name), Parameters(std::move(Parameters)) {}

    const std::string &getName() const { return Name; }

    void PrintAST(int NumIndents) override;
};

class DeclarationAST : public AST {
    std::vector<std::unique_ptr<VariableDeclAST>> VarDeclarations;

   public:
    DeclarationAST(
        std::vector<std::unique_ptr<VariableDeclAST>> VarDeclarations)
        : VarDeclarations(std::move(VarDeclarations)) {}
    void PrintAST(int NumIndents) override;
};

class CompoundStatementAST : public StatementAST {
    std::vector<std::unique_ptr<StatementAST>> Statements;

   public:
    CompoundStatementAST(std::vector<std::unique_ptr<StatementAST>> Statements)
        : Statements(std::move(Statements)) {}

    void PrintAST(int NumIndents) override;
};

class BlockAST : public AST {
    std::unique_ptr<DeclarationAST> Declaration;
    std::unique_ptr<CompoundStatementAST> CompoundStatement;

   public:
    BlockAST(std::unique_ptr<DeclarationAST> Declaration,
             std::unique_ptr<CompoundStatementAST> CompoundStatement)
        : Declaration(std::move(Declaration)),
          CompoundStatement(std::move(CompoundStatement)) {}
    void PrintAST(int NumIndents) override;
};

class FunctionAST : public AST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<BlockAST> Body;

   public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::unique_ptr<BlockAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}
    void PrintAST(int NumIndents) override;
};

class ProgramAST : public AST {
    std::string Name;
    std::vector<std::unique_ptr<FunctionAST>> Functions;
    std::unique_ptr<BlockAST> Block;

   public:
    ProgramAST(std::string &Name,
               std::vector<std::unique_ptr<FunctionAST>> Functions,
               std::unique_ptr<BlockAST> Block)
        : Name(Name),
          Functions(std::move(Functions)),
          Block(std::move(Block)) {}
    void PrintAST(int NumIndents) override;
};
#endif
