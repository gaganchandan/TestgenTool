#include "visitor.hh"
#include "ast.hh"
#include <stdexcept>

// Centralized dispatch for TypeExpr nodes
void Visitor::visit(const TypeExpr *node) {
  if (!node)
    throw std::runtime_error("Null TypeExpr node in visitor");
  switch (node->typeExprType) {
  case TypeExprType::TYPE_CONST:
    visitTypeConst(*dynamic_cast<const TypeConst *>(node));
    break;
  case TypeExprType::FUNC_TYPE:
    visitFuncType(*dynamic_cast<const FuncType *>(node));
    break;
  case TypeExprType::MAP_TYPE:
    visitMapType(*dynamic_cast<const MapType *>(node));
    break;
  case TypeExprType::TUPLE_TYPE:
    visitTupleType(*dynamic_cast<const TupleType *>(node));
    break;
  case TypeExprType::SET_TYPE:
    visitSetType(*dynamic_cast<const SetType *>(node));
    break;
    // default:
    //   throw std::runtime_error("Unknown TypeExpr type in visitor");
  }
}

// Centralized dispatch for Expr nodes
void Visitor::visit(const Expr *node) {
  if (!node)
    throw std::runtime_error("Null Expr node in visitor");
  switch (node->exprType) {
  case ExprType::VAR:
    visitVar(*dynamic_cast<const Var *>(node));
    break;
  case ExprType::FUNC_CALL_EXPR:
    visitFuncCall(*dynamic_cast<const FuncCall *>(node));
    break;
  case ExprType::NUM:
    visitNum(*dynamic_cast<const Num *>(node));
    break;
  case ExprType::BOOL:
    visitBool(*dynamic_cast<const Bool *>(node));
    break;
  case ExprType::STRING:
    visitString(*dynamic_cast<const String *>(node));
    break;
  case ExprType::SET:
    visitSet(*dynamic_cast<const Set *>(node));
    break;
  case ExprType::MAP:
    visitMap(*dynamic_cast<const Map *>(node));
    break;
  case ExprType::TUPLE:
    visitTuple(*dynamic_cast<const Tuple *>(node));
    break;
  default:
    throw std::runtime_error("Unknown Expr type in visitor");
  }
}

// if (node->exprType == ExprType::VAR) {
//   visitVar(*dynamic_cast<const Var *>(node));
// } else if (node->exprType == ExprType::FUNC_CALL_EXPR) {
//   visitFuncCall(*dynamic_cast<const FuncCall *>(node));
// } else if (node->exprType == ExprType::NUM) {
//   visitNum(*dynamic_cast<const Num *>(node));
// } else if (node->exprType == ExprType::STRING) {
//   visitString(*dynamic_cast<const String *>(node));
// } else if (node->exprType == ExprType::SET) {
//   visitSet(*dynamic_cast<const Set *>(node));
// } else if (node->exprType == ExprType::MAP) {
//   visitMap(*dynamic_cast<const Map *>(node));
// } else if (node->exprType == ExprType::TUPLE) {
//   visitTuple(*dynamic_cast<const Tuple *>(node));
// } else {
//   throw std::runtime_error("Unknown Expr type in visitor");
// }

// Centralized dispatch for Stmt nodes
void Visitor::visit(const Stmt *node) {
  if (!node)
    throw std::runtime_error("Null Stmt node in visitor");

  // if (node->statementType == StmtType::ASSIGN) {
  //   visitAssign(*dynamic_cast<const Assign *>(node));
  // } else if (node->statementType == StmtType::ASSUME) {
  //   visitAssume(*dynamic_cast<const Assume *>(node));
  // } else {
  //   throw std::runtime_error("Unknown Stmt type in visitor");
  // }
  switch (node->statementType) {
  case StmtType::ASSIGN:
    visitAssign(*dynamic_cast<const Assign *>(node));
    ;
    break;
  case StmtType::ASSUME:
    visitAssume(*dynamic_cast<const Assume *>(node));
    ;
    break;
  // default:
  //   throw std::runtime_error("Unknown Stmt type in visitor");
  case StmtType::ASSERT:
    visitAssert(*dynamic_cast<const Assert *>(node));
    break;
  case StmtType::DECL:
    visitDecl(*dynamic_cast<const Decl *>(node));
    break;
  }
}
