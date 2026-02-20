#include "typemap.hh"
#include "ast.hh"
#include <iostream>

TypeMap::TypeMap(TypeMap *p) : Env(p) {}

string TypeMap::keyToString(string *key) { return *key; }

void TypeMap::print() {
  std::cout << "TypeMap:" << std::endl;
  for (auto &d : table) {
    std::cout << "  " << d.first << " : ";
    if (d.second) {
      // Print type expression
      switch (d.second->typeExprType) {
      case TypeExprType::TYPE_CONST: {
        TypeConst *tc = dynamic_cast<TypeConst *>(d.second);
        std::cout << tc->name;
        break;
      }
      case TypeExprType::MAP_TYPE: {
        MapType *mt = dynamic_cast<MapType *>(d.second);
        std::cout << "map<...>";
        break;
      }
      case TypeExprType::SET_TYPE: {
        std::cout << "set<...>";
        break;
      }
      case TypeExprType::TUPLE_TYPE: {
        std::cout << "tuple<...>";
        break;
      }
      case TypeExprType::FUNC_TYPE: {
        std::cout << "func<...>";
        break;
      }
      default:
        std::cout << "unknown";
      }
    } else {
      std::cout << "null";
    }
    std::cout << std::endl;
  }
}

void TypeMap::setValue(const string &varName, TypeExpr *value) {
  // For value environment, we allow updating existing values (unlike
  // SymbolTable)
  table[varName] = value;
}

TypeExpr *TypeMap::getValue(const string &varName) {
  if (table.find(varName) != table.end()) {
    return table[varName];
  }
  if (parent != nullptr) {
    TypeMap *parentEnv = dynamic_cast<TypeMap *>(parent);
    if (parentEnv) {
      return parentEnv->getValue(varName);
    }
  }
  return nullptr;
}

bool TypeMap::hasValue(const string &varName) {
  if (table.find(varName) != table.end()) {
    return true;
  }
  if (parent != nullptr) {
    TypeMap *parentEnv = dynamic_cast<TypeMap *>(parent);
    if (parentEnv) {
      return parentEnv->hasValue(varName);
    }
  }
  return false;
}
