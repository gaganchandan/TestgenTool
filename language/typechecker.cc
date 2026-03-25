#include "typechecker.hh"
#include <algorithm>
#include <iostream>
#include <stdexcept>

TypeChecker::TypeChecker() : typeVarCounter(0) {
  // -- Built-in functions --
  // add: int -> int -> int;
  std::vector<std::unique_ptr<TypeExpr>> params;
  params.push_back(std::make_unique<TypeConst>("int"));
  params.push_back(std::make_unique<TypeConst>("int"));
  functions["add"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("int"));
  // sub: int -> int -> int;
  params.clear();
  params.push_back(std::make_unique<TypeConst>("int"));
  params.push_back(std::make_unique<TypeConst>("int"));
  functions["sub"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("int"));
  // mul: int -> int -> int;
  params.clear();
  params.push_back(std::make_unique<TypeConst>("int"));
  params.push_back(std::make_unique<TypeConst>("int"));
  functions["mul"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("int"));
  // div: int -> int -> int;
  params.clear();
  params.push_back(std::make_unique<TypeConst>("int"));
  params.push_back(std::make_unique<TypeConst>("int"));
  functions["div"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("int"));
  // lt: int -> int -> bool;
  params.clear();
  params.push_back(std::make_unique<TypeConst>("int"));
  params.push_back(std::make_unique<TypeConst>("int"));
  functions["lt"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // gt: int -> int -> bool;
  params.clear();
  params.push_back(std::make_unique<TypeConst>("int"));
  params.push_back(std::make_unique<TypeConst>("int"));
  functions["gt"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // le: int -> int -> bool;
  params.clear();
  params.push_back(std::make_unique<TypeConst>("int"));
  params.push_back(std::make_unique<TypeConst>("int"));
  functions["le"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // ge: int -> int -> bool;
  params.clear();
  params.push_back(std::make_unique<TypeConst>("int"));
  params.push_back(std::make_unique<TypeConst>("int"));
  functions["ge"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // equals: 'a -> 'a -> bool;
  params.clear();
  params.push_back(std::make_unique<TypeVar>("a"));
  params.push_back(std::make_unique<TypeVar>("a"));
  functions["equals"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // not_equals: 'a -> 'a -> bool;
  params.clear();
  params.push_back(std::make_unique<TypeVar>("a"));
  params.push_back(std::make_unique<TypeVar>("a"));
  functions["not_equals"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // implies: bool -> bool -> bool;
  params.clear();
  params.push_back(std::make_unique<TypeConst>("bool"));
  params.push_back(std::make_unique<TypeConst>("bool"));
  functions["implies"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // and: bool -> bool -> bool;
  params.clear();
  params.push_back(std::make_unique<TypeConst>("bool"));
  params.push_back(std::make_unique<TypeConst>("bool"));
  functions["and"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // or: bool -> bool -> bool;
  params.clear();
  params.push_back(std::make_unique<TypeConst>("bool"));
  params.push_back(std::make_unique<TypeConst>("bool"));
  functions["or"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // not: bool -> bool;
  params.clear();
  params.push_back(std::make_unique<TypeConst>("bool"));
  functions["not"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // input: string -> string;
  params.clear();
  params.push_back(std::make_unique<TypeConst>("string"));
  functions["input"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("string"));
  // in: 'a -> set<'a> -> bool;
  params.clear();
  params.push_back(std::make_unique<TypeVar>("a"));
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  functions["in"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // not_in: 'a -> set<'a> -> bool;
  params.clear();
  params.push_back(std::make_unique<TypeVar>("a"));
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  functions["not_in"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // member: 'a -> set<'a> -> bool;
  params.clear();
  params.push_back(std::make_unique<TypeVar>("a"));
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  functions["member"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // not_member: 'a -> set<'a> -> bool;
  params.clear();
  params.push_back(std::make_unique<TypeVar>("a"));
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  functions["not_member"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // contains: set<'a> -> 'a -> bool;
  params.clear();
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  params.push_back(std::make_unique<TypeVar>("a"));
  functions["contains"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // not_contains: set<'a> -> 'a -> bool;
  params.clear();
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  params.push_back(std::make_unique<TypeVar>("a"));
  functions["not_contains"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // add_to_set: set<'a> -> 'a -> set<'a>;
  params.clear();
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  params.push_back(std::make_unique<TypeVar>("a"));
  functions["add_to_set"] = std::make_unique<FuncType>(
      std::move(params),
      std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  // remove_from_set: set<'a> -> 'a -> set<'a>;
  params.clear();
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  params.push_back(std::make_unique<TypeVar>("a"));
  functions["remove_from_set"] = std::make_unique<FuncType>(
      std::move(params),
      std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  // union: set<'a> -> set<'a> -> set<'a>;
  params.clear();
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  functions["union"] = std::make_unique<FuncType>(
      std::move(params),
      std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  // intersection: set<'a> -> set<'a> -> set<'a>;
  params.clear();
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  functions["intersection"] = std::make_unique<FuncType>(
      std::move(params),
      std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  // difference: set<'a> -> set<'a> -> set<'a>;
  params.clear();
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  functions["difference"] = std::make_unique<FuncType>(
      std::move(params),
      std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  // subset: set<'a> -> set<'a> -> bool;
  params.clear();
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  functions["subset"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // is_empty_set: set<'a> -> bool;
  params.clear();
  params.push_back(std::make_unique<SetType>(std::make_unique<TypeVar>("a")));
  functions["is_empty_set"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // get: map<'k, 'v> -> 'k -> 'v;
  params.clear();
  params.push_back(std::make_unique<MapType>(std::make_unique<TypeVar>("k"),
                                             std::make_unique<TypeVar>("v")));
  params.push_back(std::make_unique<TypeVar>("k"));
  functions["get"] = std::make_unique<FuncType>(std::move(params),
                                                std::make_unique<TypeVar>("v"));
  // lookup: map<'k, 'v> -> 'k -> 'v;
  params.clear();
  params.push_back(std::make_unique<MapType>(std::make_unique<TypeVar>("k"),
                                             std::make_unique<TypeVar>("v")));
  params.push_back(std::make_unique<TypeVar>("k"));
  functions["lookup"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeVar>("v"));
  // select: map<'k, 'v> -> 'k -> 'v;
  params.clear();
  params.push_back(std::make_unique<MapType>(std::make_unique<TypeVar>("k"),
                                             std::make_unique<TypeVar>("v")));
  params.push_back(std::make_unique<TypeVar>("k"));
  functions["select"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeVar>("v"));
  // contains_key: map<'k, 'v> -> 'k -> bool;
  params.clear();
  params.push_back(std::make_unique<MapType>(std::make_unique<TypeVar>("k"),
                                             std::make_unique<TypeVar>("v")));
  params.push_back(std::make_unique<TypeVar>("k"));
  functions["contains_key"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // has_key: map<'k, 'v> -> 'k -> bool;
  params.clear();
  params.push_back(std::make_unique<MapType>(std::make_unique<TypeVar>("k"),
                                             std::make_unique<TypeVar>("v")));
  params.push_back(std::make_unique<TypeVar>("k"));
  functions["has_key"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeConst>("bool"));
  // dom: map<'k, 'v> -> set<'k>;
  params.clear();
  params.push_back(std::make_unique<MapType>(std::make_unique<TypeVar>("k"),
                                             std::make_unique<TypeVar>("v")));
  functions["dom"] = std::make_unique<FuncType>(
      std::move(params),
      std::make_unique<SetType>(std::make_unique<TypeVar>("k")));
  // range: map<'k, 'v> -> set<'v>;
  params.clear();
  params.push_back(std::make_unique<MapType>(std::make_unique<TypeVar>("k"),
                                             std::make_unique<TypeVar>("v")));
  functions["range"] = std::make_unique<FuncType>(
      std::move(params),
      std::make_unique<SetType>(std::make_unique<TypeVar>("v")));
  // put: map<'k, 'v> -> 'k -> 'v -> map<'k, 'v>;
  params.clear();
  params.push_back(std::make_unique<MapType>(std::make_unique<TypeVar>("k"),
                                             std::make_unique<TypeVar>("v")));
  params.push_back(std::make_unique<TypeVar>("k"));
  params.push_back(std::make_unique<TypeVar>("v"));
  functions["put"] = std::make_unique<FuncType>(
      std::move(params),
      std::make_unique<MapType>(std::make_unique<TypeVar>("k"),
                                std::make_unique<TypeVar>("v")));
  // store: map<'k, 'v> -> 'k -> 'v -> map<'k, 'v>;
  params.clear();
  params.push_back(std::make_unique<MapType>(std::make_unique<TypeVar>("k"),
                                             std::make_unique<TypeVar>("v")));
  params.push_back(std::make_unique<TypeVar>("k"));
  params.push_back(std::make_unique<TypeVar>("v"));
  functions["store"] = std::make_unique<FuncType>(
      std::move(params),
      std::make_unique<MapType>(std::make_unique<TypeVar>("k"),
                                std::make_unique<TypeVar>("v")));
  // update: map<'k, 'v> -> 'k -> 'v -> map<'k, 'v>;
  params.clear();
  params.push_back(std::make_unique<MapType>(std::make_unique<TypeVar>("k"),
                                             std::make_unique<TypeVar>("v")));
  params.push_back(std::make_unique<TypeVar>("k"));
  params.push_back(std::make_unique<TypeVar>("v"));
  functions["update"] = std::make_unique<FuncType>(
      std::move(params),
      std::make_unique<MapType>(std::make_unique<TypeVar>("k"),
                                std::make_unique<TypeVar>("v")));
  // primed: 'a -> 'a;
  params.clear();
  params.push_back(std::make_unique<TypeVar>("a"));
  functions["primed"] = std::make_unique<FuncType>(
      std::move(params), std::make_unique<TypeVar>("a"));

  // --- Built-in type definitions ---
  typeDefs.push_back("int");
  typeDefs.push_back("bool");
  typeDefs.push_back("string");
}

std::string TypeChecker::freshTypeVar() {
  return "T" + std::to_string(typeVarCounter++);
}

void TypeChecker::collectTypeVars(const TypeExpr *t,
                                  std::unordered_set<std::string> &vars) {
  if (!t)
    return;
  if (auto tv = dynamic_cast<const TypeVar *>(t)) {
    vars.insert(tv->name);
    return;
  }
  if (auto fn = dynamic_cast<const FuncType *>(t)) {
    for (const auto &param : fn->params) {
      collectTypeVars(param.get(), vars);
    }
    collectTypeVars(fn->returnType.get(), vars);
    return;
  }
  if (auto map = dynamic_cast<const MapType *>(t)) {
    collectTypeVars(map->domain.get(), vars);
    collectTypeVars(map->range.get(), vars);
    return;
  }
  if (auto tuple = dynamic_cast<const TupleType *>(t)) {
    for (const auto &elem : tuple->elements) {
      collectTypeVars(elem.get(), vars);
    }
    return;
  }
  if (auto set = dynamic_cast<const SetType *>(t)) {
    collectTypeVars(set->elementType.get(), vars);
    return;
  }
}

void TypeChecker::collectTypeConsts(
    const TypeExpr *t, std::unordered_set<std::string> &typeConsts) {
  if (!t)
    return;
  if (auto tc = dynamic_cast<const TypeConst *>(t)) {
    typeConsts.insert(tc->name);
    return;
  }
  if (auto fn = dynamic_cast<const FuncType *>(t)) {
    for (const auto &param : fn->params) {
      collectTypeConsts(param.get(), typeConsts);
    }
    collectTypeConsts(fn->returnType.get(), typeConsts);
    return;
  }
  if (auto map = dynamic_cast<const MapType *>(t)) {
    collectTypeConsts(map->domain.get(), typeConsts);
    collectTypeConsts(map->range.get(), typeConsts);
    return;
  }
  if (auto tuple = dynamic_cast<const TupleType *>(t)) {
    for (const auto &elem : tuple->elements) {
      collectTypeConsts(elem.get(), typeConsts);
    }
    return;
  }
  if (auto set = dynamic_cast<const SetType *>(t)) {
    collectTypeConsts(set->elementType.get(), typeConsts);
    return;
  }
}

std::unique_ptr<TypeExpr> TypeChecker::substituteTypeVars(
    const TypeExpr *t,
    const std::unordered_map<std::string, std::string> &subst) {
  if (!t)
    return nullptr;

  // --- Type Variable ---
  if (auto tv = dynamic_cast<const TypeVar *>(t)) {
    auto it = subst.find(tv->name);
    if (it != subst.end()) {
      return std::make_unique<TypeVar>(it->second);
    }
    return const_cast<TypeVar *>(tv)->clone();
  }

  // --- Function Type ---
  if (auto fn = dynamic_cast<const FuncType *>(t)) {
    std::vector<std::unique_ptr<TypeExpr>> newParams;

    for (const auto &param : fn->params) {
      newParams.push_back(substituteTypeVars(param.get(), subst));
    }

    auto newReturn = substituteTypeVars(fn->returnType.get(), subst);

    return std::make_unique<FuncType>(std::move(newParams),
                                      std::move(newReturn));
  }

  // --- Map Type ---
  if (auto map = dynamic_cast<const MapType *>(t)) {
    return std::make_unique<MapType>(
        substituteTypeVars(map->domain.get(), subst),
        substituteTypeVars(map->range.get(), subst));
  }

  // --- Tuple Type ---
  if (auto tuple = dynamic_cast<const TupleType *>(t)) {
    std::vector<std::unique_ptr<TypeExpr>> newElems;

    for (const auto &elem : tuple->elements) {
      newElems.push_back(substituteTypeVars(elem.get(), subst));
    }

    return std::make_unique<TupleType>(std::move(newElems));
  }

  // --- Set Type ---
  if (auto set = dynamic_cast<const SetType *>(t)) {
    return std::make_unique<SetType>(
        substituteTypeVars(set->elementType.get(), subst));
  }

  // --- Base Types ---
  return const_cast<TypeExpr *>(t)->clone();
}

std::unique_ptr<TypeExpr>
TypeChecker::freshenFunctionType(const TypeExpr *type) {
  std::unordered_set<std::string> vars;
  collectTypeVars(type, vars);

  std::unordered_map<std::string, std::string> subst;

  for (const auto &v : vars) {
    subst[v] = freshTypeVar();
  }

  return substituteTypeVars(type, subst);
}

void TypeChecker::instantiateAllFunctions() {
  for (auto &[name, typePtr] : functions) {
    typePtr = freshenFunctionType(typePtr.get());
  }
}

// void TypeChecker::addTypeDef(const TypeDef *typeDef) {
//   if (std::find(typeDefs.begin(), typeDefs.end(), typeDef->name) !=
//       typeDefs.end()) {
//     throw std::runtime_error("TypeError: duplicate definition of type: " +
//                              typeDef->name);
//   }
//   typeDefs.push_back(typeDef->name);
// }

void TypeChecker::addGlobal(const Decl *decl) {
  if (globals.find(decl->name) != globals.end()) {
    throw std::runtime_error(
        "TypeError: duplicate definition of global variable: " + decl->name);
  }
  if (functions.find(decl->name) != functions.end()) {
    throw std::runtime_error(
        "TypeError: global variable name conflicts with function: " +
        decl->name);
  }
  std::unordered_set<std::string> typeConsts;
  collectTypeConsts(decl->type.get(), typeConsts);
  for (const auto tc : typeConsts) {
    // if (std::find(typeDefs.begin(), typeDefs.end(), tc) == typeDefs.end())
    //   throw std::runtime_error("TypeError: undefined type: " + tc);
  }

  globals[decl->name] = decl->type.get();
}

void TypeChecker::addFunction(const FuncDecl *func) {
  if (functions.find(func->name) != functions.end()) {
    throw std::runtime_error("TypeError: duplicate definition of function: " +
                             func->name);
  }
  if (globals.find(func->name) != globals.end()) {
    throw std::runtime_error(
        "TypeError: function name conflicts with global variable: " +
        func->name);
  }
  std::unordered_set<std::string> typeConsts;
  for (const auto &param : func->params) {
    collectTypeConsts(param.get(), typeConsts);
  }
  collectTypeConsts(func->returnType.second.get(), typeConsts);
  for (const auto tc : typeConsts) {
    // if (std::find(typeDefs.begin(), typeDefs.end(), tc) == typeDefs.end())
    //   throw std::runtime_error("TypeError: undefined type: " + tc);
  }
  std::vector<std::unique_ptr<TypeExpr>> paramClones;
  for (const auto &p : func->params)
    paramClones.push_back(p->clone());
  functions[func->name] = std::make_unique<FuncType>(
      std::move(paramClones), func->returnType.second->clone());
}

// if (paramType->typeExprType == TypeExprType::TYPE_VAR) {
//   auto *tv = dynamic_cast<TypeVar *>(paramType);
//   auto it = substitution.find(tv->name);
//   if (it != substitution.end()) {
//     substitutedParamType = substitution[tv->name];
//   } else {
//     substitution[tv->name] = argType->clone().release();
//     substitutedParamType = argType;
//   }
// }

void TypeChecker::unify(TypeExpr *t1, TypeExpr *t2) {
  if (t1->typeExprType == TypeExprType::TYPE_VAR &&
      t2->typeExprType == TypeExprType::TYPE_VAR) {
    if (substitution.find(dynamic_cast<TypeVar *>(t1)->name) !=
        substitution.end()) {
      unify(substitution[dynamic_cast<TypeVar *>(t1)->name], t2);
    } else if (substitution.find(dynamic_cast<TypeVar *>(t2)->name) !=
               substitution.end()) {
      unify(t1, substitution[dynamic_cast<TypeVar *>(t2)->name]);
    } else {
      substitution[dynamic_cast<TypeVar *>(t1)->name] = t2;
    }
  } else if (t1->typeExprType == TypeExprType::TYPE_VAR) {
    auto *tv1 = dynamic_cast<TypeVar *>(t1);
    if (substitution.find(tv1->name) != substitution.end()) {
      unify(substitution[tv1->name], t2);
    } else {
      // t1 is a type variable, unify it with t2
      substitution[dynamic_cast<TypeVar *>(t1)->name] = t2;
    }
  } else if (t2->typeExprType == TypeExprType::TYPE_VAR) {
    unify(t2, t1);
  } else if (t1->typeExprType == TypeExprType::MAP_TYPE &&
             t2->typeExprType == TypeExprType::MAP_TYPE) {
    MapType *m1 = dynamic_cast<MapType *>(t1);
    MapType *m2 = dynamic_cast<MapType *>(t2);
    unify(m1->domain.get(), m2->domain.get());
    unify(m1->range.get(), m2->range.get());
  } else if (t1->typeExprType == TypeExprType::TUPLE_TYPE &&
             t2->typeExprType == TypeExprType::TUPLE_TYPE) {
    TupleType *tup1 = dynamic_cast<TupleType *>(t1);
    TupleType *tup2 = dynamic_cast<TupleType *>(t2);
    if (tup1->elements.size() != tup2->elements.size()) {
      throw std::runtime_error("TypeError: cannot unify " + t1->toString() +
                               " with " + t2->toString());
    }
    for (size_t i = 0; i < tup1->elements.size(); i++) {
      unify(tup1->elements[i].get(), tup2->elements[i].get());
    }
  } else if (t1->typeExprType == TypeExprType::SET_TYPE &&
             t2->typeExprType == TypeExprType::SET_TYPE) {
    SetType *s1 = dynamic_cast<SetType *>(t1);
    SetType *s2 = dynamic_cast<SetType *>(t2);
    unify(s1->elementType.get(), s2->elementType.get());
  } else if (!(*t1 == *t2)) {
    throw std::runtime_error("TypeError: cannot unify " + t1->toString() +
                             " with " + t2->toString());
  }
}

void TypeChecker::unifyAll() {
  for (const auto &[t1, t2] : constraints) {
    unify(t1, t2);
  }
}

TypeExpr *TypeChecker::applySubstitution(TypeExpr *t) {
  if (t->typeExprType == TypeExprType::TYPE_VAR) {
    auto it = substitution.find(dynamic_cast<TypeVar *>(t)->name);
    if (it != substitution.end()) {
      return applySubstitution(it->second);
    }
  } else if (t->typeExprType == TypeExprType::MAP_TYPE) {
    MapType *m = dynamic_cast<MapType *>(t);
    return new MapType(
        std::unique_ptr<TypeExpr>(applySubstitution(m->domain.get())),
        std::unique_ptr<TypeExpr>(applySubstitution(m->range.get())));
  } else if (t->typeExprType == TypeExprType::TUPLE_TYPE) {
    TupleType *tup = dynamic_cast<TupleType *>(t);
    std::vector<std::unique_ptr<TypeExpr>> newElements;
    for (const auto &elem : tup->elements) {
      newElements.push_back(
          std::unique_ptr<TypeExpr>(applySubstitution(elem.get())));
    }
    return new TupleType(std::move(newElements));
  } else if (t->typeExprType == TypeExprType::SET_TYPE) {
    SetType *s = dynamic_cast<SetType *>(t);
    return new SetType(
        std::unique_ptr<TypeExpr>(applySubstitution(s->elementType.get())));
  } else if (t->typeExprType == TypeExprType::FUNC_TYPE) {
    FuncType *f = dynamic_cast<FuncType *>(t);
    std::vector<std::unique_ptr<TypeExpr>> newParams;
    for (const auto &param : f->params) {
      newParams.push_back(
          std::unique_ptr<TypeExpr>(applySubstitution(param.get())));
    }
    return new FuncType(
        std::move(newParams),
        std::unique_ptr<TypeExpr>(applySubstitution(f->returnType.get())));
  }
  return t;
}

void TypeChecker::updateEnvWithSubstitution() {
  for (auto &entry : globals) {
    entry.second = applySubstitution(const_cast<TypeExpr *>(entry.second));
  }
}

void TypeChecker::visitNum(const Num &node) {
  currentTypeExpr = new TypeConst("int");
}

void TypeChecker::visitString(const String &node) {
  currentTypeExpr = new TypeConst("string");
}

void TypeChecker::visitBool(const Bool &node) {
  currentTypeExpr = new TypeConst("bool");
}

void TypeChecker::visitVar(const Var &node) {
  if (globals.find(node.name) == globals.end()) {
    currentTypeExpr = new TypeVar("T" + std::to_string(typeVarCounter++));
    globals[node.name] = currentTypeExpr;
  } else {
    currentTypeExpr = const_cast<TypeExpr *>(globals[node.name]);
  }
}

void TypeChecker::visitSet(const Set &node) {
  if (node.elements.empty()) {
    currentTypeExpr = new SetType(
        std::make_unique<TypeVar>("T" + std::to_string(typeVarCounter++)));
    return;
  }
  visit(node.elements[0].get());
  TypeExpr *elemType = currentTypeExpr;
  for (size_t i = 1; i < node.elements.size(); i++) {
    visit(node.elements[i].get());
    if (!(*currentTypeExpr == *elemType)) {
      throw std::runtime_error("TypeError: inconsistent element types in set");
    }
  }
  currentTypeExpr = new SetType(std::unique_ptr<TypeExpr>(elemType->clone()));
}

void TypeChecker::visitMap(const Map &node) {
  if (node.value.empty()) {
    auto keyType = new TypeVar("T" + std::to_string(typeVarCounter++));
    auto valueType = new TypeVar("T" + std::to_string(typeVarCounter++));
    currentTypeExpr = new MapType(std::unique_ptr<TypeExpr>(keyType),
                                  std::unique_ptr<TypeExpr>(valueType));
    return;
  }
  visit(node.value[0].first.get());
  TypeExpr *keyType = currentTypeExpr;
  visit(node.value[0].second.get());
  TypeExpr *valueType = currentTypeExpr;
  for (size_t i = 1; i < node.value.size(); i++) {
    visit(node.value[i].first.get());
    if (!(*currentTypeExpr == *keyType)) {
      throw std::runtime_error("TypeError: inconsistent key types in map");
    }
    visit(node.value[i].second.get());
    if (!(*currentTypeExpr == *valueType)) {
      throw std::runtime_error("TypeError: inconsistent value types in map");
    }
  }
  currentTypeExpr = new MapType(std::unique_ptr<TypeExpr>(keyType->clone()),
                                std::unique_ptr<TypeExpr>(valueType->clone()));
}

// void TypeChecker::visitMapAccess(const MapAccess &node) {
//   visit(node.mapName.get());
//   TypeExpr *mapTypeExpr = currentTypeExpr;
//   if (mapTypeExpr->typeExprType != TypeExprType::MAP_TYPE) {
//     throw std::runtime_error("Attempting to access a non-map type");
//   }
//   MapType *mapType = dynamic_cast<MapType *>(mapTypeExpr);
//   visit(node.keyExpr.get());
//   if (!(*currentTypeExpr == *mapType->domain.get())) {
//     throw std::runtime_error("Key type mismatch in map access" +
//                              node.mapName->toString() + ": expected " +
//                              mapType->domain->toString() + " got " +
//                              currentTypeExpr->toString());
//   }
//   currentTypeExpr = mapType->range.get();
// }

// void TypeChecker::visitMapUpdate(const MapUpdate &node) {
//   visit(node.mapName.get());
//   TypeExpr *mapTypeExpr = currentTypeExpr;
//   if (mapTypeExpr->typeExprType != TypeExprType::MAP_TYPE) {
//     throw std::runtime_error("Attempting to update a non-map type");
//   }
//   MapType *mapType = dynamic_cast<MapType *>(mapTypeExpr);
//   visit(node.keyExpr.get());
//   if (!(*currentTypeExpr == *mapType->domain.get())) {
//     throw std::runtime_error("Key type mismatch in map update");
//   }
//   visit(node.valueExpr.get());
//   if (!(*currentTypeExpr == *mapType->range.get())) {
//     throw std::runtime_error("Value type mismatch in map update");
//   }
//   currentTypeExpr = mapTypeExpr;
// }

void TypeChecker::visitTuple(const Tuple &node) {
  std::vector<std::unique_ptr<TypeExpr>> elementTypes;
  for (const auto &elem : node.exprs) {
    visit(elem.get());
    elementTypes.push_back(std::unique_ptr<TypeExpr>(currentTypeExpr->clone()));
  }
  currentTypeExpr = new TupleType(std::move(elementTypes));
}

void TypeChecker::visitFuncCall(const FuncCall &node) {
  if (functions.find(node.name) == functions.end()) {
    throw std::runtime_error("TypeError: unknown function: " + node.name);
  }
  if (node.args.size() !=
      dynamic_cast<FuncType *>(functions[node.name].get())->params.size()) {
    throw std::runtime_error("TypeError: arity mismatch in call: " + node.name);
  }
  FuncType *freshFuncType = dynamic_cast<FuncType *>(
      freshenFunctionType(functions[node.name].get()).release());
  for (size_t i = 0; i < node.args.size(); i++) {
    visit(node.args[i].get());
    auto argType = currentTypeExpr;
    TypeExpr *paramType = freshFuncType->params[i].get();
    // unify(paramType, argType);
    constraints.push_back({paramType, argType});
  }
  currentTypeExpr = dynamic_cast<FuncType *>(freshFuncType)->returnType.get();
}

// void TypeChecker::visitEquals(const Equals &node) {
//   visit(node.left.get());
//   TypeExpr *leftType = currentTypeExpr;
//   visit(node.right.get());
//   TypeExpr *rightType = currentTypeExpr;
//   if (!(*leftType == *rightType)) {
//     throw std::runtime_error("Type mismatch in equality expression");
//   }
//   currentTypeExpr = new TypeConst("bool");
// }

// void TypeChecker::visitTypeDef(const TypeDef &node) { addTypeDef(&node); }

void TypeChecker::visitDecl(const Decl &node) { addGlobal(&node); }

void TypeChecker::visitFuncDecl(const FuncDecl &node) { addFunction(&node); }

void TypeChecker::visitResponse(const Response &node) {
  visit(node.expr.get());
}

void TypeChecker::visitAPIcall(const APIcall &node) { visit(node.call.get()); }

void TypeChecker::visitAPI(const API &node) {
  std::cout << "Visiting API: " << node.name << std::endl;
  if (node.pre)
    visit(node.pre.get());
  if (!(*currentTypeExpr == *new TypeConst("bool")))
    throw std::runtime_error(
        "TypeError: API pre-condition must be of type bool");
  if (node.call)
    visitAPIcall(*node.call);
  if (node.post)
    visit(node.post.get());
  if (!(*currentTypeExpr == *new TypeConst("bool")))
    throw std::runtime_error(
        "TypeError: API post-condition must be of type bool");
}

void TypeChecker::visitInit(const Init &node) { visit(node.expr.get()); }

void TypeChecker::visitSpec(const Spec &node) {
  // for (const auto &typeDef : node.typeDefs)
  //   addTypeDef(typeDef.get());
  for (const auto &g : node.globals)
    addGlobal(g.get());
  for (const auto &f : node.functions)
    addFunction(f.get());
  for (const auto &api : node.blocks)
    visitAPI(dynamic_cast<const API &>(*api));
  // Print all constraints for debugging
  unifyAll();
  // updateEnvWithSubstitution();
}

void TypeChecker::visitProgram(const Program &node) {}
