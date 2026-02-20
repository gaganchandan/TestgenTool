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

namespace Library {

// Forward declarations
class LibraryFunctionFactory;

/* ============================================================
 * Base class for all Library API functions
 * ============================================================ */
class LibraryAPIFunction : public Function {
protected:
  LibraryFunctionFactory *factory;
  vector<Expr *> arguments;

public:
  LibraryAPIFunction(LibraryFunctionFactory *factory, vector<Expr *> args);
  virtual ~LibraryAPIFunction() = default;

  string extractString(Expr *expr);
  int extractInt(Expr *expr);
  json extractJson(Expr *expr);
};

/* ============================================================
 * Test API Helper Functions
 * ============================================================ */

class ResetFunc : public LibraryAPIFunction {
public:
  ResetFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetBFunc : public LibraryAPIFunction {
public:
  GetBFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetBFunc : public LibraryAPIFunction {
public:
  SetBFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetSFunc : public LibraryAPIFunction {
public:
  GetSFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetSFunc : public LibraryAPIFunction {
public:
  SetSFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetReqFunc : public LibraryAPIFunction {
public:
  GetReqFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetReqFunc : public LibraryAPIFunction {
public:
  SetReqFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetLoansFunc : public LibraryAPIFunction {
public:
  GetLoansFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SetLoansFunc : public LibraryAPIFunction {
public:
  SetLoansFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

/* ============================================================
 * Book API Functions
 * ============================================================ */

class GetAllBooksFunc : public LibraryAPIFunction {
public:
  GetAllBooksFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetBookByCodeFunc : public LibraryAPIFunction {
public:
  GetBookByCodeFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SaveBookFunc : public LibraryAPIFunction {
public:
  SaveBookFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class UpdateBookFunc : public LibraryAPIFunction {
public:
  UpdateBookFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class DeleteBookFunc : public LibraryAPIFunction {
public:
  DeleteBookFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

/* ============================================================
 * Student API Functions
 * ============================================================ */

class GetAllStudentsFunc : public LibraryAPIFunction {
public:
  GetAllStudentsFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetStudentByIdFunc : public LibraryAPIFunction {
public:
  GetStudentByIdFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SaveStudentFunc : public LibraryAPIFunction {
public:
  SaveStudentFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class UpdateStudentFunc : public LibraryAPIFunction {
public:
  UpdateStudentFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class DeleteStudentFunc : public LibraryAPIFunction {
public:
  DeleteStudentFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

/* ============================================================
 * Request API Functions
 * ============================================================ */

class GetAllRequestsFunc : public LibraryAPIFunction {
public:
  GetAllRequestsFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetRequestByIdFunc : public LibraryAPIFunction {
public:
  GetRequestByIdFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SaveRequestFunc : public LibraryAPIFunction {
public:
  SaveRequestFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class DeleteRequestFunc : public LibraryAPIFunction {
public:
  DeleteRequestFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

/* ============================================================
 * Loan (BookStudent) API Functions
 * ============================================================ */

class GetAllLoansFunc : public LibraryAPIFunction {
public:
  GetAllLoansFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class GetLoanByIdFunc : public LibraryAPIFunction {
public:
  GetLoanByIdFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class AcceptRequestFunc : public LibraryAPIFunction {
public:
  AcceptRequestFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class ReturnBookFunc : public LibraryAPIFunction {
public:
  ReturnBookFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

class SaveLoanFunc : public LibraryAPIFunction {
public:
  SaveLoanFunc(LibraryFunctionFactory *factory, vector<Expr *> args);
  unique_ptr<Expr> execute() override;
};

/* ============================================================
 * LibraryFunctionFactory
 * ============================================================ */
class LibraryFunctionFactory : public FunctionFactory {
private:
  unique_ptr<HttpClient> httpClient;
  string baseUrl;

  // In-memory cache of global maps
  map<string, string> B_cache;     // Books: bookCode -> bookData
  map<string, string> S_cache;     // Students: studentId -> studentData
  map<string, string> Req_cache;   // Requests: slno -> requestData
  map<string, string> Loans_cache; // Loans: slno -> loanData

public:
  LibraryFunctionFactory(const string &baseUrl = "http://localhost:8080");
  ~LibraryFunctionFactory() = default;

  unique_ptr<Function> getFunction(string fname, vector<Expr *> args) override;

  HttpClient *getHttpClient() { return httpClient.get(); }

  map<string, string> &getB() { return B_cache; }
  map<string, string> &getS() { return S_cache; }
  map<string, string> &getReq() { return Req_cache; }
  map<string, string> &getLoans() { return Loans_cache; }
};

} // namespace Library
