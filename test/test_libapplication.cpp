#include "../language/ast.hh"
#include "../language/env.hh"
#include "../language/printer.hh"
#include "../see/ecommercefunctionfactory.hh"
#include "../see/libraryfunctionfactory.hh"
#include "../see/restaurantfunctionfactory.hh"
#include "../see/see.hh"
#include "../tester/algo.hpp"
#include "../tester/tester.hh"
#include <iostream>
#include <memory>

// Import webapp-specific specs
#include "../specs/EcommerceSpec.hpp"
#include "../specs/LibrarySpec.hpp"
#include "../specs/RestaurantSpec.hpp"

using namespace std;

// ============================================
// TEST EXECUTION MODES
// ============================================

enum class TestMode {
  ORIGINAL,     // Just genATC (no rewrite)
  REWRITE_ONLY, // genATC + RewriteGlobalsVisitor (no backend)
  FULL_PIPELINE // Complete: genATC + Rewrite + SEE + Backend
};

// ============================================
// TEST EXECUTOR LIBRARY
// ============================================

class LibraryTestExecutor {
private:
  TestMode mode;
  string backendUrl;

public:
  LibraryTestExecutor(TestMode m, const string &url = "http://localhost:8080")
      : mode(m), backendUrl(url) {}

  void runTest(const string &testName, unique_ptr<Spec> spec,
               const vector<string> &testSequence) {
    cout << "\n========================================" << endl;
    cout << "TEST: " << testName << endl;
    cout << "MODE: " << getModeString() << endl;
    cout << "DEPTH: " << testSequence.size() << " API calls" << endl;
    cout << "========================================\n" << endl;

    try {
      switch (mode) {
      case TestMode::ORIGINAL:
        runOriginal(std::move(spec), testSequence);
        break;
      case TestMode::REWRITE_ONLY:
        runRewriteOnly(std::move(spec), testSequence);
        break;
      case TestMode::FULL_PIPELINE:
        runFullPipeline(std::move(spec), testSequence);
        break;
      }
      cout << "\n✓ " << testName << " COMPLETE!\n" << endl;
    } catch (const runtime_error &e) {
      string errorMsg = e.what();
      if (errorMsg.find("UNSAT") != string::npos) {
        cout << "\n⊘ " << testName << " UNSAT: Preconditions not satisfiable\n"
             << endl;
      } else {
        cout << "\n✗ " << testName << " FAILED: " << errorMsg << "\n" << endl;
      }
    } catch (const exception &e) {
      cout << "\n✗ " << testName << " FAILED: " << e.what() << "\n" << endl;
    }
  }

private:
  string getModeString() {
    switch (mode) {
    case TestMode::ORIGINAL:
      return "Original (No Rewrite)";
    case TestMode::REWRITE_ONLY:
      return "Rewrite Only (No Backend)";
    case TestMode::FULL_PIPELINE:
      return "Full Pipeline (With Backend)";
    }
    return "Unknown";
  }

  void runOriginal(unique_ptr<Spec> spec, const vector<string> &ts) {
    Program atc = genATC(*spec, ts);
    Printer printer;
    printer.visitProgram(atc);
  }

  void runRewriteOnly(unique_ptr<Spec> spec, const vector<string> &ts) {
    auto factory = make_unique<Library::LibraryFunctionFactory>(backendUrl);
    Tester tester(factory.get());

    unique_ptr<Program> testApiATC = tester.generateATC(std::move(spec), ts);
  }

  void runFullPipeline(unique_ptr<Spec> spec, const vector<string> &ts) {
    Printer printer;
    cout << "\n[Specification]" << endl;
    printer.visitSpec(*spec);
    SymbolTable *symbolTable = new SymbolTable(nullptr);

    auto factory = make_unique<Library::LibraryFunctionFactory>(backendUrl);
    Tester tester(factory.get());

    unique_ptr<Program> testApiATC = tester.generateATC(std::move(spec), ts);

    vector<Expr *> inputVars;
    ValueEnvironment *valueEnv = new ValueEnvironment(nullptr);

    unique_ptr<Program> ctc =
        tester.generateCTC(std::move(testApiATC), inputVars, valueEnv);

    if (!ctc) {
      cout << "\n[RESULT] UNSAT - Test preconditions cannot be satisfied"
           << endl;
      delete symbolTable;
      delete valueEnv;
      throw runtime_error("UNSAT: Preconditions not satisfiable");
    }

    cout << "\n[FINAL CTC]" << endl;
    printer.visitProgram(*ctc);

    delete symbolTable;
    delete valueEnv;
  }
};

// ============================================
// TEST EXECUTOR
// ============================================

class TestExecutor {
private:
  TestMode mode;
  string backendUrl;

public:
  TestExecutor(TestMode m, const string &url = "http://localhost:5002")
      : mode(m), backendUrl(url) {}

  void runTest(const string &testName, unique_ptr<Spec> spec,
               const vector<string> &testSequence) {
    cout << "\n========================================" << endl;
    cout << "TEST: " << testName << endl;
    cout << "MODE: " << getModeString() << endl;
    cout << "DEPTH: " << testSequence.size() << " API calls" << endl;
    cout << "========================================\n" << endl;

    try {
      switch (mode) {
      case TestMode::ORIGINAL:
        runOriginal(std::move(spec), testSequence);
        break;
      case TestMode::REWRITE_ONLY:
        runRewriteOnly(std::move(spec), testSequence);
        break;
      case TestMode::FULL_PIPELINE:
        runFullPipeline(std::move(spec), testSequence);
        break;
      }
      cout << "\n✓ " << testName << " COMPLETE!\n" << endl;
    } catch (const runtime_error &e) {
      string errorMsg = e.what();
      // Check if this is an UNSAT error
      if (errorMsg.find("UNSAT") != string::npos) {
        cout << "\n⊘ " << testName << " UNSAT: Preconditions not satisfiable\n"
             << endl;
      } else {
        cout << "\n✗ " << testName << " FAILED: " << errorMsg << "\n" << endl;
      }
    } catch (const exception &e) {
      cout << "\n✗ " << testName << " FAILED: " << e.what() << "\n" << endl;
    }
  }

private:
  string getModeString() {
    switch (mode) {
    case TestMode::ORIGINAL:
      return "Original (No Rewrite)";
    case TestMode::REWRITE_ONLY:
      return "Rewrite Only (No Backend)";
    case TestMode::FULL_PIPELINE:
      return "Full Pipeline (With Backend)";
    }
    return "Unknown";
  }

  void runOriginal(unique_ptr<Spec> spec, const vector<string> &ts) {
    Program atc = genATC(*spec, ts);
    Printer printer;
    printer.visitProgram(atc);
  }

  void runRewriteOnly(unique_ptr<Spec> spec, const vector<string> &ts) {
    auto factory = make_unique<RestaurantFunctionFactory>(backendUrl);
    Tester tester(factory.get());

    unique_ptr<Program> testApiATC = tester.generateATC(std::move(spec), ts);
  }

  void runFullPipeline(unique_ptr<Spec> spec, const vector<string> &ts) {
    Printer printer;
    cout << "\n[Specification]" << endl;
    printer.visitSpec(*spec);
    SymbolTable *symbolTable = new SymbolTable(nullptr);

    auto factory = make_unique<RestaurantFunctionFactory>(backendUrl);
    Tester tester(factory.get());

    unique_ptr<Program> testApiATC = tester.generateATC(std::move(spec), ts);

    vector<Expr *> inputVars;
    ValueEnvironment *valueEnv = new ValueEnvironment(nullptr);

    unique_ptr<Program> ctc =
        tester.generateCTC(std::move(testApiATC), inputVars, valueEnv);

    if (!ctc) {
      cout << "\n[RESULT] UNSAT - Test preconditions cannot be satisfied"
           << endl;
      delete symbolTable;
      delete valueEnv;
      throw runtime_error("UNSAT: Preconditions not satisfiable");
    }

    cout << "\n[FINAL CTC]" << endl;
    printer.visitProgram(*ctc);

    delete symbolTable;
    delete valueEnv;
  }
};

// ============================================
// ECOMMERCE TEST EXECUTOR
// ============================================

class EcommerceTestExecutor {
private:
  TestMode mode;
  string backendUrl;

public:
  EcommerceTestExecutor(TestMode m, const string &url = "http://localhost:3000")
      : mode(m), backendUrl(url) {}

  void runTest(const string &testName, unique_ptr<Spec> spec,
               const vector<string> &testSequence) {
    cout << "\n========================================" << endl;
    cout << "TEST: " << testName << endl;
    cout << "MODE: " << getModeString() << endl;
    cout << "DEPTH: " << testSequence.size() << " API calls" << endl;
    cout << "========================================\n" << endl;

    try {
      switch (mode) {
      case TestMode::ORIGINAL:
        runOriginal(std::move(spec), testSequence);
        break;
      case TestMode::REWRITE_ONLY:
        runRewriteOnly(std::move(spec), testSequence);
        break;
      case TestMode::FULL_PIPELINE:
        runFullPipeline(std::move(spec), testSequence);
        break;
      }
      cout << "\n✓ " << testName << " COMPLETE!\n" << endl;
    } catch (const runtime_error &e) {
      string errorMsg = e.what();
      if (errorMsg.find("UNSAT") != string::npos) {
        cout << "\n⊘ " << testName << " UNSAT: Preconditions not satisfiable\n"
             << endl;
      } else {
        cout << "\n✗ " << testName << " FAILED: " << errorMsg << "\n" << endl;
      }
    } catch (const exception &e) {
      cout << "\n✗ " << testName << " FAILED: " << e.what() << "\n" << endl;
    }
  }

private:
  string getModeString() {
    switch (mode) {
    case TestMode::ORIGINAL:
      return "Original (No Rewrite)";
    case TestMode::REWRITE_ONLY:
      return "Rewrite Only (No Backend)";
    case TestMode::FULL_PIPELINE:
      return "Full Pipeline (With Backend)";
    }
    return "Unknown";
  }

  void runOriginal(unique_ptr<Spec> spec, const vector<string> &ts) {
    Program atc = genATC(*spec, ts);
    Printer printer;
    printer.visitProgram(atc);
  }

  void runRewriteOnly(unique_ptr<Spec> spec, const vector<string> &ts) {
    auto factory = make_unique<Ecommerce::EcommerceFunctionFactory>(backendUrl);
    Tester tester(factory.get());

    unique_ptr<Program> testApiATC = tester.generateATC(std::move(spec), ts);
  }

  void runFullPipeline(unique_ptr<Spec> spec, const vector<string> &ts) {
    SymbolTable *symbolTable = new SymbolTable(nullptr);

    auto factory = make_unique<Ecommerce::EcommerceFunctionFactory>(backendUrl);
    Tester tester(factory.get());

    unique_ptr<Program> testApiATC = tester.generateATC(std::move(spec), ts);

    vector<Expr *> inputVars;
    ValueEnvironment *valueEnv = new ValueEnvironment(nullptr);

    unique_ptr<Program> ctc =
        tester.generateCTC(std::move(testApiATC), inputVars, valueEnv);

    if (!ctc) {
      cout << "\n[RESULT] UNSAT - Test preconditions cannot be satisfied"
           << endl;
      delete symbolTable;
      delete valueEnv;
      throw runtime_error("UNSAT: Preconditions not satisfiable");
    }

    cout << "\n[FINAL CTC]" << endl;
    Printer printer;
    printer.visitProgram(*ctc);

    delete symbolTable;
    delete valueEnv;
  }
};

// ============================================
// 25 COMPREHENSIVE RESTAURANT TESTS
// ============================================

namespace RestaurantTests {

// ========================================
// DEPTH 1: Single API Call Tests
// ========================================

void test01_registerLogin(TestExecutor &executor) {
  executor.runTest("Test 01: Register Customer-->Login (Depth=2)",
                   makeRestaurantSpec(),
                   {"registerCustomerOk", "loginCustomerOk"});
}

void test02_loginFailure(TestExecutor &executor) {
  executor.runTest("Test 02: loginCustomerErr (Depth=1)", makeRestaurantSpec(),
                   {"loginCustomerErr"}
                   // {"loginCustomerOk"} // Should fail: user not in U
  );
}

void test03_browseOnly(TestExecutor &executor) {
  executor.runTest(
      "Test 03: registerCustomerOk → loginWrongPasswordErr (Depth=2)",
      makeRestaurantSpec(), {"registerCustomerOk", "loginWrongPasswordErr"}
      // {"browseRestaurantsOk"} // Public API, no auth needed
  );
}

// ========================================
// DEPTH 2: Two API Call Tests
// ========================================

void test04_Login(TestExecutor &executor) {
  executor.runTest("Test 04: loginCustomerOk (Depth=1, Expected UNSAT)",
                   makeRestaurantSpec(),
                   {"loginCustomerOk"} // {"loginOk"} // should be UNSAT
  );
}

void test05_registerOwnerAndLogin(TestExecutor &executor) {
  executor.runTest("Test 05: registerCustomerOk → loginCustomerOk → "
                   "addToCartRestaurantOk (Depth=3, Expected UNSAT)",
                   makeRestaurantSpec(),
                   {"registerCustomerOk", "loginCustomerOk",
                    "addToCartRestaurantOk"} // unsat
                   // {"registerOwnerOk", "loginOk"} // should be SAT
  );
}

void test06_registerAgentAndLogin(TestExecutor &executor) {
  executor.runTest(
      "Test 06: registerOwnerOk → loginOwnerOk → createRestaurantOk → "
      "placeOrderOk (Depth=4, Expected UNSAT)",
      makeRestaurantSpec(),
      {"registerOwnerOk", "loginOwnerOk", "createRestaurantOk", "placeOrderOk"}
      // {"registerAgentOk", "loginOk"} // should be SAT
  );
}

// ========================================
// DEPTH 4: Three API Call Tests
// ========================================

void test07_loginBrowseView(TestExecutor &executor) {
  executor.runTest("Test 07: registerOwnerOk → loginOwnerOk → "
                   "createRestaurantOk → registerCustomerOk → loginCustomerOk "
                   "→ browseRestaurantsOk → viewMenuOk (Depth=7)",
                   makeRestaurantSpec(),
                   {"registerOwnerOk",     // 1. Owner registers
                    "loginOwnerOk",        // 2. Owner logs in
                    "createRestaurantOk",  // 3. Owner creates restaurant
                    "registerCustomerOk",  // 4. Customer registers
                    "loginCustomerOk",     // 5. Customer logs in
                    "browseRestaurantsOk", // 6. Customer browses
                    "viewMenuOk"}          // should be SAT
  );
}

void test08_loginAndAddToCart(TestExecutor &executor) {
  executor.runTest(
      "Test 08: registerOwnerOk → loginOwnerOk → createRestaurantOk → "
      "addMenuItemOk → registerCustomerOk → loginCustomerOk → "
      "browseRestaurantsOk → viewMenuOk → addToCartRestaurantOk (Depth=9)",
      makeRestaurantSpec(),
      {"registerOwnerOk",       // 1. Owner registers
       "loginOwnerOk",          // 2. Owner logs in
       "createRestaurantOk",    // 3. Owner creates restaurant
       "addMenuItemOk",         // 4. Owner adds menu item
       "registerCustomerOk",    // 5. Customer registers
       "loginCustomerOk",       // 6. Customer logs in
       "browseRestaurantsOk",   // 7. Customer browses
       "viewMenuOk",            // 8. Customer views menu
       "addToCartRestaurantOk"} // 9. Customer adds to cart
  );
}

void test09_loginAndReview(TestExecutor &executor) {
  executor.runTest("Test 09: registerOwnerOk → loginOwnerOk → "
                   "createRestaurantOk → addMenuItemOk → registerCustomerOk → "
                   "loginCustomerOk → browseRestaurantsOk → viewMenuOk → "
                   "addToCartRestaurantOk → placeOrderOk (Depth=10)",
                   makeRestaurantSpec(),
                   {"registerOwnerOk",     // 1. Owner registers
                    "loginOwnerOk",        // 2. Owner logs in
                    "createRestaurantOk",  // 3. Owner creates restaurant
                    "addMenuItemOk",       // 4. Owner adds menu item
                    "registerCustomerOk",  // 5. Customer registers
                    "loginCustomerOk",     // 6. Customer logs in
                    "browseRestaurantsOk", // 7. Customer browses
                    "viewMenuOk",          // 8. Customer views menu
                    "addToCartRestaurantOk", "placeOrderOk"});
}

void test10_reviewWithoutLogin(TestExecutor &executor) {
  executor.runTest(
      "Test 10: Full Order Lifecycle with Review (Depth=19)",
      makeRestaurantSpec(),
      {"registerOwnerOk",          // 1. Owner registers
       "loginOwnerOk",             // 2. Owner logs in
       "createRestaurantOk",       // 3. Owner creates restaurant
       "addMenuItemOk",            // 4. Owner adds menu item
       "registerCustomerOk",       // 5. Customer registers
       "loginCustomerOk",          // 6. Customer logs in
       "browseRestaurantsOk",      // 7. Customer browses
       "viewMenuOk",               // 8. Customer views menu
       "addToCartRestaurantOk",    // 9. Customer adds to cart
       "placeOrderOk",             // 10. Customer places order (status: placed)
       "registerAgentOk",          // 11. Delivery agent registers
       "loginAgentOk",             // 12. Delivery agent logs in
       "updateOrderStatusOwnerOk", // 13. Owner accepts order (status: accepted)
       "updateOrderStatusOwnerOk", // 14. Owner marks preparing (status:
                                   // preparing)
       "updateOrderStatusOwnerOk", // 15. Owner marks ready (status: ready)
       "assignOrderOk",            // 16. Owner assigns delivery agent
       "updateOrderStatusAgentOk", // 17. Agent picks up (status: picked_up)
       "updateOrderStatusAgentOk", // 18. Agent delivers (status: delivered)
       "leaveReviewOk"});
}

void test11_fullCustomerOrder(TestExecutor &executor) {
  executor.runTest(
      "Test 11: registerOwnerOk → loginOwnerOk → createRestaurantOk (Depth=3)",
      makeRestaurantSpec(),
      {"registerOwnerOk", "loginOwnerOk", "createRestaurantOk"}
      // {"registerCustomerOk", "loginOk", "addToCartOk", "placeOrderOk"} //
      // should fail no browse restaurants
  );
}

void test12_ownerCreateRestaurant(TestExecutor &executor) {
  executor.runTest(
      "Test 12: registerOwnerOk → loginOwnerOk → createRestaurantOk → "
      "addMenuItemOk → addMenuItemOk → addMenuItemOk (Depth=6)",
      makeRestaurantSpec(),
      {"registerOwnerOk", "loginOwnerOk", "createRestaurantOk", "addMenuItemOk",
       "addMenuItemOk", "addMenuItemOk"}
      // {"registerOwnerOk", "loginOk", "createRestaurantOk", "addMenuItemOk"}
  );
}

void test13_cartWithoutItems(TestExecutor &executor) {
  executor.runTest(
      "Test 13: registerOwnerOk → loginOwnerOk → createRestaurantOk → "
      "addMenuItemOk → registerCustomerOk → loginCustomerOk → "
      "browseRestaurantsOk → viewMenuOk → addToCartRestaurantOk (Depth=9)",
      makeRestaurantSpec(),
      {"registerOwnerOk", "loginOwnerOk", "createRestaurantOk", "addMenuItemOk",
       "registerCustomerOk", "loginCustomerOk", "browseRestaurantsOk",
       "viewMenuOk", "addToCartRestaurantOk"}
      // {"registerCustomerOk", "loginOk", "placeOrderErr"} // Should fail: cart
      // empty
  );
}

// ========================================
// DEPTH 5: Five API Call Tests
// ========================================

void test14_customerFullWorkflow(TestExecutor &executor) {
  executor.runTest(
      "Test 14: registerOwnerOk → loginOwnerOk → createRestaurantOk → "
      "addMenuItemOk → registerCustomerOk → loginCustomerOk → "
      "browseRestaurantsOk → viewMenuOk → addToCartRestaurantOk → placeOrderOk "
      "(Depth=10)",
      makeRestaurantSpec(),
      {"registerOwnerOk", "loginOwnerOk", "createRestaurantOk", "addMenuItemOk",
       "registerCustomerOk", "loginCustomerOk", "browseRestaurantsOk",
       "viewMenuOk", "addToCartRestaurantOk", "placeOrderOk"}
      // {"registerCustomerOk", "loginOk", "browseRestaurantsOk",
      // "addToCartRestaurantOk", "placeOrderOk"}
  );
}

void test15_ownerFullSetup(TestExecutor &executor) {
  executor.runTest(
      "Test 15: registerOwnerOk → loginOwnerOk → createRestaurantOk → "
      "addMenuItemOk → registerCustomerOk → loginCustomerOk → "
      "browseRestaurantsOk → viewMenuOk → addToCartRestaurantOk → placeOrderOk "
      "→ updateOrderStatusOwnerOk x3 (Depth=13)",
      makeRestaurantSpec(),
      {"registerOwnerOk", "loginOwnerOk", "createRestaurantOk", "addMenuItemOk",
       "registerCustomerOk", "loginCustomerOk", "browseRestaurantsOk",
       "viewMenuOk", "addToCartRestaurantOk", "placeOrderOk",
       "updateOrderStatusOwnerOk", // accepted
       "updateOrderStatusOwnerOk", // preparing
       "updateOrderStatusOwnerOk"}
      // {"registerOwnerOk", "loginOk", "createRestaurantOk", "addMenuItemOk",
      // "addMenuItemOk"}
  );
}

void test16_agentAssignOrder(TestExecutor &executor) {
  executor.runTest(
      "Test 16: registerCustomerOk → registerCustomerOk (Depth=2, Expected "
      "UNSAT - Duplicate Registration)",
      makeRestaurantSpec(), {"registerOwnerOk", "registerOwnerOk"}
      //{"registerAgentOk", "loginOk", "placeOrderOk", "assignOrderOk",
      //"updateOrderStatusAgentOk"} // delivery agent can't place order
  );
}

// ========================================
// DEPTH 6: Six API Call Tests
// ========================================

void test17_ownerManageOrder(TestExecutor &executor) {
  executor.runTest(
      "Test 17: registerOwnerOk → loginOwnerOk → createRestaurantOk → "
      "addMenuItemOk → registerCustomerOk → loginCustomerOk → "
      "browseRestaurantsOk → viewMenuOk → addToCartRestaurantOk → placeOrderOk "
      "→ registerAgentOk → loginAgentOk → updateOrderStatusAgentOk (Depth=13, "
      "Expected UNSAT)",
      makeRestaurantSpec(),
      {"registerOwnerOk", "loginOwnerOk", "createRestaurantOk", "addMenuItemOk",
       "registerCustomerOk", "loginCustomerOk", "browseRestaurantsOk",
       "viewMenuOk", "addToCartRestaurantOk", "placeOrderOk", "registerAgentOk",
       "loginAgentOk", "updateOrderStatusAgentOk"}
      //{"registerOwnerOk", "loginOk", "createRestaurantOk", "placeOrderOk"} //
      // should be UNSAT, owner can't place order
  );
}

void test18_multipleCartAdditions(TestExecutor &executor) {
  executor.runTest(
      "Test 18: loginCustomerOk → browseRestaurantsOk → addToCartRestaurantOk "
      "x3 → placeOrderOk (Depth=6, Expected UNSAT - No Registration)",
      makeRestaurantSpec(),
      {"loginCustomerOk", "browseRestaurantsOk", "addToCartRestaurantOk",
       "addToCartRestaurantOk", "addToCartRestaurantOk", "placeOrderOk"}
      // should return unsat as no registration
  );
}

void test19_wrongRoleAccess(TestExecutor &executor) {
  executor.runTest(
      "Test 19: registerCustomerOk → loginCustomerOk → "
      "createRestaurantCustomerErr (Depth=3, Expected UNSAT - Customer Can't "
      "Create Restaurant)",
      makeRestaurantSpec(),
      {"registerCustomerOk", "loginCustomerOk", "createRestaurantCustomerErr"});
}

// ========================================
// DEPTH 7: Seven API Call Tests
// ========================================

void test20_fullLifecycle(TestExecutor &executor) {
  executor.runTest("Test 20: registerCustomerOk → loginCustomerOk → "
                   "browseRestaurantsOk → viewMenuOk → addToCartRestaurantOk → "
                   "placeOrderOk → leaveReviewOk (Depth=7, Expected UNSAT)",
                   makeRestaurantSpec(),
                   {"registerCustomerOk", "loginCustomerOk",
                    "browseRestaurantsOk", "viewMenuOk",
                    "addToCartRestaurantOk", "placeOrderOk", "leaveReviewOk"});
}

void test21_ownerCompleteFlow(TestExecutor &executor) {
  executor.runTest("Test 21: registerOwnerOk → loginOwnerOk → "
                   "createRestaurantOk → addMenuItemOk x3 (Depth=6)",
                   makeRestaurantSpec(),
                   {"registerOwnerOk", "loginOwnerOk", "createRestaurantOk",
                    "addMenuItemOk", "addMenuItemOk", "addMenuItemOk"});
}

void test22_complexOrderManagement(TestExecutor &executor) {
  executor.runTest("Test 23: registerOwnerOk → loginOwnerOk → "
                   "createRestaurantOk → addMenuItemOk → registerCustomerOk → "
                   "loginCustomerOk → browseRestaurantsOk → viewMenuOk → "
                   "addToCartRestaurantOk → placeOrderOk (Depth=10)",
                   makeRestaurantSpec(),
                   {"registerOwnerOk",       // 1. Owner registers
                    "loginOwnerOk",          // 2. Owner logs in
                    "createRestaurantOk",    // 3. Owner creates restaurant
                    "addMenuItemOk",         // 4. Owner adds menu item
                    "registerCustomerOk",    // 5. Customer registers
                    "loginCustomerOk",       // 6. Customer logs in
                    "browseRestaurantsOk",   // 7. Customer browses
                    "viewMenuOk",            // 8. Customer views menu
                    "addToCartRestaurantOk", // 9. Customer adds to cart
                    "placeOrderOk"});
}

void test23_invalidSequence(TestExecutor &executor) {
  executor.runTest(
      "Test 24: registerCustomerOk → loginCustomerOk → leaveReviewOk (Depth=3, "
      "Expected UNSAT - Review Before Order)",
      makeRestaurantSpec(),
      {"registerCustomerOk", "loginCustomerOk", "leaveReviewOk"}
      // Should fail: can't review restaurant you haven't ordered from
  );
}

void test24_deepWorkflow(TestExecutor &executor) {
  executor.runTest("Test 25: registerOwnerOk → loginOwnerOk → "
                   "createRestaurantOk → addMenuItemOk x5 → "
                   "updateOrderStatusOwnerOk x2 (Depth=10, Expected UNSAT)",
                   makeRestaurantSpec(),
                   {"registerOwnerOk", "loginOwnerOk", "createRestaurantOk",
                    "addMenuItemOk", "addMenuItemOk", "addMenuItemOk",
                    "addMenuItemOk", "addMenuItemOk",
                    "updateOrderStatusOwnerOk", "updateOrderStatusOwnerOk"});
}

void test25_registerCustomerDuplicate(TestExecutor &executor) {
  executor.runTest("Test 26: registerCustomerOk → registerCustomerOk (Depth=2, "
                   "Expected UNSAT - Duplicate Email)",
                   makeRestaurantSpec(),
                   {"registerCustomerOk",
                    "registerCustomerOk"}); // edge case should return unsat
}

} // namespace RestaurantTests

// ============================================
// LIBRARY TESTS
// ============================================

namespace LibraryTests {

// ========================================
// DEPTH 1: Single API Call Tests
// ========================================

void test01_getAllBooks(LibraryTestExecutor &executor) {
  executor.runTest("Test 01: Get All Books (Depth=1)", makeLibrarySpec(),
                   {"getAllBooksOk"});
}

void test02_getAllStudents(LibraryTestExecutor &executor) {
  executor.runTest("Test 02: Get All Students (Depth=1)", makeLibrarySpec(),
                   {"getAllStudentsOk"});
}

void test03_saveBook(LibraryTestExecutor &executor) {
  executor.runTest("Test 03: Save Book (Depth=1)", makeLibrarySpec(),
                   {"saveBookOk"});
}

void test04_saveStudent(LibraryTestExecutor &executor) {
  executor.runTest("Test 04: Save Student (Depth=1)", makeLibrarySpec(),
                   {"saveStudentOk"});
}

// ========================================
// DEPTH 2: Two API Call Tests
// ========================================

void test05_saveAndGetBook(LibraryTestExecutor &executor) {
  executor.runTest("Test 05: Save Book → Get Book (Depth=2)", makeLibrarySpec(),
                   {"saveBookOk", "getBookByCodeOk"});
}

void test06_saveAndGetStudent(LibraryTestExecutor &executor) {
  executor.runTest("Test 06: Save Student → Get Student (Depth=2)",
                   makeLibrarySpec(), {"saveStudentOk", "getStudentByIdOk"});
}

void test07_saveBookTwice(LibraryTestExecutor &executor) {
  executor.runTest("Test 07: Save Two Books (Depth=2)", makeLibrarySpec(),
                   {"saveBookOk", "saveBookOk"});
}

void test08_getBookNotFound(LibraryTestExecutor &executor) {
  executor.runTest("Test 08: Get Book Not Found (Depth=1)", makeLibrarySpec(),
                   {"getBookByCodeErr"});
}

// ========================================
// DEPTH 3: Three API Call Tests
// ========================================

void test09_bookCRUD(LibraryTestExecutor &executor) {
  executor.runTest("Test 09: Book CRUD - Save → Update → Delete (Depth=3)",
                   makeLibrarySpec(),
                   {"saveBookOk", "updateBookOk", "deleteBookOk"});
}

void test10_studentCRUD(LibraryTestExecutor &executor) {
  executor.runTest("Test 10: Student CRUD - Save → Update → Delete (Depth=3)",
                   makeLibrarySpec(),
                   {"saveStudentOk", "updateStudentOk", "deleteStudentOk"});
}

void test11_createBookAndStudent(LibraryTestExecutor &executor) {
  executor.runTest("Test 11: Create Book and Student (Depth=2)",
                   makeLibrarySpec(), {"saveBookOk", "saveStudentOk"});
}

// ========================================
// DEPTH 4+: Request/Loan Flow Tests
// ========================================

void test12_createRequest(LibraryTestExecutor &executor) {
  executor.runTest("Test 12: Create Request Flow (Depth=3)", makeLibrarySpec(),
                   {"saveBookOk", "saveStudentOk", "saveRequestOk"});
}

void test13_acceptRequest(LibraryTestExecutor &executor) {
  executor.runTest(
      "Test 13: Accept Request Flow (Depth=4)", makeLibrarySpec(),
      {"saveBookOk", "saveStudentOk", "saveRequestOk", "acceptRequestOk"});
}

void test14_fullBorrowReturn(LibraryTestExecutor &executor) {
  executor.runTest("Test 14: Full Borrow/Return Lifecycle (Depth=5)",
                   makeLibrarySpec(),
                   {"saveBookOk", "saveStudentOk", "saveRequestOk",
                    "acceptRequestOk", "returnBookOk"});
}

void test15_directLoan(LibraryTestExecutor &executor) {
  executor.runTest("Test 15: Direct Loan Creation (Depth=3)", makeLibrarySpec(),
                   {"saveBookOk", "saveStudentOk", "saveLoanOk"});
}

void test16_rejectRequest(LibraryTestExecutor &executor) {
  executor.runTest(
      "Test 16: Reject Request (Depth=4)", makeLibrarySpec(),
      {"saveBookOk", "saveStudentOk", "saveRequestOk", "deleteRequestOk"});
}

// ========================================
// NEGATIVE TESTS (Expected UNSAT)
// ========================================

void test17_requestWithoutBook(LibraryTestExecutor &executor) {
  executor.runTest("Test 17: Request Without Book (Should be UNSAT)",
                   makeLibrarySpec(),
                   {"saveStudentOk", "saveRequestOk"}); // No book - should fail
}

void test18_requestWithoutStudent(LibraryTestExecutor &executor) {
  executor.runTest("Test 18: Request Without Student (Should be UNSAT)",
                   makeLibrarySpec(),
                   {"saveBookOk", "saveRequestOk"}); // No student - should fail
}

void test19_acceptWithoutRequest(LibraryTestExecutor &executor) {
  executor.runTest("Test 19: Accept Without Request (Should be UNSAT)",
                   makeLibrarySpec(),
                   {"acceptRequestOk"}); // No request - should fail
}

void test20_returnWithoutLoan(LibraryTestExecutor &executor) {
  executor.runTest("Test 20: Return Without Loan (Should be UNSAT)",
                   makeLibrarySpec(),
                   {"returnBookOk"}); // No loan - should fail
}

// ========================================
// COMPLEX WORKFLOWS
// ========================================

void test21_multipleBooks(LibraryTestExecutor &executor) {
  executor.runTest("Test 21: Multiple Books (Depth=5)", makeLibrarySpec(),
                   {"saveBookOk", "saveBookOk", "saveBookOk", "getAllBooksOk",
                    "getBookByCodeOk"});
}

void test22_multipleStudents(LibraryTestExecutor &executor) {
  executor.runTest("Test 22: Multiple Students (Depth=5)", makeLibrarySpec(),
                   {"saveStudentOk", "saveStudentOk", "saveStudentOk",
                    "getAllStudentsOk", "getStudentByIdOk"});
}

void test23_multipleBorrowings(LibraryTestExecutor &executor) {
  executor.runTest("Test 23: Multiple Borrowings (Depth=7)", makeLibrarySpec(),
                   {"saveBookOk", "saveBookOk", "saveStudentOk",
                    "saveRequestOk", "saveRequestOk", "acceptRequestOk",
                    "acceptRequestOk"});
}

void test24_fullLibraryWorkflow(LibraryTestExecutor &executor) {
  executor.runTest(
      "Test 24: Full Library Workflow (Depth=8)", makeLibrarySpec(),
      {"saveBookOk", "saveBookOk", "saveStudentOk", "saveStudentOk",
       "saveRequestOk", "acceptRequestOk", "returnBookOk", "getAllLoansOk"});
}

void test25_complexScenario(LibraryTestExecutor &executor) {
  executor.runTest("Test 25: Complex Multi-User Scenario (Depth=10)",
                   makeLibrarySpec(),
                   {"saveBookOk", "saveBookOk", "saveBookOk", "saveStudentOk",
                    "saveStudentOk", "saveRequestOk", "saveRequestOk",
                    "acceptRequestOk", "returnBookOk", "getAllRequestsOk"});
}
} // namespace LibraryTests

// ============================================
// E-COMMERCE COMPREHENSIVE TESTS (30 tests: 21 SAT, 9 UNSAT)
// ============================================

namespace EcommerceTests {
// ╔════════════════════════════════════════════════════════════╗
// ║  SAT TESTS (21 tests = 70%)                                ║
// ╚════════════════════════════════════════════════════════════╝

// SAT-01: Register Buyer (Depth=1)
void test01_registerBuyer(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 01: Register Buyer (Depth=1)",
                   makeEcommerceSpec(), {"registerBuyerOk"});
}

// SAT-02: Register Seller (Depth=1)
void test02_registerSeller(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 02: Register Seller (Depth=1)",
                   makeEcommerceSpec(), {"registerSellerOk"});
}

// SAT-03: Browse Products - Public API (Depth=1)
void test03_browseProducts(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 03: Browse All Products - Public (Depth=1)",
                   makeEcommerceSpec(), {"getAllProductsOk"});
}

// SAT-04: Register Buyer → Login (Depth=2)
void test04_buyerRegisterLogin(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 04: Register Buyer → Login (Depth=2)",
                   makeEcommerceSpec(), {"registerBuyerOk", "loginBuyerOk"});
}

// SAT-05: Register Seller → Login (Depth=2)
void test05_sellerRegisterLogin(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 05: Register Seller → Login (Depth=2)",
                   makeEcommerceSpec(), {"registerSellerOk", "loginSellerOk"});
}

// SAT-06: Seller Creates Product (Depth=3)
void test06_sellerCreateProduct(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 06: Seller Creates Product (Depth=3)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "createProductOk"});
}

// SAT-07: Seller Creates Multiple Products (Depth=5)
void test07_sellerCreateMultipleProducts(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 07: Seller Creates 3 Products (Depth=5)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "createProductOk",
                    "createProductOk", "createProductOk"});
}

// SAT-08: Seller Creates → Updates Product (Depth=4)
void test08_sellerUpdateProduct(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 08: Seller Creates → Updates Product (Depth=4)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "createProductOk",
                    "updateProductOk"});
}

// SAT-09: Seller Creates → Deletes Product (Depth=4)
void test09_sellerDeleteProduct(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 09: Seller Creates → Deletes Product (Depth=4)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "createProductOk",
                    "deleteProductOk"});
}

// SAT-10: Seller Views Own Inventory (Depth=4)
void test10_sellerViewInventory(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 10: Seller Creates → Views Inventory (Depth=4)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "createProductOk",
                    "getSellerProductsOk"});
}

// SAT-11: Seller creates product → Buyer browses (Depth=6)
void test11_multiUserBrowse(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 11: Seller Setup → Buyer Browses (Depth=6)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "createProductOk",
                    "registerBuyerOk", "loginBuyerOk", "getAllProductsOk"});
}

// SAT-12: Seller creates product → Buyer adds to cart (Depth=7)
void test12_multiUserAddToCart(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 12: Seller Setup → Buyer Adds to Cart (Depth=7)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "createProductOk",
                    "registerBuyerOk", "loginBuyerOk", "addToCartOk"});
}

// SAT-13: Seller creates product → Buyer views cart (Depth=8)
void test13_multiUserViewCart(EcommerceTestExecutor &executor) {
  executor.runTest(
      "[SAT] Test 13: Seller Setup → Buyer Adds & Views Cart (Depth=8)",
      makeEcommerceSpec(),
      {"registerSellerOk", "loginSellerOk", "createProductOk",
       "registerBuyerOk", "loginBuyerOk", "addToCartOk", "getCartOk"});
}

// SAT-14: Seller creates product → Buyer creates order (Depth=8)
void test14_multiUserCreateOrder(EcommerceTestExecutor &executor) {
  executor.runTest(
      "[SAT] Test 14: Seller Setup → Buyer Creates Order (Depth=8)",
      makeEcommerceSpec(),
      {"registerSellerOk", "loginSellerOk", "createProductOk",
       "registerBuyerOk", "loginBuyerOk", "addToCartOk", "createOrderOk"});
}

// SAT-15: Full order flow → Buyer views orders (Depth=9)
void test15_multiUserViewOrders(EcommerceTestExecutor &executor) {
  executor.runTest(
      "[SAT] Test 15: Full Order Flow → Buyer Views Orders (Depth=9)",
      makeEcommerceSpec(),
      {"registerSellerOk", "loginSellerOk", "createProductOk",
       "registerBuyerOk", "loginBuyerOk", "addToCartOk", "createOrderOk",
       "getBuyerOrdersOk"});
}

// SAT-16: Full order flow → Seller views orders (Depth=9)
void test16_sellerViewsOrders(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 16: Full Order → Seller Views Orders (Depth=9)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "createProductOk",
                    "registerBuyerOk", "loginBuyerOk", "addToCartOk",
                    "createOrderOk", "getSellerOrdersOk"});
}

// SAT-17: Full flow → Buyer creates review (Depth=9)
void test17_multiUserCreateReview(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 17: Full Order → Buyer Creates Review (Depth=9)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "createProductOk",
                    "registerBuyerOk", "loginBuyerOk", "addToCartOk",
                    "createOrderOk", "createReviewOk"});
}

// SAT-18: Complete E-Commerce Journey (Depth=12)
void test18_completeEcommerceFlow(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 18: Complete E-Commerce Flow (Depth=12)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "createProductOk",
                    "createProductOk", "registerBuyerOk", "loginBuyerOk",
                    "getAllProductsOk", "addToCartOk", "addToCartOk",
                    "createOrderOk", "getBuyerOrdersOk", "createReviewOk"});
}

// SAT-19: Multiple Orders by Same Buyer (Depth=12)
void test19_multipleOrders(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 19: Buyer Places Multiple Orders (Depth=12)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "createProductOk",
                    "createProductOk", "createProductOk", "registerBuyerOk",
                    "loginBuyerOk", "addToCartOk", "createOrderOk",
                    "addToCartOk", "createOrderOk", "getBuyerOrdersOk"});
}

// SAT-20: Seller Full Product Management (Depth=10)
void test20_sellerFullManagement(EcommerceTestExecutor &executor) {
  executor.runTest("[SAT] Test 20: Seller Full Product Management (Depth=10)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "createProductOk",
                    "createProductOk", "createProductOk", "updateProductOk",
                    "updateProductOk", "deleteProductOk", "getSellerProductsOk",
                    "getSellerOrdersOk"});
}

// SAT-21: Deep E-Commerce Workflow (Depth=15)
void test21_deepWorkflow(EcommerceTestExecutor &executor) {
  executor.runTest(
      "[SAT] Test 21: Deep E-Commerce Workflow (Depth=15)", makeEcommerceSpec(),
      {"registerSellerOk", "loginSellerOk", "createProductOk",
       "createProductOk", "createProductOk", "registerBuyerOk", "loginBuyerOk",
       "getAllProductsOk", "addToCartOk", "addToCartOk", "createOrderOk",
       "createReviewOk", "addToCartOk", "createOrderOk", "createReviewOk"});
}

// ╔════════════════════════════════════════════════════════════╗
// ║  UNSAT TESTS (9 tests = 30%)                               ║
// ╚════════════════════════════════════════════════════════════╝

// UNSAT-01: Login without registration (Depth=1)
void test22_loginWithoutRegister(EcommerceTestExecutor &executor) {
  executor.runTest(
      "[UNSAT] Test 22: Buyer Login Without Registration (Depth=1)",
      makeEcommerceSpec(), {"loginBuyerOk"});
}

// UNSAT-02: Seller login without registration (Depth=1)
void test23_sellerLoginWithoutRegister(EcommerceTestExecutor &executor) {
  executor.runTest(
      "[UNSAT] Test 23: Seller Login Without Registration (Depth=1)",
      makeEcommerceSpec(), {"loginSellerOk"});
}

// UNSAT-03: Duplicate registration (Depth=2)
void test24_duplicateRegistration(EcommerceTestExecutor &executor) {
  executor.runTest("[UNSAT] Test 24: Duplicate Buyer Registration (Depth=2)",
                   makeEcommerceSpec(), {"registerBuyerOk", "registerBuyerOk"});
}

// UNSAT-04: Buyer tries to create product (Depth=3)
void test25_buyerCannotCreateProduct(EcommerceTestExecutor &executor) {
  executor.runTest("[UNSAT] Test 25: Buyer Cannot Create Product (Depth=3)",
                   makeEcommerceSpec(),
                   {"registerBuyerOk", "loginBuyerOk", "createProductOk"});
}

// UNSAT-05: Seller tries to add to cart (Depth=3)
void test26_sellerCannotAddToCart(EcommerceTestExecutor &executor) {
  executor.runTest("[UNSAT] Test 26: Seller Cannot Add to Cart (Depth=3)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "addToCartOk"});
}

// UNSAT-06: Seller tries to create order (Depth=4)
void test27_sellerCannotCreateOrder(EcommerceTestExecutor &executor) {
  executor.runTest("[UNSAT] Test 27: Seller Cannot Create Order (Depth=4)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "createProductOk",
                    "createOrderOk"});
}

// UNSAT-07: Buyer adds to cart without product existing (Depth=3)
void test28_addToCartNoProduct(EcommerceTestExecutor &executor) {
  executor.runTest("[UNSAT] Test 28: Add to Cart - No Product Exists (Depth=3)",
                   makeEcommerceSpec(),
                   {"registerBuyerOk", "loginBuyerOk", "addToCartOk"});
}

// UNSAT-08: Create order without items in cart (Depth=6)
void test29_createOrderEmptyCart(EcommerceTestExecutor &executor) {
  executor.runTest("[UNSAT] Test 29: Create Order - Empty Cart (Depth=6)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "createProductOk",
                    "registerBuyerOk", "loginBuyerOk", "createOrderOk"});
}

// UNSAT-09: Create review without placing order (Depth=7)
void test30_reviewWithoutOrder(EcommerceTestExecutor &executor) {
  executor.runTest("[UNSAT] Test 30: Create Review - No Order Placed (Depth=7)",
                   makeEcommerceSpec(),
                   {"registerSellerOk", "loginSellerOk", "createProductOk",
                    "registerBuyerOk", "loginBuyerOk", "addToCartOk",
                    "createReviewOk"});
}
} // namespace EcommerceTests

// ============================================
// MAIN FUNCTION - TEST SELECTION
// ============================================

int main() {
  try {
    // ========================================
    // CONFIGURATION
    // ========================================

    // Choose test mode
    // TestMode mode = TestMode::REWRITE_ONLY;
    // TestMode mode = TestMode::ORIGINAL;
    TestMode mode = TestMode::FULL_PIPELINE; // Needs backend running!

    // Backend URL (only used for FULL_PIPELINE mode)
    string backendUrl = "http://localhost:5002"; // for restaurant

    TestExecutor executor(mode, backendUrl); // for restaurant

    // ========================================
    // RUN TESTS
    // ========================================

    cout << "\n╔════════════════════════════════════════╗"
         << endl; // for restaurant
    cout << "║  TESTGEN - RESTAURANT TEST SUITE      ║"
         << endl; // for restaurant
    cout << "║  Total Tests: 25                       ║"
         << endl;                                          // for restaurant
    cout << "╚════════════════════════════════════════╝\n" // for restaurant
         << endl;                                          // for restaurant

    // RUN ALL 25 TESTS (comment out for selective testing)
    cout << "\n=== DEPTH TESTS ===" << endl; // for restaurant
    // RestaurantTests::test01_registerLogin(executor);
    // RestaurantTests::test02_loginFailure(executor);
    // RestaurantTests::test03_browseOnly(executor);

    // RestaurantTests::test04_Login(executor);
    // RestaurantTests::test05_registerOwnerAndLogin(executor);
    //  RestaurantTests::test06_registerAgentAndLogin(executor);

    // RestaurantTests::test07_loginBrowseView(executor);
    // RestaurantTests::test08_loginAndAddToCart(executor);
    // RestaurantTests::test09_loginAndReview(executor);
    // RestaurantTests::test10_reviewWithoutLogin(executor);

    // RestaurantTests::test11_fullCustomerOrder(executor);
    // RestaurantTests::test12_ownerCreateRestaurant(executor);
    // RestaurantTests::test13_cartWithoutItems(executor);

    // cout << "\n=== DEPTH 5 TESTS ===" << endl;
    // RestaurantTests::test14_customerFullWorkflow(executor);
    // RestaurantTests::test15_ownerFullSetup(executor);
    // RestaurantTests::test16_agentAssignOrder(executor);

    // cout << "\n=== DEPTH 6 TESTS ===" << endl;
    // RestaurantTests::test17_ownerManageOrder(executor);
    // RestaurantTests::test18_multipleCartAdditions(executor);
    // RestaurantTests::test19_wrongRoleAccess(executor);

    // cout << "\n=== DEPTH 7 TESTS ===" << endl;
    // RestaurantTests::test20_fullLifecycle(executor);
    // RestaurantTests::test21_ownerCompleteFlow(executor);

    // cout << "\n===More TESTS ===" << endl;
    // RestaurantTests::test22_complexOrderManagement(executor);
    // RestaurantTests::test23_invalidSequence(executor);
    // RestaurantTests::test24_deepWorkflow(executor);
    RestaurantTests::test25_registerCustomerDuplicate(executor);

    // ========================================
    // E-COMMERCE TESTS
    // ========================================

    // cout << "\n╔════════════════════════════════════════╗" << endl;
    // //uncomment for ecom cout << "║  TESTGEN - E-COMMERCE TEST SUITE       ║"
    // << endl; //uncomment for ecom cout << "║  Total Tests: 30 (21 SAT, 9
    // UNSAT)     ║" << endl; //uncomment for ecom cout <<
    // "╚════════════════════════════════════════╝\n" //uncomment for ecom
    //      << endl; //uncomment for ecom

    // E-commerce backend URL
    // string ecommerceBackendUrl = "http://localhost:3000"; //uncomment for
    // ecom EcommerceTestExecutor ecommerceExecutor(mode, ecommerceBackendUrl);
    // //uncomment for ecom

    // === SAT TESTS ===
    // EcommerceTests::test01_registerBuyer(ecommerceExecutor);
    // EcommerceTests::test02_registerSeller(ecommerceExecutor);
    // EcommerceTests::test03_browseProducts(ecommerceExecutor);
    // EcommerceTests::test04_buyerRegisterLogin(ecommerceExecutor);
    // EcommerceTests::test05_sellerRegisterLogin(ecommerceExecutor);
    // EcommerceTests::test06_sellerCreateProduct(ecommerceExecutor);
    // EcommerceTests::test07_sellerCreateMultipleProducts(ecommerceExecutor);
    // EcommerceTests::test08_sellerUpdateProduct(ecommerceExecutor);
    // EcommerceTests::test09_sellerDeleteProduct(ecommerceExecutor);
    // EcommerceTests::test10_sellerViewInventory(ecommerceExecutor);
    // EcommerceTests::test11_multiUserBrowse(ecommerceExecutor);
    // EcommerceTests::test12_multiUserAddToCart(ecommerceExecutor);
    // EcommerceTests::test13_multiUserViewCart(ecommerceExecutor);
    // EcommerceTests::test14_multiUserCreateOrder(ecommerceExecutor);
    // EcommerceTests::test15_multiUserViewOrders(ecommerceExecutor);
    // EcommerceTests::test16_sellerViewsOrders(ecommerceExecutor);
    // EcommerceTests::test17_multiUserCreateReview(ecommerceExecutor);
    // EcommerceTests::test18_completeEcommerceFlow(ecommerceExecutor);
    // EcommerceTests::test19_multipleOrders(ecommerceExecutor);
    // EcommerceTests::test20_sellerFullManagement(ecommerceExecutor);
    // EcommerceTests::test21_deepWorkflow(ecommerceExecutor);

    // === UNSAT TESTS ===
    // EcommerceTests::test22_loginWithoutRegister(ecommerceExecutor);
    // EcommerceTests::test23_sellerLoginWithoutRegister(ecommerceExecutor);
    // EcommerceTests::test24_duplicateRegistration(ecommerceExecutor);
    // EcommerceTests::test25_buyerCannotCreateProduct(ecommerceExecutor);
    // EcommerceTests::test26_sellerCannotAddToCart(ecommerceExecutor);
    // EcommerceTests::test27_sellerCannotCreateOrder(ecommerceExecutor);
    // EcommerceTests::test28_addToCartNoProduct(ecommerceExecutor);
    // EcommerceTests::test29_createOrderEmptyCart(ecommerceExecutor);
    // EcommerceTests::test30_reviewWithoutOrder(ecommerceExecutor);

    // ========================================
    // CONFIGURATION
    // ========================================

    // Choose test mode
    // TestMode mode = TestMode::REWRITE_ONLY;
    // TestMode mode = TestMode::ORIGINAL;
    // TestMode mode = TestMode::FULL_PIPELINE; // Needs backend running!

    // Backend URL - Spring Boot default port
    // string backendUrl = "http://localhost:8080";

    // LibraryTestExecutor executor(mode, backendUrl);

    // ========================================
    // RUN TESTS
    // ========================================

    // cout << "\n╔════════════════════════════════════════╗" << endl;
    // cout << "║  TESTGEN - LIBRARY TEST SUITE          ║" << endl;
    // cout << "║  Total Tests: 25                       ║" << endl;
    // cout << "╚════════════════════════════════════════╝\n"
    //     << endl;

    // ========== BASIC TESTS ==========
    // cout << "\n=== BASIC SINGLE OPERATION TESTS ===" << endl;
    // LibraryTests::test01_getAllBooks(executor);
    // LibraryTests::test02_getAllStudents(executor);
    // LibraryTests::test03_saveBook(executor);
    // LibraryTests::test04_saveStudent(executor);

    // ========== DEPTH 2 TESTS ==========
    // cout << "\n=== DEPTH 2 TESTS ===" << endl;
    // LibraryTests::test05_saveAndGetBook(executor);
    // LibraryTests::test06_saveAndGetStudent(executor);
    // LibraryTests::test07_saveBookTwice(executor);
    // LibraryTests::test08_getBookNotFound(executor);

    // ========== CRUD TESTS ==========
    // cout << "\n=== CRUD TESTS ===" << endl;
    // LibraryTests::test09_bookCRUD(executor);
    // LibraryTests::test10_studentCRUD(executor);
    // LibraryTests::test11_createBookAndStudent(executor);

    // ========== BORROW FLOW TESTS ==========
    // cout << "\n=== BORROW FLOW TESTS ===" << endl;
    // LibraryTests::test12_createRequest(executor);
    // LibraryTests::test13_acceptRequest(executor);
    // LibraryTests::test14_fullBorrowReturn(executor);
    // LibraryTests::test15_directLoan(executor);
    // LibraryTests::test16_rejectRequest(executor);

    // ========== NEGATIVE TESTS (UNSAT) ==========
    // cout << "\n=== NEGATIVE TESTS (Expected UNSAT) ===" << endl;
    // LibraryTests::test17_requestWithoutBook(executor);
    // LibraryTests::test18_requestWithoutStudent(executor);
    // LibraryTests::test19_acceptWithoutRequest(executor);
    // LibraryTests::test20_returnWithoutLoan(executor);

    // ========== COMPLEX WORKFLOW TESTS ==========
    // cout << "\n=== COMPLEX WORKFLOW TESTS ===" << endl;
    // LibraryTests::test21_multipleBooks(executor);
    // LibraryTests::test22_multipleStudents(executor);
    // LibraryTests::test23_multipleBorrowings(executor);
    // LibraryTests::test24_fullLibraryWorkflow(executor);
    // LibraryTests::test25_complexScenario(executor);

    // cout << "\n╔════════════════════════════════════════╗" << endl;
    // cout << "║  ALL TESTS COMPLETED                   ║" << endl;
    // cout << "╚════════════════════════════════════════╝\n"
    //     << endl;
  } catch (const exception &e) {
    cerr << "\n❌ FATAL ERROR: " << e.what() << endl;
    return 1;
  }

  return 0;
}
