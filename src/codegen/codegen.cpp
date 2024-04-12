#include "codegen/codegen.h"

#include <iostream>
#include <map>

#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LoopAccessAnalysis.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "logger/logger.h"

using namespace llvm;

class GenIRVisitor : public ASTVisitor {
    // std::unique_ptr<Module> TheModule;
    Module *TheModule;
    IRBuilder<> Builder;
    std::unique_ptr<FunctionPassManager> TheFPM;
    std::unique_ptr<LoopAnalysisManager> TheLAM;
    std::unique_ptr<FunctionAnalysisManager> TheFAM;
    std::unique_ptr<CGSCCAnalysisManager> TheCGAM;
    std::unique_ptr<ModuleAnalysisManager> TheMAM;
    std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
    std::unique_ptr<StandardInstrumentations> TheSI;

    Type *Int64Ty;

    Value *V;
    Function *F;
    std::map<std::string, Value *> NamedValues;

   public:
    GenIRVisitor(Module *M, std::unique_ptr<FunctionPassManager> FPM,
                 std::unique_ptr<LoopAnalysisManager> LAM,
                 std::unique_ptr<FunctionAnalysisManager> FAM,
                 std::unique_ptr<CGSCCAnalysisManager> CGAM,
                 std::unique_ptr<ModuleAnalysisManager> MAM,
                 std::unique_ptr<PassInstrumentationCallbacks> PIC,
                 std::unique_ptr<StandardInstrumentations> SI)
        : TheModule(M),
          TheFPM(std::move(FPM)),
          TheLAM(std::move(LAM)),
          TheFAM(std::move(FAM)),
          TheCGAM(std::move(CGAM)),
          TheMAM(std::move(MAM)),
          ThePIC(std::move(PIC)),
          TheSI(std::move(SI)),
          Builder(TheModule->getContext()) {
        Int64Ty = Type::getInt64Ty(TheModule->getContext());

        TheSI->registerCallbacks(*ThePIC, TheMAM.get());

        TheFPM->addPass(InstCombinePass());
        TheFPM->addPass(ReassociatePass());
        TheFPM->addPass(GVNPass());
        TheFPM->addPass(SimplifyCFGPass());

        PassBuilder PB;
        PB.registerModuleAnalyses(*TheMAM);
        PB.registerFunctionAnalyses(*TheFAM);
        PB.crossRegisterProxies(*TheLAM, *TheFAM, *TheCGAM, *TheMAM);
    }

    void run(std::unique_ptr<AST> Ast) {
        Ast->Accept(*this);
        TheModule->print(errs(), nullptr);
    }

    virtual void Visit(NumberExprAST &E) override {
        V = ConstantInt::get(Type::getInt64Ty(TheModule->getContext()),
                             (uint64_t)E.GetVal());
    }

    virtual void Visit(ConcreteBoolExprAST &E) override {
        V = ConstantInt::get(Int64Ty, E.GetVal());
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
        S.GetCond().Accept(*this);
        if (!V) {
            LogError("Failed to codegen cond");
            return;
        }
        Value *CondV = V;

        CondV = Builder.CreateICmpNE(CondV, ConstantInt::get(Int64Ty, 0.0));

        Function *TheFunction = Builder.GetInsertBlock()->getParent();

        BasicBlock *ThenBB =
            BasicBlock::Create(TheModule->getContext(), "then", TheFunction);
        BasicBlock *ElseBB =
            BasicBlock::Create(TheModule->getContext(), "else");
        BasicBlock *MergeBB =
            BasicBlock::Create(TheModule->getContext(), "ifcont");

        Builder.CreateCondBr(CondV, ThenBB, ElseBB);

        Builder.SetInsertPoint(ThenBB);

        S.GetThen().Accept(*this);
        if (!V) {
            LogError("Failed to codegen then clause");
            return;
        }
        Builder.CreateBr(MergeBB);
        // Then block may have changed after codegen
        ThenBB = Builder.GetInsertBlock();

        // Emit else
        TheFunction->insert(TheFunction->end(), ElseBB);
        Builder.SetInsertPoint(ElseBB);

        if (S.HasElse()) {
            S.GetElse().Accept(*this);
        }
        Builder.CreateBr(MergeBB);
        ElseBB = Builder.GetInsertBlock();

        // Emit merge block
        TheFunction->insert(TheFunction->end(), MergeBB);
        Builder.SetInsertPoint(MergeBB);

        // Since we don't treat if/else as value exprs, we don't need a PHI node
    }

    virtual void Visit(ForStatementAST &S) override {
        S.GetStart().Accept(*this);
        Value *StartV = V;
        if (!StartV) {
            LogError("Failed to codegen start");
            return;
        }

        Function *TheFunction = Builder.GetInsertBlock()->getParent();
        BasicBlock *PreheaderBB = Builder.GetInsertBlock();
        BasicBlock *LoopBB =
            BasicBlock::Create(TheModule->getContext(), "loop", TheFunction);

        // go to loop from the current block if nothing else happens
        Builder.CreateBr(LoopBB);

        Builder.SetInsertPoint(LoopBB);

        // represents the loop variable that will be incremented
        PHINode *Variable = Builder.CreatePHI(Int64Ty, 2, S.GetVarName());
        Variable->addIncoming(StartV, PreheaderBB);

        Value *OldVal = NamedValues[S.GetVarName()];
        NamedValues[S.GetVarName()] = Variable;

        S.GetBody().Accept(*this);
        if (!V) {
            LogError("Error generating body code in for loop");
            return;
        }

        // Pascal has a default step size of 1 for "for ... to do" loops
        Value *StepV = ConstantInt::get(Int64Ty, 1);

        Value *NextVar = Builder.CreateNSWAdd(Variable, StepV, "nextvar");

        S.GetEnd().Accept(*this);
        if (!V) {
            LogError("Failed to codegen end cond");
            return;
        }

        Value *EndCond = V;
        EndCond = Builder.CreateICmpNE(Variable, EndCond, "loopcond");

        BasicBlock *LoopEndBB = Builder.GetInsertBlock();
        BasicBlock *AfterBB = BasicBlock::Create(TheModule->getContext(),
                                                 "afterloop", TheFunction);

        // if end cond is NOT true (Since we do CmpNE), go to loop. else, go to
        // after
        Builder.CreateCondBr(EndCond, LoopBB, AfterBB);

        Builder.SetInsertPoint(AfterBB);

        Variable->addIncoming(NextVar, LoopEndBB);

        if (OldVal) {
            NamedValues[S.GetVarName()] = OldVal;
        } else {
            NamedValues.erase(S.GetVarName());
        }

        return;
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
                    CurrentType = Type::getInt64Ty(TheModule->getContext());
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
                                              P.GetName(), TheModule);

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

        TheFPM->run(*TheFunction, *TheFAM);

        F = TheFunction;
    }

    virtual void Visit(ProgramAST &P) override {
        // Create prototype for writeln() from runtime.c
        FunctionType *WriteLnTy = FunctionType::get(
            Type::getVoidTy(TheModule->getContext()),
            {Type::getInt64Ty(TheModule->getContext())}, false);

        Function *WriteLn = Function::Create(
            WriteLnTy, Function::ExternalLinkage, "writeln", TheModule);

        for (auto &Func : P.GetFunctions()) {
            Func->Accept(*this);
        }

        FunctionType *MainFT = FunctionType::get(
            Type::getVoidTy(TheModule->getContext()), {}, false);
        Function *MainFn = Function::Create(MainFT, Function::ExternalLinkage,
                                            "toylang_main", TheModule);
        BasicBlock *BB =
            BasicBlock::Create(TheModule->getContext(), "entry", MainFn);

        Builder.SetInsertPoint(BB);

        P.GetBlock().Accept(*this);

        Builder.CreateRet(ConstantInt::get(
            Type::getInt32Ty(TheModule->getContext()), 0, true));

        TheFPM->run(*MainFn, *TheFAM);

        F = MainFn;
    }
};

void CodeGen::CompileAndRun(std::unique_ptr<AST> Ast,
                            llvm::orc::KaleidoscopeJIT &TheJIT) {
    std::unique_ptr<LLVMContext> TheContext = std::make_unique<LLVMContext>();
    M = std::make_unique<Module>("toy-lang.tl", *TheContext);

    M->setDataLayout(TheJIT.getDataLayout());

    std::unique_ptr<FunctionPassManager> FPM =
        std::make_unique<FunctionPassManager>();
    std::unique_ptr<LoopAnalysisManager> LAM =
        std::make_unique<LoopAnalysisManager>();
    std::unique_ptr<FunctionAnalysisManager> FAM =
        std::make_unique<FunctionAnalysisManager>();
    std::unique_ptr<CGSCCAnalysisManager> CGAM =
        std::make_unique<CGSCCAnalysisManager>();
    std::unique_ptr<ModuleAnalysisManager> MAM =
        std::make_unique<ModuleAnalysisManager>();
    std::unique_ptr<PassInstrumentationCallbacks> PIC =
        std::make_unique<PassInstrumentationCallbacks>();
    std::unique_ptr<StandardInstrumentations> SI =
        std::make_unique<StandardInstrumentations>(*TheContext, true);

    GenIRVisitor GenIR(M.get(), std::move(FPM), std::move(LAM), std::move(FAM),
                       std::move(CGAM), std::move(MAM), std::move(PIC),
                       std::move(SI));
    std::cerr
        << "============================   IR   ============================\n";
    GenIR.run(std::move(Ast));
    // M->print(outs(), nullptr);

    auto RT = TheJIT.getMainJITDylib().createResourceTracker();

    auto TSM = llvm::orc::ThreadSafeModule(std::move(M), std::move(TheContext));

    llvm::ExitOnError ExitOnErr;
    ExitOnErr(TheJIT.addModule(std::move(TSM), RT));

    auto ExprSymbol = ExitOnErr(TheJIT.lookup("toylang_main"));

    std::cerr << "\n";
    std::cerr
        << "============================ Result ============================\n";
    // Execute the main function
    void (*FP)() = ExprSymbol.getAddress().toPtr<void (*)()>();
    FP();

    ExitOnErr(RT->remove());
}
