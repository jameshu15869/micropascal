#include "codegen/codegen.h"

#include <iostream>
#include <map>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "logger/logger.h"

using namespace llvm;

class GenIRVisitor : public ASTVisitor {
    std::unique_ptr<Module> TheModule;
    IRBuilder<> Builder;

    Type *Int1Ty;

    Value *V;
    Function *F;
    std::map<std::string, Value *> NamedValues;

   public:
    GenIRVisitor(std::unique_ptr<Module> M)
        : TheModule(std::move(M)), Builder(TheModule->getContext()) {
        Int1Ty = Type::getInt1Ty(TheModule->getContext());
    }

    void run(std::unique_ptr<AST> Ast) {
        Ast->Accept(*this);
        TheModule->print(errs(), nullptr);
    }

    virtual void Visit(NumberExprAST &E) override {
        V = ConstantInt::get(Type::getInt64Ty(TheModule->getContext()), (uint64_t) E.GetVal());
    }

    virtual void Visit(ConcreteBoolExprAST &E) override {
        V = ConstantInt::get(Int1Ty, E.GetVal());
    }

    virtual void Visit(VariableExprAST &E) override {
        Value *Val = NamedValues[E.GetName()];
        if (!Val) {
            LogError("Unknown variable");
            return;
        }

        V = Val;
    }

    virtual void Visit(BinaryExprAST &E) override {
        E.GetLeft().Accept(*this);
        Value *L = V;
        E.GetRight().Accept(*this);
        Value *R = V;

        if (!L || !R) {
            LogError("L or R was null in visit");
            return;
        }

        switch (E.GetOp()) {
            case '+':
                V = Builder.CreateNSWAdd(L, R, "addtmp");
                break;
            case '-':
                V = Builder.CreateNSWSub(L, R, "subtmp");
                break;
            case '*':
                V = Builder.CreateNSWMul(L, R, "multmp");
                break;
            case '/':
                V = Builder.CreateSDiv(L, R, "divtmp");
                break;
            default:
                LogError("Unknown operation!");
                return;
        }
    }

    virtual void Visit(CallExprAST &E) override {
        Function *CalleeF = TheModule->getFunction(E.GetCallee());
        if (!CalleeF) {
            LogError("Could not find function");
            return;
        }

        const std::vector<std::unique_ptr<ExprAST>> &Args = E.GetArgs();
        if (CalleeF->arg_size() != Args.size()) {
            LogError("Incorrect # of arguments");
            return;
        }

        std::vector<Value *> ArgsV;
        for (unsigned i = 0, e = Args.size(); i != e; i++) {
            Args[i]->Accept(*this);
            ArgsV.push_back(V);
            if (!ArgsV.back()) {
                LogError("Error occurred while codegen function args");
                return;
            }
        }

        // Builder.CreateCall(CalleeF, ArgsV, "calltmp");
        Builder.CreateCall(CalleeF, ArgsV);
    }

    virtual void Visit(StatementCallExprAST &E) override {
        Function *CalleeF = TheModule->getFunction(E.GetCallee());
        if (!CalleeF) {
            LogError("Could not find function");
            return;
        }

        const std::vector<std::unique_ptr<ExprAST>> &Args = E.GetArgs();
        if (CalleeF->arg_size() != Args.size()) {
            LogError("Incorrect # of arguments");
            return;
        }

        std::vector<Value *> ArgsV;
        for (unsigned i = 0, e = Args.size(); i != e; i++) {
            Args[i]->Accept(*this);
            ArgsV.push_back(V);
            if (!ArgsV.back()) {
                LogError("Error occurred while codegen function args");
                return;
            }
        }

        // Builder.CreateCall(CalleeF, ArgsV, "calltmp");
        // void functions do not have a return value
        Builder.CreateCall(CalleeF, ArgsV);
    }

    virtual void Visit(IfStatementAST &S) override {
        assert(false && "NOT IMPLEMENTED: Visit IfStatementAST");
    }

    virtual void Visit(VariableAssignmentAST &S) override {
        assert(false && "NOT IMPLEMENTED: Visit VariableAssignmentAST");
    }

    virtual void Visit(VariableDeclAST &S) override {
        assert(false && "NOT IMPLEMENTED: Visit VariableDeclAST");
    }

    virtual void Visit(PrototypeAST &P) override {
        std::vector<Type *> ParameterTypes;
        std::vector<std::string> AllVars;
        for (auto &Decl : P.GetParameters()) {
            Type *CurrentType;
            switch (Decl->GetType()) {
                case TYPE_INTEGER:
                    CurrentType = Type::getDoubleTy(TheModule->getContext());
                    break;
                case TYPE_BOOLEAN:
                    CurrentType = Type::getInt1Ty(TheModule->getContext());
                    break;
                default:
                    LogError("Unknown parameter type");
                    return;
            }
            ParameterTypes.insert(ParameterTypes.end(),
                                  Decl->GetVarNames().size(), CurrentType);
            AllVars.insert(AllVars.end(), Decl->GetVarNames().begin(),
                           Decl->GetVarNames().end());
        }

        FunctionType *FT = FunctionType::get(
            Type::getVoidTy(TheModule->getContext()), ParameterTypes, false);

        Function *CreatedF = Function::Create(FT, Function::ExternalLinkage,
                                              P.GetName(), TheModule.get());

        unsigned Idx = 0;
        for (auto &Arg : CreatedF->args()) {
            Arg.setName(AllVars[Idx++]);
        }

        F = CreatedF;
    }

    virtual void Visit(DeclarationAST &D) override {
        assert(false && "NOT IMPLEMENTED: Visit DeclarationAST");
    }

    virtual void Visit(CompoundStatementAST &S) override {
        for (auto &S : S.GetStatements()) {
            S->Accept(*this);
        }
    }

    virtual void Visit(BlockAST &B) override {
        // B.GetDeclaration().Accept(*this);

        B.GetCompoundStatementAST().Accept(*this);
    }

    virtual void Visit(FunctionAST &Func) override {
        Function *TheFunction =
            TheModule->getFunction(Func.GetPrototype().GetName());
        if (!TheFunction) {
            Func.GetPrototype().Accept(*this);
            TheFunction = F;
        }

        if (!TheFunction) {
            LogError("Failed to codegen function prototype");
            return;
        }

        if (!TheFunction->empty()) {
            LogError("Function cannot be redefined");
            return;
        }

        BasicBlock *BB =
            BasicBlock::Create(TheModule->getContext(), "entry", TheFunction);
        Builder.SetInsertPoint(BB);
        NamedValues.clear();
        for (auto &Arg : TheFunction->args()) {
            NamedValues[std::string(Arg.getName())] = &Arg;
        }

        Func.GetBody().Accept(*this);
        if (!V) {
            LogError("Error while generating function body");
            TheFunction->eraseFromParent();
            return;
        }
        Builder.CreateRet(V);
        verifyFunction(*TheFunction);
        F = TheFunction;
    }

    virtual void Visit(ProgramAST &P) override {
        for (auto &Func : P.GetFunctions()) {
            std::cerr << "Func: " << Func->GetPrototype().GetName();
        }

        // Create prototype for writeln() from runtime.c
        FunctionType *WriteLnTy = FunctionType::get(
            Type::getVoidTy(TheModule->getContext()),
            {Type::getInt64Ty(TheModule->getContext())}, false);

        Function *WriteLn = Function::Create(
            WriteLnTy, Function::ExternalLinkage, "writeln", TheModule.get());

        FunctionType *MainFT = FunctionType::get(
            Type::getVoidTy(TheModule->getContext()), {}, false);
        Function *MainFn = Function::Create(MainFT, Function::ExternalLinkage,
                                            "main", TheModule.get());
        BasicBlock *BB =
            BasicBlock::Create(TheModule->getContext(), "entry", MainFn);

        Builder.SetInsertPoint(BB);

        P.GetBlock().Accept(*this);

        Builder.CreateRet(ConstantInt::get(
            Type::getInt32Ty(TheModule->getContext()), 0, true));
        F = MainFn;
    }
};

void CodeGen::Compile(std::unique_ptr<AST> Ast) {
    std::unique_ptr<LLVMContext> TheContext = std::make_unique<LLVMContext>();
    std::unique_ptr<Module> M =
        std::make_unique<Module>("toy-lang.tl", *TheContext);

    GenIRVisitor GenIR(std::move(M));
    GenIR.run(std::move(Ast));
    // M->print(outs(), nullptr);
}