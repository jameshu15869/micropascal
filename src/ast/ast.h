#ifndef AST_H
#define AST_H

class ExprAST {
   public:
    virtual ~ExprAST() = default;
};

class NumberExprAST : public ExprAST {
    double Val;

   public:
    NumberExprAST(double Val) : Val(Val) {}
};

#endif
