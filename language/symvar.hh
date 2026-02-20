#pragma once

#include "ast.hh"
#include "visitor.hh"
#include <memory>

using namespace std;

class SymVar : public Expr {
private:
  static unsigned int count;
  unsigned int num;

public:
  SymVar(unsigned int);
  static unique_ptr<SymVar> getNewSymVar();
  bool operator==(SymVar &);
  unsigned int getNum() const { return num; }
  virtual std::string toString() const;
  virtual std::unique_ptr<Expr> clone();
  virtual bool isEqual(const Expr &other) const;
};
