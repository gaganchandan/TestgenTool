#include "rewrite_globals_visitor.hh"
#include <iostream>

/* ============================================================
 * CONSTRUCTOR
 * ============================================================ */

RewriteGlobalsVisitor::RewriteGlobalsVisitor() {}

/* ============================================================
 * HELPER: Fresh Temporary Variable
 * ============================================================ */

std::string RewriteGlobalsVisitor::freshTemp(const std::string &globalName) {
  int &counter = tmpCounters[globalName];
  std::string result = "tmp_" + globalName + "_" + std::to_string(counter);
  counter++;
  return result;
}

/* ============================================================
 * HELPER: Detect Init Assignment
 * ============================================================ */

bool RewriteGlobalsVisitor::isInitAssign(const Stmt *stmt) {
  const Assign *as = dynamic_cast<const Assign *>(stmt);
  if (!as)
    return false;

  const Var *lhs = dynamic_cast<const Var *>(as->left.get());
  const Map *rhs = dynamic_cast<const Map *>(as->right.get());

  if (!lhs || !rhs)
    return false;

  // Empty map means initialization
  return rhs->value.empty();
}

/* ============================================================
 * HELPER: Check if expression is a global variable
 * ============================================================ */

bool RewriteGlobalsVisitor::isGlobalVar(const Expr *expr) {
  if (!expr)
    return false;

  const Var *v = dynamic_cast<const Var *>(expr);
  if (!v)
    return false;

  return globals.count(v->name) > 0;
}

/* ============================================================
 * HELPER: Check if expression contains globals
 * ============================================================ */

bool RewriteGlobalsVisitor::containsGlobals(const Expr *expr) {
  if (!expr)
    return false;

  // Check if it's a global variable
  if (isGlobalVar(expr))
    return true;

  // Recursively check function call arguments
  if (const FuncCall *fc = dynamic_cast<const FuncCall *>(expr)) {
    for (const auto &arg : fc->args) {
      if (containsGlobals(arg.get()))
        return true;
    }
  }

  // Check tuple elements
  if (const Tuple *t = dynamic_cast<const Tuple *>(expr)) {
    for (const auto &e : t->exprs) {
      if (containsGlobals(e.get()))
        return true;
    }
  }

  // Check set elements
  if (const Set *s = dynamic_cast<const Set *>(expr)) {
    for (const auto &e : s->elements) {
      if (containsGlobals(e.get()))
        return true;
    }
  }

  // Check map values
  if (const Map *m = dynamic_cast<const Map *>(expr)) {
    for (const auto &kv : m->value) {
      if (containsGlobals(kv.first.get()))
        return true;
      if (containsGlobals(kv.second.get()))
        return true;
    }
  }

  return false;
}

void RewriteGlobalsVisitor::visitBool(const Bool &node) {
  // No-op: actual rewriting happens in rewriteExpr()
}

/* ============================================================
 * MAIN ENTRY POINT: visitProgram
 * ============================================================ */

void RewriteGlobalsVisitor::visitProgram(const Program &p) {
  // STEP 1: Detect globals from init statements
  globals.clear();
  tmpCounters.clear();
  newStmts.clear();

  for (const auto &stmt : p.statements) {
    if (isInitAssign(stmt.get())) {
      const Assign *as = dynamic_cast<const Assign *>(stmt.get());
      const Var *lhs = dynamic_cast<const Var *>(as->left.get());
      globals.insert(lhs->name);
      tmpCounters[lhs->name] = 0; // Initialize counter
    }
  }

  std::cout << "[RewriteGlobalsVisitor] Detected " << globals.size()
            << " globals: ";
  for (const auto &g : globals)
    std::cout << g << " ";
  std::cout << std::endl;

  // STEP 2: Insert reset() call
  {
    std::vector<std::unique_ptr<Expr>> noArgs;
    auto resetCall = std::make_unique<FuncCall>("reset", std::move(noArgs));
    newStmts.push_back(std::make_unique<Assign>(std::make_unique<Var>("_"),
                                                std::move(resetCall)));
  }

  // STEP 3: Process all non-init statements
  for (const auto &stmt : p.statements) {
    // Skip init statements
    if (isInitAssign(stmt.get())) {
      continue;
    }

    // Dispatch to statement rewriters
    this->visit(stmt.get());
  }

  // STEP 4: Create final program
  rewrittenProgram = std::make_unique<Program>(std::move(newStmts));

  std::cout << "[RewriteGlobalsVisitor] Generated " << newStmts.size()
            << " statements in rewritten program" << std::endl;
}

/* ============================================================
 * STATEMENT VISITORS
 * ============================================================ */

void RewriteGlobalsVisitor::visitAssign(const Assign &s) {
  rewriteAssignStmt(s);
}

void RewriteGlobalsVisitor::visitAssume(const Assume &s) {
  // Rewrite the condition expression
  auto result = rewriteExpr(s.expr.get());

  // Add hoisted statements first
  for (auto &stmt : result.hoistedStmts) {
    newStmts.push_back(std::move(stmt));
  }

  // Then add the assume with rewritten condition
  newStmts.push_back(std::make_unique<Assume>(std::move(result.expr)));
}

void RewriteGlobalsVisitor::visitAssert(const Assert &s) {
  // Rewrite the condition expression
  auto result = rewriteExpr(s.expr.get());

  // Add hoisted statements first
  for (auto &stmt : result.hoistedStmts) {
    newStmts.push_back(std::move(stmt));
  }

  // Then add the assert with rewritten condition
  newStmts.push_back(std::make_unique<Assert>(std::move(result.expr)));
}

/* ============================================================
 * ASSIGNMENT REWRITING
 * ============================================================ */

void RewriteGlobalsVisitor::rewriteAssignStmt(const Assign &s) {
  // Detect if this is a global map update
  auto updateInfo = detectMapUpdate(s.left.get(), s.right.get());

  if (updateInfo.isMapUpdate) {
    // Case: G[k] = v
    emitMapUpdate(updateInfo.globalName, std::move(updateInfo.key),
                  std::move(updateInfo.value));
  } else if (!updateInfo.globalName.empty()) {
    // Case: G = expr
    emitMapReplace(updateInfo.globalName, std::move(updateInfo.value));
  } else {
    // Case: Regular assignment (not a global)
    // Rewrite RHS
    auto rhsResult = rewriteExpr(s.right.get());

    // Add hoisted statements
    for (auto &stmt : rhsResult.hoistedStmts) {
      newStmts.push_back(std::move(stmt));
    }

    // Clone LHS
    auto lhsClone = s.left.get()->clone();

    // Add assignment
    newStmts.push_back(std::make_unique<Assign>(std::move(lhsClone),
                                                std::move(rhsResult.expr)));
  }
}

/* ============================================================
 * DETECT MAP UPDATE
 * ============================================================ */

RewriteGlobalsVisitor::MapUpdateInfo
RewriteGlobalsVisitor::detectMapUpdate(const Expr *lhs, const Expr *rhs) {
  MapUpdateInfo info;
  info.globalName = "";
  info.isMapUpdate = false;

  // Case 1: Direct global assignment (G = expr)
  if (const Var *v = dynamic_cast<const Var *>(lhs)) {
    if (globals.count(v->name)) {
      info.globalName = v->name;
      info.isMapUpdate = false;

      // Rewrite RHS
      auto rhsResult = rewriteExpr(rhs);
      info.value = std::move(rhsResult.expr);

      // Hoist statements from RHS rewrite
      // (Will be added by caller)
      // For now, we'll handle this in emitMapReplace

      return info;
    }
  }

  // Case 2: Map index assignment (G[k] = v)
  // This comes as: [] (G, k) on LHS
  if (const FuncCall *fc = dynamic_cast<const FuncCall *>(lhs)) {
    if (fc->name == "[]" && fc->args.size() == 2) {
      const Expr *base = fc->args[0].get();
      const Expr *keyExpr = fc->args[1].get();

      // Check if base is a global
      if (const Var *baseVar = dynamic_cast<const Var *>(base)) {
        if (globals.count(baseVar->name)) {
          info.globalName = baseVar->name;
          info.isMapUpdate = true;

          // Clone key and value

          info.key = const_cast<Expr *>(keyExpr)->clone();
          info.value = const_cast<Expr *>(rhs)->clone();

          return info;
        }
      }
    }
  }

  return info;
}

/* ============================================================
 * EMIT MAP UPDATE: G[k] = v
 * ============================================================ */

void RewriteGlobalsVisitor::emitMapUpdate(const std::string &globalName,
                                          std::unique_ptr<Expr> key,
                                          std::unique_ptr<Expr> value) {
  std::string tmpName = freshTemp(globalName);

  // STEP 1: tmp := get_G()
  {
    std::vector<std::unique_ptr<Expr>> noArgs;
    auto getCall =
        std::make_unique<FuncCall>("get_" + globalName, std::move(noArgs));

    newStmts.push_back(std::make_unique<Assign>(std::make_unique<Var>(tmpName),
                                                std::move(getCall)));
  }

  // STEP 2: tmp[k] := v
  {
    std::vector<std::unique_ptr<Expr>> indexArgs;
    indexArgs.push_back(std::make_unique<Var>(tmpName));
    indexArgs.push_back(std::move(key));

    auto indexExpr = std::make_unique<FuncCall>("[]", std::move(indexArgs));

    newStmts.push_back(
        std::make_unique<Assign>(std::move(indexExpr), std::move(value)));
  }

  // STEP 3: _ := set_G(tmp)
  {
    std::vector<std::unique_ptr<Expr>> setArgs;
    setArgs.push_back(std::make_unique<Var>(tmpName));

    auto setCall =
        std::make_unique<FuncCall>("set_" + globalName, std::move(setArgs));

    newStmts.push_back(std::make_unique<Assign>(std::make_unique<Var>("_"),
                                                std::move(setCall)));
  }

  std::cout << "[RewriteGlobalsVisitor] Emitted map update for " << globalName
            << std::endl;
}

/* ============================================================
 * EMIT MAP REPLACE: G = expr
 * ============================================================ */

void RewriteGlobalsVisitor::emitMapReplace(const std::string &globalName,
                                           std::unique_ptr<Expr> expr) {
  // Rewrite the expression first
  auto exprResult = rewriteExpr(expr.get());

  // Add hoisted statements
  for (auto &stmt : exprResult.hoistedStmts) {
    newStmts.push_back(std::move(stmt));
  }

  // _ := set_G(expr')
  std::vector<std::unique_ptr<Expr>> setArgs;
  setArgs.push_back(std::move(exprResult.expr));

  auto setCall =
      std::make_unique<FuncCall>("set_" + globalName, std::move(setArgs));

  newStmts.push_back(
      std::make_unique<Assign>(std::make_unique<Var>("_"), std::move(setCall)));

  std::cout << "[RewriteGlobalsVisitor] Emitted map replace for " << globalName
            << std::endl;
}

/* ============================================================
 * EXPRESSION REWRITING (WITH HOISTING)
 * ============================================================ */

RewriteGlobalsVisitor::RewriteResult
RewriteGlobalsVisitor::rewriteExpr(const Expr *e) {
  if (!e) {
    RewriteResult result;
    result.expr = nullptr;
    return result;
  }

  // Dispatch based on expression type
  if (auto v = dynamic_cast<const Var *>(e)) {
    return rewriteVar(v);
  }
  if (auto f = dynamic_cast<const FuncCall *>(e)) {
    return rewriteFuncCall(f);
  }
  if (auto n = dynamic_cast<const Num *>(e)) {
    return rewriteNum(n);
  }
  if (auto s = dynamic_cast<const String *>(e)) {
    return rewriteString(s);
  }
  if (auto t = dynamic_cast<const Tuple *>(e)) {
    return rewriteTuple(t);
  }
  if (auto s = dynamic_cast<const Set *>(e)) {
    return rewriteSet(s);
  }
  if (auto m = dynamic_cast<const Map *>(e)) {
    return rewriteMap(m);
  }
  if (auto bc = dynamic_cast<const Bool *>(e)) {
    return rewriteBool(bc);
  }

  // Default: clone as-is

  RewriteResult result;
  result.expr = const_cast<Expr *>(e)->clone();
  return result;
}

/* ============================================================
 * REWRITE VAR
 * ============================================================ */

RewriteGlobalsVisitor::RewriteResult
RewriteGlobalsVisitor::rewriteVar(const Var *v) {
  RewriteResult result;

  // If it's a global, hoist get_G() call
  if (globals.count(v->name)) {
    std::string tmpName = freshTemp(v->name);

    // Hoist: tmp := get_G()
    std::vector<std::unique_ptr<Expr>> noArgs;
    auto getCall =
        std::make_unique<FuncCall>("get_" + v->name, std::move(noArgs));

    result.hoistedStmts.push_back(std::make_unique<Assign>(
        std::make_unique<Var>(tmpName), std::move(getCall)));

    // Return tmp
    result.expr = std::make_unique<Var>(tmpName);
  } else {
    // Not a global, clone as-is
    result.expr = std::make_unique<Var>(v->name);
  }

  return result;
}

/* ============================================================
 * REWRITE FUNCCALL
 * ============================================================ */

RewriteGlobalsVisitor::RewriteResult
RewriteGlobalsVisitor::rewriteFuncCall(const FuncCall *f) {
  // Special cases
  if (f->name == "[]") {
    return rewriteMapAccess(f);
  }
  if (f->name == "dom") {
    return rewriteDom(f);
  }

  // General function call: rewrite all arguments
  RewriteResult result;
  std::vector<std::unique_ptr<Expr>> newArgs;

  for (const auto &arg : f->args) {
    auto argResult = rewriteExpr(arg.get());

    // Collect hoisted statements
    for (auto &stmt : argResult.hoistedStmts) {
      result.hoistedStmts.push_back(std::move(stmt));
    }

    // Collect rewritten argument
    newArgs.push_back(std::move(argResult.expr));
  }

  // Create new function call with rewritten args
  result.expr = std::make_unique<FuncCall>(f->name, std::move(newArgs));

  return result;
}

/* ============================================================
 * REWRITE MAP ACCESS: G[k]
 * ============================================================ */

RewriteGlobalsVisitor::RewriteResult
RewriteGlobalsVisitor::rewriteMapAccess(const FuncCall *f) {
  RewriteResult result;

  if (f->args.size() != 2) {
    // Malformed [], shouldn't happen

    result.expr = const_cast<FuncCall *>(f)->clone();
    return result;
  }

  const Expr *base = f->args[0].get();
  const Expr *key = f->args[1].get();

  // Check if base is a global
  if (const Var *baseVar = dynamic_cast<const Var *>(base)) {
    if (globals.count(baseVar->name)) {
      // Hoist: tmp := get_G()
      std::string tmpName = freshTemp(baseVar->name);

      std::vector<std::unique_ptr<Expr>> noArgs;
      auto getCall =
          std::make_unique<FuncCall>("get_" + baseVar->name, std::move(noArgs));

      result.hoistedStmts.push_back(std::make_unique<Assign>(
          std::make_unique<Var>(tmpName), std::move(getCall)));

      // Rewrite key
      auto keyResult = rewriteExpr(key);
      for (auto &stmt : keyResult.hoistedStmts) {
        result.hoistedStmts.push_back(std::move(stmt));
      }

      // Return: tmp[key']
      std::vector<std::unique_ptr<Expr>> indexArgs;
      indexArgs.push_back(std::make_unique<Var>(tmpName));
      indexArgs.push_back(std::move(keyResult.expr));

      result.expr = std::make_unique<FuncCall>("[]", std::move(indexArgs));
      return result;
    }
  }

  // Not a global, rewrite base and key recursively
  auto baseResult = rewriteExpr(base);
  auto keyResult = rewriteExpr(key);

  for (auto &stmt : baseResult.hoistedStmts) {
    result.hoistedStmts.push_back(std::move(stmt));
  }
  for (auto &stmt : keyResult.hoistedStmts) {
    result.hoistedStmts.push_back(std::move(stmt));
  }

  std::vector<std::unique_ptr<Expr>> indexArgs;
  indexArgs.push_back(std::move(baseResult.expr));
  indexArgs.push_back(std::move(keyResult.expr));

  result.expr = std::make_unique<FuncCall>("[]", std::move(indexArgs));
  return result;
}

/* ============================================================
 * REWRITE DOM: dom(G)
 * ============================================================ */

RewriteGlobalsVisitor::RewriteResult
RewriteGlobalsVisitor::rewriteDom(const FuncCall *f) {
  RewriteResult result;

  if (f->args.size() != 1) {

    result.expr = const_cast<FuncCall *>(f)->clone();
    return result;
  }

  const Expr *base = f->args[0].get();

  // Check if base is a global
  if (const Var *baseVar = dynamic_cast<const Var *>(base)) {
    if (globals.count(baseVar->name)) {
      // Hoist: tmp := get_G()
      std::string tmpName = freshTemp(baseVar->name);

      std::vector<std::unique_ptr<Expr>> noArgs;
      auto getCall =
          std::make_unique<FuncCall>("get_" + baseVar->name, std::move(noArgs));

      result.hoistedStmts.push_back(std::make_unique<Assign>(
          std::make_unique<Var>(tmpName), std::move(getCall)));

      // Return: dom(tmp)
      std::vector<std::unique_ptr<Expr>> domArgs;
      domArgs.push_back(std::make_unique<Var>(tmpName));

      result.expr = std::make_unique<FuncCall>("dom", std::move(domArgs));
      return result;
    }
  }

  // Not a global, rewrite argument
  auto argResult = rewriteExpr(base);

  for (auto &stmt : argResult.hoistedStmts) {
    result.hoistedStmts.push_back(std::move(stmt));
  }

  std::vector<std::unique_ptr<Expr>> domArgs;
  domArgs.push_back(std::move(argResult.expr));

  result.expr = std::make_unique<FuncCall>("dom", std::move(domArgs));
  return result;
}

/* ============================================================
 * REWRITE NUM, STRING (Literals - No Rewrite Needed)
 * ============================================================ */

RewriteGlobalsVisitor::RewriteResult
RewriteGlobalsVisitor::rewriteNum(const Num *n) {
  RewriteResult result;
  result.expr = std::make_unique<Num>(n->value);
  return result;
}

RewriteGlobalsVisitor::RewriteResult
RewriteGlobalsVisitor::rewriteString(const String *s) {
  RewriteResult result;
  result.expr = std::make_unique<String>(s->value);
  return result;
}

/* ============================================================
 * REWRITE TUPLE
 * ============================================================ */

RewriteGlobalsVisitor::RewriteResult
RewriteGlobalsVisitor::rewriteTuple(const Tuple *t) {
  RewriteResult result;
  std::vector<std::unique_ptr<Expr>> newElements;

  for (const auto &elem : t->exprs) {
    auto elemResult = rewriteExpr(elem.get());

    for (auto &stmt : elemResult.hoistedStmts) {
      result.hoistedStmts.push_back(std::move(stmt));
    }

    newElements.push_back(std::move(elemResult.expr));
  }

  result.expr = std::make_unique<Tuple>(std::move(newElements));
  return result;
}

/* ============================================================
 * REWRITE SET
 * ============================================================ */

RewriteGlobalsVisitor::RewriteResult
RewriteGlobalsVisitor::rewriteSet(const Set *s) {
  RewriteResult result;
  std::vector<std::unique_ptr<Expr>> newElements;

  for (const auto &elem : s->elements) {
    auto elemResult = rewriteExpr(elem.get());

    for (auto &stmt : elemResult.hoistedStmts) {
      result.hoistedStmts.push_back(std::move(stmt));
    }

    newElements.push_back(std::move(elemResult.expr));
  }

  result.expr = std::make_unique<Set>(std::move(newElements));
  return result;
}

RewriteGlobalsVisitor::RewriteResult
RewriteGlobalsVisitor::rewriteBool(const Bool *bc) {
  RewriteResult result;
  result.expr = std::make_unique<Bool>(bc->value);
  return result;
}

/* ============================================================
 * REWRITE MAP
 * ============================================================ */

RewriteGlobalsVisitor::RewriteResult
RewriteGlobalsVisitor::rewriteMap(const Map *m) {
  RewriteResult result;
  std::vector<std::pair<std::unique_ptr<Var>, std::unique_ptr<Expr>>> newPairs;

  for (const auto &kv : m->value) {
    // Rewrite key (should be Var, but rewrite anyway)
    auto keyResult = rewriteExpr(kv.first.get());
    for (auto &stmt : keyResult.hoistedStmts) {
      result.hoistedStmts.push_back(std::move(stmt));
    }

    // Rewrite value
    auto valResult = rewriteExpr(kv.second.get());
    for (auto &stmt : valResult.hoistedStmts) {
      result.hoistedStmts.push_back(std::move(stmt));
    }

    // Key must be Var for Map AST
    Var *keyVar = dynamic_cast<Var *>(keyResult.expr.get());
    if (!keyVar) {
      throw std::runtime_error("Map key must be Var after rewrite");
    }

    std::unique_ptr<Var> keyOwned(static_cast<Var *>(keyResult.expr.release()));

    newPairs.push_back({std::move(keyOwned), std::move(valResult.expr)});
  }

  result.expr = std::make_unique<Map>(std::move(newPairs));
  return result;
}
