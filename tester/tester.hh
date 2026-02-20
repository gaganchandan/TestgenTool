#ifndef TESTER_HH
#define TESTER_HH

#include "../language/ast.hh"
#include "../language/env.hh"
#include "../see/see.hh"
#include "../see/z3solver.hh"
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace std;

class Tester {
private:
  SEE see;
  Z3Solver solver;
  vector<Expr *> pathConstraints;
  vector<string> currentApiSequence;

public:
  // Constructor
  Tester(FunctionFactory *functionFactory)
      : see(functionFactory), solver(), pathConstraints() {}

  // Main test generation methods
  void generateTest();
  unique_ptr<Program> generateATC(unique_ptr<Spec>, vector<string>);
  unique_ptr<Program> generateCTC(unique_ptr<Program>,
                                  vector<Expr *> ConcreteVals,
                                  ValueEnvironment *ve);
  unique_ptr<Program> rewriteATC(unique_ptr<Program> &,
                                 vector<Expr *> ConcreteVals);

  // Helper methods for value generation
  string findKeyFromMapInSigma(const string &prefix);
  Expr *generateValueForBaseName(const string &baseName, const string &varName,
                                 int index,
                                 map<string, Expr *> &baseNameToValue,
                                 bool lookupFromSigma = false);

  // Getters for testing
  SEE &getSEE() { return see; }
  Z3Solver &getSolver() { return solver; }
  vector<Expr *> &getPathConstraints() { return pathConstraints; }
  const vector<string> &getApiSequence() const { return currentApiSequence; }
};

#endif
