#include <vector>
#include <string>

class ExprAST {
public:
  virtual ~ExprAST() {}
};

class NumberExprAST : public ExprAST {
  double Val;
public:
  NumberExprAST(double val) : Val(val) {}
};

class VariableExprAST : public ExprAST {
  std::string Name;
public:
  VariableExprAST(const std::string &name) : Name(name) {}
};

class BinaryExprAST : public ExprAST {
  char Op;
  ExprAST *Left, *Right;
public:
  BinaryExprAST(char op, ExprAST *left, ExprAST *right)
      : Op(op), Left(left), Right(right) {}
};

class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<ExprAST*> Args;
public:
  CallExprAST(const std::string &callee, std::vector<ExprAST*> &args)
      : Callee(callee), Args(args) {}
};

class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;
public:
  PrototypeAST(const std::string &name = "",
                const std::vector<std::string> &args = std::vector<std::string>())
      : Name(name), Args(args) {}

};

class FunctionAST {
  PrototypeAST *Proto;
  ExprAST *Body;
public:
  FunctionAST(PrototypeAST *proto, ExprAST *body)
      : Proto(proto), Body(body) {}
};
