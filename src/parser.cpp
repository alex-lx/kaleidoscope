#include "ast.cpp"
#include "lexer.cpp"

static int CurTok;
static int getNextToken() {
    return CurTok = gettok();
}

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
