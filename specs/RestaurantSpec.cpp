#include "RestaurantSpec.hpp"
#include "../language/ast.hh"

#include <memory>
#include <vector>

using namespace std;

std::unique_ptr<Spec> makeRestaurantSpec() {

  /* =====================================================
   * 1. GLOBAL DECLARATIONS
   * ===================================================== */

  vector<unique_ptr<Decl>> globals;

  auto mkString = []() { return make_unique<TypeConst>("string"); };

  auto mkRole = []() { return make_unique<TypeConst>("Role"); };

  // User authentication & authorization
  globals.push_back(
      make_unique<Decl>("U", make_unique<MapType>(mkString(), mkString())));
  globals.push_back(
      make_unique<Decl>("T", make_unique<MapType>(mkString(), mkString())));
  globals.push_back(
      make_unique<Decl>("Roles", make_unique<MapType>(mkString(), mkRole())));

  // Customer data
  globals.push_back(
      make_unique<Decl>("C", make_unique<MapType>(mkString(), mkString())));

  // Restaurant data
  globals.push_back(
      make_unique<Decl>("R", make_unique<MapType>(mkString(), mkString())));
  globals.push_back(
      make_unique<Decl>("M", make_unique<MapType>(mkString(), mkString())));
  globals.push_back(make_unique<Decl>(
      "Owners", make_unique<MapType>(mkString(), mkString())));

  // Order data
  globals.push_back(
      make_unique<Decl>("O", make_unique<MapType>(mkString(), mkString())));
  globals.push_back(make_unique<Decl>(
      "Assignments", make_unique<MapType>(mkString(), mkString())));

  // Review data
  globals.push_back(
      make_unique<Decl>("Rev", make_unique<MapType>(mkString(), mkString())));

  /* =====================================================
   * 2. INITIALIZATIONS
   * ===================================================== */

  vector<unique_ptr<Init>> init;
  for (const auto &g : globals) {
    init.push_back(make_unique<Init>(
        g->name,
        make_unique<Map>(vector<pair<unique_ptr<Var>, unique_ptr<Expr>>>{})));
  }

  /* =====================================================
   * 3. FUNCTION DECLARATIONS (none)
   * ===================================================== */

  vector<unique_ptr<APIFuncDecl>> functions;

  /* =====================================================
   * 4. API BLOCKS
   * ===================================================== */

  vector<unique_ptr<API>> blocks;

  /* ---------- loginWrongPasswordErr (correct email, wrong password) ----------
   */
  {
    // PRE: customerEmail in dom(U) AND U[customerEmail] != wrongPassword
    vector<unique_ptr<Expr>> preArgs;

    // customerEmail in dom(U) - user exists
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("customerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("U"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs)));

    // U[customerEmail] != wrongPassword (password mismatch)
    vector<unique_ptr<Expr>> neqArgs;
    vector<unique_ptr<Expr>> indexArgs;
    indexArgs.push_back(make_unique<Var>("U"));
    indexArgs.push_back(make_unique<Var>("customerEmail"));
    neqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs)));
    neqArgs.push_back(make_unique<Var>("wrongPassword"));
    preArgs.push_back(make_unique<FuncCall>("!=", std::move(neqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: login(customerEmail, wrongPassword)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("customerEmail"));
    callArgs.push_back(make_unique<Var>("wrongPassword"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("login", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true (no state change, returns 401)
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("loginWrongPasswordErr", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- loginCustomerErr (login without registration) ---------- */
  {
    // PRE: customerEmail NOT in dom(U) - user doesn't exist
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("customerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("U"));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("not_in", std::move(notInArgs));

    // CALL: login(customerEmail, customerPassword)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("customerEmail"));
    callArgs.push_back(make_unique<Var>("customerPassword"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("login", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true (no state change, returns 401)
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("loginCustomerErr", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ========================================================================
   * CUSTOMER APIs
   * ======================================================================== */

  /* ---------- registerCustomerOk ---------- */
  {
    // PRE: customerEmail not_in dom(U)
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("customerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("U"));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("not_in", std::move(notInArgs));

    // CALL: registerCustomer(customerEmail, customerPassword, customerFullName,
    // customerMobile)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("customerEmail"));
    callArgs.push_back(make_unique<Var>("customerPassword"));
    callArgs.push_back(make_unique<Var>("customerFullName"));
    callArgs.push_back(make_unique<Var>("customerMobile"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("registerCustomer", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: U'[customerEmail] = customerPassword AND Roles'[customerEmail] =
    // CUSTOMER
    vector<unique_ptr<Expr>> postArgs;

    vector<unique_ptr<Expr>> eq1Args;
    vector<unique_ptr<Expr>> uPrimeArgs;
    uPrimeArgs.push_back(make_unique<Var>("U"));
    vector<unique_ptr<Expr>> indexArgs1;
    indexArgs1.push_back(
        make_unique<FuncCall>("primed", std::move(uPrimeArgs)));
    indexArgs1.push_back(make_unique<Var>("customerEmail"));
    eq1Args.push_back(make_unique<FuncCall>("[]", std::move(indexArgs1)));
    eq1Args.push_back(make_unique<Var>("customerPassword"));
    postArgs.push_back(make_unique<FuncCall>("=", std::move(eq1Args)));

    vector<unique_ptr<Expr>> eq2Args;
    vector<unique_ptr<Expr>> rolesPrimeArgs;
    rolesPrimeArgs.push_back(make_unique<Var>("Roles"));
    vector<unique_ptr<Expr>> indexArgs2;
    indexArgs2.push_back(
        make_unique<FuncCall>("primed", std::move(rolesPrimeArgs)));
    indexArgs2.push_back(make_unique<Var>("customerEmail"));
    eq2Args.push_back(make_unique<FuncCall>("[]", std::move(indexArgs2)));
    eq2Args.push_back(make_unique<Var>("CUSTOMER"));
    postArgs.push_back(make_unique<FuncCall>("=", std::move(eq2Args)));

    auto post = make_unique<FuncCall>("AND", std::move(postArgs));

    blocks.push_back(make_unique<API>("registerCustomerOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- loginCustomerOk ---------- */
  {
    // PRE: customerEmail in dom(U) AND U[customerEmail] = customerPassword
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("customerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("U"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs)));

    vector<unique_ptr<Expr>> eqArgs;
    vector<unique_ptr<Expr>> indexArgs;
    indexArgs.push_back(make_unique<Var>("U"));
    indexArgs.push_back(make_unique<Var>("customerEmail"));
    eqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs)));
    eqArgs.push_back(make_unique<Var>("customerPassword"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(eqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: login(customerEmail, customerPassword)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("customerEmail"));
    callArgs.push_back(make_unique<Var>("customerPassword"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("login", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: T'[customerEmail] = _result
    vector<unique_ptr<Expr>> postEqArgs;
    vector<unique_ptr<Expr>> tPrimeArgs;
    tPrimeArgs.push_back(make_unique<Var>("T"));
    vector<unique_ptr<Expr>> indexArgs2;
    indexArgs2.push_back(
        make_unique<FuncCall>("primed", std::move(tPrimeArgs)));
    indexArgs2.push_back(make_unique<Var>("customerEmail"));
    postEqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs2)));
    postEqArgs.push_back(make_unique<Var>("_result"));
    auto post = make_unique<FuncCall>("=", std::move(postEqArgs));

    blocks.push_back(make_unique<API>("loginCustomerOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- browseRestaurantsOk ---------- */
  {
    // PRE: customerEmail in dom(T)
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("customerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("T"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("in", std::move(inArgs));

    // CALL: browseRestaurants(customerEmail)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("customerEmail"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("browseRestaurants", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("browseRestaurantsOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- viewMenuOk ---------- */
  {
    // PRE: customerEmail in dom(T) AND restaurantId in dom(R)
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("customerEmail"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("T"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("restaurantId"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("R"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: viewMenu(customerEmail, restaurantId)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("customerEmail"));
    callArgs.push_back(make_unique<Var>("restaurantId"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("viewMenu", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("viewMenuOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- addToCartOk ---------- */
  {
    // PRE: customerEmail in dom(T) AND menuItemId in dom(M)
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("customerEmail"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("T"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("menuItemId"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("M"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: addToCart(customerEmail, menuItemId, quantity)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("customerEmail"));
    callArgs.push_back(make_unique<Var>("menuItemId"));
    callArgs.push_back(make_unique<Var>("quantity"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("addToCart", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: customerEmail in dom(C')
    vector<unique_ptr<Expr>> inArgs3;
    inArgs3.push_back(make_unique<Var>("customerEmail"));
    vector<unique_ptr<Expr>> cPrimeArgs;
    cPrimeArgs.push_back(make_unique<Var>("C"));
    vector<unique_ptr<Expr>> domArgs3;
    domArgs3.push_back(make_unique<FuncCall>("primed", std::move(cPrimeArgs)));
    inArgs3.push_back(make_unique<FuncCall>("dom", std::move(domArgs3)));
    auto post = make_unique<FuncCall>("in", std::move(inArgs3));

    blocks.push_back(make_unique<API>("addToCartRestaurantOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- placeOrderOk ---------- */
  {
    // PRE: customerEmail in dom(T) AND customerEmail in dom(C)
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("customerEmail"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("T"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("customerEmail"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("C"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: placeOrder(customerEmail, deliveryAddress, paymentMethod)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("customerEmail"));
    callArgs.push_back(make_unique<Var>("deliveryAddress"));
    callArgs.push_back(make_unique<Var>("paymentMethod"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("placeOrder", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: _result in dom(O') AND customerEmail not_in dom(C')
    vector<unique_ptr<Expr>> postArgs;

    // _result in dom(O')
    vector<unique_ptr<Expr>> inArgs3;
    inArgs3.push_back(make_unique<Var>("_result"));
    vector<unique_ptr<Expr>> oPrimeArgs;
    oPrimeArgs.push_back(make_unique<Var>("O"));
    vector<unique_ptr<Expr>> domArgs3;
    domArgs3.push_back(make_unique<FuncCall>("primed", std::move(oPrimeArgs)));
    inArgs3.push_back(make_unique<FuncCall>("dom", std::move(domArgs3)));
    postArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs3)));

    // customerEmail not_in dom(C') - cart cleared
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("customerEmail"));
    vector<unique_ptr<Expr>> cPrimeArgs;
    cPrimeArgs.push_back(make_unique<Var>("C"));
    vector<unique_ptr<Expr>> domArgs4;
    domArgs4.push_back(make_unique<FuncCall>("primed", std::move(cPrimeArgs)));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs4)));
    postArgs.push_back(make_unique<FuncCall>("not_in", std::move(notInArgs)));

    auto post = make_unique<FuncCall>("AND", std::move(postArgs));

    blocks.push_back(make_unique<API>("placeOrderOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- leaveReviewOk ---------- */
  {
    // PRE: customerEmail in dom(T) AND orderId in dom(O)
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("customerEmail"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("T"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("orderId"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("O"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: leaveReview(customerEmail, orderId, restaurantRating,
    // deliveryRating, reviewComment)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("customerEmail"));
    callArgs.push_back(make_unique<Var>("orderId"));
    callArgs.push_back(make_unique<Var>("restaurantRating"));
    callArgs.push_back(make_unique<Var>("deliveryRating"));
    callArgs.push_back(make_unique<Var>("reviewComment"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("leaveReview", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: _result in dom(Rev')
    vector<unique_ptr<Expr>> inArgs3;
    inArgs3.push_back(make_unique<Var>("_result"));
    vector<unique_ptr<Expr>> revPrimeArgs;
    revPrimeArgs.push_back(make_unique<Var>("Rev"));
    vector<unique_ptr<Expr>> domArgs3;
    domArgs3.push_back(
        make_unique<FuncCall>("primed", std::move(revPrimeArgs)));
    inArgs3.push_back(make_unique<FuncCall>("dom", std::move(domArgs3)));
    auto post = make_unique<FuncCall>("in", std::move(inArgs3));

    blocks.push_back(make_unique<API>("leaveReviewOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ========================================================================
   * OWNER
   * ======================================================================== */

  /* ---------- registerOwnerOk ---------- */
  {
    // PRE: ownerEmail not_in dom(U)
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("ownerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("U"));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("not_in", std::move(notInArgs));

    // CALL: registerOwner(ownerEmail, ownerPassword, ownerFullName,
    // ownerMobile)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("ownerEmail"));
    callArgs.push_back(make_unique<Var>("ownerPassword"));
    callArgs.push_back(make_unique<Var>("ownerFullName"));
    callArgs.push_back(make_unique<Var>("ownerMobile"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("registerOwner", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: U'[ownerEmail] = ownerPassword AND Roles'[ownerEmail] = OWNER
    vector<unique_ptr<Expr>> postArgs;

    vector<unique_ptr<Expr>> eq1Args;
    vector<unique_ptr<Expr>> uPrimeArgs;
    uPrimeArgs.push_back(make_unique<Var>("U"));
    vector<unique_ptr<Expr>> indexArgs1;
    indexArgs1.push_back(
        make_unique<FuncCall>("primed", std::move(uPrimeArgs)));
    indexArgs1.push_back(make_unique<Var>("ownerEmail"));
    eq1Args.push_back(make_unique<FuncCall>("[]", std::move(indexArgs1)));
    eq1Args.push_back(make_unique<Var>("ownerPassword"));
    postArgs.push_back(make_unique<FuncCall>("=", std::move(eq1Args)));

    vector<unique_ptr<Expr>> eq2Args;
    vector<unique_ptr<Expr>> rolesPrimeArgs;
    rolesPrimeArgs.push_back(make_unique<Var>("Roles"));
    vector<unique_ptr<Expr>> indexArgs2;
    indexArgs2.push_back(
        make_unique<FuncCall>("primed", std::move(rolesPrimeArgs)));
    indexArgs2.push_back(make_unique<Var>("ownerEmail"));
    eq2Args.push_back(make_unique<FuncCall>("[]", std::move(indexArgs2)));
    eq2Args.push_back(make_unique<Var>("OWNER"));
    postArgs.push_back(make_unique<FuncCall>("=", std::move(eq2Args)));

    auto post = make_unique<FuncCall>("AND", std::move(postArgs));

    blocks.push_back(make_unique<API>("registerOwnerOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- loginOwnerOk ---------- */
  {
    // PRE: ownerEmail in dom(U) AND U[ownerEmail] = ownerPassword
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("ownerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("U"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs)));

    vector<unique_ptr<Expr>> eqArgs;
    vector<unique_ptr<Expr>> indexArgs;
    indexArgs.push_back(make_unique<Var>("U"));
    indexArgs.push_back(make_unique<Var>("ownerEmail"));
    eqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs)));
    eqArgs.push_back(make_unique<Var>("ownerPassword"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(eqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: login(ownerEmail, ownerPassword)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("ownerEmail"));
    callArgs.push_back(make_unique<Var>("ownerPassword"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("login", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: T'[ownerEmail] = _result
    vector<unique_ptr<Expr>> postEqArgs;
    vector<unique_ptr<Expr>> tPrimeArgs;
    tPrimeArgs.push_back(make_unique<Var>("T"));
    vector<unique_ptr<Expr>> indexArgs2;
    indexArgs2.push_back(
        make_unique<FuncCall>("primed", std::move(tPrimeArgs)));
    indexArgs2.push_back(make_unique<Var>("ownerEmail"));
    postEqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs2)));
    postEqArgs.push_back(make_unique<Var>("_result"));
    auto post = make_unique<FuncCall>("=", std::move(postEqArgs));

    blocks.push_back(make_unique<API>("loginOwnerOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- createRestaurantOk ---------- */
  {
    // PRE: ownerEmail in dom(T) AND Roles[ownerEmail] = OWNER
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("ownerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("T"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs)));

    vector<unique_ptr<Expr>> eqArgs;
    vector<unique_ptr<Expr>> indexArgs;
    indexArgs.push_back(make_unique<Var>("Roles"));
    indexArgs.push_back(make_unique<Var>("ownerEmail"));
    eqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs)));
    eqArgs.push_back(make_unique<Var>("OWNER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(eqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: createRestaurant(ownerEmail, restaurantName, restaurantAddress,
    // restaurantContact)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("ownerEmail"));
    callArgs.push_back(make_unique<Var>("restaurantName"));
    callArgs.push_back(make_unique<Var>("restaurantAddress"));
    callArgs.push_back(make_unique<Var>("restaurantContact"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("createRestaurant", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: _result in dom(R') AND Owners'[_result] = ownerEmail
    vector<unique_ptr<Expr>> postArgs;

    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("_result"));
    vector<unique_ptr<Expr>> rPrimeArgs;
    rPrimeArgs.push_back(make_unique<Var>("R"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<FuncCall>("primed", std::move(rPrimeArgs)));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    postArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    vector<unique_ptr<Expr>> eq2Args;
    vector<unique_ptr<Expr>> ownersPrimeArgs;
    ownersPrimeArgs.push_back(make_unique<Var>("Owners"));
    vector<unique_ptr<Expr>> indexArgs2;
    indexArgs2.push_back(
        make_unique<FuncCall>("primed", std::move(ownersPrimeArgs)));
    indexArgs2.push_back(make_unique<Var>("_result"));
    eq2Args.push_back(make_unique<FuncCall>("[]", std::move(indexArgs2)));
    eq2Args.push_back(make_unique<Var>("ownerEmail"));
    postArgs.push_back(make_unique<FuncCall>("=", std::move(eq2Args)));

    auto post = make_unique<FuncCall>("AND", std::move(postArgs));

    blocks.push_back(make_unique<API>("createRestaurantOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- createRestaurantCustomerErr ---------- */
  // Tests that a CUSTOMER cannot create a restaurant (requires OWNER role)
  {
    // PRE: customerEmail in dom(T) AND Roles[customerEmail] = OWNER
    // NOTE: This precondition will be UNSAT because customer has role CUSTOMER,
    // not OWNER
    vector<unique_ptr<Expr>> preArgs;

    // customerEmail in dom(T) - customer is logged in
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("customerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("T"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs)));

    // Roles[customerEmail] = OWNER (will be FALSE - customer has CUSTOMER
    // role!)
    vector<unique_ptr<Expr>> eqArgs;
    vector<unique_ptr<Expr>> indexArgs;
    indexArgs.push_back(make_unique<Var>("Roles"));
    indexArgs.push_back(make_unique<Var>("customerEmail"));
    eqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs)));
    eqArgs.push_back(make_unique<Var>("OWNER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(eqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: createRestaurant(customerEmail, restaurantName, restaurantAddress,
    // restaurantContact)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("customerEmail"));
    callArgs.push_back(make_unique<Var>("restaurantName"));
    callArgs.push_back(make_unique<Var>("restaurantAddress"));
    callArgs.push_back(make_unique<Var>("restaurantContact"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("createRestaurant", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true (should never reach here - precondition will fail)
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("createRestaurantCustomerErr",
                                      std::move(pre), std::move(call),
                                      std::move(post)));
  }

  /* ---------- addMenuItemOk ---------- */
  {
    // PRE: ownerEmail in dom(T) AND Roles[ownerEmail] = OWNER AND restaurantId
    // in dom(R)
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("ownerEmail"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("T"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    vector<unique_ptr<Expr>> eqArgs;
    vector<unique_ptr<Expr>> indexArgs;
    indexArgs.push_back(make_unique<Var>("Roles"));
    indexArgs.push_back(make_unique<Var>("ownerEmail"));
    eqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs)));
    eqArgs.push_back(make_unique<Var>("OWNER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(eqArgs)));

    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("restaurantId"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("R"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: addMenuItem(ownerEmail, restaurantId, itemName, itemPrice)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("ownerEmail"));
    callArgs.push_back(make_unique<Var>("restaurantId"));
    callArgs.push_back(make_unique<Var>("itemName"));
    callArgs.push_back(make_unique<Var>("itemPrice"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("addMenuItem", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: _result in dom(M')
    vector<unique_ptr<Expr>> inArgs3;
    inArgs3.push_back(make_unique<Var>("_result"));
    vector<unique_ptr<Expr>> mPrimeArgs;
    mPrimeArgs.push_back(make_unique<Var>("M"));
    vector<unique_ptr<Expr>> domArgs3;
    domArgs3.push_back(make_unique<FuncCall>("primed", std::move(mPrimeArgs)));
    inArgs3.push_back(make_unique<FuncCall>("dom", std::move(domArgs3)));
    auto post = make_unique<FuncCall>("in", std::move(inArgs3));

    blocks.push_back(make_unique<API>("addMenuItemOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- assignOrderOk ---------- */
  {
    // PRE: ownerEmail in dom(T) AND Roles[ownerEmail] = OWNER AND orderId in
    // dom(O) AND agentEmail in dom(U) AND Roles[agentEmail] = AGENT
    vector<unique_ptr<Expr>> preArgs;

    // ownerEmail in dom(T)
    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("ownerEmail"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("T"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    // Roles[ownerEmail] = OWNER
    vector<unique_ptr<Expr>> eqArgs1;
    vector<unique_ptr<Expr>> indexArgs1;
    indexArgs1.push_back(make_unique<Var>("Roles"));
    indexArgs1.push_back(make_unique<Var>("ownerEmail"));
    eqArgs1.push_back(make_unique<FuncCall>("[]", std::move(indexArgs1)));
    eqArgs1.push_back(make_unique<Var>("OWNER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(eqArgs1)));

    // orderId in dom(O)
    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("orderId"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("O"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    // agentEmail in dom(U)
    vector<unique_ptr<Expr>> inArgs3;
    inArgs3.push_back(make_unique<Var>("agentEmail"));
    vector<unique_ptr<Expr>> domArgs3;
    domArgs3.push_back(make_unique<Var>("U"));
    inArgs3.push_back(make_unique<FuncCall>("dom", std::move(domArgs3)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs3)));

    // Roles[agentEmail] = AGENT
    vector<unique_ptr<Expr>> eqArgs2;
    vector<unique_ptr<Expr>> indexArgs2;
    indexArgs2.push_back(make_unique<Var>("Roles"));
    indexArgs2.push_back(make_unique<Var>("agentEmail"));
    eqArgs2.push_back(make_unique<FuncCall>("[]", std::move(indexArgs2)));
    eqArgs2.push_back(make_unique<Var>("AGENT"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(eqArgs2)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: assignOrder(ownerEmail, orderId, agentEmail)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("ownerEmail"));
    callArgs.push_back(make_unique<Var>("orderId"));
    callArgs.push_back(make_unique<Var>("agentEmail"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("assignOrder", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: Assignments'[orderId] = agentEmail
    vector<unique_ptr<Expr>> postEqArgs;
    vector<unique_ptr<Expr>> assignPrimeArgs;
    assignPrimeArgs.push_back(make_unique<Var>("Assignments"));
    vector<unique_ptr<Expr>> indexArgs3;
    indexArgs3.push_back(
        make_unique<FuncCall>("primed", std::move(assignPrimeArgs)));
    indexArgs3.push_back(make_unique<Var>("orderId"));
    postEqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs3)));
    postEqArgs.push_back(make_unique<Var>("agentEmail"));
    auto post = make_unique<FuncCall>("=", std::move(postEqArgs));

    blocks.push_back(make_unique<API>("assignOrderOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- updateOrderStatusOwnerOk ---------- */
  {
    // PRE: ownerEmail in dom(T) AND Roles[ownerEmail] = OWNER AND orderId in
    // dom(O)
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("ownerEmail"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("T"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    vector<unique_ptr<Expr>> eqArgs;
    vector<unique_ptr<Expr>> indexArgs;
    indexArgs.push_back(make_unique<Var>("Roles"));
    indexArgs.push_back(make_unique<Var>("ownerEmail"));
    eqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs)));
    eqArgs.push_back(make_unique<Var>("OWNER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(eqArgs)));

    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("orderId"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("O"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: updateOrderStatusOwner(ownerEmail, orderId, orderStatus)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("ownerEmail"));
    callArgs.push_back(make_unique<Var>("orderId"));
    callArgs.push_back(make_unique<Var>("orderStatus"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("updateOrderStatusOwner", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true (status updated)
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("updateOrderStatusOwnerOk",
                                      std::move(pre), std::move(call),
                                      std::move(post)));
  }
  /* ========================================================================
   * AGENT APIs
   * ======================================================================== */

  /* ---------- registerAgentOk ---------- */
  {
    // PRE: agentEmail not_in dom(U)
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("agentEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("U"));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("not_in", std::move(notInArgs));

    // CALL: registerAgent(agentEmail, agentPassword, agentFullName,
    // agentMobile)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("agentEmail"));
    callArgs.push_back(make_unique<Var>("agentPassword"));
    callArgs.push_back(make_unique<Var>("agentFullName"));
    callArgs.push_back(make_unique<Var>("agentMobile"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("registerAgent", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: U'[agentEmail] = agentPassword AND Roles'[agentEmail] = AGENT
    vector<unique_ptr<Expr>> postArgs;

    vector<unique_ptr<Expr>> eq1Args;
    vector<unique_ptr<Expr>> uPrimeArgs;
    uPrimeArgs.push_back(make_unique<Var>("U"));
    vector<unique_ptr<Expr>> indexArgs1;
    indexArgs1.push_back(
        make_unique<FuncCall>("primed", std::move(uPrimeArgs)));
    indexArgs1.push_back(make_unique<Var>("agentEmail"));
    eq1Args.push_back(make_unique<FuncCall>("[]", std::move(indexArgs1)));
    eq1Args.push_back(make_unique<Var>("agentPassword"));
    postArgs.push_back(make_unique<FuncCall>("=", std::move(eq1Args)));

    vector<unique_ptr<Expr>> eq2Args;
    vector<unique_ptr<Expr>> rolesPrimeArgs;
    rolesPrimeArgs.push_back(make_unique<Var>("Roles"));
    vector<unique_ptr<Expr>> indexArgs2;
    indexArgs2.push_back(
        make_unique<FuncCall>("primed", std::move(rolesPrimeArgs)));
    indexArgs2.push_back(make_unique<Var>("agentEmail"));
    eq2Args.push_back(make_unique<FuncCall>("[]", std::move(indexArgs2)));
    eq2Args.push_back(make_unique<Var>("AGENT"));
    postArgs.push_back(make_unique<FuncCall>("=", std::move(eq2Args)));

    auto post = make_unique<FuncCall>("AND", std::move(postArgs));

    blocks.push_back(make_unique<API>("registerAgentOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- loginAgentOk ---------- */
  {
    // PRE: agentEmail in dom(U) AND U[agentEmail] = agentPassword
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("agentEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("U"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs)));

    vector<unique_ptr<Expr>> eqArgs;
    vector<unique_ptr<Expr>> indexArgs;
    indexArgs.push_back(make_unique<Var>("U"));
    indexArgs.push_back(make_unique<Var>("agentEmail"));
    eqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs)));
    eqArgs.push_back(make_unique<Var>("agentPassword"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(eqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: login(agentEmail, agentPassword)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("agentEmail"));
    callArgs.push_back(make_unique<Var>("agentPassword"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("login", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: T'[agentEmail] = _result
    vector<unique_ptr<Expr>> postEqArgs;
    vector<unique_ptr<Expr>> tPrimeArgs;
    tPrimeArgs.push_back(make_unique<Var>("T"));
    vector<unique_ptr<Expr>> indexArgs2;
    indexArgs2.push_back(
        make_unique<FuncCall>("primed", std::move(tPrimeArgs)));
    indexArgs2.push_back(make_unique<Var>("agentEmail"));
    postEqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs2)));
    postEqArgs.push_back(make_unique<Var>("_result"));
    auto post = make_unique<FuncCall>("=", std::move(postEqArgs));

    blocks.push_back(make_unique<API>("loginAgentOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- updateOrderStatusAgentOk ---------- */
  {
    // PRE: agentEmail in dom(T) AND Roles[agentEmail] = AGENT AND orderId in
    // dom(Assignments) AND Assignments[orderId] = agentEmail
    vector<unique_ptr<Expr>> preArgs;

    // agentEmail in dom(T)
    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("agentEmail"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("T"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    // Roles[agentEmail] = AGENT
    vector<unique_ptr<Expr>> eqArgs1;
    vector<unique_ptr<Expr>> indexArgs1;
    indexArgs1.push_back(make_unique<Var>("Roles"));
    indexArgs1.push_back(make_unique<Var>("agentEmail"));
    eqArgs1.push_back(make_unique<FuncCall>("[]", std::move(indexArgs1)));
    eqArgs1.push_back(make_unique<Var>("AGENT"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(eqArgs1)));

    // orderId in dom(Assignments)
    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("orderId"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("Assignments"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    // Assignments[orderId] = agentEmail (agent is assigned to this order)
    vector<unique_ptr<Expr>> eqArgs2;
    vector<unique_ptr<Expr>> indexArgs2;
    indexArgs2.push_back(make_unique<Var>("Assignments"));
    indexArgs2.push_back(make_unique<Var>("orderId"));
    eqArgs2.push_back(make_unique<FuncCall>("[]", std::move(indexArgs2)));
    eqArgs2.push_back(make_unique<Var>("agentEmail"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(eqArgs2)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: updateOrderStatusAgent(agentEmail, orderId, orderStatus)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("agentEmail"));
    callArgs.push_back(make_unique<Var>("orderId"));
    callArgs.push_back(make_unique<Var>("orderStatus"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("updateOrderStatusAgent", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true (status updated)
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("updateOrderStatusAgentOk",
                                      std::move(pre), std::move(call),
                                      std::move(post)));
  }

  /* =====================================================
   * 5. BUILD SPEC
   * ===================================================== */

  return make_unique<Spec>(std::move(globals), std::move(init),
                           std::move(functions), std::move(blocks));
}
