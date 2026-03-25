#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "ast.hh"
#include "visitor.hh"

/**
 * RewriteGlobalsVisitor - CORRECT IMPLEMENTATION
 *
 * Rewrites global variables (U, T, R, M, C, O, Rev, Roles) into Test API calls.
 *
 * INPUT:  Logical ATC with global map operations
 * OUTPUT: Test-API ATC with get_G() / set_G() calls
 *
 * RULES:
 * 1. Detect globals from init statements (G := {})
 * 2. Remove all init statements
 * 3. Prepend: _ := reset()
 * 4. Rewrite reads:  U[k]  →  tmp_U_i := get_U(); ... tmp_U_i[k]
 * 5. Rewrite writes: U[k] = v  →  tmp := get_U(); tmp[k] := v; set_U(tmp)
 *
 * KEY IMPROVEMENTS:
 * - Uses CloneVisitor for all AST copying
 * - Proper temp variable counters per global
 * - Hoists all get_G() to statement level
 * - Handles nested expressions correctly
 * - Separates GET and SET operations cleanly
 */
class RewriteGlobalsVisitor : public Visitor {

public:
  RewriteGlobalsVisitor();

  // Final output after rewrite
  std::unique_ptr<Program> rewrittenProgram;

  // Main entry point
  void visitProgram(const Program &p) override;

private:
  // === STATE ===

  // Set of detected global variable names
  std::set<std::string> globals;

  // Per-global temp counters: "U" → 5 means next is tmp_U_5
  std::map<std::string, int> tmpCounters;

  // Output statements buffer
  std::vector<std::unique_ptr<Stmt>> newStmts;

  // === HELPERS ===

  // Generate fresh temp name: tmp_U_5, tmp_T_3, etc.
  std::string freshTemp(const std::string &globalName);

  // Check if statement is init: G := {}
  bool isInitAssign(const Stmt *stmt);

  // Check if expression is a global variable reference
  bool isGlobalVar(const Expr *expr);

  // Check if expression contains any global references
  bool containsGlobals(const Expr *expr);

  // === STATEMENT REWRITING ===

  void visitAssign(const Assign &s) override;
  void visitAssume(const Assume &s) override;
  void visitAssert(const Assert &s) override;

  // Rewrite a single assignment statement
  void rewriteAssignStmt(const Assign &s);

  // === EXPRESSION REWRITING ===

  /**
   * Main expression rewriter with temp hoisting.
   *
   * Returns: (rewritten_expr, hoisted_stmts)
   *
   * hoisted_stmts must be inserted BEFORE the statement using rewritten_expr.
   */
  struct RewriteResult {
    std::unique_ptr<Expr> expr;
    std::vector<std::unique_ptr<Stmt>> hoistedStmts;
  };

  RewriteResult rewriteExpr(const Expr *e);

  // Specific expression rewriters
  RewriteResult rewriteVar(const Var *v);
  RewriteResult rewriteFuncCall(const FuncCall *f);
  RewriteResult rewriteNum(const Num *n);
  RewriteResult rewriteString(const String *s);
  RewriteResult rewriteTuple(const Tuple *t);
  RewriteResult rewriteSet(const Set *s);
  RewriteResult rewriteMap(const Map *m);
  RewriteResult rewriteBool(const Bool *bc);

  // Rewrite map access: G[k] → tmp_G_i[k]
  RewriteResult rewriteMapAccess(const FuncCall *f);

  // Rewrite dom(G) → dom(tmp_G_i)
  RewriteResult rewriteDom(const FuncCall *f);

  // === ASSIGNMENT LHS DETECTION ===

  /**
   * Detect if LHS is a global map update: G[k] or G
   * Returns: (globalName, key, value) or ("", nullptr, nullptr)
   */
  struct MapUpdateInfo {
    std::string globalName;      // "U", "T", etc.
    std::unique_ptr<Expr> key;   // email (for U[email] = pass)
    std::unique_ptr<Expr> value; // password
    bool isMapUpdate;            // true if G[k]=v, false if G=expr
  };

  MapUpdateInfo detectMapUpdate(const Expr *lhs, const Expr *rhs);

  // === GLOBAL WRITE REWRITING ===

  // Rewrite: G[k] = v  →  tmp := get_G(); tmp[k] := v; set_G(tmp)
  void emitMapUpdate(const std::string &globalName, std::unique_ptr<Expr> key,
                     std::unique_ptr<Expr> value);

  // Rewrite: G := expr  →  set_G(expr')
  void emitMapReplace(const std::string &globalName,
                      std::unique_ptr<Expr> expr);

  // === REQUIRED VISITOR OVERRIDES (No-ops) ===

  void visitTypeVar(const TypeVar &node) override {}
  void visitTypeConst(const TypeConst &) override {}
  void visitFuncType(const FuncType &) override {}
  void visitMapType(const MapType &) override {}
  void visitTupleType(const TupleType &) override {}
  void visitSetType(const SetType &) override {}
  void visitVar(const Var &) override {}
  void visitFuncCall(const FuncCall &) override {}
  void visitNum(const Num &) override {}
  void visitString(const String &) override {}
  void visitSet(const Set &) override {}
  void visitMap(const Map &) override {}
  void visitTuple(const Tuple &) override {}
  void visitBool(const Bool &node) override;
  void visitDecl(const Decl &) override {}
  void visitFuncDecl(const FuncDecl &) override {}
  void visitAPIcall(const APIcall &) override {}
  void visitAPI(const API &) override {}
  void visitResponse(const Response &) override {}
  void visitInit(const Init &) override {}
  void visitSpec(const Spec &) override {}
};
