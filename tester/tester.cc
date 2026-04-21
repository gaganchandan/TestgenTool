#include "tester.hh"
#include "../language/printer.hh"
#include "../language/rewrite_globals_visitor.hh"
#include "../tester/algo.hpp"
#include <iostream>
#include <set>

static const std::map<std::string, std::set<std::string>>
    operationProducesState = {

        // ========================================
        // LIBRARY: Book operations
        // ========================================
        {"saveBookOk", {"B"}},
        {"updateBookOk", {"B"}},
        {"deleteBookOk", {}},
        {"getAllBooksOk", {}},
        {"getBookByCodeOk", {}},
        {"getBookByCodeErr", {}},

        // LIBRARY: Student operations
        {"saveStudentOk", {"S"}},
        {"updateStudentOk", {"S"}},
        {"deleteStudentOk", {}},
        {"getAllStudentsOk", {}},
        {"getStudentByIdOk", {}},
        {"getStudentByIdErr", {}},

        // LIBRARY: Request operations
        {"saveRequestOk", {"Req"}},
        {"deleteRequestOk", {}},
        {"getAllRequestsOk", {}},
        {"getRequestByIdOk", {}},

        // LIBRARY: Loan operations
        {"acceptRequestOk", {"Loans"}}, // Creates loan, removes request
        {"returnBookOk", {}},
        {"saveLoanOk", {"Loans"}},
        {"getAllLoansOk", {}},
        {"getLoanByIdOk", {}},

        // ========================================
        // RESTAURANT APP OPERATIONS
        // ========================================

        // Registration operations - add to U (users) and Roles
        {"registerOwnerOk", {"U", "Roles"}},
        {"registerCustomerOk", {"U", "Roles"}},
        {"registerAgentOk", {"U", "Roles"}},

        // Login operations - add to T (tokens)
        {"loginOwnerOk", {"T"}},
        {"loginCustomerOk", {"T"}},
        {"loginAgentOk", {"T"}},
        {"loginOk", {"T"}},

        // Restaurant management
        {"createRestaurantOk", {"R", "Owners"}},
        {"addMenuItemOk", {"M"}},

        // Customer operations
        {"addToCartRestaurantOk", {"C"}},
        {"placeOrderOk", {"O"}}, // Creates order, clears cart

        // Review operations
        {"leaveReviewOk", {"Rev"}},

        // Agent operations
        {"assignOrderOk", {"Assignments"}},
        {"updateOrderStatusAgentOk", {}},
        {"updateOrderStatusOwnerOk", {}},

        // Read-only operations (produce no new state)
        {"browseRestaurantsOk", {}},
        {"viewMenuOk", {}},

        // Error operations (produce no state - they should fail)
        {"createRestaurantCustomerErr", {}},

        // ========================================
        // E-COMMERCE APP OPERATIONS
        // ========================================

        // Buyer registration & login
        {"registerBuyerOk", {"U", "Roles"}},
        {"loginBuyerOk", {"T"}},

        // Seller registration & login
        {"registerSellerOk", {"U", "Roles"}},
        {"loginSellerOk", {"T"}},

        // Product management (Seller)
        {"createProductOk", {"P", "Stock", "Sellers"}},
        {"updateProductOk", {}},
        {"deleteProductOk", {}},
        {"getSellerProductsOk", {}},

        // Cart operations (Buyer)
        {"addToCartOk", {"C"}}, // Already exists for restaurant, same key
        {"getCartOk", {}},
        {"updateCartOk", {}},

        // Order operations
        {"createOrderOk", {"O", "OrderStatus"}},
        {"getBuyerOrdersOk", {}},
        {"getSellerOrdersOk", {}},
        {"updateOrderStatusOk", {}},

        // Review operations
        {"createReviewOk", {"Rev"}},
        {"getProductReviewsOk", {}},

        // Public read-only
        {"getAllProductsOk", {}},
        {"getProductByIdOk", {}},
};

// Mapping: Operation -> State variables that MUST be non-empty before this op
// These are "hard" dependencies that cannot be satisfied by Z3 alone
static const std::map<std::string, std::set<std::string>>
    operationRequiresState = {

        // ========================================
        // LIBRARY: Required state dependencies
        // ========================================

        // Book operations that need book to exist
        {"getBookByCodeOk", {"B"}},
        {"updateBookOk", {"B"}},
        {"deleteBookOk", {"B"}},

        // Student operations that need student to exist
        {"getStudentByIdOk", {"S"}},
        {"updateStudentOk", {"S"}},
        {"deleteStudentOk", {"S"}},

        // Request operations - need student AND book
        {"saveRequestOk", {"S", "B"}},
        {"getRequestByIdOk", {"Req"}},
        {"deleteRequestOk", {"Req"}},

        // Loan operations
        {"acceptRequestOk", {"Req"}}, // Need existing request
        {"getLoanByIdOk", {"Loans"}},
        {"returnBookOk", {"Loans"}},
        {"saveLoanOk", {"S", "B"}},

        // ========================================
        // RESTAURANT APP REQUIREMENTS
        // ========================================

        // Login requires user to exist
        {"loginOwnerOk", {"U"}},
        {"loginCustomerOk", {"U"}},
        {"loginAgentOk", {"U"}},
        {"loginOk", {"U"}},

        // Restaurant operations require restaurant to exist
        {"viewMenuOk", {"R"}},
        {"addMenuItemOk", {"R"}},

        // Cart operations require menu items to exist
        {"addToCartRestaurantOk", {"M"}},

        // Order operations require cart to have items
        {"placeOrderOk", {"C"}},

        // Review requires order to exist
        {"leaveReviewOk", {"O"}},

        // Agent assignment requires order to exist
        {"assignOrderOk", {"O"}},
        // Customer trying to create restaurant (requires login but will fail
        // role check)
        {"createRestaurantCustomerErr",
         {"U", "T"}}, // Needs user registered and logged in
        // Status update requires assignment
        {"updateOrderStatusAgentOk", {"Assignments"}},
        {"updateOrderStatusOwnerOk", {"O"}},

        // ========================================
        // E-COMMERCE APP REQUIREMENTS
        // ========================================

        // Login requires registration
        {"loginBuyerOk", {"U"}},
        {"loginSellerOk", {"U"}},

        // Product management requires seller to be logged in (T)
        {"createProductOk", {"T"}},
        {"updateProductOk", {"P"}},
        {"deleteProductOk", {"P"}},
        {"getSellerProductsOk", {"T"}},

        // Cart operations require product to exist
        {"addToCartOk",
         {"P"}}, // Buyer adds product to cart - product must exist
        {"getCartOk", {"T"}},
        {"updateCartOk", {"C"}},

        // Order requires cart with items
        {"createOrderOk", {"C"}},
        {"getBuyerOrdersOk", {"T"}},
        {"getSellerOrdersOk", {"T"}},
        {"updateOrderStatusOk", {"O"}},

        // Review requires order to exist
        {"createReviewOk", {"O"}},
        {"getProductReviewsOk", {"P"}},

        // Product browsing - public, no requirements
        // {"getAllProductsOk", {}},
        // {"getProductByIdOk", {"P"}},  // Needs product to exist, but public
};
/**
 * Check if the operation sequence has all required dependencies.
 * Returns true if sequence is TRULY unsatisfiable (missing required prior
 * operations). Returns false if sequence COULD be satisfiable (all dependencies
 * can be met).
 */
bool isSequenceTrulyUnsat(const std::vector<std::string> &sequence) {
  std::set<std::string> availableState;

  std::cout << "\n[DEPENDENCY-CHECK] Analyzing sequence for true UNSAT..."
            << std::endl;

  for (size_t i = 0; i < sequence.size(); i++) {
    const std::string &op = sequence[i];

    // Check if this operation has hard state requirements
    auto reqIt = operationRequiresState.find(op);
    if (reqIt != operationRequiresState.end()) {
      for (const auto &requiredState : reqIt->second) {
        if (availableState.find(requiredState) == availableState.end()) {
          // Required state is not available from any prior operation
          std::cout << "[DEPENDENCY-CHECK] ✗ Operation '" << op
                    << "' requires state '" << requiredState
                    << "' but no prior operation produces it." << std::endl;
          std::cout << "[DEPENDENCY-CHECK] Sequence is TRULY UNSAT."
                    << std::endl;
          return true; // Truly UNSAT
        }
      }
    }

    // Add state that this operation produces
    auto prodIt = operationProducesState.find(op);
    if (prodIt != operationProducesState.end()) {
      for (const auto &producedState : prodIt->second) {
        availableState.insert(producedState);
      }
    }
  }

  std::cout << "[DEPENDENCY-CHECK] ✓ All dependencies satisfied - sequence is "
               "potentially satisfiable."
            << std::endl;
  return false; // Not truly UNSAT - all dependencies can be met
}

// Generate realistic test values based on variable name hints
Expr *generateRealisticValue(const string &varName, int index) {
  string baseName = varName;
  size_t i = varName.length();
  while (i > 0 && isdigit(varName[i - 1])) {
    i--;
  }
  if (i < varName.length()) {
    baseName = varName.substr(0, i);
  }

  string suffix = to_string(index);

  if (baseName == "email") {
    return new String("testuser" + suffix + "@example.com");
  } else if (baseName == "password") {
    return new String("Password" + suffix + "!");
  } else if (baseName == "fullName") {
    return new String("Test User " + suffix);
  } else if (baseName == "mobile") {
    string paddedIndex = suffix;
    while (paddedIndex.length() < 4) {
      paddedIndex = "0" + paddedIndex;
    }
    return new String("555000" + paddedIndex.substr(paddedIndex.length() - 4));
  } else if (baseName == "token") {
    return new String("token_" + suffix);
  } else if (baseName == "name") {
    return new String("TestName" + suffix);
  } else if (baseName == "address") {
    return new String("123 Test Street " + suffix);
  } else if (baseName == "contact") {
    return new String("contact" + suffix + "@test.com");
  }

  // ========================================
  // LIBRARY WEBAPP MAPPINGS
  // ========================================

  // Book fields
  else if (baseName == "bookTitle") {
    return new String("Test Book " + suffix);
  } else if (baseName == "bookAuthor") {
    return new String("Author " + suffix);
  } else if (baseName == "bookDesc") {
    return new String("Description for test book " + suffix);
  } else if (baseName == "bookCode") {
    // This should typically be resolved from sigma (existing book)
    return new String("__BOOK_CODE_" + suffix + "__");
  }

  // Student fields
  else if (baseName == "studentName") {
    return new String("Student " + suffix);
  } else if (baseName == "studentEmail") {
    return new String("student" + suffix + "@test.com");
  } else if (baseName == "studentPhone") {
    string paddedIndex = suffix;
    while (paddedIndex.length() < 4) {
      paddedIndex = "0" + paddedIndex;
    }
    return new String("555100" + paddedIndex.substr(paddedIndex.length() - 4));
  } else if (baseName == "studentId") {
    // This should typically be resolved from sigma (existing student)
    return new String("__STUDENT_ID_" + suffix + "__");
  }

  // Request/Loan date fields
  else if (baseName == "startDate") {
    return new String("2025-01-01");
  } else if (baseName == "endDate") {
    return new String("2025-01-15");
  }

  // Request ID
  else if (baseName == "requestId") {
    // This should typically be resolved from sigma (existing request)
    return new String("__REQUEST_ID_" + suffix + "__");
  }

  // Loan ID
  else if (baseName == "loanId") {
    // This should typically be resolved from sigma (existing loan)
    return new String("__LOAN_ID_" + suffix + "__");
  }

  // ========================================
  // DEFAULT FALLBACK
  // ========================================
  else {
    return new String("value_" + suffix);
  }
}

void TestGen::generateTest() {}

bool isInputStmt(const Stmt &stmt) {
  if (stmt.statementType == StmtType::ASSIGN) {
    const Assign *assign = dynamic_cast<const Assign *>(&stmt);
    if (assign && assign->right->exprType == ExprType::FUNC_CALL_EXPR) {
      const FuncCall *fc = dynamic_cast<const FuncCall *>(assign->right.get());
      if (fc) {
        return (fc->name == "input" && fc->args.size() == 0);
      }
    }
  }
  return false;
}

bool isAbstract(const Program &prog) {
  for (const auto &stmt : prog.statements) {
    if (isInputStmt(*stmt)) {
      return true;
    }
  }
  return false;
}

// Helper to extract base name from versioned variable name
string extractBaseName(const string &varName) {
  string baseName = varName;
  size_t idx = varName.length();
  while (idx > 0 && isdigit(varName[idx - 1]))
    idx--;
  if (idx < varName.length())
    baseName = varName.substr(0, idx);
  return baseName;
}
// Check if a variable needs to be resolved from sigma (constraint-aware IDs)
bool needsSigmaLookup(const string &baseName) {

  // Library app IDs
  if (baseName == "bookCode" || baseName == "studentId" ||
      baseName == "requestId" || baseName == "loanId") {
    return true;
  }

  // Restaurant app IDs
  if (baseName == "restaurantId" || baseName == "menuItemId" ||
      baseName == "orderId") {
    return true;
  }
  // E-commerce app IDs
  if (baseName == "productId" || baseName == "cartId" ||
      baseName == "reviewId") {
    return true;
  }
  return false;
}

// Search sigma for the LATEST non-empty map with given prefix and return first
// key
string TestGen::findKeyFromMapInSigma(const string &prefix) {
  cout << "    [findKeyFromMapInSigma] Searching for prefix: " << prefix
       << endl;

  // Search from high to low to find the LATEST version
  for (int i = 20; i >= 0; i--) {
    string varName = prefix + to_string(i);

    if (see.getSigma().hasValue(varName)) {
      Expr *value = see.getSigma().getValue(varName);

      if (value && value->exprType == ExprType::MAP) {
        Map *map = dynamic_cast<Map *>(value);

        if (map && !map->value.empty()) {
          Var *firstKey = map->value[0].first.get();
          cout << "    [findKeyFromMapInSigma] Found " << varName << " with "
               << map->value.size()
               << " entries, returning key: " << firstKey->name << endl;
          return firstKey->name;
        } else {
          cout << "    [findKeyFromMapInSigma] " << varName << " is empty map"
               << endl;
        }
      }
    }
  }

  cout << "    [findKeyFromMapInSigma] No non-empty map found for " << prefix
       << endl;
  return "";
}

// Generate value for a variable based on its base name
Expr *TestGen::generateValueForBaseName(const string &baseName,
                                        const string &varName, int index,
                                        map<string, Expr *> &baseNameToValue,
                                        bool lookupFromSigma) {
  Expr *value = nullptr;

  // OWNER ROLE VARIABLES
  if (baseName == "ownerEmail") {
    value = new String("owner@example.com");
  } else if (baseName == "ownerPassword") {
    value = new String("OwnerPass1!");
  } else if (baseName == "ownerFullName") {
    value = new String("Test Owner");
  } else if (baseName == "ownerMobile") {
    value = new String("5550000001");
  }
  // CUSTOMER ROLE VARIABLES
  else if (baseName == "customerEmail") {
    value = new String("customer@example.com");
  } else if (baseName == "customerPassword") {
    value = new String("CustomerPass1!");
  } else if (baseName == "customerFullName") {
    value = new String("Test Customer");
  } else if (baseName == "customerMobile") {
    value = new String("5550000002");
  }
  // WRONG PASSWORD FOR ERROR TESTING
  else if (baseName == "wrongPassword") {
    value = new String("WrongPass123!");
  }
  // AGENT ROLE VARIABLES
  else if (baseName == "agentEmail") {
    value = new String("agent@example.com");
  } else if (baseName == "agentPassword") {
    value = new String("AgentPass1!");
  } else if (baseName == "agentFullName") {
    value = new String("Test Agent");
  } else if (baseName == "agentMobile") {
    value = new String("5550000003");
  }
  // ========================================
  // E-COMMERCE: BUYER ROLE VARIABLES
  // ========================================
  else if (baseName == "buyerEmail") {
    value = new String("buyer@example.com");
  } else if (baseName == "buyerPassword") {
    value = new String("BuyerPass123!");
  } else if (baseName == "buyerFullName") {
    value = new String("Test Buyer");
  }
  // E-COMMERCE: SELLER ROLE VARIABLES
  else if (baseName == "sellerEmail") {
    value = new String("seller@example.com");
  } else if (baseName == "sellerPassword") {
    value = new String("SellerPass123!");
  } else if (baseName == "sellerFullName") {
    value = new String("Test Seller");
  } else if (baseName == "storeName") {
    value = new String("Test Store");
  } else if (baseName == "storeDescription") {
    value = new String("A quality test store");
  }
  // E-COMMERCE: PRODUCT FIELDS
  else if (baseName == "title") {
    value = new String("Test Product");
  } else if (baseName == "description") {
    value = new String("A great test product description");
  } else if (baseName == "category") {
    value = new String("Electronics");
  } else if (baseName == "price") {
    value = new Num(99);
  }
  // E-COMMERCE: CONSTRAINT-AWARE IDs
  else if (baseName == "productId") {
    if (lookupFromSigma) {
      string realId = findKeyFromMapInSigma("tmp_P_");
      if (!realId.empty()) {
        value = new String(realId);
        cout << "    [RESOLVED from sigma] productId = \"" << realId << "\""
             << endl;
      } else {
        value = new String("__NEEDS_PRODUCT_ID__");
        cout << "    [DEFERRED] productId - no product in sigma yet" << endl;
      }
    } else {
      value = new String("__NEEDS_PRODUCT_ID__");
    }
  } else if (baseName == "cartId") {
    if (lookupFromSigma) {
      string realId = findKeyFromMapInSigma("tmp_C_");
      if (!realId.empty()) {
        value = new String(realId);
        cout << "    [RESOLVED from sigma] cartId = \"" << realId << "\""
             << endl;
      } else {
        value = new String("__NEEDS_CART_ID__");
        cout << "    [DEFERRED] cartId - no cart in sigma yet" << endl;
      }
    } else {
      value = new String("__NEEDS_CART_ID__");
    }
  } else if (baseName == "reviewId") {
    if (lookupFromSigma) {
      string realId = findKeyFromMapInSigma("tmp_Rev_");
      if (!realId.empty()) {
        value = new String(realId);
        cout << "    [RESOLVED from sigma] reviewId = \"" << realId << "\""
             << endl;
      } else {
        value = new String("__NEEDS_REVIEW_ID__");
        cout << "    [DEFERRED] reviewId - no review in sigma yet" << endl;
      }
    } else {
      value = new String("__NEEDS_REVIEW_ID__");
    }
  }
  // E-COMMERCE: SHIPPING ADDRESS
  else if (baseName == "shippingAddress") {
    value = new String("123 Test St,Test City,TS,12345,USA");
  }
  // E-COMMERCE: RATING
  else if (baseName == "rating") {
    value = new Num(5);
  } else if (baseName == "comment") {
    value = new String("Great product, highly recommend!");
  }
  // E-COMMERCE: ORDER STATUS (for updateOrderStatus)
  else if (baseName == "status") {
    // Use static counter to track order status progression
    static int ecommerceStatusCounter = 0;
    const char *statusSequence[] = {
        "Processing", // 1st call
        "Shipped",    // 2nd call
        "Delivered"   // 3rd call
    };
    int seqIndex = ecommerceStatusCounter % 3;
    value = new String(statusSequence[seqIndex]);
    cout << "    [ORDER STATUS] Call #" << (ecommerceStatusCounter + 1)
         << " -> status = \"" << statusSequence[seqIndex] << "\"" << endl;
    ecommerceStatusCounter++;
    return value; // Return early, don't cache
  }
  // CONSTRAINT-AWARE IDs - Look up from sigma if flag is set
  else if (baseName == "restaurantId") {
    if (lookupFromSigma) {
      string realId = findKeyFromMapInSigma("tmp_R_");
      if (!realId.empty()) {
        value = new String(realId);
        cout << "    [RESOLVED from sigma] restaurantId = \"" << realId << "\""
             << endl;
      } else {
        // Keep as placeholder - will be resolved in later iteration
        value = new String("__NEEDS_RESTAURANT_ID__");
        cout << "    [DEFERRED] restaurantId - no restaurant in sigma yet"
             << endl;
      }
    } else {
      value = new String("__NEEDS_RESTAURANT_ID__");
    }
  } else if (baseName == "menuItemId") {
    if (lookupFromSigma) {
      string realId = findKeyFromMapInSigma("tmp_M_");
      if (!realId.empty()) {
        value = new String(realId);
        cout << "    [RESOLVED from sigma] menuItemId = \"" << realId << "\""
             << endl;
      } else {
        value = new String("__NEEDS_MENUITEM_ID__");
        cout << "    [DEFERRED] menuItemId - no menu item in sigma yet" << endl;
      }
    } else {
      value = new String("__NEEDS_MENUITEM_ID__");
    }
  } else if (baseName == "orderId") {
    if (lookupFromSigma) {
      string realId = findKeyFromMapInSigma("tmp_O_");
      if (!realId.empty()) {
        value = new String(realId);
        cout << "    [RESOLVED from sigma] orderId = \"" << realId << "\""
             << endl;
      } else {
        value = new String("__NEEDS_ORDER_ID__");
        cout << "    [DEFERRED] orderId - no order in sigma yet" << endl;
      }
    } else {
      value = new String("__NEEDS_ORDER_ID__");
    }
  }
  // INTEGER TYPES
  else if (baseName == "quantity") {
    value = new Num(2);
  } else if (baseName == "itemPrice") {
    value = new Num(150);
  } else if (baseName == "restaurantRating") {
    value = new Num(5);
  } else if (baseName == "deliveryRating") {
    value = new Num(4);
  }
  // RESTAURANT DETAILS
  else if (baseName == "restaurantName") {
    value = new String("Test Restaurant");
  } else if (baseName == "restaurantAddress") {
    value = new String("123 Restaurant Street");
  } else if (baseName == "restaurantContact") {
    value = new String("restaurant@test.com");
  }
  // MENU ITEM DETAILS
  else if (baseName == "itemName") {
    value = new String("Delicious Dish");
  } else if (baseName == "itemDescription") {
    value = new String("A very tasty menu item");
  } else if (baseName == "itemCategory") {
    value = new String("Main Course");
  }
  // ORDER & DELIVERY DETAILS
  else if (baseName == "deliveryAddress") {
    value = new String("456 Customer Lane, Apt 7");
  } else if (baseName == "paymentMethod") {
    value = new String("card");
  }

  else if (baseName == "orderStatus") {
    // Use static counter to track order status progression
    // This counter resets implicitly when the program restarts
    static int orderStatusCounter = 0;

    // Status sequence for complete order flow:
    // Owner calls: accepted -> preparing -> ready
    // Agent calls: picked_up -> delivered
    const char *statusSequence[] = {
        "accepted",  // 1st call (owner accepts)
        "preparing", // 2nd call (owner starts preparing)
        "ready",     // 3rd call (owner marks ready)
        "picked_up", // 4th call (agent picks up)
        "delivered"  // 5th call (agent delivers)
    };

    int seqIndex = orderStatusCounter % 5;
    value = new String(statusSequence[seqIndex]);

    cout << "    [ORDER STATUS] Call #" << (orderStatusCounter + 1)
         << " (index=" << index << ") -> status = \""
         << statusSequence[seqIndex] << "\"" << endl;

    orderStatusCounter++;

    // DON'T cache this value - each call needs a different status
    // Return early to skip the caching at the end of the function
    return value;
  }
  // REVIEW DETAILS
  else if (baseName == "reviewComment") {
    value = new String("Great food and fast delivery!");
  }

  // ========================================
  // LIBRARY: Book fields (MOVED BEFORE GENERIC FALLBACK)
  // ========================================
  else if (baseName == "bookTitle") {
    static int bookCounter = 0;
    value = new String("Test Book " + to_string(++bookCounter));
  } else if (baseName == "bookAuthor") {
    value = new String("Test Author");
  } else if (baseName == "bookDesc") {
    value = new String("A comprehensive test book description");
  } else if (baseName == "bookCode") {
    if (lookupFromSigma) {
      string realId = findKeyFromMapInSigma("tmp_B_");
      if (!realId.empty()) {
        value = new String(realId);
        cout << "    [RESOLVED from sigma] bookCode = \"" << realId << "\""
             << endl;
      } else {
        value = new String("__NEEDS_BOOK_CODE__");
        cout << "    [DEFERRED] bookCode - no book in sigma yet" << endl;
      }
    } else {
      value = new String("__NEEDS_BOOK_CODE__");
    }
  }

  // ========================================
  // LIBRARY: Student fields
  // ========================================
  else if (baseName == "studentName") {
    static int studentNameCounter = 0;
    value = new String("Test Student " + to_string(++studentNameCounter));
  } else if (baseName == "studentEmail") {
    static int studentEmailCounter = 0;
    value = new String("student" + to_string(++studentEmailCounter) +
                       "@library.edu");
  } else if (baseName == "studentPhone") {
    static int phoneCounter = 0;
    value = new String("555-000-" + to_string(1000 + phoneCounter++));
  } else if (baseName == "studentId") {
    if (lookupFromSigma) {
      string realId = findKeyFromMapInSigma("tmp_S_");
      if (!realId.empty()) {
        value = new String(realId);
        cout << "    [RESOLVED from sigma] studentId = \"" << realId << "\""
             << endl;
      } else {
        value = new String("__NEEDS_STUDENT_ID__");
        cout << "    [DEFERRED] studentId - no student in sigma yet" << endl;
      }
    } else {
      value = new String("__NEEDS_STUDENT_ID__");
    }
  }

  // ========================================
  // LIBRARY: Request/Loan fields
  // ========================================
  else if (baseName == "requestId") {
    if (lookupFromSigma) {
      string realId = findKeyFromMapInSigma("tmp_Req_");
      if (!realId.empty()) {
        value = new String(realId);
        cout << "    [RESOLVED from sigma] requestId = \"" << realId << "\""
             << endl;
      } else {
        value = new String("__NEEDS_REQUEST_ID__");
        cout << "    [DEFERRED] requestId - no request in sigma yet" << endl;
      }
    } else {
      value = new String("__NEEDS_REQUEST_ID__");
    }
  } else if (baseName == "loanId") {
    if (lookupFromSigma) {
      string realId = findKeyFromMapInSigma("tmp_Loans_");
      if (!realId.empty()) {
        value = new String(realId);
        cout << "    [RESOLVED from sigma] loanId = \"" << realId << "\""
             << endl;
      } else {
        value = new String("__NEEDS_LOAN_ID__");
        cout << "    [DEFERRED] loanId - no loan in sigma yet" << endl;
      }
    } else {
      value = new String("__NEEDS_LOAN_ID__");
    }
  } else if (baseName == "startDate") {
    // Generate future date for loan start
    static int dateOffset = 0;
    value = new String("2025-02-" + to_string(10 + (dateOffset++ % 15)) +
                       "T00:00:00.000Z");
  } else if (baseName == "endDate") {
    // Generate date 14 days after start date
    static int endDateOffset = 0;
    value = new String("2025-02-" + to_string(24 + (endDateOffset++ % 5)) +
                       "T00:00:00.000Z");
  }

  // ========================================
  // GENERIC FALLBACK (MUST BE LAST!)
  // ========================================
  else {
    value = generateRealisticValue(varName, index);
  }

  // Cache the value if generated
  if (value != nullptr) {
    baseNameToValue[baseName] = value;
  }

  return value;
}

// Check if program has any unresolved placeholder values
bool hasUnresolvedPlaceholders(const Program &prog) {
  for (const auto &stmt : prog.statements) {
    if (stmt->statementType == StmtType::ASSIGN) {
      const Assign *assign = dynamic_cast<const Assign *>(stmt.get());
      if (assign && assign->right->exprType == ExprType::STRING) {
        const String *str = dynamic_cast<const String *>(assign->right.get());
        if (str && (str->value.find("__NEEDS_") == 0)) {
          return true;
        }
      }
    }
  }
  return false;
}

// Resolve placeholder values in concrete values list using sigma
// IMPORTANT: Only resolve if value is available, otherwise KEEP the placeholder
void resolvePlaceholdersInPlace(vector<Expr *> &concreteVals,
                                const vector<string> &varNames,
                                TestGen *tester) {
  for (size_t i = 0; i < concreteVals.size(); i++) {
    if (concreteVals[i]->exprType != ExprType::STRING)
      continue;

    String *strVal = dynamic_cast<String *>(concreteVals[i]);
    if (!strVal)
      continue;

    if (strVal->value == "__NEEDS_RESTAURANT_ID__") {
      string realId = tester->findKeyFromMapInSigma("tmp_R_");
      if (!realId.empty()) {
        delete concreteVals[i];
        concreteVals[i] = new String(realId);
        cout << "    [RESOLVED] " << varNames[i] << " = \"" << realId << "\""
             << endl;
      } else {
        // KEEP THE PLACEHOLDER - don't convert to fallback!
        cout << "    [STILL PENDING] " << varNames[i]
             << " - keeping placeholder for next iteration" << endl;
      }
    } else if (strVal->value == "__NEEDS_MENUITEM_ID__") {
      string realId = tester->findKeyFromMapInSigma("tmp_M_");
      if (!realId.empty()) {
        delete concreteVals[i];
        concreteVals[i] = new String(realId);
        cout << "    [RESOLVED] " << varNames[i] << " = \"" << realId << "\""
             << endl;
      } else {
        // KEEP THE PLACEHOLDER
        cout << "    [STILL PENDING] " << varNames[i]
             << " - keeping placeholder for next iteration" << endl;
      }
    } else if (strVal->value == "__NEEDS_ORDER_ID__") {
      string realId = tester->findKeyFromMapInSigma("tmp_O_");
      if (!realId.empty()) {
        delete concreteVals[i];
        concreteVals[i] = new String(realId);
        cout << "    [RESOLVED] " << varNames[i] << " = \"" << realId << "\""
             << endl;
      } else {
        // KEEP THE PLACEHOLDER
        cout << "    [STILL PENDING] " << varNames[i]
             << " - keeping placeholder for next iteration" << endl;
      }
    } else if (strVal->value == "__NEEDS_PRODUCT_ID__") {
      string realId = tester->findKeyFromMapInSigma("tmp_P_");
      if (!realId.empty()) {
        delete concreteVals[i];
        concreteVals[i] = new String(realId);
        cout << "    [RESOLVED] " << varNames[i] << " = \"" << realId << "\""
             << endl;
      } else {
        cout << "    [STILL PENDING] " << varNames[i]
             << " - keeping placeholder for next iteration" << endl;
      }
    } else if (strVal->value == "__NEEDS_CART_ID__") {
      string realId = tester->findKeyFromMapInSigma("tmp_C_");
      if (!realId.empty()) {
        delete concreteVals[i];
        concreteVals[i] = new String(realId);
        cout << "    [RESOLVED] " << varNames[i] << " = \"" << realId << "\""
             << endl;
      } else {
        cout << "    [STILL PENDING] " << varNames[i]
             << " - keeping placeholder for next iteration" << endl;
      }
    } else if (strVal->value == "__NEEDS_REVIEW_ID__") {
      string realId = tester->findKeyFromMapInSigma("tmp_Rev_");
      if (!realId.empty()) {
        delete concreteVals[i];
        concreteVals[i] = new String(realId);
        cout << "    [RESOLVED] " << varNames[i] << " = \"" << realId << "\""
             << endl;
      } else {
        cout << "    [STILL PENDING] " << varNames[i]
             << " - keeping placeholder for next iteration" << endl;
      }
    }
    // LIBRARY CHECK
    else if (strVal->value == "__NEEDS_BOOK_CODE__") {
      string realId = tester->findKeyFromMapInSigma("tmp_B_");
      if (!realId.empty()) {
        delete concreteVals[i];
        concreteVals[i] = new String(realId);
        cout << "    [RESOLVED] " << varNames[i] << " = \"" << realId << "\""
             << endl;
      } else {
        cout << "    [STILL PENDING] " << varNames[i]
             << " - keeping placeholder for next iteration" << endl;
      }
    } else if (strVal->value == "__NEEDS_STUDENT_ID__") {
      string realId = tester->findKeyFromMapInSigma("tmp_S_");
      if (!realId.empty()) {
        delete concreteVals[i];
        concreteVals[i] = new String(realId);
        cout << "    [RESOLVED] " << varNames[i] << " = \"" << realId << "\""
             << endl;
      } else {
        cout << "    [STILL PENDING] " << varNames[i]
             << " - keeping placeholder for next iteration" << endl;
      }
    } else if (strVal->value == "__NEEDS_REQUEST_ID__") {
      string realId = tester->findKeyFromMapInSigma("tmp_Req_");
      if (!realId.empty()) {
        delete concreteVals[i];
        concreteVals[i] = new String(realId);
        cout << "    [RESOLVED] " << varNames[i] << " = \"" << realId << "\""
             << endl;
      } else {
        cout << "    [STILL PENDING] " << varNames[i]
             << " - keeping placeholder for next iteration" << endl;
      }
    } else if (strVal->value == "__NEEDS_LOAN_ID__") {
      string realId = tester->findKeyFromMapInSigma("tmp_Loans_");
      if (!realId.empty()) {
        delete concreteVals[i];
        concreteVals[i] = new String(realId);
        cout << "    [RESOLVED] " << varNames[i] << " = \"" << realId << "\""
             << endl;
      } else {
        cout << "    [STILL PENDING] " << varNames[i]
             << " - keeping placeholder for next iteration" << endl;
      }
    }
  }
}

// Resolve placeholders in the program AST itself (for statements that already
// have placeholder values)
void resolvePlaceholdersInProgram(Program &prog, TestGen *tester) {
  cout << "\n>>> Resolving placeholders in program AST" << endl;

  // We need to modify the statements vector, but it's const
  // So we'll cast away const (this is safe since we own the Program)
  auto &stmts = const_cast<vector<unique_ptr<Stmt>> &>(prog.statements);

  for (size_t i = 0; i < stmts.size(); i++) {
    if (stmts[i]->statementType == StmtType::ASSIGN) {
      Assign *assign = dynamic_cast<Assign *>(stmts[i].get());
      if (assign && assign->right->exprType == ExprType::STRING) {
        const String *str = dynamic_cast<const String *>(assign->right.get());
        if (str) {
          string newValue = "";

          if (str->value == "__NEEDS_RESTAURANT_ID__") {
            newValue = tester->findKeyFromMapInSigma("tmp_R_");
            if (newValue.empty()) {
              newValue = "no_restaurant_available"; // Final fallback
            }
          } else if (str->value == "__NEEDS_MENUITEM_ID__") {
            newValue = tester->findKeyFromMapInSigma("tmp_M_");
            if (newValue.empty()) {
              newValue = "no_menuitem_available";
            }
          } else if (str->value == "__NEEDS_ORDER_ID__") {
            newValue = tester->findKeyFromMapInSigma("tmp_O_");
            if (newValue.empty()) {
              newValue = "no_order_available";
            }
          }
          // ecommerce IDs
          else if (str->value == "__NEEDS_PRODUCT_ID__") {
            newValue = tester->findKeyFromMapInSigma("tmp_P_");
            if (newValue.empty()) {
              newValue = "no_product_available";
            }
          } else if (str->value == "__NEEDS_CART_ID__") {
            newValue = tester->findKeyFromMapInSigma("tmp_C_");
            if (newValue.empty()) {
              newValue = "no_cart_available";
            }
          } else if (str->value == "__NEEDS_REVIEW_ID__") {
            newValue = tester->findKeyFromMapInSigma("tmp_Rev_");
            if (newValue.empty()) {
              newValue = "no_review_available";
            }
          }
          // LIBRARY IDs
          else if (str->value == "__NEEDS_BOOK_CODE__") {
            newValue = tester->findKeyFromMapInSigma("tmp_B_");
            if (newValue.empty()) {
              newValue = "no_book_available";
            }
          } else if (str->value == "__NEEDS_STUDENT_ID__") {
            newValue = tester->findKeyFromMapInSigma("tmp_S_");
            if (newValue.empty()) {
              newValue = "no_student_available";
            }
          } else if (str->value == "__NEEDS_REQUEST_ID__") {
            newValue = tester->findKeyFromMapInSigma("tmp_Req_");
            if (newValue.empty()) {
              newValue = "no_request_available";
            }
          } else if (str->value == "__NEEDS_LOAN_ID__") {
            newValue = tester->findKeyFromMapInSigma("tmp_Loans_");
            if (newValue.empty()) {
              newValue = "no_loan_available";
            }
          }

          if (!newValue.empty()) {
            const Var *leftVar = dynamic_cast<const Var *>(assign->left.get());
            string varName = leftVar ? leftVar->name : "unknown";
            cout << "    [AST RESOLVED] " << varName << " = \"" << newValue
                 << "\"" << endl;

            // Create a new statement with the resolved value
            auto newLeft = make_unique<Var>(varName);
            auto newRight = make_unique<String>(newValue);
            stmts[i] =
                make_unique<Assign>(std::move(newLeft), std::move(newRight));
          }
        }
      }
    }
  }
}
bool isPathConstraintUnsat(SEE &see, const std::vector<std::string> &sequence) {
  vector<Expr *> &pc = see.getPathConstraint();

  bool hasConcretelyFalse = false;

  for (Expr *constraint : pc) {
    // Check for Bool(false)
    if (constraint->exprType == ExprType::BOOL) {
      Bool *bc = dynamic_cast<Bool *>(constraint);
      if (bc && !bc->value) {
        hasConcretelyFalse = true;
        break;
      }
    }
    // Check for Num(0) which also represents false
    else if (constraint->exprType == ExprType::NUM) {
      Num *num = dynamic_cast<Num *>(constraint);
      if (num && num->value == 0) {
        hasConcretelyFalse = true;
        break;
      }
    }
  }

  if (!hasConcretelyFalse) {
    return false; // No false in path constraint - not UNSAT
  }

  // Path constraint has false - but is it TRULY unsat?
  // Check if the sequence has proper dependencies
  std::cout << "\n[UNSAT-CHECK] Path constraint contains FALSE - checking "
               "dependencies..."
            << std::endl;
  return isSequenceTrulyUnsat(sequence);
}
unique_ptr<Program> TestGen::generateCTC(unique_ptr<Program> atc,
                                         vector<Expr *> ConcreteVals,
                                         ValueEnvironment *ve) {
  cout << "\n========================================" << endl;
  cout << ">>> generateCTC: Starting iteration" << endl;
  cout << "========================================" << endl;

  // Check if program is concrete AND has no unresolved placeholders
  if (!isAbstract(*atc) && !hasUnresolvedPlaceholders(*atc)) {
    cout << ">>> generateCTC: Program is fully concrete, returning" << endl;
    return atc;
  }

  cout << ">>> generateCTC: Program needs processing" << endl;
  cout << ">>> generateCTC: Concrete values provided: " << ConcreteVals.size()
       << endl;
  cout << ">>> generateCTC: Is abstract: " << isAbstract(*atc) << endl;
  cout << ">>> generateCTC: Has placeholders: "
       << hasUnresolvedPlaceholders(*atc) << endl;

  // STEP 1: Rewrite ATC with provided concrete values
  cout << "\n>>> generateCTC: STEP 1 - Rewriting ATC with concrete values"
       << endl;
  unique_ptr<Program> rewritten = rewriteATC(atc, ConcreteVals);

  // STEP 2: Run symbolic execution to populate sigma
  cout << "\n>>> generateCTC: STEP 2 - Running symbolic execution" << endl;
  SymbolTable st(nullptr);
  see.execute(*rewritten, st);

  // NEW: STEP 2a - Check if path constraint is satisfiable
  if (isPathConstraintUnsat(see, currentApiSequence)) {
    cout << "\n>>> generateCTC: UNSAT DETECTED!" << endl;
    cout << ">>> Precondition cannot be satisfied with current database state."
         << endl;
    cout << ">>> This test sequence requires prerequisite operations." << endl;
    return nullptr; // Return nullptr to signal UNSAT
  }
  // STEP 3: Check if program still has input() statements
  bool stillAbstract = isAbstract(*rewritten);
  bool stillHasPlaceholders = hasUnresolvedPlaceholders(*rewritten);

  cout << ">>> generateCTC: After symex - Is abstract: " << stillAbstract
       << ", Has placeholders: " << stillHasPlaceholders << endl;

  // STEP 4: If program has placeholders but no input(), try to resolve them now
  if (!stillAbstract && stillHasPlaceholders) {
    cout << "\n>>> generateCTC: STEP 3a - Resolving placeholders in AST"
         << endl;
    resolvePlaceholdersInProgram(*rewritten, this);

    // Check again
    if (!hasUnresolvedPlaceholders(*rewritten)) {
      cout << ">>> generateCTC: All placeholders resolved, program is fully "
              "concrete"
           << endl;
      return rewritten;
    } else {
      cout << ">>> generateCTC: Some placeholders still unresolved (will use "
              "fallback)"
           << endl;
      // Force resolve with fallbacks
      resolvePlaceholdersInProgram(*rewritten, this);
      return rewritten;
    }
  }

  // STEP 5: If still abstract, generate values for remaining inputs
  if (!stillAbstract) {
    cout << ">>> generateCTC: Program is now fully concrete" << endl;
    return rewritten;
  }

  cout << "\n>>> generateCTC: STEP 3 - Generating values with sigma lookup"
       << endl;

  vector<Expr *> newConcreteVals;
  vector<string> newVarNames;
  map<string, Expr *> baseNameToValue;

  // Collect existing concrete values (skip placeholders)
  for (const auto &stmt : rewritten->statements) {
    if (stmt->statementType == StmtType::ASSIGN) {
      const Assign *assign = dynamic_cast<const Assign *>(stmt.get());
      if (!assign)
        continue;

      const Var *leftVar = dynamic_cast<const Var *>(assign->left.get());
      if (!leftVar)
        continue;

      // Skip input statements
      if (assign->right->exprType == ExprType::FUNC_CALL_EXPR) {
        const FuncCall *fc =
            dynamic_cast<const FuncCall *>(assign->right.get());
        if (fc && fc->name == "input")
          continue;
      }

      // Skip placeholder values - don't reuse them!
      if (assign->right->exprType == ExprType::STRING) {
        const String *str = dynamic_cast<const String *>(assign->right.get());
        if (str && str->value.find("__NEEDS_") == 0)
          continue;
      }

      string baseName = extractBaseName(leftVar->name);

      if (baseNameToValue.find(baseName) == baseNameToValue.end()) {
        baseNameToValue[baseName] = assign->right.get();
        cout << "    [Found existing] " << baseName << " -> " << leftVar->name
             << endl;
      }
    }
  }

  // Generate values for remaining input statements
  int inputIndex = 0;
  for (const auto &stmt : rewritten->statements) {
    if (!isInputStmt(*stmt))
      continue;

    const Assign *assign = dynamic_cast<const Assign *>(stmt.get());
    const Var *leftVar = dynamic_cast<const Var *>(assign->left.get());
    string varName = leftVar ? leftVar->name : "unknown";
    string baseName = extractBaseName(varName);

    Expr *value = nullptr;

    // For IDs that need sigma lookup, ALWAYS try sigma first
    if (needsSigmaLookup(baseName)) {
      value = generateValueForBaseName(baseName, varName, inputIndex,
                                       baseNameToValue, true);
      cout << "    " << varName << " = ";
    } else if (baseNameToValue.find(baseName) != baseNameToValue.end()) {
      Expr *existing = baseNameToValue[baseName];
      if (existing->exprType == ExprType::STRING) {
        value = new String(dynamic_cast<String *>(existing)->value);
      } else if (existing->exprType == ExprType::NUM) {
        value = new Num(dynamic_cast<Num *>(existing)->value);
      } else {
        value = generateValueForBaseName(baseName, varName, inputIndex,
                                         baseNameToValue, true);
      }
      cout << "    " << varName << " = (reusing " << baseName << ") ";
    } else {
      // Generate with sigma lookup ENABLED
      value = generateValueForBaseName(baseName, varName, inputIndex,
                                       baseNameToValue, true);
      cout << "    " << varName << " = ";
    }

    newConcreteVals.push_back(value);
    newVarNames.push_back(varName);

    if (value->exprType == ExprType::STRING) {
      cout << "\"" << dynamic_cast<String *>(value)->value << "\"" << endl;
    } else if (value->exprType == ExprType::NUM) {
      cout << dynamic_cast<Num *>(value)->value << endl;
    }

    inputIndex++;
  }

  // STEP 6: Try to resolve any placeholders in the concrete values
  cout << "\n>>> generateCTC: STEP 4 - Resolving placeholders from sigma"
       << endl;
  resolvePlaceholdersInPlace(newConcreteVals, newVarNames, this);

  if (newConcreteVals.empty()) {
    cout << ">>> generateCTC: No new concrete values needed" << endl;
    return rewritten;
  }

  // STEP 7: Recurse
  cout << "\n>>> generateCTC: STEP 5 - Recursing with "
       << newConcreteVals.size() << " concrete values" << endl;
  return generateCTC(std::move(rewritten), newConcreteVals, ve);
}

unique_ptr<Program> TestGen::generateATC(unique_ptr<Spec> spec,
                                         vector<string> ts) {
  // Store the API sequence for later use in UNSAT detection
  currentApiSequence = ts;

  Program raw = genATCFromString(*spec, ts);

  auto &mutable_stmts = const_cast<vector<unique_ptr<Stmt>> &>(raw.statements);
  auto logicalATC = unique_ptr<Program>(new Program(std::move(mutable_stmts)));

  Printer printer;

  RewriteGlobalsVisitor rewriter;
  rewriter.visitProgram(*logicalATC);

  unique_ptr<Program> testApiATC = std::move(rewriter.rewrittenProgram);

  cout << "\n=== TEST-API ATC (After Rewrite) ===" << endl;
  printer.visitProgram(*testApiATC);

  return testApiATC;
}

unique_ptr<Program> TestGen::rewriteATC(unique_ptr<Program> &atc,
                                        vector<Expr *> ConcreteVals) {
  if (atc->statements.size() == 0 && ConcreteVals.size() != 0) {
    throw runtime_error("Empty test case but concrete values provided");
  }

  vector<unique_ptr<Stmt>> newStmts;
  int concreteValIndex = 0;

  for (int i = 0; i < atc->statements.size(); i++) {
    const auto &stmt = atc->statements[i];

    if (stmt->statementType == StmtType::ASSIGN) {
      Assign *assign = dynamic_cast<Assign *>(stmt.get());

      if (assign && assign->right->exprType == ExprType::FUNC_CALL_EXPR) {
        FuncCall *fc = dynamic_cast<FuncCall *>(assign->right.get());

        if (fc && fc->name == "input" && fc->args.size() == 0) {
          if (concreteValIndex < ConcreteVals.size()) {
            Var *leftVarPtr = dynamic_cast<Var *>(assign->left.get());
            if (!leftVarPtr) {
              throw runtime_error(
                  "Expected Var on left side of input assignment");
            }
            unique_ptr<Var> leftVar = make_unique<Var>(leftVarPtr->name);
            unique_ptr<Expr> rightExpr =
                ConcreteVals[concreteValIndex]->clone();

            newStmts.push_back(
                make_unique<Assign>(move(leftVar), move(rightExpr)));
            concreteValIndex++;
          } else {
            newStmts.push_back(stmt.get()->clone());
          }
          continue;
        }
      }
    }

    newStmts.push_back(stmt.get()->clone());
  }

  return make_unique<Program>(move(newStmts));
}
