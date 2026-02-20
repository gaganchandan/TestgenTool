#include "EcommerceSpec.hpp"
#include "../language/ast.hh"

#include <memory>
#include <vector>

using namespace std;

std::unique_ptr<Spec> makeEcommerceSpec() {

  /* =====================================================
   * 1. GLOBAL DECLARATIONS
   * ===================================================== */

  vector<unique_ptr<Decl>> globals;

  auto mkString = []() { return make_unique<TypeConst>("string"); };

  auto mkUserType = []() { return make_unique<TypeConst>("Role"); };

  auto mkOrderStatus = []() { return make_unique<TypeConst>("OrderStatus"); };

  // User authentication & authorization
  globals.push_back(
      make_unique<Decl>("U", make_unique<MapType>(mkString(), mkString())));
  globals.push_back(
      make_unique<Decl>("T", make_unique<MapType>(mkString(), mkString())));
  globals.push_back(make_unique<Decl>(
      "Roles", make_unique<MapType>(mkString(), mkUserType())));

  // Product data
  globals.push_back(
      make_unique<Decl>("P", make_unique<MapType>(mkString(), mkString())));
  globals.push_back(
      make_unique<Decl>("Stock", make_unique<MapType>(mkString(), mkString())));
  globals.push_back(make_unique<Decl>(
      "Sellers", make_unique<MapType>(mkString(), mkString())));

  // Cart data
  globals.push_back(
      make_unique<Decl>("C", make_unique<MapType>(mkString(), mkString())));

  // Order data
  globals.push_back(
      make_unique<Decl>("O", make_unique<MapType>(mkString(), mkString())));
  globals.push_back(make_unique<Decl>(
      "OrderStatus", make_unique<MapType>(mkString(), mkOrderStatus())));

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

  /* ========================================================================
   * BUYER APIs
   * ======================================================================== */

  /* ---------- registerBuyerOk ---------- */
  {
    // PRE: buyerEmail not_in dom(U)
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("buyerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("U"));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("not_in", std::move(notInArgs));

    // CALL: registerBuyer(buyerEmail, buyerPassword, buyerFullName)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("buyerEmail"));
    callArgs.push_back(make_unique<Var>("buyerPassword"));
    callArgs.push_back(make_unique<Var>("buyerFullName"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("registerBuyer", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: U'[buyerEmail] = buyerPassword AND Roles'[buyerEmail] = BUYER
    vector<unique_ptr<Expr>> postArgs;

    vector<unique_ptr<Expr>> eq1Args;
    vector<unique_ptr<Expr>> uPrimeArgs;
    uPrimeArgs.push_back(make_unique<Var>("U"));
    vector<unique_ptr<Expr>> indexArgs1;
    indexArgs1.push_back(make_unique<FuncCall>("'", std::move(uPrimeArgs)));
    indexArgs1.push_back(make_unique<Var>("buyerEmail"));
    eq1Args.push_back(make_unique<FuncCall>("[]", std::move(indexArgs1)));
    eq1Args.push_back(make_unique<Var>("buyerPassword"));
    postArgs.push_back(make_unique<FuncCall>("=", std::move(eq1Args)));

    vector<unique_ptr<Expr>> eq2Args;
    vector<unique_ptr<Expr>> rolesPrimeArgs;
    rolesPrimeArgs.push_back(make_unique<Var>("Roles"));
    vector<unique_ptr<Expr>> indexArgs2;
    indexArgs2.push_back(make_unique<FuncCall>("'", std::move(rolesPrimeArgs)));
    indexArgs2.push_back(make_unique<Var>("buyerEmail"));
    eq2Args.push_back(make_unique<FuncCall>("[]", std::move(indexArgs2)));
    eq2Args.push_back(make_unique<Var>("BUYER"));
    postArgs.push_back(make_unique<FuncCall>("=", std::move(eq2Args)));

    auto post = make_unique<FuncCall>("AND", std::move(postArgs));

    blocks.push_back(make_unique<API>("registerBuyerOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- loginBuyerOk ---------- */
  {
    // PRE: buyerEmail in dom(U) AND U[buyerEmail] = buyerPassword AND
    // Roles[buyerEmail] = BUYER
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("buyerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("U"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs)));

    vector<unique_ptr<Expr>> eqArgs;
    vector<unique_ptr<Expr>> indexArgs;
    indexArgs.push_back(make_unique<Var>("U"));
    indexArgs.push_back(make_unique<Var>("buyerEmail"));
    eqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs)));
    eqArgs.push_back(make_unique<Var>("buyerPassword"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(eqArgs)));

    vector<unique_ptr<Expr>> roleEqArgs;
    vector<unique_ptr<Expr>> roleIndexArgs;
    roleIndexArgs.push_back(make_unique<Var>("Roles"));
    roleIndexArgs.push_back(make_unique<Var>("buyerEmail"));
    roleEqArgs.push_back(make_unique<FuncCall>("[]", std::move(roleIndexArgs)));
    roleEqArgs.push_back(make_unique<Var>("BUYER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(roleEqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: login(buyerEmail, buyerPassword)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("buyerEmail"));
    callArgs.push_back(make_unique<Var>("buyerPassword"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("login", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: T'[buyerEmail] = _result
    vector<unique_ptr<Expr>> postEqArgs;
    vector<unique_ptr<Expr>> tPrimeArgs;
    tPrimeArgs.push_back(make_unique<Var>("T"));
    vector<unique_ptr<Expr>> indexArgs2;
    indexArgs2.push_back(make_unique<FuncCall>("'", std::move(tPrimeArgs)));
    indexArgs2.push_back(make_unique<Var>("buyerEmail"));
    postEqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs2)));
    postEqArgs.push_back(make_unique<Var>("_result"));
    auto post = make_unique<FuncCall>("=", std::move(postEqArgs));

    blocks.push_back(make_unique<API>("loginBuyerOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- getAllProductsOk ---------- */
  {
    // PRE: true (public endpoint)
    auto pre = make_unique<Num>(1);

    // CALL: getAllProducts()
    vector<unique_ptr<Expr>> callArgs;
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getAllProducts", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getAllProductsOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- getProductByIdOk ---------- */
  {
    // PRE: productId in dom(P)
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("productId"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("P"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("in", std::move(inArgs));

    // CALL: getProductById(productId)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("productId"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getProductById", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getProductByIdOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- addToCartOk ---------- */
  {
    // PRE: buyerEmail in dom(T) AND Roles[buyerEmail] = BUYER AND productId in
    // dom(P)
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("buyerEmail"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("T"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    vector<unique_ptr<Expr>> roleEqArgs;
    vector<unique_ptr<Expr>> roleIndexArgs;
    roleIndexArgs.push_back(make_unique<Var>("Roles"));
    roleIndexArgs.push_back(make_unique<Var>("buyerEmail"));
    roleEqArgs.push_back(make_unique<FuncCall>("[]", std::move(roleIndexArgs)));
    roleEqArgs.push_back(make_unique<Var>("BUYER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(roleEqArgs)));

    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("productId"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("P"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: addToCart(buyerEmail, productId, quantity)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("buyerEmail"));
    callArgs.push_back(make_unique<Var>("productId"));
    callArgs.push_back(make_unique<Var>("quantity"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("addToCart", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: buyerEmail in dom(C')
    vector<unique_ptr<Expr>> inArgs3;
    inArgs3.push_back(make_unique<Var>("buyerEmail"));
    vector<unique_ptr<Expr>> cPrimeArgs;
    cPrimeArgs.push_back(make_unique<Var>("C"));
    vector<unique_ptr<Expr>> domArgs3;
    domArgs3.push_back(make_unique<FuncCall>("'", std::move(cPrimeArgs)));
    inArgs3.push_back(make_unique<FuncCall>("dom", std::move(domArgs3)));
    auto post = make_unique<FuncCall>("in", std::move(inArgs3));

    blocks.push_back(make_unique<API>("addToCartOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- getCartOk ---------- */
  {
    // PRE: buyerEmail in dom(T) AND Roles[buyerEmail] = BUYER
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("buyerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("T"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs)));

    vector<unique_ptr<Expr>> roleEqArgs;
    vector<unique_ptr<Expr>> roleIndexArgs;
    roleIndexArgs.push_back(make_unique<Var>("Roles"));
    roleIndexArgs.push_back(make_unique<Var>("buyerEmail"));
    roleEqArgs.push_back(make_unique<FuncCall>("[]", std::move(roleIndexArgs)));
    roleEqArgs.push_back(make_unique<Var>("BUYER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(roleEqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: getCart(buyerEmail)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("buyerEmail"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getCart", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getCartOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- updateCartOk ---------- */
  {
    // PRE: buyerEmail in dom(T) AND Roles[buyerEmail] = BUYER AND buyerEmail in
    // dom(C)
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("buyerEmail"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("T"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    vector<unique_ptr<Expr>> roleEqArgs;
    vector<unique_ptr<Expr>> roleIndexArgs;
    roleIndexArgs.push_back(make_unique<Var>("Roles"));
    roleIndexArgs.push_back(make_unique<Var>("buyerEmail"));
    roleEqArgs.push_back(make_unique<FuncCall>("[]", std::move(roleIndexArgs)));
    roleEqArgs.push_back(make_unique<Var>("BUYER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(roleEqArgs)));

    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("buyerEmail"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("C"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: updateCart(buyerEmail, productId, quantity)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("buyerEmail"));
    callArgs.push_back(make_unique<Var>("productId"));
    callArgs.push_back(make_unique<Var>("quantity"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("updateCart", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("updateCartOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- createOrderOk ---------- */
  {
    // PRE: buyerEmail in dom(T) AND Roles[buyerEmail] = BUYER AND buyerEmail in
    // dom(C)
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("buyerEmail"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("T"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    vector<unique_ptr<Expr>> roleEqArgs;
    vector<unique_ptr<Expr>> roleIndexArgs;
    roleIndexArgs.push_back(make_unique<Var>("Roles"));
    roleIndexArgs.push_back(make_unique<Var>("buyerEmail"));
    roleEqArgs.push_back(make_unique<FuncCall>("[]", std::move(roleIndexArgs)));
    roleEqArgs.push_back(make_unique<Var>("BUYER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(roleEqArgs)));

    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("buyerEmail"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("C"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: createOrder(buyerEmail, shippingAddress, paymentMethod)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("buyerEmail"));
    callArgs.push_back(make_unique<Var>("shippingAddress"));
    callArgs.push_back(make_unique<Var>("paymentMethod"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("createOrder", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: _result in dom(O') AND OrderStatus'[_result] = PENDING AND
    // buyerEmail not_in dom(C')
    vector<unique_ptr<Expr>> postArgs;

    // _result in dom(O')
    vector<unique_ptr<Expr>> inArgs3;
    inArgs3.push_back(make_unique<Var>("_result"));
    vector<unique_ptr<Expr>> oPrimeArgs;
    oPrimeArgs.push_back(make_unique<Var>("O"));
    vector<unique_ptr<Expr>> domArgs3;
    domArgs3.push_back(make_unique<FuncCall>("'", std::move(oPrimeArgs)));
    inArgs3.push_back(make_unique<FuncCall>("dom", std::move(domArgs3)));
    postArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs3)));

    // OrderStatus'[_result] = PENDING
    vector<unique_ptr<Expr>> statusEqArgs;
    vector<unique_ptr<Expr>> statusPrimeArgs;
    statusPrimeArgs.push_back(make_unique<Var>("OrderStatus"));
    vector<unique_ptr<Expr>> statusIndexArgs;
    statusIndexArgs.push_back(
        make_unique<FuncCall>("'", std::move(statusPrimeArgs)));
    statusIndexArgs.push_back(make_unique<Var>("_result"));
    statusEqArgs.push_back(
        make_unique<FuncCall>("[]", std::move(statusIndexArgs)));
    statusEqArgs.push_back(make_unique<Var>("PENDING"));
    postArgs.push_back(make_unique<FuncCall>("=", std::move(statusEqArgs)));

    // buyerEmail not_in dom(C') - cart cleared
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("buyerEmail"));
    vector<unique_ptr<Expr>> cPrimeArgs;
    cPrimeArgs.push_back(make_unique<Var>("C"));
    vector<unique_ptr<Expr>> domArgs4;
    domArgs4.push_back(make_unique<FuncCall>("'", std::move(cPrimeArgs)));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs4)));
    postArgs.push_back(make_unique<FuncCall>("not_in", std::move(notInArgs)));

    auto post = make_unique<FuncCall>("AND", std::move(postArgs));

    blocks.push_back(make_unique<API>("createOrderOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- getBuyerOrdersOk ---------- */
  {
    // PRE: buyerEmail in dom(T) AND Roles[buyerEmail] = BUYER
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("buyerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("T"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs)));

    vector<unique_ptr<Expr>> roleEqArgs;
    vector<unique_ptr<Expr>> roleIndexArgs;
    roleIndexArgs.push_back(make_unique<Var>("Roles"));
    roleIndexArgs.push_back(make_unique<Var>("buyerEmail"));
    roleEqArgs.push_back(make_unique<FuncCall>("[]", std::move(roleIndexArgs)));
    roleEqArgs.push_back(make_unique<Var>("BUYER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(roleEqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: getBuyerOrders(buyerEmail)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("buyerEmail"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getBuyerOrders", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getBuyerOrdersOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- createReviewOk ---------- */
  {
    // PRE: buyerEmail in dom(T) AND Roles[buyerEmail] = BUYER AND orderId in
    // dom(O) AND O[orderId] = buyerEmail
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("buyerEmail"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("T"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    vector<unique_ptr<Expr>> roleEqArgs;
    vector<unique_ptr<Expr>> roleIndexArgs;
    roleIndexArgs.push_back(make_unique<Var>("Roles"));
    roleIndexArgs.push_back(make_unique<Var>("buyerEmail"));
    roleEqArgs.push_back(make_unique<FuncCall>("[]", std::move(roleIndexArgs)));
    roleEqArgs.push_back(make_unique<Var>("BUYER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(roleEqArgs)));

    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("orderId"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("O"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    // O[orderId] = buyerEmail (buyer owns the order)
    vector<unique_ptr<Expr>> ownerEqArgs;
    vector<unique_ptr<Expr>> ownerIndexArgs;
    ownerIndexArgs.push_back(make_unique<Var>("O"));
    ownerIndexArgs.push_back(make_unique<Var>("orderId"));
    ownerEqArgs.push_back(
        make_unique<FuncCall>("[]", std::move(ownerIndexArgs)));
    ownerEqArgs.push_back(make_unique<Var>("buyerEmail"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(ownerEqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: createReview(buyerEmail, productId, orderId, rating, comment)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("buyerEmail"));
    callArgs.push_back(make_unique<Var>("productId"));
    callArgs.push_back(make_unique<Var>("orderId"));
    callArgs.push_back(make_unique<Var>("rating"));
    callArgs.push_back(make_unique<Var>("comment"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("createReview", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: _result in dom(Rev')
    vector<unique_ptr<Expr>> inArgs3;
    inArgs3.push_back(make_unique<Var>("_result"));
    vector<unique_ptr<Expr>> revPrimeArgs;
    revPrimeArgs.push_back(make_unique<Var>("Rev"));
    vector<unique_ptr<Expr>> domArgs3;
    domArgs3.push_back(make_unique<FuncCall>("'", std::move(revPrimeArgs)));
    inArgs3.push_back(make_unique<FuncCall>("dom", std::move(domArgs3)));
    auto post = make_unique<FuncCall>("in", std::move(inArgs3));

    blocks.push_back(make_unique<API>("createReviewOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- getProductReviewsOk ---------- */
  {
    // PRE: productId in dom(P)
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("productId"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("P"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("in", std::move(inArgs));

    // CALL: getProductReviews(productId)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("productId"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getProductReviews", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getProductReviewsOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ========================================================================
   * SELLER APIs
   * ======================================================================== */

  /* ---------- registerSellerOk ---------- */
  {
    // PRE: sellerEmail not_in dom(U)
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("sellerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("U"));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("not_in", std::move(notInArgs));

    // CALL: registerSeller(sellerEmail, sellerPassword, sellerFullName,
    // storeName, storeDescription)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("sellerEmail"));
    callArgs.push_back(make_unique<Var>("sellerPassword"));
    callArgs.push_back(make_unique<Var>("sellerFullName"));
    callArgs.push_back(make_unique<Var>("storeName"));
    callArgs.push_back(make_unique<Var>("storeDescription"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("registerSeller", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: U'[sellerEmail] = sellerPassword AND Roles'[sellerEmail] = SELLER
    vector<unique_ptr<Expr>> postArgs;

    vector<unique_ptr<Expr>> eq1Args;
    vector<unique_ptr<Expr>> uPrimeArgs;
    uPrimeArgs.push_back(make_unique<Var>("U"));
    vector<unique_ptr<Expr>> indexArgs1;
    indexArgs1.push_back(make_unique<FuncCall>("'", std::move(uPrimeArgs)));
    indexArgs1.push_back(make_unique<Var>("sellerEmail"));
    eq1Args.push_back(make_unique<FuncCall>("[]", std::move(indexArgs1)));
    eq1Args.push_back(make_unique<Var>("sellerPassword"));
    postArgs.push_back(make_unique<FuncCall>("=", std::move(eq1Args)));

    vector<unique_ptr<Expr>> eq2Args;
    vector<unique_ptr<Expr>> rolesPrimeArgs;
    rolesPrimeArgs.push_back(make_unique<Var>("Roles"));
    vector<unique_ptr<Expr>> indexArgs2;
    indexArgs2.push_back(make_unique<FuncCall>("'", std::move(rolesPrimeArgs)));
    indexArgs2.push_back(make_unique<Var>("sellerEmail"));
    eq2Args.push_back(make_unique<FuncCall>("[]", std::move(indexArgs2)));
    eq2Args.push_back(make_unique<Var>("SELLER"));
    postArgs.push_back(make_unique<FuncCall>("=", std::move(eq2Args)));

    auto post = make_unique<FuncCall>("AND", std::move(postArgs));

    blocks.push_back(make_unique<API>("registerSellerOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- loginSellerOk ---------- */
  {
    // PRE: sellerEmail in dom(U) AND U[sellerEmail] = sellerPassword AND
    // Roles[sellerEmail] = SELLER
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("sellerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("U"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs)));

    vector<unique_ptr<Expr>> eqArgs;
    vector<unique_ptr<Expr>> indexArgs;
    indexArgs.push_back(make_unique<Var>("U"));
    indexArgs.push_back(make_unique<Var>("sellerEmail"));
    eqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs)));
    eqArgs.push_back(make_unique<Var>("sellerPassword"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(eqArgs)));

    vector<unique_ptr<Expr>> roleEqArgs;
    vector<unique_ptr<Expr>> roleIndexArgs;
    roleIndexArgs.push_back(make_unique<Var>("Roles"));
    roleIndexArgs.push_back(make_unique<Var>("sellerEmail"));
    roleEqArgs.push_back(make_unique<FuncCall>("[]", std::move(roleIndexArgs)));
    roleEqArgs.push_back(make_unique<Var>("SELLER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(roleEqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: login(sellerEmail, sellerPassword)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("sellerEmail"));
    callArgs.push_back(make_unique<Var>("sellerPassword"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("login", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: T'[sellerEmail] = _result
    vector<unique_ptr<Expr>> postEqArgs;
    vector<unique_ptr<Expr>> tPrimeArgs;
    tPrimeArgs.push_back(make_unique<Var>("T"));
    vector<unique_ptr<Expr>> indexArgs2;
    indexArgs2.push_back(make_unique<FuncCall>("'", std::move(tPrimeArgs)));
    indexArgs2.push_back(make_unique<Var>("sellerEmail"));
    postEqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs2)));
    postEqArgs.push_back(make_unique<Var>("_result"));
    auto post = make_unique<FuncCall>("=", std::move(postEqArgs));

    blocks.push_back(make_unique<API>("loginSellerOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- createProductOk ---------- */
  {
    // PRE: sellerEmail in dom(T) AND Roles[sellerEmail] = SELLER
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("sellerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("T"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs)));

    vector<unique_ptr<Expr>> eqArgs;
    vector<unique_ptr<Expr>> indexArgs;
    indexArgs.push_back(make_unique<Var>("Roles"));
    indexArgs.push_back(make_unique<Var>("sellerEmail"));
    eqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs)));
    eqArgs.push_back(make_unique<Var>("SELLER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(eqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: createProduct(sellerEmail, title, description, category, price,
    // quantity)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("sellerEmail"));
    callArgs.push_back(make_unique<Var>("title"));
    callArgs.push_back(make_unique<Var>("description"));
    callArgs.push_back(make_unique<Var>("category"));
    callArgs.push_back(make_unique<Var>("price"));
    callArgs.push_back(make_unique<Var>("quantity"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("createProduct", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: _result in dom(P') AND Sellers'[_result] = sellerEmail
    vector<unique_ptr<Expr>> postArgs;

    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("_result"));
    vector<unique_ptr<Expr>> pPrimeArgs;
    pPrimeArgs.push_back(make_unique<Var>("P"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<FuncCall>("'", std::move(pPrimeArgs)));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    postArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    vector<unique_ptr<Expr>> eq2Args;
    vector<unique_ptr<Expr>> sellersPrimeArgs;
    sellersPrimeArgs.push_back(make_unique<Var>("Sellers"));
    vector<unique_ptr<Expr>> indexArgs2;
    indexArgs2.push_back(
        make_unique<FuncCall>("'", std::move(sellersPrimeArgs)));
    indexArgs2.push_back(make_unique<Var>("_result"));
    eq2Args.push_back(make_unique<FuncCall>("[]", std::move(indexArgs2)));
    eq2Args.push_back(make_unique<Var>("sellerEmail"));
    postArgs.push_back(make_unique<FuncCall>("=", std::move(eq2Args)));

    auto post = make_unique<FuncCall>("AND", std::move(postArgs));

    blocks.push_back(make_unique<API>("createProductOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- updateProductOk ---------- */
  {
    // PRE: sellerEmail in dom(T) AND Roles[sellerEmail] = SELLER AND productId
    // in dom(P) AND Sellers[productId] = sellerEmail
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("sellerEmail"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("T"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    vector<unique_ptr<Expr>> roleEqArgs;
    vector<unique_ptr<Expr>> roleIndexArgs;
    roleIndexArgs.push_back(make_unique<Var>("Roles"));
    roleIndexArgs.push_back(make_unique<Var>("sellerEmail"));
    roleEqArgs.push_back(make_unique<FuncCall>("[]", std::move(roleIndexArgs)));
    roleEqArgs.push_back(make_unique<Var>("SELLER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(roleEqArgs)));

    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("productId"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("P"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    vector<unique_ptr<Expr>> ownerEqArgs;
    vector<unique_ptr<Expr>> ownerIndexArgs;
    ownerIndexArgs.push_back(make_unique<Var>("Sellers"));
    ownerIndexArgs.push_back(make_unique<Var>("productId"));
    ownerEqArgs.push_back(
        make_unique<FuncCall>("[]", std::move(ownerIndexArgs)));
    ownerEqArgs.push_back(make_unique<Var>("sellerEmail"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(ownerEqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: updateProduct(sellerEmail, productId, title, description, category,
    // price, quantity)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("sellerEmail"));
    callArgs.push_back(make_unique<Var>("productId"));
    callArgs.push_back(make_unique<Var>("title"));
    callArgs.push_back(make_unique<Var>("description"));
    callArgs.push_back(make_unique<Var>("category"));
    callArgs.push_back(make_unique<Var>("price"));
    callArgs.push_back(make_unique<Var>("quantity"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("updateProduct", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("updateProductOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- deleteProductOk ---------- */
  {
    // PRE: sellerEmail in dom(T) AND Roles[sellerEmail] = SELLER AND
    // Sellers[productId] = sellerEmail
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("sellerEmail"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("T"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    vector<unique_ptr<Expr>> roleEqArgs;
    vector<unique_ptr<Expr>> roleIndexArgs;
    roleIndexArgs.push_back(make_unique<Var>("Roles"));
    roleIndexArgs.push_back(make_unique<Var>("sellerEmail"));
    roleEqArgs.push_back(make_unique<FuncCall>("[]", std::move(roleIndexArgs)));
    roleEqArgs.push_back(make_unique<Var>("SELLER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(roleEqArgs)));

    vector<unique_ptr<Expr>> ownerEqArgs;
    vector<unique_ptr<Expr>> ownerIndexArgs;
    ownerIndexArgs.push_back(make_unique<Var>("Sellers"));
    ownerIndexArgs.push_back(make_unique<Var>("productId"));
    ownerEqArgs.push_back(
        make_unique<FuncCall>("[]", std::move(ownerIndexArgs)));
    ownerEqArgs.push_back(make_unique<Var>("sellerEmail"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(ownerEqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: deleteProduct(sellerEmail, productId)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("sellerEmail"));
    callArgs.push_back(make_unique<Var>("productId"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("deleteProduct", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: productId not_in dom(P')
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("productId"));
    vector<unique_ptr<Expr>> pPrimeArgs;
    pPrimeArgs.push_back(make_unique<Var>("P"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<FuncCall>("'", std::move(pPrimeArgs)));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    auto post = make_unique<FuncCall>("not_in", std::move(notInArgs));

    blocks.push_back(make_unique<API>("deleteProductOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- getSellerProductsOk ---------- */
  {
    // PRE: sellerEmail in dom(T) AND Roles[sellerEmail] = SELLER
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("sellerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("T"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs)));

    vector<unique_ptr<Expr>> roleEqArgs;
    vector<unique_ptr<Expr>> roleIndexArgs;
    roleIndexArgs.push_back(make_unique<Var>("Roles"));
    roleIndexArgs.push_back(make_unique<Var>("sellerEmail"));
    roleEqArgs.push_back(make_unique<FuncCall>("[]", std::move(roleIndexArgs)));
    roleEqArgs.push_back(make_unique<Var>("SELLER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(roleEqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: getSellerProducts(sellerEmail)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("sellerEmail"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getSellerProducts", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getSellerProductsOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- getSellerOrdersOk ---------- */
  {
    // PRE: sellerEmail in dom(T) AND Roles[sellerEmail] = SELLER
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("sellerEmail"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("T"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs)));

    vector<unique_ptr<Expr>> roleEqArgs;
    vector<unique_ptr<Expr>> roleIndexArgs;
    roleIndexArgs.push_back(make_unique<Var>("Roles"));
    roleIndexArgs.push_back(make_unique<Var>("sellerEmail"));
    roleEqArgs.push_back(make_unique<FuncCall>("[]", std::move(roleIndexArgs)));
    roleEqArgs.push_back(make_unique<Var>("SELLER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(roleEqArgs)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: getSellerOrders(sellerEmail)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("sellerEmail"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getSellerOrders", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getSellerOrdersOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- updateOrderStatusOk ---------- */
  {
    // PRE: sellerEmail in dom(T) AND Roles[sellerEmail] = SELLER AND orderId in
    // dom(O)
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("sellerEmail"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("T"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    vector<unique_ptr<Expr>> roleEqArgs;
    vector<unique_ptr<Expr>> roleIndexArgs;
    roleIndexArgs.push_back(make_unique<Var>("Roles"));
    roleIndexArgs.push_back(make_unique<Var>("sellerEmail"));
    roleEqArgs.push_back(make_unique<FuncCall>("[]", std::move(roleIndexArgs)));
    roleEqArgs.push_back(make_unique<Var>("SELLER"));
    preArgs.push_back(make_unique<FuncCall>("=", std::move(roleEqArgs)));

    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("orderId"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("O"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: updateOrderStatus(sellerEmail, orderId, status)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("sellerEmail"));
    callArgs.push_back(make_unique<Var>("orderId"));
    callArgs.push_back(make_unique<Var>("status"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("updateOrderStatus", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: OrderStatus'[orderId] = status
    vector<unique_ptr<Expr>> postEqArgs;
    vector<unique_ptr<Expr>> statusPrimeArgs;
    statusPrimeArgs.push_back(make_unique<Var>("OrderStatus"));
    vector<unique_ptr<Expr>> indexArgs2;
    indexArgs2.push_back(
        make_unique<FuncCall>("'", std::move(statusPrimeArgs)));
    indexArgs2.push_back(make_unique<Var>("orderId"));
    postEqArgs.push_back(make_unique<FuncCall>("[]", std::move(indexArgs2)));
    postEqArgs.push_back(make_unique<Var>("status"));
    auto post = make_unique<FuncCall>("=", std::move(postEqArgs));

    blocks.push_back(make_unique<API>("updateOrderStatusOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* =====================================================
   * 5. BUILD SPEC
   * ===================================================== */

  return make_unique<Spec>(std::move(globals), std::move(init),
                           std::move(functions), std::move(blocks));
}
