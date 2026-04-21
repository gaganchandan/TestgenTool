#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

// Core AST
#include "../language/ast.hh"

// Environments
#include "../language/env.hh"
#include "../language/typemap.hh"

using namespace std;

/* ============================================================
 * Helper Functions
 * ============================================================ */

// Rename variables based on block suffix (e.g., uid → uid0)
unique_ptr<Expr> convert1(const unique_ptr<Expr> &expr, SymbolTable *symtable,
                          const string &suffix);

// Collect primed variables (U', T', etc.)
void addthedashexpr(const unique_ptr<Expr> &expr, set<string> &res);

// Remove prime notation and introduce _old variables
unique_ptr<Expr> removethedashexpr(const unique_ptr<Expr> &expr,
                                   set<string> &res, int flag = 0);

// Create input statement: x := input()
unique_ptr<Stmt> makeInputStmt(unique_ptr<Var> var);

// Collect free (input) variables from expressions
void getInputVars(const unique_ptr<Expr> &expr,
                  vector<unique_ptr<Expr>> &inputVars, const string &suffix,
                  SymbolTable *symtable, TypeMap &localTM);

/* ============================================================
 * ATC Builders
 * ============================================================ */

// Generate initialization statements for globals
vector<unique_ptr<Stmt>> genInit(const Spec &spec, SymbolTable *globalST,
                                 TypeMap *globalTM);

// Replace a variable name in expression (for result variable renaming)
unique_ptr<Expr> replaceResultVar(unique_ptr<Expr> expr, const string &oldName,
                                  const string &newName);

// Build ATC from a resolved sequence of API blocks
Program buildATCFromBlockSequence(const vector<const API *> &blockSeq);

Program genATCFromString(const Spec &spec, const vector<string> &testString);
