#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <vector>
#include <string>

using namespace std;

vector<string> instructions;

void executeInstruction()
{
    std::stack<int> data;                       // data stack
    std::unordered_map<std::string,int> locals; // temps: _t1 = x

    for (const auto& raw : instructions)
    {
        if (raw.empty()) continue;

        std::string line = raw.substr(raw.find_first_not_of(" \t"));
        if (line.rfind("label", 0) == 0) continue;

        std::istringstream iss(line);
        std::string op;
        iss >> op;

        if (op == "iconst")
        {
            int v;
            iss >> v;
            data.push(v);
        }
        else if (op == "iload")
        {
            std::string v;
            iss >> v;
            data.push(locals[v]);
        }
        else if (op == "istore")
        {
            std::string v;
            iss >> v;
            int val = data.top();
            data.pop();
            locals[v] = val;
        }
        else if (op == "iadd")
        {
            int b = data.top(); data.pop();
            int a = data.top(); data.pop();
            data.push(a + b);
        }
        else if (op == "isub")
        {
            int b = data.top(); data.pop();
            int a = data.top(); data.pop();
            data.push(a - b);
        }
        else if (op == "imul")
        {
            int b = data.top(); data.pop();
            int a = data.top(); data.pop();
            data.push(a * b);
        }
        else if (op == "idiv")
        {
            int b = data.top(); data.pop();
            int a = data.top(); data.pop();
            data.push(a / b);
        }
        else if (op == "iand")
        {
            int b = data.top(); data.pop();
            int a = data.top(); data.pop();
            data.push((a && b) ? 1 : 0);
        }
        else if (op == "ior")
        {
            int b = data.top(); data.pop();
            int a = data.top(); data.pop();
            data.push((a || b) ? 1 : 0);
        }
        else if (op == "inot")
        {
            int a = data.top(); data.pop();
            data.push(!a);
        }
        else if (op == "ieq")
        {
            int b = data.top(); data.pop();
            int a = data.top(); data.pop();
            data.push((a == b) ? 1 : 0);
        }
        else if (op == "igt")
        {
            int b = data.top(); data.pop();
            int a = data.top(); data.pop();
            data.push((a > b) ? 1 : 0);
        }
        else if (op == "ilt")
        {
            int b = data.top(); data.pop();
            int a = data.top(); data.pop();
            data.push((a < b) ? 1 : 0);
        }
        else if (op == "print")
        {
            std::cout << data.top() << '\n';
            data.pop();
        }
        else if (op == "stop")
        {
            break;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2 || std::string(argv[1]).substr(std::string(argv[1]).find_last_of('.')) != ".class") {
        std::cerr << "Usage: " << argv[0] << " <filename.class>\n";
        return 1;
    }

    ifstream inputFile(argv[1]);
    if (!inputFile) {
        std::cerr << "Error opening file: " << argv[1] << '\n';
        return 1;
    }

    string line;
    while (getline(inputFile, line))
        instructions.push_back(line);
    inputFile.close();

    executeInstruction();
    return 0;
}
