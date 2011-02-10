#include <vector>
#include <string>
#include "llvm/DerivedTypes.h"
using namespace llvm;

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

class VariableExprAST : public ExprAST {
  std::string Name;
public:
  VariableExprAST(const std::string &name) : Name(name) {}
  virtual Value *Codegen();
};

class BinaryExprAST : public ExprAST {
  char Op;
  ExprAST *Left, *Right;
public:
  BinaryExprAST(char op, ExprAST *left, ExprAST *right)
      : Op(op), Left(left), Right(right) {}
  virtual Value *Codegen();
};

class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<ExprAST*> Args;
public:
  CallExprAST(const std::string &callee, std::vector<ExprAST*> &args)
      : Callee(callee), Args(args) {}
  virtual Value *Codegen();
};

class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;
public:
  PrototypeAST(const std::string &name = "",
                const std::vector<std::string> &args = std::vector<std::string>())
      : Name(name), Args(args) {}
  virtual Value *Codegen();
};

class FunctionAST {
  PrototypeAST *Proto;
  ExprAST *Body;
public:
  FunctionAST(PrototypeAST *proto, ExprAST *body)
      : Proto(proto), Body(body) {}
  virtual Value *Codegen();
};
