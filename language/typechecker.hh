#include "ast.hh"
#include "visitor.hh"
#include <unordered_map>
#include <unordered_set>

class TypeChecker : public Visitor {
private:
  std::vector<std::string> typeDefs;
  std::unordered_map<std::string, const TypeExpr *> globals;
  std::unordered_map<std::string, std::unique_ptr<TypeExpr>> functions;
  TypeExpr *currentTypeExpr;
  int typeVarCounter;
  std::unordered_map<std::string, TypeExpr *> substitution;
  std::vector<std::pair<TypeExpr *, TypeExpr *>> constraints;

protected:
  // Type Expression visitors
  void visitTypeVar(const TypeVar &node) override {}
  void visitTypeConst(const TypeConst &node) override {}
  void visitFuncType(const FuncType &node) override {}
  void visitMapType(const MapType &node) override {}
  void visitTupleType(const TupleType &node) override {}
  void visitSetType(const SetType &node) override {}

  void visitNum(const Num &node) override;
  void visitString(const String &node) override;
  void visitBool(const Bool &node) override;
  void visitVar(const Var &node) override;
  void visitSet(const Set &node) override;
  void visitMap(const Map &node) override;
  // void visitMapAccess(const MapAccess &node) override;
  // void visitMapUpdate(const MapUpdate &node) override;
  void visitTuple(const Tuple &node) override;
  void visitFuncCall(const FuncCall &node) override;
  // void visitEquals(const Equals &node) override;

  // void visitAssign(const Assign &node) override;
  // void visitFuncCallStmt(const FuncCallStmt &node) override;
  void visitAssign(const Assign &node) override {}
  void visitAssume(const Assume &node) override {}
  void visitAssert(const Assert &node) override {}

public:
  std::string freshTypeVar();
  void instantiateAllFunctions();
  void collectTypeVars(const TypeExpr *t,
                       std::unordered_set<std::string> &vars);
  void collectTypeConsts(const TypeExpr *t,
                         std::unordered_set<std::string> &consts);
  std::unique_ptr<TypeExpr>
  substituteTypeVars(const TypeExpr *t,
                     const std::unordered_map<std::string, std::string> &subst);
  std::unique_ptr<TypeExpr> freshenFunctionType(const TypeExpr *type);
  TypeChecker();
  // void addTypeDef(const TypeDef *typeDef);
  void addGlobal(const Decl *decl);
  void addFunction(const FuncDecl *func);
  void unify(TypeExpr *t1, TypeExpr *t2);
  void unifyAll();
  TypeExpr *applySubstitution(TypeExpr *type);
  void updateEnvWithSubstitution();

  // void visitTypeDef(const TypeDef &node) override;
  void visitDecl(const Decl &node) override;
  void visitFuncDecl(const FuncDecl &node) override;

  void visitAPIcall(const APIcall &node) override;
  void visitAPI(const API &node) override;
  void visitResponse(const Response &node) override;

  void visitInit(const Init &node) override;

  void visitSpec(const Spec &node) override;
  void visitProgram(const Program &node) override;
};
