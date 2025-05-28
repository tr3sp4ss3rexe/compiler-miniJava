#include "Node.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <regex>
#include <queue>
#include <iomanip>

std::string cleanType(const std::string &s) {
    if (!s.empty() && s.back() == ':')
        return s.substr(0, s.size() - 1);
    return s;
}

Node* getChild(Node* node, int index) {
    if (!node) return nullptr;
    if (index < 0 || static_cast<size_t>(index) >= node->children.size()) {
        std::cerr << "ERROR: Attempting to access invalid child index " << index
                  << " for node type '" << node->type << "' (has "
                  << node->children.size() << " children)." << std::endl;
        return nullptr;
    }
    auto it = node->children.begin();
    std::advance(it, index);
    return *it;
}

std::string getNodeValue(Node* node) {
     if (!node) return "";
     if (node->type == "intLiteral" || node->type == "true" || node->type == "false" || node->type == "Identifier") {

          return node->value;
     }
     return cleanType(node->type);
}

class Instruction {
public:
    std::string text;
    Instruction(const std::string &t) : text(t) {}
};

class BasicBlock {
public:
    int id;
    std::vector<Instruction> instructions;
    std::vector<BasicBlock*> successors;

    BasicBlock(int id) : id(id) {}

    void addInstruction(const Instruction &inst) {
        instructions.push_back(inst);
    }

    void addSuccessor(BasicBlock* succ) {
        successors.push_back(succ);
    }
    std::string getCfgLabel() const;
};

class IR {
public:
    std::vector<BasicBlock*> blocks;
    BasicBlock* currentBlock = nullptr;
    int blockCounter = 0;
    int tempCounter = 0;
    bool errorOccurred = false;

    IR() {}
    ~IR() {
        for (auto block : blocks) delete block;
    }

    BasicBlock* getCurrentBlock() {
        if (!currentBlock) {
             if(blocks.empty()) { currentBlock = createBlock(); }
             else { currentBlock = blocks[0]; }
        }
        return currentBlock;
    }

    BasicBlock* createBlock() {
        auto *b = new BasicBlock(blockCounter++);
        blocks.push_back(b);
        return b;
    }

    std::string newTemp() { return "_t" + std::to_string(tempCounter++); }

    void addInstruction(const std::string& instructionText) {
         if (errorOccurred) return;
         BasicBlock* cb = getCurrentBlock();
         if (!cb) { errorOccurred = true; return; }
         cb->addInstruction(Instruction(instructionText));
    }

    std::string genExp(Node* node) {
        if (errorOccurred) return "";
        if (!node) { errorOccurred = true; return ""; }

        std::string type = node->type;
        std::string temp;

        if (type == "intLiteral" || type == "true" || type == "false") {
            temp = newTemp();
            addInstruction(temp + " = " + getNodeValue(node) + "; // literal " + type);
            return temp;
        }
        if (type == "Identifier") {
            return getNodeValue(node);
        }
         if (type == "This") {
             return "this";
         }

        if (type == "notExpression") {
            std::string operand_var = genExp(getChild(node, 0));
             if (errorOccurred || operand_var.empty()) { errorOccurred = true; return ""; }
            temp = newTemp();
            addInstruction(temp + " = !" + operand_var + ";");
            return temp;
        }

        if (type == "addExpression" || type == "subExpression" || type == "multExpression" ||
            type == "lessThan" || type == "greaterThan" ||
            type == "isEqualExpression" ||
            type == "andExpression" || type == "orExpression")
        {
            std::string op_symbol;
            if (type == "addExpression") op_symbol = "+";
            else if (type == "subExpression") op_symbol = "-";
            else if (type == "multExpression") op_symbol = "*";
            else if (type == "lessThan") op_symbol = "<";
            else if (type == "greaterThan") op_symbol = ">";
            else if (type == "isEqualExpression") op_symbol = "==";
            else if (type == "andExpression") op_symbol = "&&";
            else if (type == "orExpression") op_symbol = "||";

            std::string left_var = genExp(getChild(node, 0));
            if (errorOccurred || left_var.empty()) { errorOccurred = true; return ""; }

            std::string right_var = genExp(getChild(node, 1));
             if (errorOccurred || right_var.empty()) { errorOccurred = true; return ""; }

            temp = newTemp();
            addInstruction(temp + " = " + left_var + " " + op_symbol + " " + right_var + ";");
            return temp;
        }

        if (type == "methodCall") {
            Node* objNode = getChild(node, 0);
            Node* methodNameIdentNode = getChild(node, 1);
            Node* argListNode = getChild(node, 2);

            if (!methodNameIdentNode || methodNameIdentNode->type != "Identifier") {
                 std::cerr << "ERROR: Expected Identifier node for method name in methodCall." << std::endl;
                 errorOccurred = true; return "";
            }

            std::string object_var = genExp(objNode);
             if (errorOccurred || object_var.empty()) { errorOccurred = true; return ""; }

            std::string methodName = getNodeValue(methodNameIdentNode);

            std::vector<std::string> argVars;
            std::function<void(Node*)> processArgs =
                [&](Node* argNode) {
                if (!argNode || errorOccurred) return;
                std::string argNodeType = argNode->type;
                if (argNodeType == "argumentList") {
                    processArgs(getChild(argNode, 0));
                    std::string argVar = genExp(getChild(argNode, 1));
                    if (errorOccurred || argVar.empty()) { errorOccurred = true; return; }
                    argVars.push_back(argVar);
                } else if (argNodeType == "argument") {
                    std::string argVar = genExp(getChild(argNode, 0));
                     if (errorOccurred || argVar.empty()) { errorOccurred = true; return; }
                     argVars.push_back(argVar);
                } else if (argNodeType == "non_empty_argument_list") {
                      processArgs(getChild(argNode, 0));
                      if(argNode->children.size() > 1) {
                          std::string argVar = genExp(getChild(argNode, 1));
                          if (errorOccurred || argVar.empty()) { errorOccurred = true; return; }
                          argVars.push_back(argVar);
                      }
                 } else if (argNodeType != "noArguments") {
                    std::cerr << "Warning: Unexpected node type in argument list processing: " << argNodeType << std::endl;
                }
            };

            processArgs(argListNode);
            if (errorOccurred) return "";

            std::ostringstream call_args_ss;
             for (size_t i = 0; i < argVars.size(); ++i) {
                 call_args_ss << argVars[i] << (i + 1 < argVars.size() ? ", " : "");
             }

            temp = newTemp();
            addInstruction(temp + " = call " + object_var + "." + methodName + "(" + call_args_ss.str() + ");");
            return temp;
        }
        if (type == "newID") {
             Node* classNameIdentNode = getChild(node, 0);
             if (!classNameIdentNode || classNameIdentNode->type != "Identifier") {
                  std::cerr << "ERROR: Expected Identifier node for class name in newID." << std::endl;
                  errorOccurred = true; return "";
             }
             std::string className = getNodeValue(classNameIdentNode);
             temp = newTemp();
             addInstruction(temp + " = new " + className + ";");
             return temp;
        }

         if (type == "AllocateIdentifier" || type == "lengthMethod" || type == "newInt") {
             std::cerr << "Warning: IR Generation for type '" << type << "' is not implemented." << std::endl;
             return "";
         }

        std::cerr << "ERROR: Unhandled node type in genExp: '" << type << "'" << std::endl;
        errorOccurred = true;
        return "";
    }

     void genStmt(Node* node) {
         if (errorOccurred) return;
         if (!node) return;

         std::string type = node->type;

         if (type == "assign") {
              Node* lhsIdentNode = getChild(node, 0);
              if (!lhsIdentNode || lhsIdentNode->type != "Identifier") { errorOccurred = true; return;}
              std::string lhs_var = getNodeValue(lhsIdentNode);

              std::string rhs_var = genExp(getChild(node, 1));
              if (errorOccurred || rhs_var.empty()) { errorOccurred = true; return; }
              addInstruction(lhs_var + " = " + rhs_var + "; // assign");
         }
         else if (type == "printMethod") {
             std::string exp_var = genExp(getChild(node, 0));
              if (errorOccurred || exp_var.empty()) { errorOccurred = true; return; }
             addInstruction("print " + exp_var + ";");
         }
         else if (type == "if") {
             std::string cond_var = genExp(getChild(node, 0));
             if (errorOccurred || cond_var.empty()) { errorOccurred = true; return; }

             BasicBlock* currentBlockBeforeIf = getCurrentBlock();
             BasicBlock* thenB = createBlock();
             Node* elseHandlerNode = getChild(node, 2);
             Node* elseStmtNode = nullptr;
             bool hasElse = (elseHandlerNode && elseHandlerNode->type == "elseBranch");
             if (hasElse) {
                 elseStmtNode = getChild(elseHandlerNode, 0);
             }

             BasicBlock* elseB = hasElse ? createBlock() : nullptr;
             BasicBlock* joinB = createBlock();

             currentBlockBeforeIf->addInstruction(Instruction("iffalse " + cond_var + " goto block_" + std::to_string(hasElse ? elseB->id : joinB->id) + ";"));
             currentBlockBeforeIf->addSuccessor(thenB);
             currentBlockBeforeIf->addSuccessor(hasElse ? elseB : joinB);

             currentBlock = thenB;
             genStmt(getChild(node, 1));
             if (!errorOccurred) {
                 if (currentBlock && (currentBlock->instructions.empty() || !std::regex_search(currentBlock->instructions.back().text, std::regex("goto|return")))) {
                    addInstruction("goto block_" + std::to_string(joinB->id) + ";");
                 }
                 if(currentBlock) currentBlock->addSuccessor(joinB);
             }

             if (hasElse && elseStmtNode) {
                 currentBlock = elseB;
                 genStmt(elseStmtNode);
                 if (!errorOccurred) {
                     if (currentBlock && (currentBlock->instructions.empty() || !std::regex_search(currentBlock->instructions.back().text, std::regex("goto|return")))) {
                          addInstruction("goto block_" + std::to_string(joinB->id) + ";");
                     }
                     if(currentBlock) currentBlock->addSuccessor(joinB);
                 }
             }

             currentBlock = joinB;
         }
         else if (type == "while") {
             BasicBlock* condB = createBlock();
             BasicBlock* bodyB = createBlock();
             BasicBlock* exitB = createBlock();
             BasicBlock* currentBlockBeforeWhile = getCurrentBlock();

             if (currentBlockBeforeWhile && (currentBlockBeforeWhile->instructions.empty() || !std::regex_search(currentBlockBeforeWhile->instructions.back().text, std::regex("goto|return")))) {
                 currentBlockBeforeWhile->addInstruction(Instruction("goto block_" + std::to_string(condB->id) + ";"));
             }
              if (currentBlockBeforeWhile) currentBlockBeforeWhile->addSuccessor(condB);

             currentBlock = condB;
             std::string cond_var = genExp(getChild(node, 0));
              if (errorOccurred || cond_var.empty()) {
                   addInstruction("goto block_" + std::to_string(exitB->id) + "; // while condition failed");
                   if(currentBlock) currentBlock->addSuccessor(exitB);
                   currentBlock = exitB;
                   return;
               }
             addInstruction("iffalse " + cond_var + " goto block_" + std::to_string(exitB->id) + ";");
             if(currentBlock) {
                 currentBlock->addSuccessor(bodyB);
                 currentBlock->addSuccessor(exitB);
             }

             currentBlock = bodyB;
             genStmt(getChild(node, 1));
              if (!errorOccurred) {
                 if (currentBlock && (currentBlock->instructions.empty() || !std::regex_search(currentBlock->instructions.back().text, std::regex("goto|return")))) {
                     addInstruction("goto block_" + std::to_string(condB->id) + ";");
                 }
                  if (currentBlock) currentBlock->addSuccessor(condB);
              }

             currentBlock = exitB;
         }
         else if (type == "block" || type == "statements" || type == "goal"
                  || type == "mainClass" || type == "classDeclarations" || type == "classDeclaration"
                  || type == "methodDeclarations" || type == "varDeclarations"
                  || type == "ParameterList" || type == "Parameters" || type == "Parameter"
                  || type == "argument_list" || type == "non_empty_argument_list" || type == "argument"
                  || type == "elseHandler" || type == "elseBranch"
                  || type.find("empty") != std::string::npos
                  || type == "Type" || type == "ArrayType" || type == "boolean"
                  || type == "IntType" || type == "floatType" || type == "charType"
                  || type == "varOrStatements"
                  || type == "chooseParam"
                  )
        {
             for (auto child : node->children) {
                 genStmt(child);
                 if (errorOccurred) return;
             }
         }
          else if (type == "varDeclaration") {
               for (auto child : node->children) { genStmt(child); if (errorOccurred) return; }
          }
          else if (type == "methodDeclaration") {
               Node* methodNameIdent = getChild(node, 1);
               if (methodNameIdent && methodNameIdent->type == "Identifier") {
                   addInstruction("// Method Start: " + getNodeValue(methodNameIdent));
               }
               genStmt(getChild(node, 2));
               genStmt(getChild(node, 3));
               if(errorOccurred) return;

               std::string return_var = genExp(getChild(node, 4));
               if (errorOccurred || return_var.empty()) { errorOccurred = true; return; }
               addInstruction("ireturn " + return_var + ";");

               addInstruction("// Method End: " + (methodNameIdent ? getNodeValue(methodNameIdent) : ""));
          }
          else if (type == "array") {
               std::cerr << "Warning: IR Generation for array assignment ('" << type << "') is not implemented." << std::endl;
          }
         else {
             std::cerr << "Warning: Unhandled statement type in genStmt: '" << type << "'" << std::endl;
              for (auto child : node->children) {
                 genStmt(child);
                  if (errorOccurred) return;
              }
         }
     }

    void start(Node* root);
    void printCFG(const std::string &filename);
    void generateBytecode(const std::string& filename);

};

void IR::start(Node* root) {
    errorOccurred = false;
    if (!root) { errorOccurred = true; return; }
    if(blocks.empty()) { currentBlock = createBlock(); }
    else { currentBlock = blocks[0]; }
    genStmt(root);
    if (!errorOccurred && currentBlock && (currentBlock->instructions.empty() || !std::regex_search(currentBlock->instructions.back().text, std::regex("goto|return|stop|iffalse|ireturn")))) {
        addInstruction("stop; // implicit end");
    }
    if(errorOccurred) { std::cerr << "\n--- IR Generation Failed ---\n" << std::endl; }
}

void IR::printCFG(const std::string &filename) {
         std::ofstream out(filename);
         if (!out) { return; }
         out << "digraph CFG {\n";
         out << "  rankdir=TB;\n";
         out << "  node [shape=box, fontname=\"Courier New\", fontsize=10];\n";
         out << "  edge [fontname=\"Helvetica\", fontsize=9];\n";
         for (const auto *b : blocks) {
             std::ostringstream labelStream; labelStream << "[Block " << b->id << "]\\n"; labelStream << std::left;
             for(const auto& inst : b->instructions) { std::string escaped_text = inst.text; std::regex esc_chars("[\\\\\"<>{}]"); escaped_text = std::regex_replace(escaped_text, esc_chars, "\\$&"); labelStream << std::setw(30) << escaped_text << "\\l"; }
             out << "  block_" << b->id << " [label=\"" << labelStream.str() << "\"];\n";
         }
         std::set<std::pair<int, int>> drawn_edges;
         for (const auto *b : blocks) {
              std::string trueLabel = "", falseLabel = "", gotoLabel = ""; BasicBlock* trueSucc = nullptr, *falseSucc = nullptr, *gotoSucc = nullptr; int falseTargetId = -1, gotoTargetId = -1;
              if (!b->instructions.empty()) { const std::string& lastInst = b->instructions.back().text; std::smatch m; if (std::regex_match(lastInst, m, std::regex(R"(iffalse\s+\S+\s+goto\s+block_(\d+);?)"))) { falseTargetId = std::stoi(m[1].str()); falseLabel = "false"; trueLabel = "true"; } else if (std::regex_match(lastInst, m, std::regex(R"(goto\s+block_(\d+);?)"))) { gotoTargetId = std::stoi(m[1].str()); gotoLabel = "goto"; } }
              for (auto* succ : b->successors) { if(!succ) continue; if (succ->id == falseTargetId) falseSucc = succ; else if (succ->id == gotoTargetId) gotoSucc = succ; else { if(falseTargetId != -1 && !trueSucc) trueSucc = succ; } }
              if (trueSucc && drawn_edges.find({b->id, trueSucc->id}) == drawn_edges.end()) { out << "  block_" << b->id << " -> block_" << trueSucc->id << " [label=\"" << trueLabel << "\"];\n"; drawn_edges.insert({b->id, trueSucc->id}); }
              if (falseSucc && drawn_edges.find({b->id, falseSucc->id}) == drawn_edges.end()) { out << "  block_" << b->id << " -> block_" << falseSucc->id << " [label=\"" << falseLabel << "\"];\n"; drawn_edges.insert({b->id, falseSucc->id}); }
              if (gotoSucc && drawn_edges.find({b->id, gotoSucc->id}) == drawn_edges.end()) { out << "  block_" << b->id << " -> block_" << gotoSucc->id << " [label=\"" << gotoLabel << "\"];\n"; drawn_edges.insert({b->id, gotoSucc->id}); }
              for (auto* succ : b->successors) { if (!succ) continue; if (succ != trueSucc && succ != falseSucc && succ != gotoSucc && drawn_edges.find({b->id, succ->id}) == drawn_edges.end()) { out << "  block_" << b->id << " -> block_" << succ->id << " [style=dashed, label=\"?\"];\n"; drawn_edges.insert({b->id, succ->id}); } }
         }
         out << "}\n";
         if (out.good()) { std::cout << "CFG written to " << filename << std::endl; }
         else { std::cerr << "Error: Failed to write CFG completely to " << filename << std::endl; }
     }

void IR::generateBytecode(const std::string& filename) {
         if (errorOccurred) { std::cerr << "Skipping bytecode generation due to errors in IR phase." << std::endl; std::ofstream out(filename); out << "// BYTECODE GENERATION FAILED DUE TO IR ERRORS\nstop\n"; out.close(); return; }
         std::ofstream out(filename);
         if (!out) { std::cerr << "Error opening " << filename << std::endl; return; }
         std::set<int> visited; std::queue<BasicBlock*> queue;
         if (blocks.empty()) { out << "stop\n"; out.close(); std::cout << "Bytecode written (empty IR)." << std::endl; return; }
         queue.push(blocks[0]);
         auto emit = [&](const std::string& op, const std::string& arg = "") { out << op; if (!arg.empty()) out << " " << arg; out << "\n"; };

         while (!queue.empty()) {
             BasicBlock* block = queue.front(); queue.pop();
             if (!block || visited.count(block->id)) continue; visited.insert(block->id);
             out << "label block_" << block->id << ":\n";
             for (const auto& instr : block->instructions) {
                 std::string line = instr.text; std::smatch m;
                 if (std::regex_match(line, m, std::regex(R"(\s*(\S+)\s*=\s*(-?\d+)\s*;\s*(?:\/\/.*)?)"))) { emit("iconst", m[2]); emit("istore", m[1]); }
                 else if (std::regex_match(line, m, std::regex(R"(\s*(\S+)\s*=\s*!(\S+)\s*;\s*(?:\/\/.*)?)"))) { emit("iload", m[2]); emit("inot"); emit("istore", m[1]); }
                 else if (std::regex_match(line, m, std::regex(R"(\s*(\S+)\s*=\s*(\S+)\s*([+\-*/<>=&|]{1,2})\s*(\S+)\s*;\s*(?:\/\/.*)?)"))) { std::string dst = m[1], lhs = m[2], op = m[3], rhs = m[4]; emit("iload", lhs); emit("iload", rhs); if (op == "+") emit("iadd"); else if (op == "-") emit("isub"); else if (op == "*") emit("imul"); else if (op == "/") emit("idiv"); else if (op == "<") emit("ilt"); else if (op == ">") emit("igt"); else if (op == "==") emit("ieq"); else if (op == "&&") emit("iand"); else if (op == "||") emit("ior"); else emit("// unknown binary op: " + op); emit("istore", dst); }
                 else if (std::regex_match(line, m, std::regex(R"(\s*(\S+)\s*=\s*([a-zA-Z_][\w]*)\s*;\s*(?:\/\/.*)?)"))) { emit("iload", m[2]); emit("istore", m[1]); }
                 else if (std::regex_match(line, m, std::regex(R"(\s*print\s+(\S+)\s*;\s*(?:\/\/.*)?)"))) { emit("iload", m[1]); emit("print"); }
                 else if (std::regex_match(line, m, std::regex(R"(\s*iffalse\s+(\S+)\s+goto\s+(block_\d+)\s*;\s*(?:\/\/.*)?)"))) { emit("iload", m[1]); emit("iffalse goto", m[2]); }
                 else if (std::regex_match(line, m, std::regex(R"(\s*goto\s+(block_\d+)\s*;\s*(?:\/\/.*)?)"))) { emit("goto", m[1]); }
                 else if (std::regex_match(line, m, std::regex(R"(\s*ireturn\s+(\S+)\s*;\s*(?:\/\/.*)?)"))) { emit("iload", m[1]); emit("ireturn"); }
                 else if (std::regex_match(line, m, std::regex(R"(\s*ireturn\s*;\s*(?:\/\/.*)?)"))) { emit("ireturn"); }
                 else if (std::regex_match(line, m, std::regex(R"(\s*stop\s*;\s*(?:\/\/.*)?)"))) { emit("stop"); }
                 else if (std::regex_match(line, m, std::regex(R"(\s*(\S+)\s*=\s*call\s*(\S+)\.(\S+)\((.*)\);\s*(?:\/\/.*)?)"))) { emit("// Call: " + m[1].str() + " = " + m[2].str() + "." + m[3].str() + "(" + m[4].str() + ")"); emit("iconst", "0"); emit("istore", m[1]); }
                 else if (std::regex_match(line, m, std::regex(R"(\s*(\S+)\s*=\s*new\s+(\S+)\s*;\s*(?:\/\/.*)?)"))) { emit("// New: " + m[1].str() + " = new " + m[2].str()); emit("iconst", "0"); emit("istore", m[1]); }
                 else { out << "// UNMATCHED IR: " << line << "\n"; }
             }
             for (BasicBlock* succ : block->successors) { if (succ && visited.find(succ->id) == visited.end()) { queue.push(succ); } }
         }
         bool last_block_terminated = true; if (!blocks.empty() && visited.count(blocks.back()->id)) { BasicBlock* last_gen_block = blocks.back(); if (last_gen_block && (last_gen_block->instructions.empty() || !std::regex_search(last_gen_block->instructions.back().text, std::regex("goto|return|stop|iffalse|ireturn")))) { last_block_terminated = false; } } else if (blocks.empty()) { last_block_terminated = true; }
         if (!last_block_terminated) { emit("stop"); }
         out.close();
         std::cout << "Bytecode written to " << filename << "\n";
     }
