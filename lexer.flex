%top{
    #include "parser.tab.hh"
    #define YY_DECL yy::parser::symbol_type yylex()
    #include "Node.h"
    #include <iostream>
    int lexical_errors = 0;
}
%option yylineno noyywrap nounput batch noinput stack 
%%

    /* Keywords */
"int"                    { if(USE_LEX_ONLY) {printf("TYPE_INT ");} else {return yy::parser::make_TYPE_INT(yytext);} }
"float"                   { if(USE_LEX_ONLY) {printf("TYPE_FLOAT ");} else {return yy::parser::make_TYPE_FLOAT(yytext);} }
"char"                    { if(USE_LEX_ONLY) {printf("TYPE_CHAR ");} else {return yy::parser::make_TYPE_CHAR(yytext);} }
"boolean"                 { if(USE_LEX_ONLY) {printf("TYPE_BOOL ");} else {return yy::parser::make_TYPE_BOOL(yytext);} }
"String"                  { if(USE_LEX_ONLY) {printf("TYPE_STRING ");} else {return yy::parser::make_TYPE_STRING(yytext);} }
"void"                    { if(USE_LEX_ONLY) {printf("TYPE_VOID ");} else {return yy::parser::make_TYPE_VOID(yytext);} }

"public"                  { if(USE_LEX_ONLY) {printf("PUBLIC ");} else {return yy::parser::make_PUBLIC(yytext);} }
"if"                      { if(USE_LEX_ONLY) {printf("IF ");} else {return yy::parser::make_IF(yytext);} }
"else"                    { if(USE_LEX_ONLY) {printf("ELSE ");} else {return yy::parser::make_ELSE(yytext);} }
"while"                   { if(USE_LEX_ONLY) {printf("WHILE ");} else {return yy::parser::make_WHILE(yytext);} }
"for"                     { if(USE_LEX_ONLY) {printf("FOR ");} else {return yy::parser::make_FOR(yytext);} }
"return"                  { if(USE_LEX_ONLY) {printf("RETURN ");} else {return yy::parser::make_RETURN(yytext);} }
"true"                    { if(USE_LEX_ONLY) {printf("TRUE ");} else {return yy::parser::make_TRUE(yytext);} }
"false"                   { if(USE_LEX_ONLY) {printf("FALSE ");} else {return yy::parser::make_FALSE(yytext);} }
"this"                    { if(USE_LEX_ONLY) {printf("THIS ");} else {return yy::parser::make_THIS(yytext);} }
"new"                     { if(USE_LEX_ONLY) {printf("NEW ");} else {return yy::parser::make_NEW(yytext);} }
"main"                    { if(USE_LEX_ONLY) {printf("MAIN ");} else {return yy::parser::make_MAIN(yytext);} }
"static"                  { if(USE_LEX_ONLY) {printf("STATIC ");} else {return yy::parser::make_STATIC(yytext);} }
"class"                   { if(USE_LEX_ONLY) {printf("CLASS ");} else {return yy::parser::make_CLASS(yytext);} }
"length"                   { if(USE_LEX_ONLY) {printf("LENGTH ");} else {return yy::parser::make_LENGTH(yytext);} }
"System.out.println"      { if(USE_LEX_ONLY) {printf("PRINT_METHOD ");} else {return yy::parser::make_PRINT_METHOD(yytext);} }

    /* Operators */
"=="                    { if(USE_LEX_ONLY) {printf("IS_EQUAL ");} else {return yy::parser::make_IS_EQUAL(yytext);} }
"="                     { if(USE_LEX_ONLY) {printf("ASSIGNOP ");} else {return yy::parser::make_ASSIGNOP(yytext);} }
"+"                     { if(USE_LEX_ONLY) {printf("PLUSOP ");} else {return yy::parser::make_PLUSOP(yytext);} }
"-"                     { if(USE_LEX_ONLY) {printf("MINUSOP ");} else {return yy::parser::make_MINUSOP(yytext);} }
"*"                     { if(USE_LEX_ONLY) {printf("MULTOP ");} else {return yy::parser::make_MULTOP(yytext);} }
"&&"                    { if(USE_LEX_ONLY) {printf("AND ");} else {return yy::parser::make_AND(yytext);} }
"||"                    { if(USE_LEX_ONLY) {printf("OR ");} else {return yy::parser::make_OR(yytext);} }
"!"                     { if(USE_LEX_ONLY) {printf("NOT ");} else {return yy::parser::make_NOT(yytext);} }

"("                     { if(USE_LEX_ONLY) {printf("LPAREN ");} else {return yy::parser::make_LPAREN(yytext);} }
")"                     { if(USE_LEX_ONLY) {printf("RPAREN ");} else {return yy::parser::make_RPAREN(yytext);} }
"{"                     { if(USE_LEX_ONLY) {printf("LBRACE ");} else {return yy::parser::make_LBRACE(yytext);} }
"}"                     { if(USE_LEX_ONLY) {printf("RBRACE ");} else {return yy::parser::make_RBRACE(yytext);} }
"["                     { if(USE_LEX_ONLY) {printf("LBRACKET ");} else {return yy::parser::make_LBRACKET(yytext);} }
"]"                     { if(USE_LEX_ONLY) {printf("RBRACKET ");} else {return yy::parser::make_RBRACKET(yytext);} }
";"                     { if(USE_LEX_ONLY) {printf("SEMICOLON ");} else {return yy::parser::make_SEMICOLON(yytext);} }
","                     { if(USE_LEX_ONLY) {printf("COMMA ");} else {return yy::parser::make_COMMA(yytext);} }
">"                     { if(USE_LEX_ONLY) {printf("GT ");} else {return yy::parser::make_GT(yytext);} }
"<"                     { if(USE_LEX_ONLY) {printf("LT ");} else {return yy::parser::make_LT(yytext);} }
"."                     { if(USE_LEX_ONLY) {printf("DOT ");} else {return yy::parser::make_DOT(yytext);} }


[a-zA-Z_][a-zA-Z0-9_]* { if(USE_LEX_ONLY) {printf("IDENTIFIER ");} else {return yy::parser::make_IDENTIFIER(yytext);} }
0|[1-9][0-9]*           {if(USE_LEX_ONLY) {printf("INT ");} else {return yy::parser::make_INT(yytext);}}

    /* Whitespace and comments */
[ \t\n\r]+              {}
"//"[^\n]*              {}
.                       { if(!lexical_errors) fprintf(stderr, "Lexical errors found! See the logs below: \n"); fprintf(stderr,"\t@error at line %d. Character %s is not recognized\n", yylineno, yytext); lexical_errors = 1;}
<<EOF>>                  {return yy::parser::make_END();}
%%