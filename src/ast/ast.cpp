#include "ast.h"

#include <iostream>

void AST::PrintIndents(int NumIndents) {
    std::cerr << std::string(NumIndents, ' ');
}

void NumberExprAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << Val << '\n';
}

void ConcreteBoolExprAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << (Val ? "true" : "false") << '\n';
}

void VariableExprAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << Name << '\n';
}

void BinaryExprAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << Op << '\n';
    LHS->PrintAST(NumIndents + 1);
    RHS->PrintAST(NumIndents + 1);
}

void CallExprAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << "Called: " << Callee << '\n';
    for (auto &Arg : Args) {
        Arg->PrintAST(NumIndents + 1);
    }
}

void StatementCallExprAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << "Statement Call: " << Callee << "\n";
    for (auto &Arg : Args) {
        Arg->PrintAST(NumIndents + 1);
    }
}

void IfStatementAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << "If Statement\n";

    PrintIndents(NumIndents + 1);
    std::cerr << "Cond:\n";
    Cond->PrintAST(NumIndents + 2);

    PrintIndents(NumIndents + 1);
    std::cerr << "Then:\n";
    Then->PrintAST(NumIndents + 2);

    if (Else) {
        PrintIndents(NumIndents + 1);
        std::cerr << "Else:\n";
        Else->PrintAST(NumIndents + 2);
    }

    PrintIndents(NumIndents);
    std::cerr << "End If Statement\n";
}

void ForStatementAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << "For Statement\n";

    PrintIndents(NumIndents + 1);
    std::cerr << "Var Name: " << VarName << "\n";

    PrintIndents(NumIndents + 1);
    std::cerr << "Start:\n";
    Start->PrintAST(NumIndents + 2);

    PrintIndents(NumIndents + 1);
    std::cerr << "End:\n";
    End->PrintAST(NumIndents + 2);

    PrintIndents(NumIndents + 1);
    std::cerr << "Body:\n";
    Body->PrintAST(NumIndents + 2);

    PrintIndents(NumIndents);
    std::cerr << "End For Statement\n";
}

void VariableAssignmentAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << "Assignment: " << VarName << '\n';
    Value->PrintAST(NumIndents + 1);
    PrintIndents(NumIndents);
    std::cerr << "End Assignment: " << VarName << '\n';
}

void VariableDeclAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << "Variable Declaration Block: " << Type << '\n';
    for (auto &Name : VarNames) {
        PrintIndents(NumIndents + 1);
        std::cerr << Name << " " << Type << '\n';
    }
}

void PrototypeAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << "Start Proto: " << Name << '\n';

    for (auto &Parameter : Parameters) {
        Parameter->PrintAST(NumIndents + 1);
    }

    PrintIndents(NumIndents);
    std::cerr << "End Proto: " << Name << '\n';
}

void DeclarationAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << "Variable declarations:\n";

    for (auto &VarDecl : VarDeclarations) {
        VarDecl->PrintAST(NumIndents + 1);
    }
}

void CompoundStatementAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << "Statements\n";

    for (auto &Statement : Statements) {
        Statement->PrintAST(NumIndents + 1);
    }

    PrintIndents(NumIndents);
    std::cerr << "End Statements\n";
}

void BlockAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << "Block\n";

    Declaration->PrintAST(NumIndents + 1);
    CompoundStatement->PrintAST(NumIndents + 1);

    PrintIndents(NumIndents);
    std::cerr << "End block\n";
}

void FunctionAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << "Fn: " << Proto->GetName() << "\n";

    Proto->PrintAST(NumIndents + 1);
    Body->PrintAST(NumIndents + 1);

    PrintIndents(NumIndents);
    std::cerr << "End Fn: " << Proto->GetName() << "\n";
}

void ProgramAST::PrintAST(int NumIndents) {
    PrintIndents(NumIndents);
    std::cerr << "Program: " << Name << "\n";

    PrintIndents(NumIndents + 1);
    std::cerr << "Functions:\n";
    for (auto &Function : Functions) {
        Function->PrintAST(NumIndents + 2);
    }
    PrintIndents(NumIndents + 1);
    std::cerr << "End Functions\n";

    Block->PrintAST(NumIndents + 1);

    PrintIndents(NumIndents);
    std::cerr << "End Program: " << Name << "\n";
}
