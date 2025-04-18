#include <iostream>
#include <fstream>
#include "parser.tab.hh"
#include "symbolT.cc"
#include "Node.h"
#include "IR.cc"  // Contains both low-level TAC generators and high-level code generator functions

extern Node* root;
extern FILE* yyin;
extern int yylineno;
extern int lexical_errors;
extern yy::parser::symbol_type yylex();

enum errCodes {
    SUCCESS = 0,
    LEXICAL_ERROR = 1,
    SYNTAX_ERROR = 2,
    AST_ERROR = 3,
    SEMANTIC_ERROR = 4,
    SEGMENTATION_FAULT = 139
};

int errCode = errCodes::SUCCESS;

// This error method is invoked by the parser when a syntax error occurs.
void yy::parser::error(std::string const &err) {
    if (!lexical_errors) {
        std::cerr << "Syntax errors found! See the logs below:" << std::endl;
        std::cerr << "\t@error at line " << yylineno
                  << ". Cannot generate a syntax for this input: "
                  << err.c_str() << std::endl;
        std::cerr << "End of syntax errors!" << std::endl;
        errCode = errCodes::SYNTAX_ERROR;
    }
}

int main(int argc, char **argv) {
    // Open input file if provided.
    if (argc > 1) {
        if (!(yyin = fopen(argv[1], "r"))) {
            perror(argv[1]);
            return 1;
        }
    }
    
    yy::parser parser;
    SymbolTable *symbolTable = new SymbolTable(); // Create and initialize symbol table

    bool parseSuccess = !parser.parse();
    
    if (lexical_errors)
        errCode = errCodes::LEXICAL_ERROR;
    
    if (parseSuccess && !lexical_errors) {
        std::cout << "\nThe compiler successfully generated a syntax tree!\n";
        try {
            // Generate the AST and output it as a DOT file (tree.dot).
            root->generate_tree();
            
            // Build the symbol table from the AST.
            std::cout << "\nBuilding the symbol table...\n";
            traverseTree(root, *symbolTable);
            
            // Perform semantic analysis.
            std::cout << "\nPerforming semantic analysis...\n";
            performSemanticAnalysis(root, *symbolTable);
            
            std::cout << "\nSymbol Table:\n";
            printSymbolTable(*symbolTable);
            
            if (symbolTable->hasErrors()) {
                std::cout << "\nSemantic Errors:\n";
                symbolTable->printErrors();
            } else {
                std::cout << "\nNo semantic errors found.\n";
                
                // --- IR Generation Phase ---
                std::cout << "\nGenerating Intermediate Representation (IR)...\n";
                IR ir;
                ir.start(root);            // Build TAC from AST
                ir.printCFG("ir.dot");     // Write the CFG to ir.dot
                ir.generateBytecode("output.class");
            }
        }
        catch (...) {
            errCode = errCodes::AST_ERROR;
        }
    }

    return errCode;
}
