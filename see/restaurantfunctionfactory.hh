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

// Forward declarations
class RestaurantFunctionFactory;

/* ============================================================
 * Base class for all API functions
 * ============================================================ */
class APIFunction : public Function {
protected:
  RestaurantFunctionFactory *factory;
  vector<Expr *> arguments;

public:
  APIFunction(RestaurantFunctionFactory *factory, vector<Expr *> args);
  virtual ~APIFunction() = default;

  // Helpers to extract concrete values from Expr*
  string extractString(Expr *expr);
  int extractInt(Expr *expr);
  json extractJson(Expr *expr);
  string getCurrentToken(const string &email);
};

/* ============================================================
 * Test API Helper Functions
 * ============================================================ */

class ResetFunc : public APIFunction {
public:
  ResetFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetUFunc : public APIFunction {
public:
  GetUFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetUFunc : public APIFunction {
public:
  SetUFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetTFunc : public APIFunction {
public:
  GetTFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetTFunc : public APIFunction {
public:
  SetTFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetRolesFunc : public APIFunction {
public:
  GetRolesFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetRolesFunc : public APIFunction {
public:
  SetRolesFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetCFunc : public APIFunction {
public:
  GetCFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetCFunc : public APIFunction {
public:
  SetCFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetRFunc : public APIFunction {
public:
  GetRFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetRFunc : public APIFunction {
public:
  SetRFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetMFunc : public APIFunction {
public:
  GetMFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetMFunc : public APIFunction {
public:
  SetMFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetOFunc : public APIFunction {
public:
  GetOFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetOFunc : public APIFunction {
public:
  SetOFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetRevFunc : public APIFunction {
public:
  GetRevFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetRevFunc : public APIFunction {
public:
  SetRevFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetOwnersFunc : public APIFunction {
public:
  GetOwnersFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetOwnersFunc : public APIFunction {
public:
  SetOwnersFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetAssignmentsFunc : public APIFunction {
public:
  GetAssignmentsFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetAssignmentsFunc : public APIFunction {
public:
  SetAssignmentsFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

/* ============================================================
 * Business API Functions
 * ============================================================ */

class RegisterCustomerFunc : public APIFunction {
public:
  RegisterCustomerFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class RegisterOwnerFunc : public APIFunction {
public:
  RegisterOwnerFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class RegisterAgentFunc : public APIFunction {
public:
  RegisterAgentFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class LoginFunc : public APIFunction {
public:
  LoginFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class BrowseRestaurantsFunc : public APIFunction {
public:
  BrowseRestaurantsFunc(RestaurantFunctionFactory *factory,
                        vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class ViewMenuFunc : public APIFunction {
public:
  ViewMenuFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class AddToCartFunc : public APIFunction {
public:
  AddToCartFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class PlaceOrderFunc : public APIFunction {
public:
  PlaceOrderFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class LeaveReviewFunc : public APIFunction {
public:
  LeaveReviewFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class CreateRestaurantFunc : public APIFunction {
public:
  CreateRestaurantFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class AddMenuItemFunc : public APIFunction {
public:
  AddMenuItemFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class AssignOrderFunc : public APIFunction {
public:
  AssignOrderFunc(RestaurantFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class UpdateOrderStatusOwnerFunc : public APIFunction {
public:
  UpdateOrderStatusOwnerFunc(RestaurantFunctionFactory *factory,
                             vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class UpdateOrderStatusAgentFunc : public APIFunction {
public:
  UpdateOrderStatusAgentFunc(RestaurantFunctionFactory *factory,
                             vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

/* ============================================================
 * RestaurantFunctionFactory
 * ============================================================ */
class RestaurantFunctionFactory : public FunctionFactory {
private:
  unique_ptr<HttpClient> httpClient;
  string baseUrl;

  // In-memory cache of global maps
  map<string, string> U_cache;
  map<string, string> T_cache;
  map<string, string> Roles_cache;
  map<string, string> C_cache;
  map<string, string> R_cache;
  map<string, string> M_cache;
  map<string, string> O_cache;
  map<string, string> Rev_cache;
  map<string, string> Owners_cache;
  map<string, string> Assignments_cache;

public:
  RestaurantFunctionFactory(const string &baseUrl = "http://localhost:5002");
  ~RestaurantFunctionFactory() = default;

  unique_ptr<Function> getFunction(string fname, vector<Expr *> args) override;

  HttpClient *getHttpClient() { return httpClient.get(); }

  map<string, string> &getU() { return U_cache; }
  map<string, string> &getT() { return T_cache; }
  map<string, string> &getRoles() { return Roles_cache; }
  map<string, string> &getC() { return C_cache; }
  map<string, string> &getR() { return R_cache; }
  map<string, string> &getM() { return M_cache; }
  map<string, string> &getO() { return O_cache; }
  map<string, string> &getRev() { return Rev_cache; }
  map<string, string> &getOwners() { return Owners_cache; }
  map<string, string> &getAssignments() { return Assignments_cache; }
};
