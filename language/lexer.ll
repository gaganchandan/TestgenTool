%{
#include <string>
#include "ast.hh"
#include "parser.tab.hh" // Include the generated header with token definitions

#define YY_DECL int yylex()

extern "C" int yywrap() { return 1; }
%}

%option yylineno

%%

\n                     { yylineno++; /* count lines */ }
[ \t]+                 { /* ignore other whitespace */ }
"//".*                 { /* ignore comments */ }

":"                    { return COLON; }
";"                    { return SEMICOLON; }
"="                    { return EQUALS; }
"<"                    { return LT; }
">"                    { return GT; }
"->"                   { return ARROW; }
"==>"                  { return BIG_ARROW; }
"POSTCONDITION"        { return POSTCONDITION; }
"PRECONDITION"         { return PRECONDITION; }
"CALL"                 { return CALL; }

"string"               { return TYPE_STRING; }
"int"                  { return TYPE_INT; }
"bool"                 { return TYPE_BOOL; }
"void"                 { return TYPE_VOID; }
"map"                  { return TYPE_MAP; }
"set"                  { return TYPE_SET; }
"tuple"                { return TYPE_TUPLE; }

"NIL"                  { return NIL; }
"OK"                   { return OK; }
"true"                 { return TRUE; }
"false"                { return FALSE; }

[a-zA-Z][a-zA-Z0-9_']*  { 
                         yylval.stringVal = new std::string(yytext);
                         return IDENTIFIER; 
                       }

[0-9]+                 {
                         yylval.intVal = std::stoi(yytext);
                         return NUMBER;
                       }

\"[^\"]*\"             {
                         // Remove the quotes
                         std::string str(yytext);
                         str = str.substr(1, str.size() - 2);
                         yylval.stringVal = new std::string(str);
                         return STRING_LITERAL;
                       }

"("                    { return LPAREN; }
")"                    { return RPAREN; }
"{"                    { return LBRACE; }
"}"                    { return RBRACE; }
"["                    { return LBRACKET; }
"]"                    { return RBRACKET; }
","                    { return COMMA; }

.                      { /* ignore any other character */ }

%%
