%skeleton "lalr1.cc"
%defines
%define parse.error verbose
%define api.value.type variant
%define api.token.constructor

%code requires{
  #include <string>
  #include "Node.h"
  #define USE_LEX_ONLY false
}

%code{
  #define YY_DECL yy::parser::symbol_type yylex()
  YY_DECL;
  
  Node* root;
  extern int yylineno;
}

%token <std::string> TYPE_INT TYPE_FLOAT TYPE_CHAR TYPE_BOOL TYPE_STRING TYPE_VOID
%token <std::string> PUBLIC IF ELSE WHILE FOR RETURN TRUE FALSE THIS NEW MAIN STATIC CLASS PRINT_METHOD
%token <std::string> IS_EQUAL ASSIGNOP PLUSOP MINUSOP MULTOP AND OR NOT
%token <std::string> LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET SEMICOLON COMMA GT LT DOT LENGTH
%token <std::string> IDENTIFIER INT

%token END 0 "end of file"

%left PLUSOP MINUSOP
%left MULTOP
%left IS_EQUAL LT GT
%left AND OR
%right NOT

%type <Node *> root goal Parameter mainClass classDeclaration identifier statement statements elseHandler classDeclarations varDeclarations methodDeclaration methodDeclarations parameters chooseParam ParameterList varDeclaration type expression varOrStatements argument_list non_empty_argument_list

%%

root: goal {root = $1;};

goal: mainClass classDeclarations END {$$ = new Node("goal", "", yylineno); $$->children.push_back($1); $$->children.push_back($2);};

mainClass: PUBLIC CLASS identifier LBRACE PUBLIC STATIC TYPE_VOID MAIN LPAREN TYPE_STRING LBRACKET RBRACKET identifier RPAREN LBRACE statement statements RBRACE RBRACE {$$ = new Node("mainClass", "", yylineno); $$->children.push_back($3); $$->children.push_back($13); $$->children.push_back($16); $$->children.push_back($17);};


statement: LBRACE statements RBRACE {$$ = new Node("block", "", yylineno); $$->children.push_back($2);}
         | IF LPAREN expression RPAREN statement elseHandler {$$ = new Node("if", "", yylineno); $$->children.push_back($3); $$->children.push_back($5); $$->children.push_back($6);}
         | WHILE LPAREN expression RPAREN statement {$$ = new Node("while", "", yylineno); $$->children.push_back($3); $$->children.push_back($5);}
         | PRINT_METHOD LPAREN expression RPAREN SEMICOLON {$$ = new Node("printMethod", "", yylineno); $$->children.push_back($3);}
         | identifier ASSIGNOP expression SEMICOLON {$$ = new Node("assign", "", yylineno); $$->children.push_back($1); $$->children.push_back($3);}
         | identifier LBRACKET expression RBRACKET ASSIGNOP expression SEMICOLON {$$ = new Node("array", "", yylineno); $$->children.push_back($1); $$->children.push_back($3); $$->children.push_back($6);}
         ;

statements: statement {$$ = new Node("statements", "", yylineno); $$->children.push_back($1);}
          | statements statement {$$ = $1; $$->children.push_back($2);}
          | %empty {$$ = new Node("emptyStatements", "", yylineno);}
          ;

elseHandler: ELSE statement {$$ = new Node("elseBranch", "", yylineno);$$->children.push_back($2);}
           | %empty {$$ = new Node("noElse", "", yylineno);}
           ;

expression: expression AND expression { $$ = new Node("andExpression", "", yylineno); $$->children.push_back($1); $$->children.push_back($3); }
          | expression OR expression { $$ = new Node("orExpression", "", yylineno); $$->children.push_back($1); $$->children.push_back($3); }
          | expression LT expression { $$ = new Node("lessThan", "", yylineno); $$->children.push_back($1); $$->children.push_back($3); }
          | expression GT expression { $$ = new Node("greaterThan", "", yylineno); $$->children.push_back($1); $$->children.push_back($3); }
          | expression IS_EQUAL expression { $$ = new Node("isEqualExpression", "", yylineno); $$->children.push_back($1); $$->children.push_back($3); }
          | expression PLUSOP expression { $$ = new Node("addExpression", "", yylineno); $$->children.push_back($1); $$->children.push_back($3); }
          | expression MINUSOP expression { $$ = new Node("subExpression", "", yylineno); $$->children.push_back($1); $$->children.push_back($3); }
          | expression MULTOP expression { $$ = new Node("multExpression", "", yylineno); $$->children.push_back($1); $$->children.push_back($3); }
          | expression LBRACKET expression RBRACKET { $$ = new Node("AllocateIdentifier", "", yylineno); $$->children.push_back($1); $$->children.push_back($3); }
          | expression DOT LENGTH { $$ = new Node("lengthMethod", "", yylineno); $$->children.push_back($1); }
          | expression DOT identifier LPAREN argument_list RPAREN { $$ = new Node("methodCall", "", yylineno); $$->children.push_back($1); $$->children.push_back($3); $$->children.push_back($5);}
          | INT { $$ = new Node("intLiteral", $1, yylineno); } // change $1 back to nothing
          | TRUE { $$ = new Node("true", "1", yylineno); }
          | FALSE { $$ = new Node("false", "0", yylineno); }
          | identifier { $$ = $1; }
          | THIS { $$ = new Node("This", "", yylineno); }
          | NEW TYPE_INT LBRACKET expression RBRACKET { $$ = new Node("newInt", "", yylineno); $$->children.push_back($4); }
          | NEW identifier LPAREN RPAREN { $$ = new Node("newID", "", yylineno); $$->children.push_back($2); }
          | NOT expression { $$ = new Node("notExpression", "", yylineno); $$->children.push_back($2); }
          | LPAREN expression RPAREN {$$ = new Node("ParenExpression", "", yylineno); $$ = $2;}
          ;

argument_list: %empty { $$ = new Node("noArguments", "", yylineno); }
             | non_empty_argument_list { $$ = $1; }
             ;

non_empty_argument_list: expression {$$ = new Node("argument", "", yylineno); $$->children.push_back($1); }
                       | non_empty_argument_list COMMA expression {$$ = new Node("argumentList", "", yylineno); $$->children.push_back($1); $$->children.push_back($3); }
                       ;

classDeclaration: CLASS identifier LBRACE varDeclarations methodDeclarations RBRACE {$$ = new Node("classDeclaration", "", yylineno); $$->children.push_back($2); $$->children.push_back($4); $$->children.push_back($5);};

classDeclarations: classDeclaration {$$ = new Node("classDeclarations", "", yylineno); $$->children.push_back($1);}
                 | classDeclarations classDeclaration {$$ = new Node("classDeclarations", "", yylineno); $$->children.push_back($1); $$->children.push_back($2);}
                 | %empty {$$ = new Node("emptyClassDeclarations", "", yylineno);}
                 ;

varOrStatements: varDeclaration varOrStatements {$$ = $2; $$->children.push_back($1);}
               | statement varOrStatements {$$ = $2; $$->children.push_back($1);}
               | %empty {$$ = new Node("emptyVarOrStatement", "", yylineno);}
               ;

parameters: %empty {$$ = new Node("noParameters", "", yylineno);}
          | ParameterList { $$ = $1; }
          ;

Parameter:
      type identifier 
          {
             /* Create a new Parameter node. 
                Adjust the constructor arguments as needed (e.g., using the first token’s line number). */
             $$ = new Node("Parameter", "", yylineno);
             $$->children.push_back($1);
             $$->children.push_back($2);
          }
;

ParameterList:
      Parameter 
          {
             $$ = new Node("ParameterList", "", yylineno);
             $$->children.push_back($1);
          }
    | ParameterList COMMA Parameter 
          {
             /* Reuse the existing ParameterList node and add the new Parameter */
             $$ = $1;
             $$->children.push_back($3);
          }
;

chooseParam: parameters { $$ = $1; }
           ;

methodDeclaration: PUBLIC type identifier LPAREN chooseParam RPAREN LBRACE varOrStatements RETURN expression SEMICOLON RBRACE {$$ = new Node("methodDeclaration", "", yylineno); $$->children.push_back($2); $$->children.push_back($3); $$->children.push_back($5); $$->children.push_back($8); $$->children.push_back($10);};

methodDeclarations: methodDeclarations methodDeclaration {$$ = new Node("methodDeclarations", "", yylineno); $$->children.push_back($1); $$->children.push_back($2);}
                  | %empty {$$ = new Node("emptyMethodDeclarations", "", yylineno);}
                  ;

varDeclaration: type identifier SEMICOLON {$$ = new Node("varDeclaration", "", yylineno); $$->children.push_back($1); $$->children.push_back($2);}

varDeclarations: varDeclaration {$$ = new Node("varDecleration", "", yylineno); $$->children.push_back($1); }
               | varDeclarations varDeclaration {$$ = $1; $$->children.push_back($2);}
               | %empty {$$ = new Node("emptyVarDeclarations", "", yylineno);}  
               ;  

type: TYPE_INT LBRACKET RBRACKET { $$ = new Node("ArrayType", "", yylineno); }
  | TYPE_BOOL { $$ = new Node("boolean", "", yylineno); }
  | TYPE_INT { $$ = new Node("IntType", "", yylineno); }
  | TYPE_FLOAT { $$ = new Node("floatType", "", yylineno); }
  | TYPE_CHAR { $$ = new Node("charType", "", yylineno); }
  | identifier { $$ = $1; }
  ; 

identifier: IDENTIFIER {$$ = new Node("Identifier", $1, yylineno);};