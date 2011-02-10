#include <vector>
#include <string>
#include <map>
#include "llvm/DerivedTypes.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Support/IRBuilder.h"
using namespace llvm;

static Module *TheModule;
static IRBuilder<> Builder(getGlobalContext());
static std::map<std::string, Value*> NamedValues;

class ExprAST;
class FunctionAST;
ExprAST *Error(const char *Str);
FunctionAST *ErrorF(const char *Str);
Value *ErrorV(const char *Str) { Error(Str); return 0; }

class ExprAST {
public:
  virtual ~ExprAST() {}
  virtual Value *Codegen() = 0;
};

class NumberExprAST : public ExprAST {
  double Val;
public:
  NumberExprAST(double val) : Val(val) {}
  virtual Value *Codegen();
};

Value *NumberExprAST::Codegen() {
  return ConstantFP::get(getGlobalContext(), APFloat(Val));
}

class VariableExprAST : public ExprAST {
  std::string Name;
public:
  VariableExprAST(const std::string &name) : Name(name) {}
  virtual Value *Codegen();
};

Value *VariableExprAST::Codegen() {
  Value *V = NamedValues[Name];
  return V ? V : ErrorV("Unknown variable name");
}

class BinaryExprAST : public ExprAST {
  char Op;
  ExprAST *Left, *Right;
public:
  BinaryExprAST(char op, ExprAST *left, ExprAST *right)
      : Op(op), Left(left), Right(right) {}
  virtual Value *Codegen();
};

Value *BinaryExprAST::Codegen() {
  Value *L = Left->Codegen();
  Value *R = Right->Codegen();
  if (L == 0 || R == 0)
    return 0;

  switch (Op) {
  case '+': return Builder.CreateFAdd(L, R, "addtmp");
  case '-': return Builder.CreateFSub(L, R, "subtmp");
  case '*': return Builder.CreateFMul(L, R, "multmp");
  case '<':
    L = Builder.CreateFCmpULT(L, R, "cmptmp");
    return Builder.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()), "booltmp");
  default: return ErrorV("invalid binary operator");
  }
}

class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<ExprAST*> Args;
public:
  CallExprAST(const std::string &callee, std::vector<ExprAST*> &args)
      : Callee(callee), Args(args) {}
  virtual Value *Codegen();
};

Value *CallExprAST::Codegen() {
  Function *CalleeF = TheModule->getFunction(Callee);
  if (CalleeF == 0)
    return ErrorV("Unknown function referenced");
  if (CalleeF->arg_size() != Args.size())
    return ErrorV("Incorrect # arguments passed");

  std::vector<Value*> ArgsV;
  for (unsigned i = 0; i < Args.size(); ++i) {
    ArgsV.push_back(Args[i]->Codegen());
    if (ArgsV.back() == 0)
        return 0;
  }

  return Builder.CreateCall(CalleeF, ArgsV.begin(), ArgsV.end(), "calltmp");
}

class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;
public:
  PrototypeAST(const std::string &name = "",
                const std::vector<std::string> &args = std::vector<std::string>())
      : Name(name), Args(args) {}
  virtual Function *Codegen();
};

Function *PrototypeAST::Codegen() {
    std::vector<const Type*> Doubles(Args.size(), Type::getDoubleTy(getGlobalContext()));
    FunctionType *FT = FunctionType::get(Type::getDoubleTy(getGlobalContext()), Doubles, false);

    Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule);

    if (F->getName() != Name) {
      F->eraseFromParent();
      F = TheModule->getFunction(Name);

      if (!F->empty()) {
        ErrorF("redefinition of function");
        return 0;
      }

      if (F->arg_size() != Args.size()) {
        ErrorF("redefinition of function with different # args");
        return 0;
      }
    }

    unsigned idx = 0;
    for (Function::arg_iterator AI = F->arg_begin(); idx != Args.size(); ++AI, ++idx) {
      AI->setName(Args[idx]);
      NamedValues[Args[idx]] = AI;
    }
    return F;
}

class FunctionAST {
  PrototypeAST *Proto;
  ExprAST *Body;
public:
  FunctionAST(PrototypeAST *proto, ExprAST *body)
      : Proto(proto), Body(body) {}
  virtual Function *Codegen();
};

Function *FunctionAST::Codegen() {
    NamedValues.clear();
    Function *TheFunction = Proto->Codegen();
    if (TheFunction == 0)
        return 0;

    BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "entry", TheFunction);
    Builder.SetInsertPoint(bb);

    if (Value *RetVal = Body->Codegen()) {
        Builder.CreateRet(RetVal);
        verifyFunction(*TheFunction);
        return TheFunction;
    }

    TheFunction->eraseFromParent();
    return 0;
}
