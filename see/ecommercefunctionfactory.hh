#pragma once
#include "../language/ast.hh"
#include "functionfactory.hh"
#include "httpclient.hh"
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;
using namespace std;

namespace Ecommerce {

// Forward declarations
class EcommerceFunctionFactory;

/* ============================================================
 * Base class for all API functions
 * ============================================================ */
class EcommerceAPIFunction : public Function {
protected:
  EcommerceFunctionFactory *factory;
  vector<Expr *> arguments;

public:
  EcommerceAPIFunction(EcommerceFunctionFactory *factory, vector<Expr *> args);
  virtual ~EcommerceAPIFunction() = default;

  string extractString(Expr *expr);
  int extractInt(Expr *expr);
  json extractJson(Expr *expr);
  string getCurrentToken(const string &email);
};

/* ============================================================
 * Test API Helper Functions
 * ============================================================ */

class ResetFunc : public EcommerceAPIFunction {
public:
  ResetFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetUFunc : public EcommerceAPIFunction {
public:
  GetUFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetUFunc : public EcommerceAPIFunction {
public:
  SetUFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetTFunc : public EcommerceAPIFunction {
public:
  GetTFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetTFunc : public EcommerceAPIFunction {
public:
  SetTFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetRolesFunc : public EcommerceAPIFunction {
public:
  GetRolesFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetRolesFunc : public EcommerceAPIFunction {
public:
  SetRolesFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetPFunc : public EcommerceAPIFunction {
public:
  GetPFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetPFunc : public EcommerceAPIFunction {
public:
  SetPFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetStockFunc : public EcommerceAPIFunction {
public:
  GetStockFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetStockFunc : public EcommerceAPIFunction {
public:
  SetStockFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetSellersFunc : public EcommerceAPIFunction {
public:
  GetSellersFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetSellersFunc : public EcommerceAPIFunction {
public:
  SetSellersFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetCFunc : public EcommerceAPIFunction {
public:
  GetCFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetCFunc : public EcommerceAPIFunction {
public:
  SetCFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetOFunc : public EcommerceAPIFunction {
public:
  GetOFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetOFunc : public EcommerceAPIFunction {
public:
  SetOFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetOrderStatusFunc : public EcommerceAPIFunction {
public:
  GetOrderStatusFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetOrderStatusFunc : public EcommerceAPIFunction {
public:
  SetOrderStatusFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetRevFunc : public EcommerceAPIFunction {
public:
  GetRevFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetRevFunc : public EcommerceAPIFunction {
public:
  SetRevFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

/* ============================================================
 * Business API Functions
 * ============================================================ */

class RegisterBuyerFunc : public EcommerceAPIFunction {
public:
  RegisterBuyerFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class RegisterSellerFunc : public EcommerceAPIFunction {
public:
  RegisterSellerFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class LoginFunc : public EcommerceAPIFunction {
public:
  LoginFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetAllProductsFunc : public EcommerceAPIFunction {
public:
  GetAllProductsFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetProductByIdFunc : public EcommerceAPIFunction {
public:
  GetProductByIdFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class CreateProductFunc : public EcommerceAPIFunction {
public:
  CreateProductFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class UpdateProductFunc : public EcommerceAPIFunction {
public:
  UpdateProductFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class DeleteProductFunc : public EcommerceAPIFunction {
public:
  DeleteProductFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetSellerProductsFunc : public EcommerceAPIFunction {
public:
  GetSellerProductsFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class AddToCartFunc : public EcommerceAPIFunction {
public:
  AddToCartFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetCartFunc : public EcommerceAPIFunction {
public:
  GetCartFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class UpdateCartFunc : public EcommerceAPIFunction {
public:
  UpdateCartFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class CreateOrderFunc : public EcommerceAPIFunction {
public:
  CreateOrderFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetBuyerOrdersFunc : public EcommerceAPIFunction {
public:
  GetBuyerOrdersFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetSellerOrdersFunc : public EcommerceAPIFunction {
public:
  GetSellerOrdersFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class UpdateOrderStatusFunc : public EcommerceAPIFunction {
public:
  UpdateOrderStatusFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class CreateReviewFunc : public EcommerceAPIFunction {
public:
  CreateReviewFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetProductReviewsFunc : public EcommerceAPIFunction {
public:
  GetProductReviewsFunc(EcommerceFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

/* ============================================================
 * EcommerceFunctionFactory
 * ============================================================ */
class EcommerceFunctionFactory : public FunctionFactory {
private:
  unique_ptr<HttpClient> httpClient;
  string baseUrl;

  map<string, string> U_cache;
  map<string, string> T_cache;
  map<string, string> Roles_cache;
  map<string, string> P_cache;
  map<string, int> Stock_cache;
  map<string, string> Sellers_cache;
  map<string, string> C_cache;
  map<string, string> O_cache;
  map<string, string> OrderStatus_cache;
  map<string, string> Rev_cache;

public:
  EcommerceFunctionFactory(const string &baseUrl = "http://localhost:3000");
  ~EcommerceFunctionFactory() = default;

  unique_ptr<Function> getFunction(string fname, vector<Expr *> args) override;

  HttpClient *getHttpClient() { return httpClient.get(); }

  map<string, string> &getU() { return U_cache; }
  map<string, string> &getT() { return T_cache; }
  map<string, string> &getRoles() { return Roles_cache; }
  map<string, string> &getP() { return P_cache; }
  map<string, int> &getStock() { return Stock_cache; }
  map<string, string> &getSellers() { return Sellers_cache; }
  map<string, string> &getC() { return C_cache; }
  map<string, string> &getO() { return O_cache; }
  map<string, string> &getOrderStatus() { return OrderStatus_cache; }
  map<string, string> &getRev() { return Rev_cache; }
};

} // namespace Ecommerce
