#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../language/ast.hh"
#include "../language/env.hh"
#include "../language/symvar.hh"

// Forward declaration
class FunctionFactory;

using namespace std;
// see = symbolic execution engine

// Two variables to be added
// one corresponding to sigma (value environment - string to expr mapping)
// and one corresponding to the path constraint (vector of exprs)

class SEE {
private:
  SymVar *symVar = nullptr;

  ValueEnvironment
      sigma; // Value environment: maps variable names to their values
  vector<Expr *> pathConstraint;
  FunctionFactory *functionFactory; // Factory for creating API functions

  // Maps base variable names to their current suffixed names
  // e.g., "email" -> "email0", "password" -> "password0"
  map<string, string> baseNameToSuffixed;

  // Helper to extract base name from suffixed name
  // "email0" -> "email", "password1" -> "password"
  string extractBaseName(const string &suffixedName);

  // Helper methods for runtime placeholder resolution, restaurant ID, menu item
  // ID, order ID
  string findKeyFromMapInSigma(const string &prefix);
  string findRestaurantIdFromSigma();
  string findMenuItemIdFromSigma();
  string findOrderIdFromSigma();
  // E-commerce ID resolution helpers
  string findProductIdFromSigma();
  string findCartIdFromSigma();
  string findReviewIdFromSigma();
  string findOrderStatusFromSigma();
  string findSellersFromSigma();
  string findStockFromSigma();
  // Library ID resolution helpers
  string findBookCodeFromSigma();
  string findStudentIdFromSigma();
  string findRequestIdFromSigma();
  string findLoanIdFromSigma();

  unique_ptr<Expr> computePathConstraint(vector<Expr *>);
  // If the statement is a call to an API function, then none of its parameters
  // should be variables whose values are symbolic expression.
  bool isReady(Stmt &, SymbolTable &);

  // If the expression is a variable and is not mapped to a symbolic expression
  // then the expression is ready. For any other type of expression, if it has
  // a variable in it, e.g. a function call with a variable being passed as an
  // argument, then all such expression should be ready.
  bool isReady(Expr &, SymbolTable &);
  // If an expression has a symbolic variable as one of its subexpressions, then
  // it is symbolic expression.
  bool isSymbolic(Expr &, SymbolTable &);

  // Check if a function call is an API call (not a built-in function)
  // Built-in functions: Add, Sub, Mul, Eq, Lt, Gt, And, Or, Not, input
  bool isAPI(const FuncCall &fc);

  void executeStmt(Stmt &, SymbolTable &);
  Expr *evaluateExpr(Expr &, SymbolTable &);

public:
  SEE(FunctionFactory *functionFactory) : sigma(nullptr) {
    this->functionFactory = functionFactory;
  }

  // Program and Type Env
  void execute(Program &, SymbolTable &);

  // Solve path constraints and return a result
  unique_ptr<Expr> computePathConstraint();

  // Getters for testing
  ValueEnvironment &getSigma() { return sigma; }
  vector<Expr *> &getPathConstraint() { return pathConstraint; }
};
