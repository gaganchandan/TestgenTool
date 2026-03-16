%{
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include "ast.hh"

extern int yylex();
extern int yylineno;
void yyerror(const char* s);

// Root of our AST
Spec* astRoot = nullptr;

// Helper variables for building the AST
std::vector<std::unique_ptr<Decl>> globals;
std::vector<std::unique_ptr<Init>> inits;
std::vector<std::unique_ptr<APIFuncDecl>> functions;
std::vector<std::unique_ptr<API>> apis;

%}

%define parse.error verbose

%union {
    int intVal;
    std::string* stringVal;
    TypeExpr* typeExpr;
    Expr* expr;
    Var* var;
    FuncCall* funcCall;
    APIcall* apiCall;
    std::vector<std::unique_ptr<Expr>>* exprList;
    std::vector<std::unique_ptr<TypeExpr>>* typeList;
    std::vector<std::pair<std::unique_ptr<Var>, std::unique_ptr<Expr>>>* mappings;
    Decl* decl;
    APIFuncDecl* funcDecl;
    Init* init;
    Spec* spec;
    API* api;
    HTTPResponseCode httpCode;
}

/* Define all tokens here */
%token <stringVal> IDENTIFIER STRING_LITERAL
%token <intVal> NUMBER
%token TRUE FALSE
%token COLON ARROW BIG_ARROW SEMICOLON
%token LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET COMMA
%token EQUALS LT GT
%token PRECONDITION CALL POSTCONDITION
%token TYPE_STRING TYPE_INT TYPE_BOOL TYPE_VOID TYPE_MAP TYPE_SET TYPE_TUPLE
%token NIL OK

%type <typeExpr> type_expr
%type <expr> expr
%type <var> var
%type <funcCall> func_call
%type <exprList> exprs
%type <mappings> mappings
%type <typeList> type_list type_exprs
%type <decl> global_decl
%type <funcDecl> func_decl
%type <init> init
%type <api> api_block 
%type <apiCall> api_call
%type <expr> precondition postcondition
%type <spec> program
%type <httpCode> response_code

%start program

%%

program:
    declarations { 
        astRoot = new Spec(std::move(globals), std::move(inits), 
                           std::move(functions), std::move(apis));
        $$ = astRoot;
    }
    ;

declarations:
    declarations global_decl SEMICOLON {
        globals.push_back(std::unique_ptr<Decl>($2));
    }
    | declarations func_decl SEMICOLON {
        functions.push_back(std::unique_ptr<APIFuncDecl>($2));
    }
    | declarations init SEMICOLON {
        inits.push_back(std::unique_ptr<Init>($2));
    }
    | declarations api_block {
        apis.push_back(std::unique_ptr<API>($2));
    }
    | /* empty */ {}
    ;

init:
    IDENTIFIER COLON EQUALS expr {
        $$ = new Init(*$1, std::unique_ptr<Expr>($4));
    }
    ;

global_decl:
    IDENTIFIER COLON type_expr {
        $$ = new Decl(*$1, std::unique_ptr<TypeExpr>($3));
        delete $1;
    }
    ;

func_decl:
    IDENTIFIER COLON type_list {
        // For now, default HTTP response code to OK_200
        // std::vector<std::unique_ptr<TypeExpr>> returnTypes;
        std::unique_ptr<TypeExpr> returnType;

        // The last type in the list is the return type
        if (!$3->empty()) {
            returnType = std::move($3->back());
            $3->pop_back();
        }
        
        $$ = new APIFuncDecl(*$1, std::move(*$3), 
                          std::make_pair(HTTPResponseCode::OK_200, std::move(returnType)));
        delete $1;
        delete $3;
    }
    ;

type_list:
    type_expr {
        $$ = new std::vector<std::unique_ptr<TypeExpr>>();
        $$->push_back(std::unique_ptr<TypeExpr>($1));
    }
    | type_list ARROW type_expr {
        $1->push_back(std::unique_ptr<TypeExpr>($3));
        $$ = $1;
    }
    ;

type_exprs:
    type_expr {
        $$ = new std::vector<std::unique_ptr<TypeExpr>>();
        $$->push_back(std::unique_ptr<TypeExpr>($1));
    }
    | type_exprs COMMA type_expr {
        $1->push_back(std::unique_ptr<TypeExpr>($3));
        $$ = $1;
    }
    ;

type_expr:
    TYPE_STRING { $$ = new TypeConst("string"); }
    | TYPE_INT  { $$ = new TypeConst("int"); }
    | TYPE_BOOL { $$ = new TypeConst("bool"); }
    | TYPE_VOID { $$ = new TypeConst("void"); }
    | TYPE_MAP LT type_expr COMMA type_expr GT {
        $$ = new MapType(std::unique_ptr<TypeExpr>($3), std::unique_ptr<TypeExpr>($5));
    }
    | TYPE_SET LT type_expr GT {
        $$ = new SetType(std::unique_ptr<TypeExpr>($3));
    }
    | TYPE_TUPLE LT type_exprs GT {
        $$ = new TupleType(std::move(*$3));
        delete $3;
    }
    ;

api_block:
         IDENTIFIER COLON precondition api_call postcondition {
             $$ = new API(
             std::string(*$1),
             std::unique_ptr<Expr>($3),
             std::unique_ptr<APIcall>($4),
                std::unique_ptr<Expr>($5)
             );
         }
    ;

precondition:
    PRECONDITION COLON expr { $$ = $3; }
    | PRECONDITION COLON  { $$ = new Bool(true); }

api_call:
    CALL COLON func_call BIG_ARROW LPAREN response_code COMMA IDENTIFIER RPAREN {
        auto funcCall = $3;
        auto responseVar = new Var(*$8);
        
        $$ = new APIcall(
            std::unique_ptr<FuncCall>(funcCall),
            Response($6, std::unique_ptr<Expr>(responseVar))
        );
    }
    | CALL COLON func_call BIG_ARROW LPAREN response_code RPAREN {
        auto funcCall = $3;
        auto responseVar = new Var("dummy");
        
        $$ = new APIcall(
            std::unique_ptr<FuncCall>(funcCall),
            Response($6, std::unique_ptr<Expr>(responseVar))
        );
    }
    ;

postcondition:
    POSTCONDITION COLON expr { $$ = $3; }
    | POSTCONDITION COLON  { $$ = new Bool(true); }
    ;

expr:
    var { $$ = $1; }
    | STRING_LITERAL { $$ = new String(*$1); delete $1; }
    | NUMBER { $$ = new Num($1); }
    | TRUE { $$ = new Bool(true); }
    | FALSE { $$ = new Bool(false); }
    | func_call { $$ = $1; }
    | LBRACKET mappings RBRACKET {
        $$ = new Map(std::move(*$2));
        delete $2;
    }
    | LBRACE exprs RBRACE {
        $$ = new Set(std::move(*$2));
        delete $2;
    }
    | LPAREN exprs RPAREN {
        $$ = new Tuple(std::move(*$2));
        delete $2;
    }
    // | var LBRACKET expr RBRACKET {
    //     $$ = new MapAccess(std::unique_ptr<Var>($1), std::unique_ptr<Expr>($3));
    // }
    // | var LBRACKET expr ARROW expr RBRACKET {
    //     $$ = new MapUpdate(std::unique_ptr<Var>($1), 
    //                        std::unique_ptr<Expr>($3), 
    //                        std::unique_ptr<Expr>($5));
    // }
    // | expr EQUALS expr { 
    //     $$ = new Equals(std::unique_ptr<Expr>($1), std::unique_ptr<Expr>($3)); 
    // }
    | LPAREN expr RPAREN { $$ = $2; }
    | NIL {
        // Represent NIL as a special variable
        $$ = new Var("NIL");
    }
    ;

exprs:
    expr {
        $$ = new std::vector<std::unique_ptr<Expr>>();
        $$->push_back(std::unique_ptr<Expr>($1));
    }
    | exprs COMMA expr {
        $1->push_back(std::unique_ptr<Expr>($3));
        $$ = $1;
    }
    ;

mappings:
        var ARROW expr {
        $$ = new std::vector<std::pair<std::unique_ptr<Var>, std::unique_ptr<Expr>>>();
        $$->push_back(std::make_pair(std::unique_ptr<Var>($1), std::unique_ptr<Expr>($3)));
        }
    | mappings COMMA var ARROW expr {
        $1->push_back(std::make_pair(std::unique_ptr<Var>($3), std::unique_ptr<Expr>($5)));
        $$ = $1;
        }
    ;
var:
    IDENTIFIER { $$ = new Var(*$1); delete $1; }
    ;

func_call:
    IDENTIFIER LPAREN exprs RPAREN {
        $$ = new FuncCall(*$1, std::move(*$3));
        delete $1;
        delete $3;
    }
    | IDENTIFIER LPAREN RPAREN {
        $$ = new FuncCall(*$1, std::vector<std::unique_ptr<Expr>>());
        delete $1;
    }
    ;

response_code:
    OK { $$ = HTTPResponseCode::OK_200; }
    ;

%%

void yyerror(const char* s) {
    std::cerr << "Error (line " << yylineno << "): " << s << std::endl;
}
