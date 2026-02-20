#include "LibrarySpec.hpp"
#include "../language/ast.hh"

#include <memory>
#include <vector>

using namespace std;

std::unique_ptr<Spec> makeLibrarySpec() {

  /* =====================================================
   * 1. GLOBAL DECLARATIONS
   * ===================================================== */

  vector<unique_ptr<Decl>> globals;

  auto mkString = []() { return make_unique<TypeConst>("string"); };

  auto mkNum = []() { return make_unique<TypeConst>("int"); };

  // Book catalog: bookCode -> book data (title, author, desc)
  globals.push_back(
      make_unique<Decl>("B", make_unique<MapType>(mkString(), mkString())));

  // Students: studentId -> student data (name, email, phone)
  globals.push_back(
      make_unique<Decl>("S", make_unique<MapType>(mkString(), mkString())));

  // Pending borrow requests: slno -> request data (studentId, bookCode,
  // startDate, endDate)
  globals.push_back(
      make_unique<Decl>("Req", make_unique<MapType>(mkString(), mkString())));

  // Active loans (BookStudent): slno -> loan data (studentId, bookCode,
  // startDate, endDate)
  globals.push_back(
      make_unique<Decl>("Loans", make_unique<MapType>(mkString(), mkString())));

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
   * BOOK MANAGEMENT APIs
   * ======================================================================== */

  /* ---------- getAllBooksOk ---------- */
  {
    // PRE: true (public endpoint)
    auto pre = make_unique<Num>(1);

    // CALL: getAllBooks()
    vector<unique_ptr<Expr>> callArgs;
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getAllBooks", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getAllBooksOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- getBookByCodeOk ---------- */
  {
    // PRE: bookCode in dom(B)
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("bookCode"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("B"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("in", std::move(inArgs));

    // CALL: getBookByCode(bookCode)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("bookCode"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getBookByCode", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getBookByCodeOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- getBookByCodeErr (book not found) ---------- */
  {
    // PRE: bookCode NOT in dom(B)
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("bookCode"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("B"));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("not_in", std::move(notInArgs));

    // CALL: getBookByCode(bookCode)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("bookCode"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getBookByCode", std::move(callArgs)),
        Response(HTTPResponseCode::BAD_REQUEST_400, nullptr));

    // POST: true (returns 404)
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getBookByCodeErr", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- saveBookOk ---------- */
  {
    // PRE: true (anyone can add books in test mode)
    auto pre = make_unique<Num>(1);

    // CALL: saveBook(bookTitle, bookAuthor, bookDesc)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("bookTitle"));
    callArgs.push_back(make_unique<Var>("bookAuthor"));
    callArgs.push_back(make_unique<Var>("bookDesc"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("saveBook", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: _result in dom(B')
    vector<unique_ptr<Expr>> postInArgs;
    postInArgs.push_back(make_unique<Var>("_result"));
    vector<unique_ptr<Expr>> bPrimeArgs;
    bPrimeArgs.push_back(make_unique<Var>("B"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<FuncCall>("'", std::move(bPrimeArgs)));
    postInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto post = make_unique<FuncCall>("in", std::move(postInArgs));

    blocks.push_back(make_unique<API>("saveBookOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- updateBookOk ---------- */
  {
    // PRE: bookCode in dom(B)
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("bookCode"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("B"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("in", std::move(inArgs));

    // CALL: updateBook(bookCode, bookTitle, bookAuthor, bookDesc)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("bookCode"));
    callArgs.push_back(make_unique<Var>("bookTitle"));
    callArgs.push_back(make_unique<Var>("bookAuthor"));
    callArgs.push_back(make_unique<Var>("bookDesc"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("updateBook", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: bookCode in dom(B')
    vector<unique_ptr<Expr>> postInArgs;
    postInArgs.push_back(make_unique<Var>("bookCode"));
    vector<unique_ptr<Expr>> bPrimeArgs;
    bPrimeArgs.push_back(make_unique<Var>("B"));
    vector<unique_ptr<Expr>> postDomArgs;
    postDomArgs.push_back(make_unique<FuncCall>("'", std::move(bPrimeArgs)));
    postInArgs.push_back(make_unique<FuncCall>("dom", std::move(postDomArgs)));
    auto post = make_unique<FuncCall>("in", std::move(postInArgs));

    blocks.push_back(make_unique<API>("updateBookOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- deleteBookOk ---------- */
  {
    // PRE: bookCode in dom(B)
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("bookCode"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("B"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("in", std::move(inArgs));

    // CALL: deleteBook(bookCode)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("bookCode"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("deleteBook", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: bookCode not_in dom(B')
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("bookCode"));
    vector<unique_ptr<Expr>> bPrimeArgs;
    bPrimeArgs.push_back(make_unique<Var>("B"));
    vector<unique_ptr<Expr>> postDomArgs;
    postDomArgs.push_back(make_unique<FuncCall>("'", std::move(bPrimeArgs)));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(postDomArgs)));
    auto post = make_unique<FuncCall>("not_in", std::move(notInArgs));

    blocks.push_back(make_unique<API>("deleteBookOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ========================================================================
   * STUDENT MANAGEMENT APIs
   * ======================================================================== */

  /* ---------- getAllStudentsOk ---------- */
  {
    // PRE: true
    auto pre = make_unique<Num>(1);

    // CALL: getAllStudents()
    vector<unique_ptr<Expr>> callArgs;
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getAllStudents", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getAllStudentsOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- getStudentByIdOk ---------- */
  {
    // PRE: studentId in dom(S)
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("studentId"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("S"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("in", std::move(inArgs));

    // CALL: getStudentById(studentId)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("studentId"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getStudentById", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getStudentByIdOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- getStudentByIdErr ---------- */
  {
    // PRE: studentId NOT in dom(S)
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("studentId"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("S"));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("not_in", std::move(notInArgs));

    // CALL: getStudentById(studentId)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("studentId"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getStudentById", std::move(callArgs)),
        Response(HTTPResponseCode::BAD_REQUEST_400, nullptr));

    // POST: true (returns 404)
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getStudentByIdErr", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- saveStudentOk ---------- */
  {
    // PRE: true
    auto pre = make_unique<Num>(1);

    // CALL: saveStudent(studentName, studentEmail, studentPhone)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("studentName"));
    callArgs.push_back(make_unique<Var>("studentEmail"));
    callArgs.push_back(make_unique<Var>("studentPhone"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("saveStudent", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: _result in dom(S')
    vector<unique_ptr<Expr>> postInArgs;
    postInArgs.push_back(make_unique<Var>("_result"));
    vector<unique_ptr<Expr>> sPrimeArgs;
    sPrimeArgs.push_back(make_unique<Var>("S"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<FuncCall>("'", std::move(sPrimeArgs)));
    postInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto post = make_unique<FuncCall>("in", std::move(postInArgs));

    blocks.push_back(make_unique<API>("saveStudentOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- updateStudentOk ---------- */
  {
    // PRE: studentId in dom(S)
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("studentId"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("S"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("in", std::move(inArgs));

    // CALL: updateStudent(studentId, studentName, studentEmail, studentPhone)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("studentId"));
    callArgs.push_back(make_unique<Var>("studentName"));
    callArgs.push_back(make_unique<Var>("studentEmail"));
    callArgs.push_back(make_unique<Var>("studentPhone"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("updateStudent", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: studentId in dom(S')
    vector<unique_ptr<Expr>> postInArgs;
    postInArgs.push_back(make_unique<Var>("studentId"));
    vector<unique_ptr<Expr>> sPrimeArgs;
    sPrimeArgs.push_back(make_unique<Var>("S"));
    vector<unique_ptr<Expr>> postDomArgs;
    postDomArgs.push_back(make_unique<FuncCall>("'", std::move(sPrimeArgs)));
    postInArgs.push_back(make_unique<FuncCall>("dom", std::move(postDomArgs)));
    auto post = make_unique<FuncCall>("in", std::move(postInArgs));

    blocks.push_back(make_unique<API>("updateStudentOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- deleteStudentOk ---------- */
  {
    // PRE: studentId in dom(S)
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("studentId"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("S"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("in", std::move(inArgs));

    // CALL: deleteStudent(studentId)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("studentId"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("deleteStudent", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: studentId not_in dom(S')
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("studentId"));
    vector<unique_ptr<Expr>> sPrimeArgs;
    sPrimeArgs.push_back(make_unique<Var>("S"));
    vector<unique_ptr<Expr>> postDomArgs;
    postDomArgs.push_back(make_unique<FuncCall>("'", std::move(sPrimeArgs)));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(postDomArgs)));
    auto post = make_unique<FuncCall>("not_in", std::move(notInArgs));

    blocks.push_back(make_unique<API>("deleteStudentOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ========================================================================
   * REQUEST MANAGEMENT APIs (Borrow Requests)
   * ======================================================================== */

  /* ---------- getAllRequestsOk ---------- */
  {
    // PRE: true
    auto pre = make_unique<Num>(1);

    // CALL: getAllRequests()
    vector<unique_ptr<Expr>> callArgs;
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getAllRequests", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getAllRequestsOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- getRequestByIdOk ---------- */
  {
    // PRE: requestId in dom(Req)
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("requestId"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("Req"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("in", std::move(inArgs));

    // CALL: getRequestById(requestId)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("requestId"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getRequestById", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getRequestByIdOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- saveRequestOk ---------- */
  {
    // PRE: studentId in dom(S) AND bookCode in dom(B)
    vector<unique_ptr<Expr>> preArgs;

    // studentId in dom(S)
    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("studentId"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("S"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    // bookCode in dom(B)
    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("bookCode"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("B"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: saveRequest(studentId, bookCode, startDate, endDate)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("studentId"));
    callArgs.push_back(make_unique<Var>("bookCode"));
    callArgs.push_back(make_unique<Var>("startDate"));
    callArgs.push_back(make_unique<Var>("endDate"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("saveRequest", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: _result in dom(Req')
    vector<unique_ptr<Expr>> postInArgs;
    postInArgs.push_back(make_unique<Var>("_result"));
    vector<unique_ptr<Expr>> reqPrimeArgs;
    reqPrimeArgs.push_back(make_unique<Var>("Req"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<FuncCall>("'", std::move(reqPrimeArgs)));
    postInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto post = make_unique<FuncCall>("in", std::move(postInArgs));

    blocks.push_back(make_unique<API>("saveRequestOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- deleteRequestOk ---------- */
  {
    // PRE: requestId in dom(Req)
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("requestId"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("Req"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("in", std::move(inArgs));

    // CALL: deleteRequest(requestId)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("requestId"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("deleteRequest", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: requestId not_in dom(Req')
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("requestId"));
    vector<unique_ptr<Expr>> reqPrimeArgs;
    reqPrimeArgs.push_back(make_unique<Var>("Req"));
    vector<unique_ptr<Expr>> postDomArgs;
    postDomArgs.push_back(make_unique<FuncCall>("'", std::move(reqPrimeArgs)));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(postDomArgs)));
    auto post = make_unique<FuncCall>("not_in", std::move(notInArgs));

    blocks.push_back(make_unique<API>("deleteRequestOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ========================================================================
   * LOAN MANAGEMENT APIs (BookStudent / Book Issues)
   * ======================================================================== */

  /* ---------- getAllLoansOk ---------- */
  {
    // PRE: true
    auto pre = make_unique<Num>(1);

    // CALL: getAllLoans()
    vector<unique_ptr<Expr>> callArgs;
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getAllLoans", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getAllLoansOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- getLoanByIdOk ---------- */
  {
    // PRE: loanId in dom(Loans)
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("loanId"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("Loans"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("in", std::move(inArgs));

    // CALL: getLoanById(loanId)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("loanId"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("getLoanById", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: true
    auto post = make_unique<Num>(1);

    blocks.push_back(make_unique<API>("getLoanByIdOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- acceptRequestOk (key workflow: Request -> Loan) ---------- */
  {
    // PRE: requestId in dom(Req)
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("requestId"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("Req"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("in", std::move(inArgs));

    // CALL: acceptRequest(requestId)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("requestId"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("acceptRequest", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: _result in dom(Loans') AND requestId not_in dom(Req')
    vector<unique_ptr<Expr>> postArgs;

    // _result in dom(Loans')
    vector<unique_ptr<Expr>> inArgsPost;
    inArgsPost.push_back(make_unique<Var>("_result"));
    vector<unique_ptr<Expr>> loansPrimeArgs;
    loansPrimeArgs.push_back(make_unique<Var>("Loans"));
    vector<unique_ptr<Expr>> domArgsPost;
    domArgsPost.push_back(
        make_unique<FuncCall>("'", std::move(loansPrimeArgs)));
    inArgsPost.push_back(make_unique<FuncCall>("dom", std::move(domArgsPost)));
    postArgs.push_back(make_unique<FuncCall>("in", std::move(inArgsPost)));

    // requestId not_in dom(Req')
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("requestId"));
    vector<unique_ptr<Expr>> reqPrimeArgs;
    reqPrimeArgs.push_back(make_unique<Var>("Req"));
    vector<unique_ptr<Expr>> domArgsPost2;
    domArgsPost2.push_back(make_unique<FuncCall>("'", std::move(reqPrimeArgs)));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgsPost2)));
    postArgs.push_back(make_unique<FuncCall>("not_in", std::move(notInArgs)));

    auto post = make_unique<FuncCall>("AND", std::move(postArgs));

    blocks.push_back(make_unique<API>("acceptRequestOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- returnBookOk (delete loan / return book) ---------- */
  {
    // PRE: loanId in dom(Loans)
    vector<unique_ptr<Expr>> inArgs;
    inArgs.push_back(make_unique<Var>("loanId"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<Var>("Loans"));
    inArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto pre = make_unique<FuncCall>("in", std::move(inArgs));

    // CALL: returnBook(loanId)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("loanId"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("returnBook", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: loanId not_in dom(Loans')
    vector<unique_ptr<Expr>> notInArgs;
    notInArgs.push_back(make_unique<Var>("loanId"));
    vector<unique_ptr<Expr>> loansPrimeArgs;
    loansPrimeArgs.push_back(make_unique<Var>("Loans"));
    vector<unique_ptr<Expr>> postDomArgs;
    postDomArgs.push_back(
        make_unique<FuncCall>("'", std::move(loansPrimeArgs)));
    notInArgs.push_back(make_unique<FuncCall>("dom", std::move(postDomArgs)));
    auto post = make_unique<FuncCall>("not_in", std::move(notInArgs));

    blocks.push_back(make_unique<API>("returnBookOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* ---------- saveLoanOk (direct loan creation - admin bypass) ---------- */
  {
    // PRE: studentId in dom(S) AND bookCode in dom(B)
    vector<unique_ptr<Expr>> preArgs;

    vector<unique_ptr<Expr>> inArgs1;
    inArgs1.push_back(make_unique<Var>("studentId"));
    vector<unique_ptr<Expr>> domArgs1;
    domArgs1.push_back(make_unique<Var>("S"));
    inArgs1.push_back(make_unique<FuncCall>("dom", std::move(domArgs1)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs1)));

    vector<unique_ptr<Expr>> inArgs2;
    inArgs2.push_back(make_unique<Var>("bookCode"));
    vector<unique_ptr<Expr>> domArgs2;
    domArgs2.push_back(make_unique<Var>("B"));
    inArgs2.push_back(make_unique<FuncCall>("dom", std::move(domArgs2)));
    preArgs.push_back(make_unique<FuncCall>("in", std::move(inArgs2)));

    auto pre = make_unique<FuncCall>("AND", std::move(preArgs));

    // CALL: saveLoan(studentId, bookCode, startDate, endDate)
    vector<unique_ptr<Expr>> callArgs;
    callArgs.push_back(make_unique<Var>("studentId"));
    callArgs.push_back(make_unique<Var>("bookCode"));
    callArgs.push_back(make_unique<Var>("startDate"));
    callArgs.push_back(make_unique<Var>("endDate"));
    auto call = make_unique<APIcall>(
        make_unique<FuncCall>("saveLoan", std::move(callArgs)),
        Response(HTTPResponseCode::OK_200, nullptr));

    // POST: _result in dom(Loans')
    vector<unique_ptr<Expr>> postInArgs;
    postInArgs.push_back(make_unique<Var>("_result"));
    vector<unique_ptr<Expr>> loansPrimeArgs;
    loansPrimeArgs.push_back(make_unique<Var>("Loans"));
    vector<unique_ptr<Expr>> domArgs;
    domArgs.push_back(make_unique<FuncCall>("'", std::move(loansPrimeArgs)));
    postInArgs.push_back(make_unique<FuncCall>("dom", std::move(domArgs)));
    auto post = make_unique<FuncCall>("in", std::move(postInArgs));

    blocks.push_back(make_unique<API>("saveLoakOk", std::move(pre),
                                      std::move(call), std::move(post)));
  }

  /* =====================================================
   * 5. BUILD SPEC
   * ===================================================== */

  return make_unique<Spec>(std::move(globals), std::move(init),
                           std::move(functions), std::move(blocks));
}
