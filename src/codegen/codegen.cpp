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
    std::map<std::string, AllocaInst *> NamedValues;

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

    AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
                                       const std::string &VarName) {
        IRBuilder<> TmpBuilder(&TheFunction->getEntryBlock(),
                               TheFunction->getEntryBlock().begin());
        return TmpBuilder.CreateAlloca(Int64Ty, nullptr, VarName);
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
        AllocaInst *A = NamedValues[E.GetName()];
        if (!A) {
            LogError("Unknown variable");
            return;
        }

        V = Builder.CreateLoad(A->getAllocatedType(), A, E.GetName().c_str());
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
            case '<':
                V = Builder.CreateICmpSLT(L, R, "cmptmp");
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
        Function *TheFunction = Builder.GetInsertBlock()->getParent();

        AllocaInst *Alloca =
            CreateEntryBlockAlloca(TheFunction, S.GetVarName());

        S.GetStart().Accept(*this);
        Value *StartV = V;
        if (!StartV) {
            LogError("Failed to codegen start");
            return;
        }

        // store the start variable value into alloca
        Builder.CreateStore(StartV, Alloca);

        BasicBlock *LoopBB =
            BasicBlock::Create(TheModule->getContext(), "loop", TheFunction);

        // go to loop from the current block if nothing else happens
        Builder.CreateBr(LoopBB);

        Builder.SetInsertPoint(LoopBB);

        // load the new value and shadow the old value
        AllocaInst *OldAlloca = NamedValues[S.GetVarName()];
        NamedValues[S.GetVarName()] = Alloca;

        S.GetBody().Accept(*this);
        if (!V) {
            LogError("Error generating body code in for loop");
            return;
        }

        // Pascal has a default step size of 1 for "for ... to do" loops
        Value *StepV = ConstantInt::get(Int64Ty, 1);

        S.GetEnd().Accept(*this);
        if (!V) {
            LogError("Failed to codegen end cond");
            return;
        }
        Value *EndCond = V;

        Value *CurVar = Builder.CreateLoad(Alloca->getAllocatedType(), Alloca,
                                           S.GetVarName().c_str());
        Value *NextVar = Builder.CreateNSWAdd(CurVar, StepV, "nextvar");
        Builder.CreateStore(NextVar, Alloca);

        EndCond = Builder.CreateICmpSLT(CurVar, EndCond, "loopcond");

        BasicBlock *AfterBB = BasicBlock::Create(TheModule->getContext(),
                                                 "afterloop", TheFunction);

        // if end cond is NOT true (Since we do CmpNE), go to loop. else, go to
        // after
        Builder.CreateCondBr(EndCond, LoopBB, AfterBB);

        Builder.SetInsertPoint(AfterBB);

        if (OldAlloca) {
            NamedValues[S.GetVarName()] = OldAlloca;
        } else {
            NamedValues.erase(S.GetVarName());
        }

        return;
    }

    virtual void Visit(VariableAssignmentAST &S) override {
        S.GetValue().Accept(*this);
        Value *Val = V;
        if (!Val) {
            LogError("Failed to codegen expression in assignment");
            return;
        }

        Value *Variable = NamedValues[S.GetVarName()];
        if (!Variable) {
            LogError("Unknown variable");
            return;
        }
        Builder.CreateStore(Val, Variable);
    }

    virtual void Visit(VariableDeclAST &S) override {
        std::vector<AllocaInst *> OldBindings;

        Function *F = Builder.GetInsertBlock()->getParent();

        for (int i = 0; i < S.GetVarNames().size(); i++) {
            const std::string &VarName = S.GetVarNames()[i];
            Value *InitVal = ConstantInt::get(Int64Ty, 0, true);

            AllocaInst *Alloca = CreateEntryBlockAlloca(F, VarName);
            Builder.CreateStore(InitVal, Alloca);
            OldBindings.push_back(NamedValues[VarName]);
            NamedValues[VarName] = Alloca;
        }
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
                    CurrentType = Type::getInt64Ty(TheModule->getContext());
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
        for (auto &VarDecl : D.GetVarDeclarations()) {
            for (auto Name : VarDecl->GetVarNames()) {
                NamedValues[Name] = nullptr;
            }
        }
    }

    virtual void Visit(CompoundStatementAST &S) override {
        for (auto &S : S.GetStatements()) {
            S->Accept(*this);
        }
    }

    virtual void Visit(BlockAST &B) override {
        B.GetDeclaration().Accept(*this);

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
            AllocaInst *Alloca =
                CreateEntryBlockAlloca(TheFunction, Arg.getName().str());

            Builder.CreateStore(&Arg, Alloca);

            NamedValues[std::string(Arg.getName())] = Alloca;
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
