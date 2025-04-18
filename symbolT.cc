#include <sstream>
#include "Node.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <stack>


string whatClassWeAreInRn = "";

// SymbolKinds to define the type of symbol (variable, method, class, parameter)
enum class SymbolKind {
    Variable, // e.g., local or global variable
    Method,   // e.g., a function
    Class,   // e.g., a class declaration
    Parameter
};

// Class for Symbol which represents each symbol in the symbol table.
// Note: We no longer store the symbol's scope inside Symbol.
class Symbol {
public:
    std::string name;               // Name of the symbol
    SymbolKind kind;                // Kind of symbol (Variable, Method, Class, Parameter)
    std::string type;               // Data type (e.g., "int", "boolean")
    std::vector<Symbol> parameters; // If it's a method, its parameter list
    int lineOfDeclaration;

    // Default constructor
    Symbol() : kind(SymbolKind::Variable), name(""), type("") {}

    // User-defined constructor
    Symbol(std::string name, SymbolKind kind, std::string type, std::vector<Symbol> parameters, int lineOfDeclaration)
    : name(name), kind(kind), type(type), parameters(parameters), lineOfDeclaration(lineOfDeclaration)  {}

};

// Class for Scope representing a scope (a context where symbols are defined)
class Scope {
public:
    std::unordered_map<std::string, Symbol> symbols;  // Map from identifier name to Symbol
    std::string scopeName;  // Name of the scope (e.g., "global", a class name, or a method name)
    Scope* parent;          // Pointer to the parent scope

    // Constructor for Scope
    Scope(std::string name) : scopeName(name), parent(nullptr) {}
    bool addSymbol(const std::string& name, SymbolKind kind, const std::string& type, const std::vector<Symbol>& parameters, int lineOfDeclaration);
};

// Class for SymbolTable which handles all scopes and symbols.
// It stores scopes in a vector and uses a stack to track the current scope.
class SymbolTable {
public:
    std::vector<Scope> scopes;
    std::stack<size_t> currentScopeStack;
    std::vector<std::pair<std::string, int>> errors;  // Error messages and line numbers

public:
    // Constructor: initialize with a global scope.
    SymbolTable();

    // Enter a new scope with the given name.
    void enterScope(std::string scopeName);

    // Exit the current scope.
    void exitScope();

    void printAllScopes();

    // Add a symbol to the current scope.
    bool addSymbolST(std::string name, SymbolKind kind, std::string type, std::vector<Symbol> parameters, int lineOfDeclaration);


    // Find a symbol in all scopes (from innermost to outermost).
    Symbol* findSymbol(std::string name);

    void printCurrentScopeStack();



    // Check if a symbol exists in a specific scope (e.g., in the given class or method scope).
    bool checkSymbolInScope(const std::string& symbolName);

    // Print the symbol table.
    void printTable() const;

    // Get the name of the current scope.
    std::string getCurrentScope() const;

    // Add an error message.
    void addError(const std::string& message, int line);

    // Check if there are errors.
    bool hasErrors() const;

    // Return the errors.
    const std::vector<std::pair<std::string, int>>& getErrors() const;

    void printErrors() const;
};

//
// Other function declarations for semantic analysis:
//
void performSemanticAnalysis(Node* node, SymbolTable& symbolTable);
void printSymbolTable(const SymbolTable& symbolTable);
void printNode(const Node* node);
void traverseTree(Node* node, SymbolTable& symbolTable);


std::string evaluateExpressionType(Node* node, SymbolTable& symbolTable);



void SymbolTable::addError(const std::string& message, int line) {
    errors.push_back({message, line});
}
bool SymbolTable::hasErrors() const {
    return !errors.empty();
}

const std::vector<std::pair<std::string, int>>& SymbolTable::getErrors() const {
    return errors;
}

#include <algorithm> // for std::sort

void SymbolTable::printErrors() const {
    if (errors.empty()) {
        std::cout << "No semantic errors found." << std::endl;
    } else {
        // Create a copy of the errors vector and sort it by line number (the second element of each pair)
        std::vector<std::pair<std::string, int>> sortedErrors = errors;
        std::sort(sortedErrors.begin(), sortedErrors.end(),
                  [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                      return a.second < b.second;
                  });
        
        std::cout << "Semantic errors:" << std::endl;
        for (const auto& err : sortedErrors) {
            std::cout << "Line " << err.second << ": " << err.first << std::endl;
        }
    }
}


// Searches the symbol table's scopes for the class scope with the given className,
// and then looks for a method named methodName in that scope.
Symbol* lookupMethodInClassScope(const std::string& className, const std::string& methodName, const SymbolTable& symbolTable) {
    // Iterate over all scopes in the symbol table.
    // We assume that the class scope's name is the same as the class name,
    // and that its parent is the global scope.
    for (const Scope& scope : symbolTable.scopes) {
        if (scope.scopeName == className && scope.parent != nullptr && scope.parent->scopeName == "global") {
            // Found the class scope; now look for the method.
            auto it = scope.symbols.find(methodName);
            if (it != scope.symbols.end() && it->second.kind == SymbolKind::Method) {
                std::cout << "[DEBUG] Found method: '" << methodName 
                          << "' in class scope: '" << scope.scopeName << "'"  << "With className " << className << std::endl;
                return const_cast<Symbol*>(&it->second);
            }
        }
    }
    std::cout << "[DEBUG] Method '" << methodName << "' not found in class scope '" << className << "'" << std::endl;
    return nullptr;
}





std::string evaluateExpressionType(Node* node, SymbolTable& symbolTable) {
    if (!node) return "unknown";

    // Handle literals
    if (node->type == "intLiteral") {
        return "IntType";
    }
    if (node->type == "IntType") {
        return "IntType";
    }
    if (node->type == "true" || node->type == "false") {
        return "boolean";
    }

    // Handle "This" expression:
    // Instead of returning the current scope (which might be a method), we return the parent's scope name,
    // which is the enclosing class.
    if (node->type == "This") {
        // If the current scope is a method, return its parent's scope name (the enclosing class)
        if (!symbolTable.currentScopeStack.empty()) {
            size_t currentIndex = symbolTable.currentScopeStack.top();
            Scope& currScope = symbolTable.scopes[currentIndex];
            if (currScope.parent != nullptr) {
                std::cout << "[DEBUG] 'This' evaluated to parent scope: " << currScope.parent->scopeName << std::endl;
                return currScope.parent->scopeName;
            }
        }
        // If no current scope, fall back.
        return "unknown";
    }

    // If it's an identifier, look it up in the symbol table.
    if (node->type == "Identifier") {
        cout << "Node type Here: " << node->type << endl;
        cout << "Node Value Here: " << node->value << endl;
        cout << "Node Lineno Here: " << node->lineno << endl;

        Symbol* sym = symbolTable.findSymbol(node->value);

        if (!sym) {
            std::cerr << "@error at line " << node->lineno << ": Undeclared identifier '" << node->value << "'" << std::endl;
            symbolTable.addError("Undeclared identifier", node->lineno);
            return "unknown";
        }
        return sym->type;
    }

    // Handle arithmetic expressions (add, sub, mult)
    if (node->type == "addExpression" || node->type == "subExpression" || node->type == "multExpression") {
        if (node->children.size() < 2) return "unknown";
        cout << " Test  1 " << endl;
        auto it = node->children.begin();
        std::string leftType = evaluateExpressionType(*it, symbolTable);
        ++it;
        std::string rightType = evaluateExpressionType(*it, symbolTable);
        cout << rightType << " Test  2 " << endl;
        cout << leftType << " Test  4LEFT " << endl;

        
        if (leftType == "unknown" && rightType == "unknown")
            return "unknown";
        if (rightType == "IntType" || leftType == "IntType") {
            return "IntType";
        }
        std::cerr << "@error at line " << node->lineno << ": Type mismatch in arithmetic expression" << std::endl;
        symbolTable.addError("Type mismatch in arithmetic expression", node->lineno);
        return "unknown";
    }

    // Handle logical expressions (and, or)
    if (node->type == "andExpression" || node->type == "orExpression") {
        if (node->children.size() < 2) return "unknown";
        auto it = node->children.begin();
        std::string leftType = evaluateExpressionType(*it, symbolTable);
        ++it;
        std::string rightType = evaluateExpressionType(*it, symbolTable);
        if (leftType == "unknown" || rightType == "unknown")
            return "unknown";
        if (leftType == "boolean" && rightType == "boolean") {
            return "boolean";
        }
        std::cerr << "@error at line " << node->lineno << ": Logical operators require boolean operands" << std::endl;
        symbolTable.addError("Logical operator type mismatch", node->lineno);
        return "unknown";
    }

    // Handle relational expressions (lessThan, greaterThan, isEqualExpression)
    if (node->type == "lessThan" || node->type == "greaterThan" || node->type == "isEqualExpression") {
        if (node->children.size() < 2) return "unknown";
        auto it = node->children.begin();
        std::string leftType = evaluateExpressionType(*it, symbolTable);
        ++it;
        std::string rightType = evaluateExpressionType(*it, symbolTable);
        if (leftType == "unknown" || rightType == "unknown")
            return "unknown";
        if (leftType == "IntType" && rightType == "IntType") {
            return "boolean";
        }
        std::cerr << "@error at line " << node->lineno << ": Relational expression type mismatch" << std::endl;
        symbolTable.addError("Relational expression type mismatch", node->lineno);
        return "unknown";
    }

    // Handle not expression
    if (node->type == "notExpression") {
        if (node->children.empty()) return "unknown";
        std::string innerType = evaluateExpressionType(*node->children.begin(), symbolTable);
        if (innerType == "boolean") {
            return "boolean";
        }
        std::cerr << "@error at line " << node->lineno << ": 'not' operator requires boolean operand" << std::endl;
        symbolTable.addError("'not' operator type mismatch", node->lineno);
        return "unknown";
    }

    // Parenthesized expression: just evaluate the inner expression.
    if (node->type == "ParenExpression") {
        if (node->children.empty()) return "unknown";
        return evaluateExpressionType(*node->children.begin(), symbolTable);
    }

    // Handle method calls.
    if (node->type == "methodCall") {
        // Expect at least two children: object and method identifier.
        if (node->children.size() < 2) return "unknown";
        auto it = node->children.begin();
        Node* objectNode = *it;
        std::string objectType = evaluateExpressionType(objectNode, symbolTable);
        cout << "objectType " << objectType << endl;
        ++it;
        Node* methodIdNode = *it;
        
        // Instead of searching in the class symbol's parameters vector,
        // use lookupMethodInClassScope to search the actual class scope.
        Symbol* methodSymbol = lookupMethodInClassScope(objectType, methodIdNode->value, symbolTable);
        if (!methodSymbol) {
            std::cerr << "@error at line " << methodIdNode->lineno << ": Undeclared method '" 
                      << methodIdNode->value << "' in class '" << objectType << "'" << std::endl;
            symbolTable.addError("Undeclared method", methodIdNode->lineno);
            return "unknown";                                  // cout to cerr valid invalid
        }
        cout << "methodSymbol->type; " << methodSymbol->type << endl;
        return methodSymbol->type;
    }
    
    
    // Handle object creation for a class.
    if (node->type == "newID") {
        if (!node->children.empty())
            return (*node->children.begin())->value;  // The class name becomes the type.
        return "unknown";
    }

    // Handle array creation.
    if (node->type == "newInt") {
        return "ArrayType";
    }

    // If the node's type is already one of the known types, return it.
    if (node->type == "IntType" || node->type == "boolean" || node->type == "floatType" ||
        node->type == "charType" || node->type == "ArrayType")
         return node->type;

    return "unknown";
}








bool Scope::addSymbol(const std::string& name, SymbolKind kind, const std::string& type, const std::vector<Symbol>& parameters, int lineOfDeclaration) {

    // Check for duplicates based on name and kind
    for (const auto& entry : symbols) {
        if (entry.second.name == name && entry.second.kind == kind) {
            std::cerr << "Error: Symbol '" << name << "' of kind " << static_cast<int>(kind) 
                      << " already exists in scope '" << scopeName << "'." << std::endl;
            return false; // Duplicate name+kind
        }
    }

    // Add the symbol to the current scope
    symbols[name] = Symbol(name, kind, type, parameters, lineOfDeclaration);
    //std::cout << "Added symbol: " << name << " of kind: " << static_cast<int>(kind) 
              //<< " to scope: " << scopeName << std::endl; // Log symbol addition
    return true;
}

bool SymbolTable::addSymbolST(std::string name, SymbolKind kind, std::string type, std::vector<Symbol> parameters, int lineOfDeclaration) {
    if (!currentScopeStack.empty()) {
        return scopes[currentScopeStack.top()].addSymbol(name, kind, type, parameters, lineOfDeclaration);
    }
    return false;
}


SymbolTable::SymbolTable() {
    enterScope("global");
}

void SymbolTable::enterScope(std::string scopeName) {
    // If there is a current scope, attempt to reuse an existing child scope.
    if (!currentScopeStack.empty()) {
        size_t parentIndex = currentScopeStack.top();
        Scope& parentScope = scopes[parentIndex];
        
        // Iterate through all scopes to find one with matching parent and scopeName.
        for (size_t i = 0; i < scopes.size(); ++i) {
            if (scopes[i].scopeName == scopeName && scopes[i].parent == &parentScope) {
                std::cout << "Reusing existing scope: " << scopeName
                          << " (Parent: " << parentScope.scopeName << ")" << std::endl;
                currentScopeStack.push(i);
                return;
            }
        }
    }
    
    // Reserve extra capacity to avoid reallocation.
    if (scopes.capacity() - scopes.size() < 1) {
        scopes.reserve(scopes.size() * 2 + 100);
    }
    
    // Create a new scope.
    Scope newScope(scopeName);
    if (!currentScopeStack.empty()) {
        newScope.parent = &scopes[currentScopeStack.top()];
    } else {
        newScope.parent = nullptr;  // Global scope has no parent.
    }
    
    // Add the new scope to the vector.
    scopes.push_back(std::move(newScope));
    // Push its index onto the stack.
    currentScopeStack.push(scopes.size() - 1);
    
    std::cout << "Entering new scope: " << scopeName;
    if (!currentScopeStack.empty() && scopes.back().parent) {
        std::cout << " (Parent: " << scopes.back().parent->scopeName << ")";
    }
    std::cout << std::endl;
}








void SymbolTable::exitScope() {
    if (!currentScopeStack.empty()) {
        //std::cout << "Exiting scope: " << scopes[currentScopeStack.top()].scopeName << std::endl; // Log scope exit
        currentScopeStack.pop();
    }
}

void SymbolTable::printAllScopes() {
    std::cout << "Symbol Table Scopes:" << std::endl;

    for (const auto& scope : scopes) {
        std::cout << "Scope: " << scope.scopeName << std::endl;

        if (scope.parent) {
            std::cout << "  Parent Scope: " << scope.parent->scopeName << std::endl;
        } else {
            std::cout << "  Parent Scope: None (Global Scope)" << std::endl;
        }

        if (scope.symbols.empty()) {
            std::cout << "  No symbols in this scope." << std::endl;
        } else {
            std::cout << "  Symbols:" << std::endl;
            for (const auto& [name, symbol] : scope.symbols) {
                std::cout << "    - Name: " << name << ", TypePrintScopes: " << symbol.type 
                          << ", Kind: " << static_cast<int>(symbol.kind) << std::endl;
            }
        }
        std::cout << "--------------------------------------" << std::endl;
    }
}

// In your SymbolTable class definition (public section):
void SymbolTable::printCurrentScopeStack() {
    // Make a copy so we don't modify the original stack
    std::stack<size_t> tempStack = currentScopeStack;
    
    std::cout << " Scope Indexes Here: ";
    while (!tempStack.empty()) {
        std::cout << tempStack.top() << " ";
        tempStack.pop();
    }
    std::cout << std::endl;
}


Symbol* SymbolTable::findSymbol(std::string name) { //GOOD ONE
    if (currentScopeStack.empty()) {
        std::cout << "[DEBUG] currentScopeStack is empty. No active scopes available." << std::endl;
        return nullptr;  // No active scopes, nothing to search.
    }

    // Start from the current scope and move up
    size_t currentScopeIndex = currentScopeStack.top();
    Scope& currentScope = scopes[currentScopeIndex];
    std::cout << "[DEBUG] Starting search for symbol: '" << name << "' from scope index: " 
              << currentScopeIndex << " (Scope: " << currentScope.scopeName << ") at address: " 
              << &currentScope << std::endl;
              std::cout << " Scope Indexes Here: ";
              printCurrentScopeStack();
              std::cout << endl;

    while (true) {
        Scope& currentScope = scopes[currentScopeIndex];
        std::cout << "[DEBUG] Checking scope: '" << currentScope.scopeName 
                  << "' (Index: " << currentScopeIndex << ") at address: " 
                  << &currentScope << std::endl;

        // Print all symbols in the current scope with details
        std::cout << "[DEBUG] Symbols in scope '" << currentScope.scopeName << "':" << std::endl;
        for (const auto& entry : currentScope.symbols) {
            std::cout << "    Key: " << entry.first 
                      << ", Name: " << entry.second.name 
                      << ", Type: " << entry.second.type 
                      << ", Kind: " << static_cast<int>(entry.second.kind)
                      << std::endl;
        }

        // Check if the symbol exists in the current scope
        auto it = currentScope.symbols.find(name);
        if (it != currentScope.symbols.end()) {
            std::cout << "[DEBUG] Found symbol: '" << name 
                      << "' in scope: '" << currentScope.scopeName << "' (Index: " 
                      << currentScopeIndex << ") at address: " << &currentScope << std::endl;
            return &(it->second);  // Return pointer to the found symbol
        } else {
            std::cout << "[DEBUG] Symbol '" << name << "' not found in scope: '" 
                      << currentScope.scopeName << "'" << std::endl;
        }
        
        // If there's no parent, stop searching
        if (!currentScope.parent) {
            std::cout << "[DEBUG] Scope: '" << currentScope.scopeName 
                      << "' has no parent. Ending search." << std::endl;
            break;
        }
        
        std::cout << "[DEBUG] Moving from scope: '" << currentScope.scopeName 
                  << "' (address: " << &currentScope << ") to its parent scope." << std::endl;
        
        // Move to the parent scope (assuming the parent pointer is within the scopes array)
        size_t parentIndex = currentScope.parent - &scopes[0];  // Convert pointer to index
        std::cout << "[DEBUG] Parent scope index calculated as: " << parentIndex 
                  << " (Parent scope address: " << currentScope.parent << ")" << std::endl;
        currentScopeIndex = parentIndex;
    }
    
    std::cout << "[DEBUG] Symbol: '" << name << "' not found in any accessible scope." << std::endl;
    return nullptr;  // Symbol not found in any accessible scope
}












void SymbolTable::printTable() const {
    // Print global scope and classes first
    std::cout << "Global Scope:" << std::endl;
    for (const auto& scope : scopes) {
        if (scope.scopeName == "global") {
            std::cout << "  Scope Address: " << &scope << std::endl;
            for (const auto& symbol : scope.symbols) {
                std::cout << "    Class - Name: " << symbol.second.name << std::endl;
            }
        }
    }

    // Print details of each class scope
    for (const auto& scope : scopes) {
        if (scope.scopeName != "global") {
            std::cout << "\nScope: " << scope.scopeName 
                      << " at address: " << &scope << std::endl;
            for (const auto& symbol : scope.symbols) {
                std::string symbolType = (symbol.second.kind == SymbolKind::Variable) ? "Variable" : "Method";
                std::cout << "  " << symbolType << " - Name: " << symbol.second.name
                          << ", Type: " << symbol.second.type << std::endl;
            }
        }
    }
}


std::string SymbolTable::getCurrentScope() const {
    if (currentScopeStack.size()>0) {
        return scopes[currentScopeStack.top()].scopeName;
    }
    return "global";
}
void printNode(const Node* node) {


    cout << "Node type: " << node->type << endl;

    cout << "Node value: " << node->value << endl;

    cout << "Node line number: " << node->lineno << endl;
    
    cout << "Node has " << node->children.size() << " children." << endl;

}

void printSymbolTable(const SymbolTable& symbolTable) {
    symbolTable.printTable();
}

void traverseTree(Node* node, SymbolTable& symbolTable) {
    if (!node) return;
    
    //std::cout << "Processing node type: " << node->type << " with value: " << node->value << std::endl;

    if (node->type == "mainClass") {
        auto it = node->children.begin();
        Node* classNode = *it; // first child: class identifier
        std::string className = classNode->value;
        if (!symbolTable.addSymbolST(className, SymbolKind::Class, "ClassType", std::vector<Symbol>(), classNode->lineno)) {
            std::cerr << "@error at line " << classNode->lineno
                      << ". Already Declared Class: '" << className << "'" << std::endl;
            symbolTable.addError("Already Declared Class", classNode->lineno);
                    
        }
        symbolTable.enterScope(className);
        for (++it; it != node->children.end(); ++it)
            traverseTree(*it, symbolTable);
        symbolTable.exitScope();
    } else if (node->type == "assign"){

    } else if (node->type == "classDeclaration") {
        auto it = node->children.begin();
        Node* classNode = *it; // first child: class identifier
        std::string className = classNode->value;
        if (!symbolTable.addSymbolST(className, SymbolKind::Class, "ClassType", std::vector<Symbol>(), classNode->lineno)) {
            std::cerr << "@error at line " << classNode->lineno
                      << ". Already Declared Class: '" << className << "'" << std::endl;
            symbolTable.addError("Already Declared Class", classNode->lineno);

        }
        symbolTable.enterScope(className);
        for (++it; it != node->children.end(); ++it)
            traverseTree(*it, symbolTable);
        symbolTable.exitScope();
    } else if (node->type == "Method" || node->type == "methodDeclaration") {
        auto it = node->children.begin();
        Node* returnTypeNode = *it; // e.g., "IntType"
        cout << returnTypeNode->type << " ABABA" << endl;
        ++it;
        Node* methodNameNode = *it; // Method name node.
        std::string methodName = methodNameNode->value;
        std::cout << "[DEBUG TRAVERSE TREE] Found method declaration: '" << methodName 
                  << "' with return type: '" << returnTypeNode->type << "'" << std::endl;
        
        // Step 1: Add the method symbol to the enclosing class's scope.
        if (!symbolTable.currentScopeStack.empty()) {
            size_t classScopeIndex = symbolTable.currentScopeStack.top(); // Enclosing class scope.
            string bombaCLATTTTTTTTTT = returnTypeNode->type;
            if(returnTypeNode->type == "Identifier"){
                bombaCLATTTTTTTTTT = returnTypeNode->value;
            }
            if (!symbolTable.scopes[classScopeIndex].addSymbol(methodName, SymbolKind::Method, bombaCLATTTTTTTTTT, std::vector<Symbol>(), methodNameNode->lineno)) {
                std::cerr << "@error at line " << methodNameNode->lineno 
                          << ": Duplicate method declaration: '" << methodName << "'" << std::endl;
                symbolTable.addError("Duplicate method declaration", methodNameNode->lineno);
            }
        }
        
        // Step 2: Enter the method scope.
        symbolTable.enterScope(methodName);
        
        // Step 3: Within the method scope, add the method symbol again to allow recursive calls.
        if (!symbolTable.scopes[symbolTable.currentScopeStack.top()].addSymbol(methodName, SymbolKind::Method, returnTypeNode->type, std::vector<Symbol>(), methodNameNode->lineno)) {
            std::cerr << "@error at line " << methodNameNode->lineno 
                      << ": Duplicate recursive method declaration for '" << methodName << "'" << std::endl;
            symbolTable.addError("Duplicate recursive method declaration", methodNameNode->lineno);
        }
        
        // Step 4: Process the rest of the children (parameters, method body, etc.)
        ++it;
        for (; it != node->children.end(); ++it) {
            traverseTree(*it, symbolTable);
        }
        
        // Step 5: Exit the method scope.
        symbolTable.exitScope();
    }
     else if (node->type == "varDeclaration") {
        auto it = node->children.begin();
        Node* varTypeNode = *it;
        ++it;
        Node* varNameNode = *it;
        std::string varName = varNameNode->value;
        // If the type node's type is "Identifier", use its value as the actual type.
        std::string varType = (varTypeNode->type == "Identifier") ? varTypeNode->value : varTypeNode->type;
        
        if (!symbolTable.addSymbolST(varName, SymbolKind::Variable, varType, std::vector<Symbol>(), varNameNode->lineno)){
            std::cerr << "@error at line " << varNameNode->lineno
                      << ". Already Declared variable: '" << varName << "'" << std::endl;
            symbolTable.addError("Already declared variable", varNameNode->lineno);
        }
        
        for (++it; it != node->children.end(); ++it)
            traverseTree(*it, symbolTable);
    
    } else if (node->type == "Parameter") {
        // Revised Parameter branch:
        // If the node has exactly two children, treat it as one parameter.
        // If more than two children, assume it contains a flat list of (type, identifier) pairs.
        if (node->children.size() == 2) {
            auto it = node->children.begin();
            Node* paramTypeNode = *it;
            string itsIdentifierChangeItPls = "";
            if (paramTypeNode->type == "Identifier")
            {
                itsIdentifierChangeItPls = paramTypeNode->value;
            }
            else
            {
                itsIdentifierChangeItPls = paramTypeNode->type;
            }
            
            cout << "itsIdentifierChangeItPlsaaa " << itsIdentifierChangeItPls << endl;
            
            ++it;
            Node* paramNameNode = *it;
            std::string paramName = paramNameNode->value;
            cout << "itsIdentifierChangeItPls Parameter traverse tree i cant hold it in anymore : " << itsIdentifierChangeItPls << endl;
            if (!symbolTable.addSymbolST(paramName, SymbolKind::Variable, itsIdentifierChangeItPls, std::vector<Symbol>(), paramNameNode->lineno)){
                std::cerr << "@error at line " << paramNameNode->lineno
                          << ". Already Declared parameter: '" << paramName << "'" << std::endl;
                symbolTable.addError("Already declared parameter", paramNameNode->lineno);

            }
        } else {
            // Process children in pairs.
            auto it = node->children.begin();
            while (it != node->children.end()) {
                Node* paramTypeNode = *it;
                string itsIdentifierChangeItPls = "";
                if (paramTypeNode->type == "Identifier")
                {
                    itsIdentifierChangeItPls = paramTypeNode->value;
                }
                else
                {
                    itsIdentifierChangeItPls = paramTypeNode->type;
                }
                
                cout << "itsIdentifierChangeItPlsbbbb " << itsIdentifierChangeItPls << endl;
                ++it;
                if (it == node->children.end()) break;
                Node* paramNameNode = *it;
                ++it;
                std::string paramName = paramNameNode->value;
                if (!symbolTable.addSymbolST(paramName, SymbolKind::Variable, itsIdentifierChangeItPls, std::vector<Symbol>(), paramNameNode->lineno)){
                    std::cerr << "@error at line " << paramNameNode->lineno
                              << ". Already Declared parameter: '" << paramName << "'" << std::endl;
                    symbolTable.addError("Already declared parameter", paramNameNode->lineno);
                }
            }
        }
    } else if (node->type == "ParameterList") {
        // If children are Parameter nodes, process each.
        if (!node->children.empty() && (*node->children.begin())->type == "Parameter") {
            for (Node* param : node->children)
                traverseTree(param, symbolTable);
        } else {
            // Otherwise, process as flat pairs.
            auto it = node->children.begin();
            while (it != node->children.end()) {
                Node* typeNode = *it;
                ++it;
                if (it == node->children.end()) break;
                Node* idNode = *it;
                ++it;
                std::string paramName = idNode->value;
                Node* paramTypeNode = *it;
                ++it;
                Node* paramNameNode = *it;
                if (!symbolTable.addSymbolST(paramName, SymbolKind::Variable, typeNode->type, std::vector<Symbol>(), paramNameNode->lineno)){
                    std::cerr << "@error at line " << idNode->lineno
                              << ". Already Declared parameter: '" << paramName << "'" << std::endl;
                    symbolTable.addError("Already declared parameter", idNode->lineno);
                }
            }
        }
    } else {
        // Default: traverse all children.
        for (Node* child : node->children)
            traverseTree(child, symbolTable);
    }
}

//
// processParameterList: Processes a ParameterList node by iterating through each Parameter child.
// It extracts the parameter type and identifier, and then adds the parameter into the current method scope.
// Duplicate checks are performed during symbol table construction, so we only log an error if the insertion fails.
//
void processParameterList(Node* paramListNode, SymbolTable& symbolTable) {
    for (Node* paramNode : paramListNode->children) {
        if (paramNode->type == "Parameter") {
            auto it = paramNode->children.begin();
            if (it != paramNode->children.end()) {
                Node* typeNode = *it;  // e.g., "IntType"
                ++it;
                if (it != paramNode->children.end()) {

                    string itsIdentifierChangeItPls = "";
                    if (typeNode->type == "Identifier")
                    {
                        itsIdentifierChangeItPls = typeNode->value;
                    }
                    else
                    {
                        itsIdentifierChangeItPls = typeNode->type;
                    }
                    cout << "itsIdentifierChangeItPls processParameterList " << itsIdentifierChangeItPls << endl;

                    Node* identifierNode = *it; // e.g., "num"
                    std::string paramName = identifierNode->value;
                    std::string paramType = itsIdentifierChangeItPls; // e.g. "IntType"
                    
                    // Insert the parameter into the current scope.
                    // Duplicate checks are performed during symbol table construction,
                    // so we don't perform duplicate checking in semantic analysis.
                    bool success = symbolTable.addSymbolST(
                        paramName,
                        SymbolKind::Parameter,
                        paramType,
                        std::vector<Symbol>(),
                        identifierNode->lineno
                    );
                    
                    if (!success) {
                        std::cerr << "@error at line " << identifierNode->lineno 
                                  << ": Already declared parameter '" << paramName << "'" 
                                  << std::endl;
                        symbolTable.addError("Already declared parameter", identifierNode->lineno);
                    } else {
                        std::cout << "[DEBUG] Added parameter '" << paramName 
                                  << "' of type '" << paramType 
                                  << "' to method scope." << std::endl;
                    }
                }
            }
        } else {
            // If the AST uses a different structure for parameters, handle it here.
            performSemanticAnalysis(paramNode, symbolTable);
        }
    }
}

//
// performSemanticAnalysis: Recursively traverses the AST to perform semantic analysis.
// Duplicate identifier checks are not performed here, because they are handled during the symbol table construction.
// This function primarily checks that each identifier used is declared in an accessible scope.
//


void performSemanticAnalysis(Node* node, SymbolTable& symbolTable) {

    if (!node) return;

    std::cout << "[DEBUG] Entering node: Type='" << node->type
              << "', Value='" << node->value
              << "', Line=" << node->lineno << std::endl;

    // New class scope: process classDeclaration or mainClass.
    if (node->type == "classDeclaration" || node->type == "mainClass") {
        auto it = node->children.begin();
        Node* classIdentifierNode = *it;
        std::string className = classIdentifierNode->value;
        whatClassWeAreInRn = className;
        
        // Enter the class scope.
        symbolTable.enterScope(className);

        for (++it; it != node->children.end(); ++it) {
            performSemanticAnalysis(*it, symbolTable);
        }

        // Exit the class scope.
        symbolTable.exitScope();
    }
    // New method scope: process methodDeclaration.
    else if (node->type == "methodDeclaration" || node->type == "Method") {
        auto it = node->children.begin();
        Node* returnTypeNode = *it; // e.g., "IntType"
        ++it;
        Node* methodIdentifierNode = *it; // Method name.
        std::string methodName = methodIdentifierNode->value;
        std::cout << "[DEBUG SEMANTIC] Found method declaration: '" << methodName 
                  << "' with return type: '" << returnTypeNode->type << "'" << std::endl;

        // Enter the method scope.
        symbolTable.enterScope(methodName);

        ++it;
        for (; it != node->children.end(); ++it) {
            Node* child = *it;
            if (child->type == "ParameterList") {
                // Process parameters.
                processParameterList(child, symbolTable);
            } else {
                // Recurse for method body, statements, etc.
                performSemanticAnalysis(child, symbolTable);
            }
        }

        // Exit the method scope.
        symbolTable.exitScope();
    }
    // Assignment: Check that the identifier on the left-hand side is declared.
    else if (node->type == "assign") {
        // 1. Process the Left-Hand Side (LHS)
        auto it = node->children.begin();
        Node* lhsIdentifierNode = *it; // First child: LHS identifier
        Symbol* lhsSymbol = symbolTable.findSymbol(lhsIdentifierNode->value);
        if (!lhsSymbol) {
            std::cerr << "@error at line " << lhsIdentifierNode->lineno
                      << ": Undeclared symbol '" << lhsIdentifierNode->value << "'"
                      << std::endl;
            symbolTable.addError("Undeclared symbol", lhsIdentifierNode->lineno);
        } else {
            std::cout << "[DEBUG] LHS symbol '" << lhsSymbol->name << "' lineNr symbol declared: " <<  lhsSymbol->lineOfDeclaration
                      << "but Assign declared node at: " << node->lineno << "' with type '" << lhsSymbol->type << "' found." << std::endl;
            if(lhsSymbol->lineOfDeclaration > node->lineno){
                cout << "Variable used before declaration " << endl;
                std::cerr << "@error at line " << lhsIdentifierNode->lineno 
                      << ": Variable '" << lhsIdentifierNode->value 
                      << "' is used before its declaration (declared at line " 
                      << lhsSymbol->lineOfDeclaration << ")." << std::endl;
                symbolTable.addError("Variable used before declaration", lhsIdentifierNode->lineno);
            }
        }
    
        // 2. Process the Right-Hand Side (RHS)
        ++it;
        if (it != node->children.end()) {
            Node* rhsExpressionNode = *it;
    
            // Use evaluateExpressionType to get the final type of the RHS
            cout << "rhsExpressionNode->type here: " << rhsExpressionNode->type << endl;
            std::string rhsType = evaluateExpressionType(rhsExpressionNode, symbolTable);
            // If the RHS node itself is an identifier, ensure we use the type from the symbol table.
            std::cout << "[DEBUG] RHS expression evaluated to type if Identifier we fix: " << rhsType << std::endl;
            if (rhsExpressionNode->type == "Identifier") {
                //symbolTable.enterScope(); // where do we enter scope
                Symbol* rhsSym = symbolTable.findSymbol(rhsExpressionNode->value);
                // exit scope after?
                cout << "rhsSym " << rhsSym << endl;
                if (rhsSym) {
                    rhsType = rhsSym->type;
                    cout << "RECASTED!!" << endl;
                }
            }
            std::cout << "[DEBUG] RHS expression evaluated to type if Identifier we FIXED: " << rhsType << std::endl;

    
            // 3. If LHS is declared, check for type mismatch
            if (lhsSymbol) {
                // Only report a mismatch if we have a valid RHS type (not "unknown")
                if (rhsType != "unknown" && lhsSymbol->type != rhsType) {
                    std::cerr << "@error at line " << node->lineno
                              << ": Type mismatch; variable '" << lhsIdentifierNode->value
                              << "' has type '" << lhsSymbol->type
                              << "', but RHS expression is of type '" << rhsType << "'"
                              << std::endl;
                    symbolTable.addError("Type mismatch in assignment", node->lineno);
                }
            }
        }
    
        // 4. Recurse on children for deeper analysis
        for (auto childIt = node->children.begin(); childIt != node->children.end(); ++childIt) {
            performSemanticAnalysis(*childIt, symbolTable);
        }
    }
    else if (node->type == "varDeclaration") {
        auto it = node->children.begin();
        Node* varTypeNode = *it;
        ++it;
        Node* varNameNode = *it;
        std::string varName = varNameNode->value;
        
        // Determine the declared type.
        std::string varType;
        if (varTypeNode->type == "Identifier") {
            // When the type is an identifier, use its value as the type name.
            varType = varTypeNode->value;
            cout << "varTypeNode->value HEREEE " << varTypeNode->value << endl;
            // Look up the type (class) in the symbol table.
            Symbol* classSymbol = symbolTable.findSymbol(varType);
            // If not found or not of kind Class, report an error.
            if (!classSymbol) {
                std::cerr << "@error at line " << varTypeNode->lineno
                          << ": Undefined class '" << varType << "'" << std::endl;
                symbolTable.addError("Undefined class again", varTypeNode->lineno);
            }
        } else {
            // Otherwise, use the node's type.
            varType = varTypeNode->type;
        }
        
        // Process any additional children.
        for (++it; it != node->children.end(); ++it)
            traverseTree(*it, symbolTable);
    }
    
    
    else {
        // Default: Recursively analyze all children.
        for (Node* child : node->children) {
            performSemanticAnalysis(child, symbolTable);
        }
    }

    std::cout << "[DEBUG] Exiting node: Type='" << node->type
              << "', Value='" << node->value << "'" << std::endl;
}




bool SymbolTable::checkSymbolInScope(const std::string& symbolName) {
    if (currentScopeStack.empty()) {
        return false;  // No active scopes, so symbol can't exist.
    }

    // Start searching from the current scope and move upwards
    size_t currentScopeIndex = currentScopeStack.top();
    while (true) {
        Scope& currentScope = scopes[currentScopeIndex];

        // Check if the symbol exists in this scope
        if (currentScope.symbols.find(symbolName) != currentScope.symbols.end()) {
            return true;  // Symbol found
        }

        // If there's no parent, stop searching
        if (!currentScope.parent) {
            break;
        }

        // Move to the parent scope
        currentScopeIndex = currentScope.parent - &scopes[0];  // Convert pointer to index
    }

    return false;  // Symbol not found in any accessible scope
}
