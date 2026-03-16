#include "see.hh"
#include "../language/env.hh" // will change this to normal env.hh later
#include "functionfactory.hh"
#include <iostream>
#include <set>
using namespace std;

//  Helper to check if an expression is concrete (i.e., Num, String, Set, Map)
static bool isConcrete(Expr *expr) {
  if (!expr)
    return false;

  ExprType type = expr->exprType;
  return (type == ExprType::NUM || type == ExprType::STRING ||
          type == ExprType::SET || type == ExprType::MAP);
}

// Helper to check if a Num represents a boolean value
static bool isNumBool(Expr *expr) {
  if (expr && expr->exprType == ExprType::NUM) {
    Num *n = dynamic_cast<Num *>(expr);
    return (n->value == 0 || n->value == 1);
  }
  return false;
}

unique_ptr<Expr> SEE::computePathConstraint(vector<Expr *> C) {
  if (C.empty()) {
    // No constraints, return true (represented as 1 == 1)
    vector<unique_ptr<Expr>> args;
    args.push_back(make_unique<Num>(1));
    args.push_back(make_unique<Num>(1));
    return make_unique<FuncCall>("equals", std::move(args));
  }

  if (C.size() == 1) {
    // Single constraint, return it
    return C[0]->clone();
  }

  // Multiple constraints, conjoin them with AND
  // For now, we'll create a nested structure: C1 AND (C2 AND (C3 AND ...))
  unique_ptr<Expr> result = C.back()->clone();

  for (int i = C.size() - 2; i >= 0; i--) {
    vector<unique_ptr<Expr>> args;
    args.push_back(C[i]->clone());
    args.push_back(std::move(result));
    result = make_unique<FuncCall>("and", std::move(args));
  }

  return result;
}

unique_ptr<Expr> SEE::computePathConstraint() {
  return computePathConstraint(pathConstraint);
}

bool SEE::isReady(Stmt &s, SymbolTable &st) {
  if (s.statementType == StmtType::ASSIGN) {
    Assign &assign = dynamic_cast<Assign &>(s);

    // Check if this is an API call assignment (e.g., r1 := f(x1))
    if (assign.right->exprType == ExprType::FUNC_CALL_EXPR) {
      FuncCall &fc = dynamic_cast<FuncCall &>(*assign.right);

      if (isAPI(fc)) {
        // This is an API call - check if arguments are ready
        // If any argument is symbolic, we need to interrupt and solve first
        for (const auto &arg : fc.args) {
          if (isSymbolic(*arg, st)) {
            cout << "[SEE] API call '" << fc.name
                 << "' with symbolic arguments - interruption point" << endl;
            return false; // Not ready - need to solve constraints first
          }
        }

        // All arguments are concrete, API call is ready for execution
        cout << "[SEE] API call '" << fc.name << "' ready for actual execution"
             << endl;
        return true;
      } else {
        // Built-in function call, always ready
        return true;
      }
    }

    // Not an API call, check if right-hand side is ready
    return isReady(*assign.right, st);
  } else if (s.statementType == StmtType::ASSUME) {
    Assume &assume = dynamic_cast<Assume &>(s);
    return isReady(*assume.expr, st);
  }
  // Add ASSERT handling
  else if (s.statementType == StmtType::ASSERT) {
    Assert &assertStmt = dynamic_cast<Assert &>(s);
    // Handle null or trivial assertions
    if (!assertStmt.expr) {
      return true; // Empty assertion is always ready
    }
    if (assertStmt.expr->exprType == ExprType::NUM) {
      Num *num = dynamic_cast<Num *>(assertStmt.expr.get());
      if (num && num->value == 1) {
        return true; // Trivial assertion is always ready
      }
    }
    return isReady(*assertStmt.expr, st);
  } else if (s.statementType == StmtType::DECL) {
    // Declaration statements are always ready
    return true;
  } else {
    return false;
  }
}

bool SEE::isReady(Expr &e, SymbolTable &st) {
  if (e.exprType == ExprType::FUNC_CALL_EXPR) {
    FuncCall &fc = dynamic_cast<FuncCall &>(e);

    // Special case: input() with no arguments IS ready for symbolic execution
    // It will create a new symbolic variable
    if (fc.name == "input" && fc.args.size() == 0) {
      return true;
    }

    // Check if this is an API call
    if (isAPI(fc)) {
      // API calls need all arguments to be concrete (not symbolic)
      // Check if any argument is symbolic
      for (unsigned int i = 0; i < fc.args.size(); i++) {
        if (isSymbolic(*fc.args[i], st)) {
          return false; // Not ready - has symbolic arguments
        }
      }
      return true; // All arguments are concrete
    }

    // Built-in functions (Add, Gt, etc.) are always ready
    // They can work with symbolic arguments
    return true;
  }
  if (e.exprType == ExprType::MAP) {
    Map &map = dynamic_cast<Map &>(e);
    for (unsigned int i = 0; i < map.value.size(); i++) {
      if (isReady(*map.value[i].second, st) == false) {
        return false;
      }
    }
    return true;
  }
  if (e.exprType == ExprType::NUM) {
    return true;
  }
  if (e.exprType == ExprType::SET) {
    Set &set = dynamic_cast<Set &>(e);
    for (unsigned int i = 0; i < set.elements.size(); i++) {
      if (isReady(*set.elements[i], st) == false) {
        return false;
      }
    }
    return true;
  }
  if (e.exprType == ExprType::STRING) {
    return true;
  }
  if (e.exprType == ExprType::SYMVAR) {
    // Symbolic variables are ready (they're the result of evaluation)
    return false;
  }

  //  Boolean expression types
  if (e.exprType == ExprType::BOOL) {
    return true; // Bool is always ready
  }

  if (e.exprType == ExprType::TUPLE) {
    Tuple &tuple = dynamic_cast<Tuple &>(e);
    for (unsigned int i = 0; i < tuple.exprs.size(); i++) {
      if (isReady(*tuple.exprs[i], st) == false) {
        return false;
      }
    }
    return true;
  }
  if (e.exprType == ExprType::VAR) {
    // Variables are ready if they're bound in sigma AND their value is concrete
    Var &var = dynamic_cast<Var &>(e);

    // First, try direct lookup
    if (sigma.hasValue(var.name)) {
      Expr *val = sigma.getValue(var.name);
      if (isSymbolic(*val, st)) {
        return false;
      }
      return true;
    }

    // Check if this is a base name with a mapped suffixed name
    auto it = baseNameToSuffixed.find(var.name);
    if (it != baseNameToSuffixed.end()) {
      string suffixedName = it->second;
      if (sigma.hasValue(suffixedName)) {
        Expr *val = sigma.getValue(suffixedName);
        if (isSymbolic(*val, st)) {
          return false;
        }
        return true;
      }
    }

    // Variable not found anywhere - not ready
    return false;
  } else {
    return false;
  }
}

bool SEE::isAPI(const FuncCall &fc) {
  // Built-in functions that are NOT API calls
  static const set<string> builtInFunctions = {
      // Arithmetic
      "add", "sub", "mul", "div",
      // Comparison
      "equals", "lt", "gt", "le", "ge", "not_equals", "=", "==", "!=", "<", ">",
      "<=", ">=",
      // Logical
      "implies", "and", "or", "not", "&&", "||", "!",
      // Input
      "input",
      // Set operations
      "in", "not_in", "member", "not_member", "contains", "not_contains",
      "union", "intersection", "intersect", "difference", "diff", "minus",
      "subset", "is_subset", "add_to_set", "remove_from_set", "is_empty_set",
      // Map operations
      "get", "put", "lookup", "select", "store", "update", "contains_key",
      "has_key", "dom", "range",
      // List/Sequence operations
      "concat", "append_list", "length", "at", "nth", "prefix", "suffix",
      "contains_seq",
      // Prime notation (for postconditions)
      "'", "primed"};

  return builtInFunctions.find(fc.name) == builtInFunctions.end();
}

bool SEE::isSymbolic(Expr &e, SymbolTable &st) {
  if (e.exprType == ExprType::SYMVAR) {
    return true;
  }
  // Boolean expression types
  else if (e.exprType == ExprType::BOOL) {
    return false; // Bool is concrete
  } else if (e.exprType == ExprType::FUNC_CALL_EXPR) {
    FuncCall &fc = dynamic_cast<FuncCall &>(e);
    for (unsigned int i = 0; i < fc.args.size(); i++) {
      if (isSymbolic(*fc.args[i], st) == true) {
        return true;
      }
    }
    return false;
  } else if (e.exprType == ExprType::MAP) {
    Map &map = dynamic_cast<Map &>(e);
    for (unsigned int i = 0; i < map.value.size(); i++) {
      if (isSymbolic(*map.value[i].second, st) == true) {
        return true;
      }
    }
    return false;
  } else if (e.exprType == ExprType::NUM) {
    return false;
  } else if (e.exprType == ExprType::SET) {
    Set &set = dynamic_cast<Set &>(e);
    for (unsigned int i = 0; i < set.elements.size(); i++) {
      if (isSymbolic(*set.elements[i], st) == true) {
        return true;
      }
    }
    return false;
  } else if (e.exprType == ExprType::STRING) {
    return false;
  } else if (e.exprType == ExprType::TUPLE) {
    Tuple &tuple = dynamic_cast<Tuple &>(e);
    for (unsigned int i = 0; i < tuple.exprs.size(); i++) {
      if (isSymbolic(*tuple.exprs[i], st) == true) {
        return true;
      }
    }
    return false;
  } else if (e.exprType == ExprType::VAR) {
    Var &var = dynamic_cast<Var &>(e);

    // First, try direct lookup
    if (sigma.hasValue(var.name)) {
      Expr *val = sigma.getValue(var.name);
      return isSymbolic(*val, st);
    }

    // NEW: Check if this is a base name with a mapped suffixed name
    auto it = baseNameToSuffixed.find(var.name);
    if (it != baseNameToSuffixed.end()) {
      string suffixedName = it->second;
      if (sigma.hasValue(suffixedName)) {
        Expr *val = sigma.getValue(suffixedName);
        return isSymbolic(*val, st);
      }
    }

    return false;
  } else {
    return false;
  }
}

// Symbolic Execution function following the algorithm:
// function symex([s1, s2, ..., sn], σ)
//   C ← []
//   for i in 1..n do
//     if ¬isReady(si, σ) then
//       return ⟨C, [si, ..., sn], σ⟩  // Interrupt, return current state
//     end if
//     Execute si, updating σ and possibly adding to C
//   end for
//   return ⟨C, [], σ⟩  // All statements executed
void SEE::execute(Program &program, SymbolTable &st) {
  pathConstraint.clear();

  // Add initial constraint: true (represented as Num(1))
  pathConstraint.push_back(new Bool(true));

  for (size_t i = 0; i < program.statements.size(); i++) {
    Stmt &s = *program.statements[i];

    // Check if statement is ready for execution
    if (!isReady(s, st)) {
      cout << "[SEE] Interruption at statement " << i << endl;
      // In a full implementation, we would return here and resume later
      // For now, we'll continue to try to execute what we can
    }

    // Execute the statement
    executeStmt(s, st);
  }

  // Print the path constraint
  unique_ptr<Expr> pc = computePathConstraint();
  cout << "\n[SEE] Path Constraint: " << pc->toString() << endl;
}

void SEE::executeStmt(Stmt &s, SymbolTable &st) {
  if (s.statementType == StmtType::ASSIGN) {
    Assign &assign = dynamic_cast<Assign &>(s);

    // Get the variable name from left side
    string varName;
    if (assign.left->exprType == ExprType::VAR) {
      varName = dynamic_cast<Var &>(*assign.left).name;
    } else {
      cout << "[ASSIGN] Error: left side is not a variable" << endl;
      return;
    }

    cout << "\n[ASSIGN] Evaluating: " << varName
         << " := " << assign.right->toString() << endl;

    // Track base name to suffixed name mapping
    string baseName = extractBaseName(varName);
    if (baseName != varName) {
      cout << "[SEE] Mapping base name '" << baseName << "' -> '" << varName
           << "'" << endl;
      baseNameToSuffixed[baseName] = varName;
    }

    // Check if right side is an API call
    if (assign.right->exprType == ExprType::FUNC_CALL_EXPR) {
      FuncCall &fc = dynamic_cast<FuncCall &>(*assign.right);

      if (isAPI(fc)) {
        // Evaluate all arguments first
        vector<Expr *> evaluatedArgs;
        bool hasSymbolicArgs = false;

        for (size_t i = 0; i < fc.args.size(); i++) {
          Expr *argResult = evaluateExpr(*fc.args[i], st);
          evaluatedArgs.push_back(argResult);

          // Check if this argument is symbolic
          if (isSymbolic(*argResult, st)) {
            hasSymbolicArgs = true;
          }
        }

        if (hasSymbolicArgs) {
          // API call with symbolic arguments - DON'T execute, store symbolic
          // result
          cout << "[API_CALL] " << fc.name
               << " has symbolic arguments - skipping actual execution" << endl;
          for (size_t i = 0; i < evaluatedArgs.size(); i++) {
            cout << "  [API_ARG " << i << "] " << evaluatedArgs[i]->toString()
                 << " (symbolic: " << isSymbolic(*evaluatedArgs[i], st) << ")"
                 << endl;
          }

          // Store a symbolic placeholder
          // The actual execution will happen in a later pass with concrete
          // values
          sigma.setValue(varName, new Num(-1)); // Placeholder
          cout << "[ASSIGN] Result: " << varName
               << " := -1 (symbolic placeholder)" << endl;
          return;
        }

        // All arguments are concrete - execute the API call
        cout << "[API_CALL] Executing API function: " << fc.name << endl;
        for (size_t i = 0; i < evaluatedArgs.size(); i++) {
          cout << "  [API_ARG] " << evaluatedArgs[i]->toString() << endl;
        }

        // Get the function from the factory
        cout << "  [API_CALL] Getting function from factory..." << endl;
        auto func = functionFactory->getFunction(fc.name, evaluatedArgs);

        if (func) {
          cout << "  [API_CALL] Executing function..." << endl;
          unique_ptr<Expr> resultExpr = func->execute();
          Expr *result = resultExpr.release();
          cout << "  [API_CALL] Function returned: " << result->toString()
               << endl;

          // Store result in sigma
          cout << "  [API_CALL] Storing result in variable: " << varName
               << endl;
          sigma.setValue(varName, result);
          cout << "[ASSIGN] Result: " << varName << " := " << result->toString()
               << endl;
        } else {
          cout << "  [API_CALL] Warning: No function found for " << fc.name
               << endl;
          // Store a placeholder
          sigma.setValue(varName, new Num(-1));
        }
        return;
      }
    }

    // Not an API call, evaluate normally
    Expr *result = evaluateExpr(*assign.right, st);
    sigma.setValue(varName, result);
    cout << "[ASSIGN] Result: " << varName << " := " << result->toString()
         << endl;
  } else if (s.statementType == StmtType::ASSUME) {
    Assume &assume = dynamic_cast<Assume &>(s);
    cout << "\n[ASSUME] Evaluating: " << assume.expr->toString() << endl;

    Expr *result = evaluateExpr(*assume.expr, st);
    cout << "[ASSUME] Adding constraint: " << result->toString() << endl;
    pathConstraint.push_back(result);
  } else if (s.statementType == StmtType::ASSERT) {
    Assert &assertStmt = dynamic_cast<Assert &>(s);

    // Handle empty assertions
    if (!assertStmt.expr) {
      cout << "\n[ASSERT] Empty assertion, PASSED" << endl;
      return;
    }

    cout << "\n[ASSERT] Evaluating: " << assertStmt.expr->toString() << endl;

    Expr *result = evaluateExpr(*assertStmt.expr, st);
    cout << "[ASSERT] Result: " << result->toString() << endl;

    // Check if the assertion is concrete
    if (result->exprType == ExprType::BOOL) {
      Bool *bc = dynamic_cast<Bool *>(result);
      if (bc->value) {
        cout << "[ASSERT] ✓ Assertion PASSED" << endl;
      } else {
        cout << "[ASSERT] ✗ Assertion FAILED" << endl;
      }
    } else if (result->exprType == ExprType::NUM) {
      Num *num = dynamic_cast<Num *>(result);
      if (num->value == 1) {
        cout << "[ASSERT] ✓ Assertion PASSED (numeric true)" << endl;
      } else if (num->value == 0) {
        cout << "[ASSERT] ✗ Assertion FAILED (numeric false)" << endl;
      } else {
        cout << "[ASSERT] Adding to path constraints (not fully concrete)"
             << endl;
        pathConstraint.push_back(result);
      }
    } else {
      // Symbolic assertion - add to path constraints
      cout << "[ASSERT] Adding to path constraints (not fully concrete)"
           << endl;
      pathConstraint.push_back(result);
    }
  }
}

// Helper to find a key from a map variable in sigma
string SEE::findKeyFromMapInSigma(const string &prefix) {
  cout << "    [findKeyFromMapInSigma] Searching for prefix: " << prefix
       << endl;

  // Search through sigma for variables starting with prefix
  // Look for the highest-numbered one (most recent)
  int maxNum = -1;
  string bestKey = "";

  for (auto &entry : sigma.getAllEntries()) {
    const string &varName = entry.first;

    // Check if this variable starts with prefix
    if (varName.compare(0, prefix.length(), prefix) == 0) {
      // Extract the number suffix
      string numStr = varName.substr(prefix.length());
      int num = -1;
      try {
        num = stoi(numStr);
      } catch (...) {
        continue;
      }

      // Check if this is a Map
      Expr *value = entry.second;
      if (value && value->exprType == ExprType::MAP) {
        Map *mapExpr = dynamic_cast<Map *>(value);
        if (mapExpr && !mapExpr->value.empty()) {
          // Found a non-empty map, update if it's the highest number
          if (num > maxNum) {
            maxNum = num;
            // Get the first key from this map
            bestKey = mapExpr->value[0].first->name;
            cout << "    [findKeyFromMapInSigma] Found " << varName << " with "
                 << mapExpr->value.size() << " entries, key: " << bestKey
                 << endl;
          }
        } else {
          cout << "    [findKeyFromMapInSigma] " << varName << " is empty map"
               << endl;
        }
      }
    }
  }

  if (bestKey.empty()) {
    cout << "    [findKeyFromMapInSigma] No non-empty map found for " << prefix
         << endl;
  }

  return bestKey;
}

string SEE::findRestaurantIdFromSigma() {
  return findKeyFromMapInSigma("tmp_R_");
}

string SEE::findMenuItemIdFromSigma() {
  return findKeyFromMapInSigma("tmp_M_");
}

string SEE::findOrderIdFromSigma() { return findKeyFromMapInSigma("tmp_O_"); }

string SEE::findProductIdFromSigma() { return findKeyFromMapInSigma("tmp_P_"); }

string SEE::findCartIdFromSigma() { return findKeyFromMapInSigma("tmp_C_"); }

string SEE::findReviewIdFromSigma() {
  return findKeyFromMapInSigma("tmp_Rev_");
}

string SEE::findOrderStatusFromSigma() {
  return findKeyFromMapInSigma("tmp_OrderStatus_");
}
// restaurant webapp
string SEE::findSellersFromSigma() {
  return findKeyFromMapInSigma("tmp_Sellers_");
}

string SEE::findStockFromSigma() { return findKeyFromMapInSigma("tmp_Stock_"); }
// ========================================
// LIBRARY ID RESOLUTION HELPERS
// ========================================

string SEE::findBookCodeFromSigma() { return findKeyFromMapInSigma("tmp_B_"); }

string SEE::findStudentIdFromSigma() { return findKeyFromMapInSigma("tmp_S_"); }

string SEE::findRequestIdFromSigma() {
  return findKeyFromMapInSigma("tmp_Req_");
}

string SEE::findLoanIdFromSigma() {
  return findKeyFromMapInSigma("tmp_Loans_");
}
Expr *SEE::evaluateExpr(Expr &expr, SymbolTable &st) {
  if (expr.exprType == ExprType::FUNC_CALL_EXPR) {
    FuncCall &fc = dynamic_cast<FuncCall &>(expr);
    cout << "  [EVAL] FuncCall: " << fc.name << " with " << fc.args.size()
         << " args" << endl;

    // Handle input() - creates a new symbolic variable
    if (fc.name == "input" && fc.args.size() == 0) {
      static int symVarCounter = 0;
      SymVar *sv = new SymVar(symVarCounter++);
      cout << "    [EVAL] input() returns new symbolic variable: X"
           << sv->getNum() << endl;
      return sv;
    }

    // Handle dom() - extract domain of a map
    if (fc.name == "dom" && fc.args.size() == 1) {
      cout << "    [EVAL] Map domain: dom" << endl;
      Expr *mapExpr = evaluateExpr(*fc.args[0], st);

      if (mapExpr->exprType == ExprType::MAP) {
        Map *map = dynamic_cast<Map *>(mapExpr);
        cout << "    [EVAL] Map expr evaluated: " << mapExpr->toString()
             << endl;
        cout << "    [EVAL] Domain has " << map->value.size() << " keys"
             << endl;

        // Create a set with the domain keys
        vector<unique_ptr<Expr>> domainElements;
        for (size_t i = 0; i < map->value.size(); i++) {
          // The key is a Var, convert to String for the domain
          String *keyStr = new String(map->value[i].first->name);
          domainElements.push_back(unique_ptr<Expr>(keyStr));
          cout << "    [EVAL] Element: " << keyStr->toString() << endl;
        }

        Set *domainSet = new Set(std::move(domainElements));
        cout << "    [EVAL] Set: " << domainSet->toString() << endl;
        return domainSet;
      }

      // If not a map, return empty set
      cout << "    [EVAL] Not a map, returning empty set" << endl;
      return new Set(vector<unique_ptr<Expr>>());
    }

    // Handle in() - set membership
    if (fc.name == "in" && fc.args.size() == 2) {
      cout << "    [EVAL] Set membership: in" << endl;
      Expr *element = evaluateExpr(*fc.args[0], st);
      Expr *setExpr = evaluateExpr(*fc.args[1], st);

      if (setExpr->exprType == ExprType::SET) {
        Set *set = dynamic_cast<Set *>(setExpr);

        // Get element value as string for comparison
        string elemStr;
        if (element->exprType == ExprType::STRING) {
          elemStr = dynamic_cast<String *>(element)->value;
        } else if (element->exprType == ExprType::NUM) {
          elemStr = to_string(dynamic_cast<Num *>(element)->value);
        } else if (element->exprType == ExprType::VAR) {
          elemStr = dynamic_cast<Var *>(element)->name;
        } else {
          elemStr = element->toString();
        }

        cout << "    [EVAL] Element: " << element->toString() << endl;
        cout << "    [EVAL] Set: " << setExpr->toString() << endl;

        // Check if element is in the set
        for (size_t i = 0; i < set->elements.size(); i++) {
          string setElemStr;
          Expr *setElem = set->elements[i].get();
          if (setElem->exprType == ExprType::STRING) {
            setElemStr = dynamic_cast<String *>(setElem)->value;
          } else if (setElem->exprType == ExprType::NUM) {
            setElemStr = to_string(dynamic_cast<Num *>(setElem)->value);
          } else {
            setElemStr = setElem->toString();
          }

          if (elemStr == setElemStr) {
            cout << "    [EVAL] Element found in set: true" << endl;
            return new Bool(true);
          }
        }

        cout << "    [EVAL] Element not found in set: false" << endl;
        return new Bool(false);
      }

      // If not a set, return symbolic
      cout << "    [EVAL] Not a set, returning symbolic" << endl;
      vector<unique_ptr<Expr>> args;
      args.push_back(element->clone());
      args.push_back(setExpr->clone());
      return new FuncCall("in", std::move(args));
    }

    // Handle not_in() - set non-membership
    if (fc.name == "not_in" && fc.args.size() == 2) {
      cout << "    [EVAL] Set non-membership: not_in" << endl;
      Expr *element = evaluateExpr(*fc.args[0], st);
      Expr *setExpr = evaluateExpr(*fc.args[1], st);

      if (setExpr->exprType == ExprType::SET) {
        Set *set = dynamic_cast<Set *>(setExpr);

        // Get element value as string for comparison
        string elemStr;
        if (element->exprType == ExprType::STRING) {
          elemStr = dynamic_cast<String *>(element)->value;
        } else if (element->exprType == ExprType::NUM) {
          elemStr = to_string(dynamic_cast<Num *>(element)->value);
        } else if (element->exprType == ExprType::VAR) {
          elemStr = dynamic_cast<Var *>(element)->name;
        } else {
          elemStr = element->toString();
        }

        // Check if element is NOT in the set
        for (size_t i = 0; i < set->elements.size(); i++) {
          string setElemStr;
          Expr *setElem = set->elements[i].get();
          if (setElem->exprType == ExprType::STRING) {
            setElemStr = dynamic_cast<String *>(setElem)->value;
          } else if (setElem->exprType == ExprType::NUM) {
            setElemStr = to_string(dynamic_cast<Num *>(setElem)->value);
          } else {
            setElemStr = setElem->toString();
          }

          if (elemStr == setElemStr) {
            cout << "    [EVAL] not_in result: false (element found)" << endl;
            return new Bool(false);
          }
        }

        cout << "    [EVAL] not_in result: true (element not found)" << endl;
        return new Bool(true);
      }

      // If not a set, return symbolic
      vector<unique_ptr<Expr>> args;
      args.push_back(element->clone());
      args.push_back(setExpr->clone());
      return new FuncCall("not_in", std::move(args));
    }

    // Handle [] (map access)
    if (fc.name == "[]" && fc.args.size() == 2) {
      cout << "    [EVAL] Map access: []" << endl;
      Expr *mapExpr = evaluateExpr(*fc.args[0], st);
      Expr *keyExpr = evaluateExpr(*fc.args[1], st);

      if (mapExpr->exprType == ExprType::MAP) {
        Map *map = dynamic_cast<Map *>(mapExpr);
        cout << "    [EVAL] Map expr evaluated: " << mapExpr->toString()
             << endl;

        // Get key as string for comparison
        string keyStr;
        if (keyExpr->exprType == ExprType::STRING) {
          keyStr = dynamic_cast<String *>(keyExpr)->value;
        } else if (keyExpr->exprType == ExprType::NUM) {
          keyStr = to_string(dynamic_cast<Num *>(keyExpr)->value);
        } else if (keyExpr->exprType == ExprType::VAR) {
          keyStr = dynamic_cast<Var *>(keyExpr)->name;
        } else {
          keyStr = keyExpr->toString();
        }
        cout << "    [EVAL] Key expr evaluated: " << keyExpr->toString()
             << endl;

        // Find the key in the map
        for (size_t i = 0; i < map->value.size(); i++) {
          if (map->value[i].first->name == keyStr) {
            cout << "    [EVAL] Key found in map, returning value" << endl;
            return evaluateExpr(*map->value[i].second, st);
          }
        }

        cout << "    [EVAL] Key not found in map" << endl;
      }

      // Return symbolic if not found or not a map
      vector<unique_ptr<Expr>> args;
      args.push_back(mapExpr->clone());
      args.push_back(keyExpr->clone());
      return new FuncCall("[]", std::move(args));
    }

    // Handle = (equality)
    if ((fc.name == "=" || fc.name == "equals" || fc.name == "==") &&
        fc.args.size() == 2) {
      cout << "    [EVAL] Equality: Eq" << endl;
      Expr *left = evaluateExpr(*fc.args[0], st);
      Expr *right = evaluateExpr(*fc.args[1], st);

      // Compare concrete values
      if (left->exprType == ExprType::STRING &&
          right->exprType == ExprType::STRING) {
        bool result = dynamic_cast<String *>(left)->value ==
                      dynamic_cast<String *>(right)->value;
        cout << "    [EVAL] Eq result: " << (result ? "true" : "false") << endl;
        return new Bool(result);
      }
      if (left->exprType == ExprType::NUM && right->exprType == ExprType::NUM) {
        bool result = dynamic_cast<Num *>(left)->value ==
                      dynamic_cast<Num *>(right)->value;
        cout << "    [EVAL] Eq result: " << (result ? "true" : "false") << endl;
        return new Bool(result);
      }
      if (left->exprType == ExprType::BOOL &&
          right->exprType == ExprType::BOOL) {
        bool result = dynamic_cast<Bool *>(left)->value ==
                      dynamic_cast<Bool *>(right)->value;
        cout << "    [EVAL] Eq result: " << (result ? "true" : "false") << endl;
        return new Bool(result);
      }

      // Return symbolic comparison
      cout << "    [EVAL] Symbolic Eq, returning BinaryOpExpr(EQ)" << endl;
      std::vector<unique_ptr<Expr>> args;
      args.push_back(left->clone());
      args.push_back(right->clone());
      return new FuncCall("equals", std::move(args));
    }

    // Handle AND (n-ary)
    if ((fc.name == "AND" || fc.name == "And" || fc.name == "and" ||
         fc.name == "&&")) {
      cout << "    [EVAL] N-ary AND with " << fc.args.size() << " args" << endl;

      bool allTrue = true;
      bool anyFalse = false;
      vector<Expr *> evaluatedArgs;

      for (size_t i = 0; i < fc.args.size(); i++) {
        cout << "    [EVAL] Arg[" << i << "]: " << fc.args[i]->toString()
             << endl;
        Expr *argResult = evaluateExpr(*fc.args[i], st);
        evaluatedArgs.push_back(argResult);
        cout << "    [EVAL] Arg[" << i << "] result: " << argResult->toString()
             << endl;

        if (argResult->exprType == ExprType::BOOL) {
          Bool *bc = dynamic_cast<Bool *>(argResult);
          if (!bc->value) {
            anyFalse = true;
          }
        } else {
          allTrue = false; // Can't be fully concrete
        }
      }

      if (anyFalse) {
        cout << "    [EVAL] FuncCall result: AND(...) = false (short-circuit)"
             << endl;
        return new Bool(false);
      }

      if (allTrue && evaluatedArgs.size() > 0) {
        // Check if all are Bool(true)
        bool reallyAllTrue = true;
        for (auto &arg : evaluatedArgs) {
          if (arg->exprType != ExprType::BOOL ||
              !dynamic_cast<Bool *>(arg)->value) {
            reallyAllTrue = false;
            break;
          }
        }
        if (reallyAllTrue) {
          cout << "    [EVAL] FuncCall result: AND(...) = true" << endl;
          return new Bool(true);
        }
      }

      // Build symbolic AND
      string resultStr = "AND(";
      for (size_t i = 0; i < evaluatedArgs.size(); i++) {
        if (i > 0)
          resultStr += ", ";
        resultStr += evaluatedArgs[i]->toString();
      }
      resultStr += ")";
      cout << "    [EVAL] FuncCall result: " << resultStr << endl;

      vector<unique_ptr<Expr>> clonedArgs;
      for (auto &arg : evaluatedArgs) {
        clonedArgs.push_back(arg->clone());
      }
      return new FuncCall("AND", std::move(clonedArgs));
    }

    if ((fc.name == "And" || fc.name == "and" || fc.name == "&&") &&
        fc.args.size() == 2) {
      cout << "    [EVAL] Logical AND" << endl;

      Expr *left = evaluateExpr(*fc.args[0], st);
      Expr *right = evaluateExpr(*fc.args[1], st);

      // If both are Bool, evaluate
      if (left->exprType == ExprType::BOOL &&
          right->exprType == ExprType::BOOL) {
        bool result = dynamic_cast<Bool *>(left)->value &&
                      dynamic_cast<Bool *>(right)->value;
        cout << "    [EVAL] And result: " << (result ? "true" : "false")
             << endl;
        return new Bool(result);
      }

      cout << "    [EVAL] Symbolic And, returning BinaryOpExpr(AND)" << endl;
      std::vector<unique_ptr<Expr>> args;
      args.push_back(left->clone());
      args.push_back(right->clone());
      return new FuncCall("and", std::move(args));
    }

    if ((fc.name == "Or" || fc.name == "or" || fc.name == "||") &&
        fc.args.size() == 2) {
      cout << "    [EVAL] Logical OR" << endl;

      Expr *left = evaluateExpr(*fc.args[0], st);
      Expr *right = evaluateExpr(*fc.args[1], st);

      // If both are Bool, evaluate
      if (left->exprType == ExprType::BOOL &&
          right->exprType == ExprType::BOOL) {
        bool result = dynamic_cast<Bool *>(left)->value ||
                      dynamic_cast<Bool *>(right)->value;
        cout << "    [EVAL] Or result: " << (result ? "true" : "false") << endl;
        return new Bool(result);
      }

      cout << "    [EVAL] Symbolic Or, returning BinaryOpExpr(OR)" << endl;
      std::vector<unique_ptr<Expr>> args;
      args.push_back(left->clone());
      args.push_back(right->clone());
      return new FuncCall("or", std::move(args));
    }

    if ((fc.name == "Not" || fc.name == "not" || fc.name == "!") &&
        fc.args.size() == 1) {
      cout << "    [EVAL] Logical NOT" << endl;

      Expr *operand = evaluateExpr(*fc.args[0], st);

      // If Bool, evaluate
      if (operand->exprType == ExprType::BOOL) {
        bool result = !dynamic_cast<Bool *>(operand)->value;
        cout << "    [EVAL] Not result: " << (result ? "true" : "false")
             << endl;
        return new Bool(result);
      }

      cout << "    [EVAL] Symbolic Not, returning UnaryOpExpr(NOT)" << endl;
      std::vector<unique_ptr<Expr>> args;
      args.push_back(operand->clone());
      return new FuncCall("not", std::move(args));
    }

    // ================================================================
    // END OF MAP OPERATIONS

    // Evaluate all arguments
    vector<unique_ptr<Expr>> evaluatedArgs;
    for (size_t i = 0; i < fc.args.size(); i++) {
      cout << "    [EVAL] Arg[" << i << "]: " << fc.args[i]->toString() << endl;
      Expr *argResult = evaluateExpr(*fc.args[i], st);
      cout << "    [EVAL] Arg[" << i << "] result: " << argResult->toString()
           << endl;
      evaluatedArgs.push_back(argResult->clone());
    }

    FuncCall *result = new FuncCall(fc.name, ::move(evaluatedArgs));
    cout << "    [EVAL] FuncCall result: " << result->toString() << endl;

    return result;
  } else if (expr.exprType == ExprType::NUM) {
    Num *result = new Num(dynamic_cast<Num &>(expr).value);
    cout << "  [EVAL] Num: " << result->toString() << endl;
    return result;
  } else if (expr.exprType == ExprType::STRING) {
    String *result = new String(dynamic_cast<String &>(expr).value);
    cout << "  [EVAL] String: " << result->toString() << endl;
    return result;
  } else if (expr.exprType == ExprType::SYMVAR) {
    // Return the symbolic variable as-is
    cout << "  [EVAL] SymVar: " << expr.toString() << endl;
    return &expr;
  } else if (expr.exprType == ExprType::VAR) {
    Var &v = dynamic_cast<Var &>(expr);
    cout << "  [EVAL] Var lookup: " << v.name << endl;

    // First, try direct lookup
    if (sigma.hasValue(v.name)) {
      Expr *value = sigma.getValue(v.name);

      // Check if the value is a placeholder that needs runtime resolution
      if (value->exprType == ExprType::STRING) {
        String *strVal = dynamic_cast<String *>(value);
        if (strVal->value == "__NEEDS_RESTAURANT_ID__") {
          cout << "    [EVAL] Found placeholder __NEEDS_RESTAURANT_ID__, "
                  "attempting runtime resolution"
               << endl;
          string resolvedId = findRestaurantIdFromSigma();
          if (!resolvedId.empty()) {
            cout << "    [EVAL] Resolved to: " << resolvedId << endl;
            String *resolved = new String(resolvedId);
            sigma.setValue(v.name, resolved);
            return resolved;
          }
        } else if (strVal->value == "__NEEDS_MENUITEM_ID__") {
          cout << "    [EVAL] Found placeholder __NEEDS_MENUITEM_ID__, "
                  "attempting runtime resolution"
               << endl;
          string resolvedId = findMenuItemIdFromSigma();
          if (!resolvedId.empty()) {
            cout << "    [EVAL] Resolved to: " << resolvedId << endl;
            String *resolved = new String(resolvedId);
            sigma.setValue(v.name, resolved);
            return resolved;
          }
        } else if (strVal->value == "__NEEDS_ORDER_ID__") {
          cout << "    [EVAL] Found placeholder __NEEDS_ORDER_ID__, attempting "
                  "runtime resolution"
               << endl;
          string resolvedId = findOrderIdFromSigma();
          if (!resolvedId.empty()) {
            cout << "    [EVAL] Resolved to: " << resolvedId << endl;
            String *resolved = new String(resolvedId);
            sigma.setValue(v.name, resolved);
            return resolved;
          }
        }

        else if (strVal->value == "__NEEDS_PRODUCT_ID__") {
          cout << "    [EVAL] Found placeholder __NEEDS_PRODUCT_ID__, "
                  "attempting runtime resolution"
               << endl;
          string resolvedId = findProductIdFromSigma();
          if (!resolvedId.empty()) {
            cout << "    [EVAL] Resolved to: " << resolvedId << endl;
            String *resolved = new String(resolvedId);
            sigma.setValue(v.name, resolved);
            return resolved;
          }
        } else if (strVal->value == "__NEEDS_CART_ID__") {
          cout << "    [EVAL] Found placeholder __NEEDS_CART_ID__, attempting "
                  "runtime resolution"
               << endl;
          string resolvedId = findCartIdFromSigma();
          if (!resolvedId.empty()) {
            cout << "    [EVAL] Resolved to: " << resolvedId << endl;
            String *resolved = new String(resolvedId);
            sigma.setValue(v.name, resolved);
            return resolved;
          }
        } else if (strVal->value == "__NEEDS_REVIEW_ID__") {
          cout << "    [EVAL] Found placeholder __NEEDS_REVIEW_ID__, "
                  "attempting runtime resolution"
               << endl;
          string resolvedId = findReviewIdFromSigma();
          if (!resolvedId.empty()) {
            cout << "    [EVAL] Resolved to: " << resolvedId << endl;
            String *resolved = new String(resolvedId);
            sigma.setValue(v.name, resolved);
            return resolved;
          }
        }
        // ========================================
        // LIBRARY PLACEHOLDER RESOLUTION
        // ========================================
        else if (strVal->value == "__NEEDS_BOOK_CODE__") {
          cout << "    [EVAL] Found placeholder __NEEDS_BOOK_CODE__, "
                  "attempting runtime resolution"
               << endl;
          string resolvedId = findBookCodeFromSigma();
          if (!resolvedId.empty()) {
            cout << "    [EVAL] Resolved to: " << resolvedId << endl;
            String *resolved = new String(resolvedId);
            sigma.setValue(v.name, resolved);
            return resolved;
          }
        } else if (strVal->value == "__NEEDS_STUDENT_ID__") {
          cout << "    [EVAL] Found placeholder __NEEDS_STUDENT_ID__, "
                  "attempting runtime resolution"
               << endl;
          string resolvedId = findStudentIdFromSigma();
          if (!resolvedId.empty()) {
            cout << "    [EVAL] Resolved to: " << resolvedId << endl;
            String *resolved = new String(resolvedId);
            sigma.setValue(v.name, resolved);
            return resolved;
          }
        } else if (strVal->value == "__NEEDS_REQUEST_ID__") {
          cout << "    [EVAL] Found placeholder __NEEDS_REQUEST_ID__, "
                  "attempting runtime resolution"
               << endl;
          string resolvedId = findRequestIdFromSigma();
          if (!resolvedId.empty()) {
            cout << "    [EVAL] Resolved to: " << resolvedId << endl;
            String *resolved = new String(resolvedId);
            sigma.setValue(v.name, resolved);
            return resolved;
          }
        } else if (strVal->value == "__NEEDS_LOAN_ID__") {
          cout << "    [EVAL] Found placeholder __NEEDS_LOAN_ID__, attempting "
                  "runtime resolution"
               << endl;
          string resolvedId = findLoanIdFromSigma();
          if (!resolvedId.empty()) {
            cout << "    [EVAL] Resolved to: " << resolvedId << endl;
            String *resolved = new String(resolvedId);
            sigma.setValue(v.name, resolved);
            return resolved;
          }
        }
      }

      cout << "    [EVAL] Found in sigma: " << value->toString() << endl;
      return value;
    }

    // NEW: If not found, check if this is a base name with a mapped suffixed
    // name
    auto it = baseNameToSuffixed.find(v.name);
    if (it != baseNameToSuffixed.end()) {
      string suffixedName = it->second;
      cout << "    [EVAL] Resolved base name '" << v.name << "' -> '"
           << suffixedName << "'" << endl;

      if (sigma.hasValue(suffixedName)) {
        Expr *value = sigma.getValue(suffixedName);

        // NEW: Check for placeholder in suffixed name too
        if (value->exprType == ExprType::STRING) {
          String *strVal = dynamic_cast<String *>(value);
          if (strVal->value == "__NEEDS_RESTAURANT_ID__") {
            cout << "    [EVAL] Found placeholder __NEEDS_RESTAURANT_ID__, "
                    "attempting runtime resolution"
                 << endl;
            string resolvedId = findRestaurantIdFromSigma();
            if (!resolvedId.empty()) {
              cout << "    [EVAL] Resolved to: " << resolvedId << endl;
              String *resolved = new String(resolvedId);
              sigma.setValue(suffixedName, resolved);
              return resolved;
            }
          } else if (strVal->value == "__NEEDS_MENUITEM_ID__") {
            cout << "    [EVAL] Found placeholder __NEEDS_MENUITEM_ID__, "
                    "attempting runtime resolution"
                 << endl;
            string resolvedId = findMenuItemIdFromSigma();
            if (!resolvedId.empty()) {
              cout << "    [EVAL] Resolved to: " << resolvedId << endl;
              String *resolved = new String(resolvedId);
              sigma.setValue(suffixedName, resolved);
              return resolved;
            }
          } else if (strVal->value == "__NEEDS_ORDER_ID__") {
            cout << "    [EVAL] Found placeholder __NEEDS_ORDER_ID__, "
                    "attempting runtime resolution"
                 << endl;
            string resolvedId = findOrderIdFromSigma();
            if (!resolvedId.empty()) {
              cout << "    [EVAL] Resolved to: " << resolvedId << endl;
              String *resolved = new String(resolvedId);
              sigma.setValue(suffixedName, resolved);
              return resolved;
            }
          } else if (strVal->value == "__NEEDS_BOOK_CODE__") {
            cout << "    [EVAL] Found placeholder __NEEDS_BOOK_CODE__, "
                    "attempting runtime resolution"
                 << endl;
            string resolvedId = findBookCodeFromSigma();
            if (!resolvedId.empty()) {
              cout << "    [EVAL] Resolved to: " << resolvedId << endl;
              String *resolved = new String(resolvedId);
              sigma.setValue(suffixedName, resolved);
              return resolved;
            }
          } else if (strVal->value == "__NEEDS_STUDENT_ID__") {
            cout << "    [EVAL] Found placeholder __NEEDS_STUDENT_ID__, "
                    "attempting runtime resolution"
                 << endl;
            string resolvedId = findStudentIdFromSigma();
            if (!resolvedId.empty()) {
              cout << "    [EVAL] Resolved to: " << resolvedId << endl;
              String *resolved = new String(resolvedId);
              sigma.setValue(suffixedName, resolved);
              return resolved;
            }
          } else if (strVal->value == "__NEEDS_REQUEST_ID__") {
            cout << "    [EVAL] Found placeholder __NEEDS_REQUEST_ID__, "
                    "attempting runtime resolution"
                 << endl;
            string resolvedId = findRequestIdFromSigma();
            if (!resolvedId.empty()) {
              cout << "    [EVAL] Resolved to: " << resolvedId << endl;
              String *resolved = new String(resolvedId);
              sigma.setValue(suffixedName, resolved);
              return resolved;
            }
          } else if (strVal->value == "__NEEDS_LOAN_ID__") {
            cout << "    [EVAL] Found placeholder __NEEDS_LOAN_ID__, "
                    "attempting runtime resolution"
                 << endl;
            string resolvedId = findLoanIdFromSigma();
            if (!resolvedId.empty()) {
              cout << "    [EVAL] Resolved to: " << resolvedId << endl;
              String *resolved = new String(resolvedId);
              sigma.setValue(suffixedName, resolved);
              return resolved;
            }
          }
        }

        cout << "    [EVAL] Found in sigma: " << value->toString() << endl;
        return value;
      }
    }

    cout << "    [EVAL] Not found in sigma, returning as-is" << endl;
    return &expr;
  } else if (expr.exprType == ExprType::SET) {
    // Evaluate each element in the set
    Set &set = dynamic_cast<Set &>(expr);
    cout << "  [EVAL] Set with " << set.elements.size() << " elements" << endl;

    vector<unique_ptr<Expr>> evaluatedElements;
    for (size_t i = 0; i < set.elements.size(); i++) {
      Expr *elemResult = evaluateExpr(*set.elements[i], st);
      evaluatedElements.push_back(elemResult->clone());
    }

    Set *result = new Set(::move(evaluatedElements));
    cout << "    [EVAL] Set result: " << result->toString() << endl;
    return result;
  } else if (expr.exprType == ExprType::MAP) {
    // Evaluate each key-value pair in the map
    Map &map = dynamic_cast<Map &>(expr);
    cout << "  [EVAL] Map with " << map.value.size() << " entries" << endl;

    vector<pair<unique_ptr<Var>, unique_ptr<Expr>>> evaluatedPairs;
    for (size_t i = 0; i < map.value.size(); i++) {
      // Clone the key (Var)
      unique_ptr<Var> keyClone = make_unique<Var>(map.value[i].first->name);
      // Evaluate the value
      Expr *valResult = evaluateExpr(*map.value[i].second, st);
      evaluatedPairs.push_back(make_pair(::move(keyClone), valResult->clone()));
    }

    Map *result = new Map(::move(evaluatedPairs));
    cout << "    [EVAL] Map result: " << result->toString() << endl;
    return result;
  } else if (expr.exprType == ExprType::TUPLE) {
    // Evaluate each element in the tuple
    Tuple &tuple = dynamic_cast<Tuple &>(expr);
    cout << "  [EVAL] Tuple with " << tuple.exprs.size() << " elements" << endl;

    vector<unique_ptr<Expr>> evaluatedExprs;
    for (size_t i = 0; i < tuple.exprs.size(); i++) {
      Expr *elemResult = evaluateExpr(*tuple.exprs[i], st);
      evaluatedExprs.push_back(elemResult->clone());
    }

    Tuple *result = new Tuple(::move(evaluatedExprs));
    cout << "    [EVAL] Tuple result: " << result->toString() << endl;
    return result;
  }

  // Add Boolean expression types
  else if (expr.exprType == ExprType::BOOL) {
    Bool *result = new Bool(dynamic_cast<Bool &>(expr).value);
    cout << "  [EVAL] Bool: " << result->toString() << endl;
    return result;
  }
  // Default case: return the expression as-is
  cout << "  [EVAL] Unknown type, returning as-is" << endl;
  return &expr;
}

// Extract base name: "email0" -> "email", "password1" -> "password"
string SEE::extractBaseName(const string &suffixedName) {
  // Find trailing digits
  size_t i = suffixedName.length();
  while (i > 0 && isdigit(suffixedName[i - 1])) {
    i--;
  }

  if (i == suffixedName.length()) {
    // No trailing digits, return as-is
    return suffixedName;
  }

  return suffixedName.substr(0, i);
}
