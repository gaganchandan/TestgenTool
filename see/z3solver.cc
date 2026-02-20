#include "z3solver.hh"
#include "../language/symvar.hh"
#include <iostream>
#include <list>
#include <z3++.h>

// ============================================================================
// Z3InputMaker Implementation
// ============================================================================

Z3InputMaker::Z3InputMaker(TypeMap *tm) : typeMap(tm) {}

Z3InputMaker::~Z3InputMaker() {
  // Clean up allocated z3::expr pointers
  for (auto &entry : symVarMap) {
    delete entry.second;
  }
  for (auto &entry : namedVarMap) {
    delete entry.second;
  }
  for (auto &entry : domainVarMap) {
    delete entry.second;
  }
}

// ============================================================================
// Z3 Sort Helpers
// ============================================================================
z3::expr Z3InputMaker::convertArg(const unique_ptr<Expr> &arg) {
  if (arg->exprType == ExprType::SYMVAR) {
    SymVar *sv = dynamic_cast<SymVar *>(arg.get());
    unsigned int num = sv->getNum();

    if (symVarMap.find(num) == symVarMap.end()) {
      string varName = "X" + to_string(num);
      z3::expr *z3Var = new z3::expr(ctx.int_const(varName.c_str()));
      symVarMap[num] = z3Var;
      variables.push_back(*z3Var);
    }

    return *symVarMap[num];
  } else {
    visit(arg.get());
    z3::expr result = theStack.top();
    theStack.pop();
    return result;
  }
}
z3::sort Z3InputMaker::getStringSort() { return ctx.string_sort(); }

z3::sort Z3InputMaker::getIntSort() { return ctx.int_sort(); }

z3::sort Z3InputMaker::getSetSort(z3::sort elementSort) {
  // Z3 represents sets as arrays from element type to bool
  return ctx.array_sort(elementSort, ctx.bool_sort());
}

z3::sort Z3InputMaker::getMapSort(z3::sort keySort, z3::sort valueSort) {
  // Z3 represents maps as arrays from key type to value type
  return ctx.array_sort(keySort, valueSort);
}

z3::sort Z3InputMaker::getListSort(z3::sort elementSort) {
  // Z3 represents lists as sequences
  return ctx.seq_sort(elementSort);
}

bool sortVectorEquals(const std::vector<z3::sort> &a,
                      const std::vector<z3::sort> &b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (size_t i = 0; i < a.size(); i++) {
    // Z3 sorts equality is checked using pointer comparison
    // https://github.com/Z3Prover/z3/issues/4750
    if (&a[i] != &b[i]) {
      return false;
    }
  }
  return true;
}

z3::sort Z3InputMaker::getTupleSort(std::vector<z3::sort> sorts) {
  z3::constructors cs(ctx);
  std::vector<z3::symbol> fieldNames;
  for (size_t i = 0; i < sorts.size(); i++) {
    string fieldName = "field" + to_string(i);
    fieldNames.push_back(ctx.str_symbol(fieldName.c_str()));
  }
  for (auto &entry : tuples) {
    if (sortVectorEquals(entry.second, sorts)) {
      cs.add(ctx.str_symbol(entry.first.c_str()),
             ctx.str_symbol(("is-" + entry.first).c_str()), fieldNames.size(),
             fieldNames.data(), sorts.data());
      return ctx.datatype(ctx.str_symbol(entry.first.c_str()), cs);
    }
  }
  std::string tupleName = "tuple" + to_string(tupleCount++);
  cs.add(ctx.str_symbol(tupleName.c_str()),
         ctx.str_symbol(("is-" + tupleName).c_str()), fieldNames.size(),
         fieldNames.data(), sorts.data());
  return ctx.datatype(ctx.str_symbol(tupleName.c_str()), cs);
}

z3::sort Z3InputMaker::typeExprToSort(TypeExpr *type) {
  if (!type) {
    return ctx.int_sort(); // Default to int
  }

  switch (type->typeExprType) {
  case TypeExprType::TYPE_CONST: {
    TypeConst *tc = dynamic_cast<TypeConst *>(type);
    if (tc->name == "string") {
      return getStringSort();
    } else if (tc->name == "int" || tc->name == "integer") {
      return getIntSort();
    } else if (tc->name == "bool" || tc->name == "boolean") {
      return ctx.bool_sort();
    }
    return ctx.int_sort(); // Default
  }
  case TypeExprType::SET_TYPE: {
    SetType *st = dynamic_cast<SetType *>(type);
    z3::sort elemSort = typeExprToSort(st->elementType.get());
    return getSetSort(elemSort);
  }
  case TypeExprType::MAP_TYPE: {
    MapType *mt = dynamic_cast<MapType *>(type);
    z3::sort keySort = typeExprToSort(mt->domain.get());
    z3::sort valSort = typeExprToSort(mt->range.get());
    return getMapSort(keySort, valSort);
  }
  default:
    return ctx.int_sort();
  }
}

z3::expr Z3InputMaker::makeEmptySet(z3::sort elementSort) {
  // Empty set is represented as a constant array of false
  return z3::const_array(elementSort, ctx.bool_val(false));
}

z3::expr Z3InputMaker::makeEmptyMap(z3::sort keySort, z3::sort valueSort) {
  // Empty map - use a fresh constant for default value
  z3::expr defaultVal = ctx.constant("_default", valueSort);
  return z3::const_array(keySort, defaultVal);
}

// z3::expr Z3InputMaker::makeEmptyTuple(std::vector<z3::sort> elementSorts) {}

pair<z3::expr, z3::expr>
Z3InputMaker::makeEmptyMapWithDomain(z3::sort keySort, z3::sort valueSort) {
  // Approach 2: Explicit domain tracking
  // Value array - initialize with arbitrary default (we don't care about its
  // value)
  z3::expr defaultValue = ctx.constant("_unused_default", valueSort);
  z3::expr valueArray = z3::const_array(keySort, defaultValue);

  // Domain array - initialize to all false (nothing in domain)
  z3::expr domainArray = z3::const_array(keySort, ctx.bool_val(false));

  return make_pair(valueArray, domainArray);
}

bool Z3InputMaker::isMapVariable(const string &varName) {
  if (!typeMap || !typeMap->hasValue(varName)) {
    return false;
  }

  TypeExpr *type = typeMap->getValue(varName);
  return type->typeExprType == TypeExprType::MAP_TYPE;
}

// ============================================================================
// Main Z3 Input Conversion
// ============================================================================

z3::expr Z3InputMaker::makeZ3Input(unique_ptr<Expr> &expr) {
  if (!expr) {
    throw runtime_error("Null expression in Z3 conversion");
  }

  // Handle SymVar specially since it's not part of the ExprVisitor interface
  if (expr->exprType == ExprType::SYMVAR) {
    SymVar *sv = dynamic_cast<SymVar *>(expr.get());
    unsigned int num = sv->getNum();

    // Check if we've already created a Z3 variable for this SymVar
    if (symVarMap.find(num) == symVarMap.end()) {
      string varName = "X" + to_string(num);
      z3::expr *z3Var = new z3::expr(ctx.int_const(varName.c_str()));
      symVarMap[num] = z3Var;
      variables.push_back(*z3Var);
    }

    return *symVarMap[num];
  }

  visit(expr.get());
  if (theStack.empty()) {
    throw runtime_error("Stack empty after visiting expression");
  }
  z3::expr result = theStack.top();
  theStack.pop();
  return result;
}

z3::expr Z3InputMaker::makeZ3Input(Expr *expr) {
  if (!expr) {
    throw runtime_error("Null expression in Z3 conversion");
  }

  // Handle SymVar specially
  if (expr->exprType == ExprType::SYMVAR) {
    SymVar *sv = dynamic_cast<SymVar *>(expr);
    unsigned int num = sv->getNum();

    if (symVarMap.find(num) == symVarMap.end()) {
      string varName = "X" + to_string(num);
      z3::expr *z3Var = new z3::expr(ctx.int_const(varName.c_str()));
      symVarMap[num] = z3Var;
      variables.push_back(*z3Var);
    }

    return *symVarMap[num];
  }

  visit(expr);
  if (theStack.empty()) {
    throw runtime_error("Stack empty after visiting expression");
  }
  z3::expr result = theStack.top();
  theStack.pop();
  return result;
}

vector<z3::expr> Z3InputMaker::getVariables() { return variables; }

// ============================================================================
// Expression Visitors
// ============================================================================

void Z3InputMaker::visitVar(const Var &node) {
  // Check if we already have this variable
  if (namedVarMap.find(node.name) != namedVarMap.end()) {
    theStack.push(*namedVarMap[node.name]);
    return;
  }

  // Check if we have type information
  if (typeMap && typeMap->hasValue(node.name)) {
    TypeExpr *type = typeMap->getValue(node.name);

    // ======================================================
    // Special handling for MAP types (Approach 2)
    // ======================================================
    if (type->typeExprType == TypeExprType::MAP_TYPE) {
      MapType *mapType = dynamic_cast<MapType *>(type);

      z3::sort keySort = typeExprToSort(mapType->domain.get());
      z3::sort valueSort = typeExprToSort(mapType->range.get());

      // Create VALUE array
      z3::expr valueArray =
          ctx.constant(node.name.c_str(), ctx.array_sort(keySort, valueSort));

      // Create DOMAIN array (Approach 2)
      string domainName = node.name + "_domain";
      z3::expr domainArray = ctx.constant(
          domainName.c_str(), ctx.array_sort(keySort, ctx.bool_sort()));

      cout << "[Z3] Created map variable: " << node.name
           << " with domain tracking" << endl;

      // Store both arrays
      namedVarMap[node.name] = new z3::expr(valueArray);
      domainVarMap[node.name] = new z3::expr(domainArray);

      // Add both to variables list for model extraction
      variables.push_back(valueArray);
      variables.push_back(domainArray);

      theStack.push(valueArray);
      return;
    }

    // ======================================================
    // Regular variables (existing logic)
    // ======================================================
    z3::sort varSort = typeExprToSort(type);
    z3::expr *z3Var = new z3::expr(ctx.constant(node.name.c_str(), varSort));
    namedVarMap[node.name] = z3Var;
    variables.push_back(*z3Var);
    theStack.push(*z3Var);
    return;
  }

  // ======================================================
  // No type info - default to int sort
  // ======================================================
  z3::sort varSort = ctx.int_sort(); // Default
  z3::expr *z3Var = new z3::expr(ctx.constant(node.name.c_str(), varSort));
  namedVarMap[node.name] = z3Var;
  variables.push_back(*z3Var);
  theStack.push(*z3Var);
}

void Z3InputMaker::visitNum(const Num &node) {
  theStack.push(ctx.int_val(node.value));
}

void Z3InputMaker::visitString(const String &node) {
  theStack.push(ctx.string_val(node.value));
}

void Z3InputMaker::visitFuncCall(const FuncCall &node) {
  // Helper lambda to convert an argument

  // MAP OPERATIONS (with Domain Tracking)
  // THESE MUST COME FIRST!
  // ========================================================================

  // MAP ACCESS: [] (alias for "get")
  if (node.name == "[]" && node.args.size() == 2) {
    cout << "[Z3] Map access: []" << endl;
    z3::expr mapExpr = convertArg(node.args[0]);
    z3::expr keyExpr = convertArg(node.args[1]);
    theStack.push(z3::select(mapExpr, keyExpr));
    return;
  }

  // MAP DOMAIN: dom(map) - Returns domain array
  if (node.name == "dom" && node.args.size() == 1) {
    cout << "[Z3] Domain extraction: dom" << endl;

    // Check if argument is a Var (map variable)
    if (node.args[0]->exprType == ExprType::VAR) {
      Var *mapVar = dynamic_cast<Var *>(node.args[0].get());

      // Look up domain array
      if (domainVarMap.find(mapVar->name) != domainVarMap.end()) {
        z3::expr domainArray = *domainVarMap[mapVar->name];
        cout << "[Z3]   Found domain array for: " << mapVar->name << endl;
        theStack.push(domainArray);
        return;
      } else {
        cout << "[Z3]   WARNING: No domain array for: " << mapVar->name << endl;
      }
    }

    // Fallback: evaluate map expression
    z3::expr mapExpr = convertArg(node.args[0]);
    cout << "[Z3]   WARNING: Cannot extract domain from non-variable map"
         << endl;
    theStack.push(mapExpr);
    return;
  }

  // DOMAIN MEMBERSHIP CHECKS (Must come BEFORE general "in")
  // ========================================================================

  // DOMAIN MEMBERSHIP: in(key, dom(map))
  if ((node.name == "in" || node.name == "member" || node.name == "contains") &&
      node.args.size() == 2) {
    // Check if second arg is dom(map) - SPECIAL CASE for Approach 2
    if (node.args[1]->exprType == ExprType::FUNC_CALL_EXPR) {
      FuncCall *fc = dynamic_cast<FuncCall *>(node.args[1].get());
      if (fc && fc->name == "dom") {
        cout << "[Z3] Domain membership: in(key, dom(map))" << endl;

        // Get key
        z3::expr keyExpr = convertArg(node.args[0]);

        // Get map variable
        if (fc->args[0]->exprType == ExprType::VAR) {
          Var *mapVar = dynamic_cast<Var *>(fc->args[0].get());

          // Look up domain array
          if (domainVarMap.find(mapVar->name) != domainVarMap.end()) {
            z3::expr domainArray = *domainVarMap[mapVar->name];

            // Check: select(domainArray, key) == true
            z3::expr result = z3::select(domainArray, keyExpr);
            cout << "[Z3]   Result: select(domain_array, key)" << endl;
            theStack.push(result);
            return;
          }
        }

        cout << "[Z3]   WARNING: Cannot find domain array" << endl;
      }
    }

    // NOT a domain check - fall through to general set/map membership
    cout << "[Z3] General set/map membership: in" << endl;
    z3::expr element = convertArg(node.args[0]);
    z3::expr setOrMap = convertArg(node.args[1]);
    // For sets (array to bool): select returns true if member
    // For maps (array to value): we check if key exists
    theStack.push(z3::select(setOrMap, element));
    return;
  }

  // DOMAIN NON-MEMBERSHIP: not_in(key, dom(map))
  if ((node.name == "not_in" || node.name == "not_member" ||
       node.name == "not_contains") &&
      node.args.size() == 2) {
    // Check if second arg is dom(map) - SPECIAL CASE for Approach 2
    if (node.args[1]->exprType == ExprType::FUNC_CALL_EXPR) {
      FuncCall *fc = dynamic_cast<FuncCall *>(node.args[1].get());
      if (fc && fc->name == "dom") {
        cout << "[Z3] Domain non-membership: not_in(key, dom(map))" << endl;

        // Get key
        z3::expr keyExpr = convertArg(node.args[0]);

        // Get map variable
        if (fc->args[0]->exprType == ExprType::VAR) {
          Var *mapVar = dynamic_cast<Var *>(fc->args[0].get());

          // Look up domain array
          if (domainVarMap.find(mapVar->name) != domainVarMap.end()) {
            z3::expr domainArray = *domainVarMap[mapVar->name];

            // Check: select(domainArray, key) == false
            z3::expr domainCheck = z3::select(domainArray, keyExpr);
            z3::expr result = !domainCheck;
            cout << "[Z3]   Result: not(select(domain_array, key))" << endl;
            theStack.push(result);
            return;
          }
        }

        cout << "[Z3]   WARNING: Cannot find domain array" << endl;
      }
    }

    // NOT a domain check - fall through to general not_in
    cout << "[Z3] General set/map non-membership: not_in" << endl;
    z3::expr element = convertArg(node.args[0]);
    z3::expr setOrMap = convertArg(node.args[1]);
    theStack.push(!z3::select(setOrMap, element));
    return;
  }

  // ========== Arithmetic Operations ==========
  if (node.name == "Add" && node.args.size() == 2) {
    z3::expr left = convertArg(node.args[0]);
    z3::expr right = convertArg(node.args[1]);
    theStack.push(left + right);
  } else if (node.name == "Sub" && node.args.size() == 2) {
    z3::expr left = convertArg(node.args[0]);
    z3::expr right = convertArg(node.args[1]);
    theStack.push(left - right);
  } else if (node.name == "Mul" && node.args.size() == 2) {
    z3::expr left = convertArg(node.args[0]);
    z3::expr right = convertArg(node.args[1]);
    theStack.push(left * right);
  }

  // ========== Comparison Operations ==========
  else if ((node.name == "Eq" || node.name == "=" || node.name == "==") &&
           node.args.size() == 2) {
    z3::expr left = convertArg(node.args[0]);
    z3::expr right = convertArg(node.args[1]);
    theStack.push(left == right);
  } else if ((node.name == "Neq" || node.name == "!=" || node.name == "<>") &&
             node.args.size() == 2) {
    z3::expr left = convertArg(node.args[0]);
    z3::expr right = convertArg(node.args[1]);
    theStack.push(left != right);
  } else if ((node.name == "Lt" || node.name == "<") && node.args.size() == 2) {
    z3::expr left = convertArg(node.args[0]);
    z3::expr right = convertArg(node.args[1]);
    theStack.push(left < right);
  } else if ((node.name == "Gt" || node.name == ">") && node.args.size() == 2) {
    z3::expr left = convertArg(node.args[0]);
    z3::expr right = convertArg(node.args[1]);
    theStack.push(left > right);
  } else if ((node.name == "Le" || node.name == "<=") &&
             node.args.size() == 2) {
    z3::expr left = convertArg(node.args[0]);
    z3::expr right = convertArg(node.args[1]);
    theStack.push(left <= right);
  } else if ((node.name == "Ge" || node.name == ">=") &&
             node.args.size() == 2) {
    z3::expr left = convertArg(node.args[0]);
    z3::expr right = convertArg(node.args[1]);
    theStack.push(left >= right);
  }

  // ========== Logical Operations ==========
  else if ((node.name == "And" || node.name == "and" || node.name == "&&" ||
            node.name == "AND") &&
           node.args.size() == 2) {
    z3::expr left = convertArg(node.args[0]);
    z3::expr right = convertArg(node.args[1]);
    theStack.push(left && right);
  } else if ((node.name == "Or" || node.name == "or" || node.name == "||" ||
              node.name == "OR") &&
             node.args.size() == 2) {
    z3::expr left = convertArg(node.args[0]);
    z3::expr right = convertArg(node.args[1]);
    theStack.push(left || right);
  } else if ((node.name == "Not" || node.name == "not" || node.name == "!") &&
             node.args.size() == 1) {
    z3::expr arg = convertArg(node.args[0]);
    theStack.push(!arg);
  } else if (node.name == "Implies" && node.args.size() == 2) {
    z3::expr left = convertArg(node.args[0]);
    z3::expr right = convertArg(node.args[1]);
    theStack.push(z3::implies(left, right));
  }

  // ========== Set Operations ==========
  else if (node.name == "union" && node.args.size() == 2) {
    // union(set1, set2) - set union using Z3's set_union
    z3::expr set1 = convertArg(node.args[0]);
    z3::expr set2 = convertArg(node.args[1]);
    theStack.push(z3::set_union(set1, set2));
  } else if ((node.name == "intersection" || node.name == "intersect") &&
             node.args.size() == 2) {
    // intersection(set1, set2) - set intersection
    z3::expr set1 = convertArg(node.args[0]);
    z3::expr set2 = convertArg(node.args[1]);
    theStack.push(z3::set_intersect(set1, set2));
  } else if ((node.name == "difference" || node.name == "diff" ||
              node.name == "minus") &&
             node.args.size() == 2) {
    // difference(set1, set2) - set difference
    z3::expr set1 = convertArg(node.args[0]);
    z3::expr set2 = convertArg(node.args[1]);
    theStack.push(z3::set_difference(set1, set2));
  } else if ((node.name == "subset" || node.name == "is_subset") &&
             node.args.size() == 2) {
    // subset(set1, set2) - check if set1 is subset of set2
    z3::expr set1 = convertArg(node.args[0]);
    z3::expr set2 = convertArg(node.args[1]);
    theStack.push(z3::set_subset(set1, set2));
  } else if (node.name == "add_to_set" && node.args.size() == 2) {
    // add_to_set(set, element) - add element to set
    z3::expr set = convertArg(node.args[0]);
    z3::expr element = convertArg(node.args[1]);
    theStack.push(z3::set_add(set, element));
  } else if (node.name == "remove_from_set" && node.args.size() == 2) {
    // remove_from_set(set, element) - remove element from set
    z3::expr set = convertArg(node.args[0]);
    z3::expr element = convertArg(node.args[1]);
    theStack.push(z3::set_del(set, element));
  } else if (node.name == "is_empty_set" && node.args.size() == 1) {
    // is_empty_set(set) - check if set is empty
    z3::expr set = convertArg(node.args[0]);
    z3::sort elemSort = set.get_sort().array_domain();
    z3::expr emptySet = makeEmptySet(elemSort);
    theStack.push(set == emptySet);
  }

  // ========== Map Operations ==========
  else if ((node.name == "get" || node.name == "lookup" ||
            node.name == "select") &&
           node.args.size() == 2) {
    // get(map, key) - get value for key from map
    z3::expr map = convertArg(node.args[0]);
    z3::expr key = convertArg(node.args[1]);
    theStack.push(z3::select(map, key));
  } else if ((node.name == "put" || node.name == "store" ||
              node.name == "update") &&
             node.args.size() == 3) {
    // put(map, key, value) - store value at key in map
    z3::expr map = convertArg(node.args[0]);
    z3::expr key = convertArg(node.args[1]);
    z3::expr value = convertArg(node.args[2]);
    theStack.push(z3::store(map, key, value));
  } else if ((node.name == "contains_key" || node.name == "has_key") &&
             node.args.size() == 2) {
    cout << "[Z3] contains_key operation" << endl;

    z3::expr map = convertArg(node.args[0]);
    z3::expr key = convertArg(node.args[1]);

    // Check if first arg is a variable (map variable)
    if (node.args[0]->exprType == ExprType::VAR) {
      Var *mapVar = dynamic_cast<Var *>(node.args[0].get());

      // Look up domain array
      if (domainVarMap.find(mapVar->name) != domainVarMap.end()) {
        z3::expr domainArray = *domainVarMap[mapVar->name];

        // Check: select(domainArray, key) == true
        z3::expr result = z3::select(domainArray, key);
        cout << "[Z3] Using domain tracking for contains_key" << endl;
        theStack.push(result);
        return;
      }
    }

    // Fallback: no domain tracking available
    cout << "[Z3] WARNING: No domain tracking, assuming key exists" << endl;
    theStack.push(ctx.bool_val(true));
  }

  // ========== List/Sequence Operations ==========
  else if ((node.name == "concat" || node.name == "append_list") &&
           node.args.size() == 2) {
    // concat(list1, list2) - concatenate two lists
    z3::expr list1 = convertArg(node.args[0]);
    z3::expr list2 = convertArg(node.args[1]);
    theStack.push(z3::concat(list1, list2));
  } else if (node.name == "length" && node.args.size() == 1) {
    // length(list) - get length of list
    z3::expr list = convertArg(node.args[0]);
    theStack.push(list.length());
  } else if ((node.name == "at" || node.name == "nth") &&
             node.args.size() == 2) {
    // at(list, index) - get element at index
    z3::expr list = convertArg(node.args[0]);
    z3::expr index = convertArg(node.args[1]);
    theStack.push(list.at(index));
  } else if (node.name == "prefix" && node.args.size() == 2) {
    // prefix(list1, list2) - check if list1 is prefix of list2
    z3::expr list1 = convertArg(node.args[0]);
    z3::expr list2 = convertArg(node.args[1]);
    theStack.push(z3::prefixof(list1, list2));
  } else if (node.name == "suffix" && node.args.size() == 2) {
    // suffix(list1, list2) - check if list1 is suffix of list2
    z3::expr list1 = convertArg(node.args[0]);
    z3::expr list2 = convertArg(node.args[1]);
    theStack.push(z3::suffixof(list1, list2));
  } else if (node.name == "contains_seq" && node.args.size() == 2) {
    // contains_seq(list, sublist) - check if list contains sublist
    z3::expr list = convertArg(node.args[0]);
    z3::expr sublist = convertArg(node.args[1]);
    // Use Z3's seq.contains via the C API
    Z3_ast args[2] = {list, sublist};
    Z3_ast result = Z3_mk_seq_contains(ctx, list, sublist);
    theStack.push(z3::expr(ctx, result));
  }

  // ========== Special Functions ==========
  else if ((node.name == "Any" || node.name == "any") &&
           node.args.size() == 1) {
    // Any(x) - No condition, but ensures variable is registered
    z3::expr arg = convertArg(node.args[0]);
    // Return true (tautology) so it satisfies constraints
    theStack.push(ctx.bool_val(true));
  }

  // ========== Unknown Function ==========
  else {
    throw runtime_error("Unsupported function: " + node.name + " with " +
                        to_string(node.args.size()) + " args");
  }
}

void Z3InputMaker::visitSet(const Set &node) {
  // Create a set from elements
  // Start with empty set and add each element
  if (node.elements.empty()) {
    // Empty set - need to determine element type
    // Default to int sort
    theStack.push(makeEmptySet(ctx.int_sort()));
    return;
  }

  // Get first element to determine sort
  visit(node.elements[0].get());
  z3::expr firstElem = theStack.top();
  theStack.pop();

  z3::sort elemSort = firstElem.get_sort();
  z3::expr result = z3::set_add(makeEmptySet(elemSort), firstElem);

  // Add remaining elements
  for (size_t i = 1; i < node.elements.size(); i++) {
    visit(node.elements[i].get());
    z3::expr elem = theStack.top();
    theStack.pop();
    result = z3::set_add(result, elem);
  }

  theStack.push(result);
}

void Z3InputMaker::visitMap(const Map &node) {
  // Create a map from key-value pairs
  if (node.value.empty()) {
    // Empty map - default to string->string
    theStack.push(makeEmptyMap(ctx.string_sort(), ctx.string_sort()));
    return;
  }

  // Get first key-value to determine sorts
  visit(node.value[0].first.get());
  z3::expr firstKey = theStack.top();
  theStack.pop();

  visit(node.value[0].second.get());
  z3::expr firstVal = theStack.top();
  theStack.pop();

  z3::sort keySort = firstKey.get_sort();
  z3::sort valSort = firstVal.get_sort();

  // Start with empty map and store each pair
  z3::expr result =
      z3::store(makeEmptyMap(keySort, valSort), firstKey, firstVal);

  // Add remaining pairs
  for (size_t i = 1; i < node.value.size(); i++) {
    visit(node.value[i].first.get());
    z3::expr key = theStack.top();
    theStack.pop();

    visit(node.value[i].second.get());
    z3::expr val = theStack.top();
    theStack.pop();

    result = z3::store(result, key, val);
  }

  theStack.push(result);
}

void Z3InputMaker::visitTuple(const Tuple &node) {
  std::vector<z3::sort> sorts;
  std::vector<z3::expr> elements;
  for (size_t i = 0; i < node.exprs.size(); i++) {
    visit(node.exprs[i].get());
    elements.push_back(theStack.top());
    theStack.pop();
    z3::sort elemSort = elements.back().get_sort();
    sorts[i] = elemSort;
  }
  z3::sort tupleSort = getTupleSort(sorts);
  tupleSort.constructors()[0].range();
  z3::expr tupleExpr =
      tupleSort.constructors()[0](node.exprs.size(), elements.data());
  theStack.push(tupleExpr);
}

// ============================================================================
// Type Expression Visitors (not used for Z3 conversion directly)
// ============================================================================

void Z3InputMaker::visitTypeConst(const TypeConst &node) {
  throw runtime_error("TypeConst not supported in Z3 conversion");
}

void Z3InputMaker::visitFuncType(const FuncType &node) {
  throw runtime_error("FuncType not supported in Z3 conversion");
}

void Z3InputMaker::visitMapType(const MapType &node) {
  throw runtime_error("MapType not supported in Z3 conversion");
}

void Z3InputMaker::visitTupleType(const TupleType &node) {
  throw runtime_error("TupleType not supported in Z3 conversion");
}

void Z3InputMaker::visitSetType(const SetType &node) {
  throw runtime_error("SetType not supported in Z3 conversion");
}

// ============================================================================
// Declaration/Statement Visitors (not used for Z3 conversion)
// ============================================================================

void Z3InputMaker::visitDecl(const Decl &node) {
  throw runtime_error("Decl not supported in Z3 conversion");
}

void Z3InputMaker::visitAPIcall(const APIcall &node) {
  throw runtime_error("APIcall not supported in Z3 conversion");
}

void Z3InputMaker::visitAPI(const API &node) {
  throw runtime_error("API not supported in Z3 conversion");
}

void Z3InputMaker::visitResponse(const Response &node) {
  throw runtime_error("Response not supported in Z3 conversion");
}

void Z3InputMaker::visitInit(const Init &node) {
  throw runtime_error("Init not supported in Z3 conversion");
}

void Z3InputMaker::visitSpec(const Spec &node) {
  throw runtime_error("Spec not supported in Z3 conversion");
}

void Z3InputMaker::visitAssign(const Assign &node) {
  throw runtime_error("Assign not supported in Z3 conversion");
}

void Z3InputMaker::visitAssume(const Assume &node) {
  throw runtime_error("Assume not supported in Z3 conversion");
}
void Z3InputMaker::visitAssert(const Assert &node) { visit(node.expr.get()); }

void Z3InputMaker::visitBool(const Bool &node) {
  cout << "[Z3] Bool: " << (node.value ? "true" : "false") << endl;
  theStack.push(ctx.bool_val(node.value));
}

void Z3InputMaker::visitProgram(const Program &node) {
  throw runtime_error("Program not supported in Z3 conversion");
}

// ============================================================================
// Z3Solver Implementation
// ============================================================================

Z3Solver::Z3Solver(TypeMap *tm) : typeMap(tm) {}

Result Z3Solver::solve(unique_ptr<Expr> formula) const {
  Z3InputMaker inputMaker(typeMap);

  // Convert the formula to Z3 format
  z3::expr z3Formula = inputMaker.makeZ3Input(formula);

  // Create solver and add the constraint
  z3::solver s(inputMaker.getContext());
  s.add(z3Formula);

  cout << "[Z3Solver] Checking satisfiability..." << endl;
  cout << "[Z3Solver] Formula: " << z3Formula << endl;

  if (s.check() == z3::sat) {
    cout << "[Z3Solver] SAT - Model found!" << endl;
    z3::model m = s.get_model();

    // Extract variable values from the model
    map<string, unique_ptr<ResultValue>> var_values;

    // Get all variables that were used
    vector<z3::expr> vars = inputMaker.getVariables();
    for (const auto &var : vars) {
      z3::expr val = m.eval(var, true);
      string varName = var.to_string();

      // Handle different types of values
      if (val.is_numeral()) {
        int intVal;
        if (val.is_int() &&
            Z3_get_numeral_int(inputMaker.getContext(), val, &intVal)) {
          cout << "[Z3Solver] " << varName << " = " << intVal << endl;
          var_values[varName] = make_unique<IntResultValue>(intVal);
        }
      } else if (val.is_string_value()) {
        string strVal = val.get_string();
        cout << "[Z3Solver] " << varName << " = \"" << strVal << "\"" << endl;
        var_values[varName] = make_unique<StringResultValue>(strVal);
      } else if (val.is_bool()) {
        bool boolVal = val.is_true();
        cout << "[Z3Solver] " << varName << " = "
             << (boolVal ? "true" : "false") << endl;
        var_values[varName] = make_unique<BoolResultValue>(boolVal);
      } else if (val.is_array()) {
        // For arrays (sets/maps), store as string representation
        cout << "[Z3Solver] " << varName << " = " << val << " (array)" << endl;
        var_values[varName] = make_unique<StringResultValue>(val.to_string());
      } else {
        cout << "[Z3Solver] " << varName << " = " << val << " (unknown type)"
             << endl;
        var_values[varName] = make_unique<StringResultValue>(val.to_string());
      }
    }

    return Result(true, std::move(var_values));
  } else {
    cout << "[Z3Solver] UNSAT - No solution exists" << endl;
    return Result(false, map<string, unique_ptr<ResultValue>>());
  }
}
