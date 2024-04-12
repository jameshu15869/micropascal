#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <vector>

enum VarType {
    TYPE_INTEGER,
    TYPE_BOOLEAN,
};

class AST;
class ExprAST;
class NumberExprAST;
class ConcreteBoolExprAST;
class VariableExprAST;
class BinaryExprAST;
class CallExprAST;
class StatementCallExprAST;
class IfStatementAST;
class ForStatementAST;
class VariableAssignmentAST;
class VariableDeclAST;
class PrototypeAST;
class DeclarationAST;
class CompoundStatementAST;
class BlockAST;
class FunctionAST;
class ProgramAST;

class ASTVisitor {
   public:
    virtual void Visit(AST &) {};
    virtual void Visit(ExprAST &) {};
    virtual void Visit(NumberExprAST &) = 0;
    virtual void Visit(ConcreteBoolExprAST &) = 0;
    virtual void Visit(VariableExprAST &) = 0;
    virtual void Visit(BinaryExprAST &) = 0;
    virtual void Visit(CallExprAST &) = 0;
    virtual void Visit(StatementCallExprAST &) = 0;
    virtual void Visit(IfStatementAST &) = 0;
    virtual void Visit(ForStatementAST &) = 0;
    virtual void Visit(VariableAssignmentAST &) = 0;
    virtual void Visit(VariableDeclAST &) = 0;
    virtual void Visit(PrototypeAST &) = 0;
    virtual void Visit(DeclarationAST &) = 0;
    virtual void Visit(CompoundStatementAST &) = 0;
    virtual void Visit(BlockAST &) = 0;
    virtual void Visit(FunctionAST &) = 0;
    virtual void Visit(ProgramAST &) = 0;
};

class AST {
   public:
    virtual ~AST() = default;
    virtual void PrintAST(int NumIndents) {};
    void PrintIndents(int NumIndents);
    virtual void Accept(ASTVisitor &Visitor) = 0;
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
    const double GetVal() const { return Val; }
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
};

/**
 * should only be true or false
 */
class ConcreteBoolExprAST : public ExprAST {
    bool Val;

   public:
    ConcreteBoolExprAST(bool Val) : Val(Val) {}
    void PrintAST(int NumIndents) override;
    const int GetVal() const { return Val ? 1 : 0; }
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
};

class VariableExprAST : public ExprAST {
    std::string Name;

   public:
    VariableExprAST(const std::string &Name) : Name(Name) {}
    void PrintAST(int NumIndents) override;
    const std::string &GetName() const { return Name; }
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
};

class BinaryExprAST : public ExprAST {
    char Op;

    std::unique_ptr<ExprAST> LHS, RHS;

   public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
    void PrintAST(int NumIndents) override;
    ExprAST &GetLeft() const { return *LHS; }
    ExprAST &GetRight() const { return *RHS; }
    const char GetOp() { return Op; }
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
};

class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

   public:
    CallExprAST(std::string &Callee, std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}
    void PrintAST(int NumIndents) override;
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
    const std::string &GetCallee() const { return Callee; }
    const std::vector<std::unique_ptr<ExprAST>> &GetArgs() const {
        return Args;
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
    void PrintAST(int NumIndents) override;
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
    const std::string &GetCallee() const { return Callee; }
    const std::vector<std::unique_ptr<ExprAST>> &GetArgs() const {
        return Args;
    }
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
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
    ExprAST &GetCond() const { return *Cond; }
    StatementAST &GetThen() const { return *Then; }
    const bool HasElse() const { return Else ? true : false; }
    StatementAST &GetElse() const { return *Else; }
};

class ForStatementAST : public StatementAST {
    std::string VarName;
    std::unique_ptr<ExprAST> Start, End;
    std::unique_ptr<CompoundStatementAST> Body;

   public:
    ForStatementAST(std::string &VarName, std::unique_ptr<ExprAST> Start,
                    std::unique_ptr<ExprAST> End,
                    std::unique_ptr<CompoundStatementAST> Body)
        : VarName(VarName),
          Start(std::move(Start)),
          End(std::move(End)),
          Body(std::move(Body)) {}
    const std::string &GetVarName() const { return VarName; }
    ExprAST &GetStart() const { return *Start; }
    ExprAST &GetEnd() const { return *End; }
    CompoundStatementAST &GetBody() const { return *Body; }
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
    void PrintAST(int NumIndents) override;
};

class VariableAssignmentAST : public StatementAST {
    std::string VarName;
    std::unique_ptr<ExprAST> Value;

   public:
    VariableAssignmentAST(std::string &VarName, std::unique_ptr<ExprAST> Value)
        : VarName(VarName), Value(std::move(Value)) {}

    void PrintAST(int NumIndents) override;
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
};

class VariableDeclAST : public AST {
    std::vector<std::string> VarNames;
    VarType Type;

   public:
    VariableDeclAST(std::vector<std::string> VarNames, VarType Type)
        : VarNames(std::move(VarNames)), Type(Type) {}
    void PrintAST(int NumIndents) override;
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
    const VarType &GetType() const { return Type; }
    const std::vector<std::string> &GetVarNames() const { return VarNames; }
};

class PrototypeAST : public AST {
    std::string Name;
    std::vector<std::unique_ptr<VariableDeclAST>> Parameters;

   public:
    PrototypeAST(const std::string &Name,
                 std::vector<std::unique_ptr<VariableDeclAST>> Parameters)
        : Name(Name), Parameters(std::move(Parameters)) {}

    const std::string &GetName() const { return Name; }
    const std::vector<std::unique_ptr<VariableDeclAST>> &GetParameters() const {
        return Parameters;
    };

    void PrintAST(int NumIndents) override;
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
};

class DeclarationAST : public AST {
    std::vector<std::unique_ptr<VariableDeclAST>> VarDeclarations;

   public:
    DeclarationAST(
        std::vector<std::unique_ptr<VariableDeclAST>> VarDeclarations)
        : VarDeclarations(std::move(VarDeclarations)) {}
    void PrintAST(int NumIndents) override;
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
    const std::vector<std::unique_ptr<VariableDeclAST>> &GetVarDeclarations()
        const {
        return VarDeclarations;
    }
};

class CompoundStatementAST : public StatementAST {
    std::vector<std::unique_ptr<StatementAST>> Statements;

   public:
    CompoundStatementAST(std::vector<std::unique_ptr<StatementAST>> Statements)
        : Statements(std::move(Statements)) {}

    void PrintAST(int NumIndents) override;
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
    const std::vector<std::unique_ptr<StatementAST>> &GetStatements() {
        return Statements;
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
    void PrintAST(int NumIndents) override;
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
    DeclarationAST &GetDeclaration() const { return *Declaration; }
    CompoundStatementAST &GetCompoundStatementAST() const {
        return *CompoundStatement;
    }
};

class FunctionAST : public AST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<BlockAST> Body;

   public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::unique_ptr<BlockAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}
    void PrintAST(int NumIndents) override;
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
    PrototypeAST &GetPrototype() const { return *Proto; }
    BlockAST &GetBody() const { return *Body; }
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
    virtual void Accept(ASTVisitor &Visitor) override { Visitor.Visit(*this); }
    const std::vector<std::unique_ptr<FunctionAST>> &GetFunctions() const {
        return Functions;
    }
    BlockAST &GetBlock() const { return *Block; }
};

#endif
