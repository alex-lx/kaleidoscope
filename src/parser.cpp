#include "ast.cpp"
#include "lexer.cpp"
#include <map>

static int CurTok;
static int getNextToken() {
    return CurTok = gettok();
}

static ExprAST *ParseExpression();
static ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS);

ExprAST *Error(const char *Str) {
  fprintf(stderr, "Error (line %d, col %d): %s\n", CurrentRow, CurrentCol, Str);
  return 0;
}
PrototypeAST *ErrorP(const char *Str) {
  Error(Str);
  return 0;
}
FunctionAST *ErrorF(const char *Str) {
  Error(Str);
  return 0;
}

static ExprAST *ParseNumberExpr() {
  ExprAST *Result = new NumberExprAST(NumVal);
  getNextToken(); // eat the number
  return Result;
}

static ExprAST *ParseParenExpr() {
  getNextToken(); // eat (
  ExprAST *V = ParseExpression();
  if (!V)
    return 0;
  if (CurTok != ')')
      return Error("Expected ')'");
  getNextToken(); // eat )
  return V;
}

static ExprAST *ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;

  getNextToken();  // eat identifier

  // x = y;
  if (CurTok != '(')
      return new VariableExprAST(IdName);

  // x = y(1,2,3)
  getNextToken(); // eat (
  std::vector<ExprAST*> Args;
  if (CurTok != ')') {
    while (1) {
      ExprAST *Arg = ParseExpression();
      if (!Arg)
        return 0;

      if (CurTok == ')')
        break;

      if (CurTok != ',')
        return Error("Expected ')' or ',' in argument list");
      getNextToken();
    }
  }
  getNextToken(); // eat )

  return new CallExprAST(IdName, Args);
}

static ExprAST *ParsePrimary() {
  switch(CurTok) {
  default: return Error("Unknown token when expecting an expression");
  case tok_identifier: return ParseIdentifierExpr();
  case tok_number: return ParseNumberExpr();
  case '(': return ParseParenExpr();
  }
}

static std::map<char, int> BinOpPrecedence;

static int GetTokPrecedence() {
  if (!isascii(CurTok))
      return -1;
  int TokPrec = BinOpPrecedence[CurTok];
  if (TokPrec <= 0)
      return -1;
  return TokPrec;
}

static ExprAST *ParseExpression() {
  ExprAST *LHS = ParsePrimary();
  if (!LHS)
    return 0;
  return ParseBinOpRHS(0, LHS);
}

static ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS) {
  while (1) {
    int TokPrec = GetTokPrecedence();
    if (TokPrec < ExprPrec)
        return LHS;

    int BinOp = CurTok;
    getNextToken(); // eat current op
    ExprAST *RHS = ParsePrimary();
    if (!RHS)
        return 0;
    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrec) {
        RHS = ParseBinOpRHS(TokPrec + 1, RHS);
        if (!RHS)
          return 0;
    }
    LHS = new BinaryExprAST(BinOp, LHS, RHS);
  }
}

static PrototypeAST *ParsePrototype() {
  if (CurTok != tok_identifier)
      return ErrorP("Expected function name in prototype");

  std::string FnName = IdentifierStr;
  getNextToken();

  if (CurTok != '(')
      return ErrorP("Expected '(' in Prototype");

  std::vector<std::string> ArgNames;
  while (getNextToken() == tok_identifier)
      ArgNames.push_back(IdentifierStr);
  if (CurTok != ')')
    return ErrorP("Expected ')' in Prototype");

  getNextToken(); // eat )

  return new PrototypeAST(FnName, ArgNames);
}

static FunctionAST *ParseDefinition() {
  getNextToken();
  PrototypeAST *Proto = ParsePrototype();
  if (!Proto)
      return 0;
  if (ExprAST *E = ParseExpression())
      return new FunctionAST(Proto, E);
  return 0;
}

static PrototypeAST *ParseExtern() {
  getNextToken();
  return ParsePrototype();
}
static FunctionAST *ParseTopLevelExpr() {
  ExprAST *E = ParseExpression();
  if (!E)
    return 0;
  PrototypeAST *Proto = new PrototypeAST();
  return new FunctionAST(Proto, E);
}

static void HandleDefinition() {
  if (FunctionAST *F = ParseDefinition()) {
    if (Function *LF = F->Codegen()) {
      fprintf(stderr, "Parsed a function definition.\n");
      LF->dump();
    }
  } else {
    getNextToken();
  }
}

static void HandleExtern() {
  if (PrototypeAST *P = ParseExtern()) {
    if (Function *F = P->Codegen()) {
      fprintf(stderr, "Parsed an extern\n");
      F->dump();
    }
  } else {
    getNextToken();
  }
}

static void HandleTopLevelExpression() {
  if (FunctionAST *F = ParseTopLevelExpr()) {
    if (Function *LF = F->Codegen()) {
      fprintf(stderr, "Parsed a top-level expr\n");
      LF->dump();
    }
  } else {
    fprintf(stderr, "error?");
    getNextToken();
  }
}

static void MainLoop() {
  while (1) {
    fprintf(stderr, "ready> ");
    switch (CurTok) {
    case tok_eof:   return;
    case ';':       getNextToken(); break;
    case tok_def:  HandleDefinition(); break;
    case tok_extern: HandleExtern(); break;
    default:    HandleTopLevelExpression(); break;
    }
  }
}

int main() {
  LLVMContext &Context = getGlobalContext();

  BinOpPrecedence['<'] = 10;
  BinOpPrecedence['+'] = 20;
  BinOpPrecedence['-'] = 20;
  BinOpPrecedence['*'] = 40;

  fprintf(stderr, "ready> ");
  getNextToken();

  TheModule = new Module("my cool jit", Context);

  MainLoop();

  TheModule->dump();
  return 0;
}
