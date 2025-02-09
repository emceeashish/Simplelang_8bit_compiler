#include <bits/stdc++.h>
using namespace std;

enum astnodetype {
    decl,       
    assignment, 
    binary,     
    identifier, 
    number,     
    if_stmt     
};

struct Node {
    astnodetype nodetype; 
    string value;         
    Node *left, *right, *next; 
    Node(astnodetype t, const string &val = "")
        : nodetype(t), value(val), left(nullptr), right(nullptr), next(nullptr) {}
};

Node* parse(const string &s, int &cu);

void skipSpaces(const string &s, int &cu) {
    while(cu < s.size() && isspace(s[cu]))
        cu++;
}

Node* parseCondition(const string &s, int &cu) {
    skipSpaces(s, cu);
    string leftId = "";
    while(cu < s.size() && isalnum(s[cu])) {
        leftId.push_back(s[cu]);
        cu++;
    }
    skipSpaces(s, cu);
    if(cu + 1 < s.size() && s.substr(cu, 2) == "==") {
        cu += 2; 
    } else {
        cerr << "Syntax error: expected '==' in condition\n";
    }
    skipSpaces(s, cu);
    string rightId = "";
    while(cu < s.size() && isalnum(s[cu])) {
        rightId.push_back(s[cu]);
        cu++;
    }
    Node* cond = new Node(binary, "==");
    cond->left = new Node(identifier, leftId);
    cond->right = new Node(identifier, rightId);
    return cond;
}

Node* parseBlock(const string &s, int &cu) {
    skipSpaces(s, cu);
    if(cu < s.size() && s[cu] == '{') {
        cu++; 
    } else {
        cerr << "Syntax error: expected '{'\n";
        return nullptr;
    }
    Node* head = nullptr;
    Node* current = nullptr;
    while(cu < s.size() && s[cu] != '}') {
        skipSpaces(s, cu);
        Node* stmt = parse(s, cu);
        if (!stmt) break;
        if (!head) {
            head = stmt;
            current = stmt;
        } else {
            current->next = stmt;
            current = stmt;
        }
    }
    if(cu < s.size() && s[cu] == '}') {
        cu++; 
    } else {
        cerr << "Syntax error: expected '}'\n";
    }
    return head;
}

Node* parse(const string &s, int &cu) {
    skipSpaces(s, cu);
    if(s.substr(cu, 2) == "if") {
        cu += 2; 
        skipSpaces(s, cu);
        if(cu < s.size() && s[cu] == '(') {
            cu++; 
        } else {
            cerr << "Syntax error: expected '(' after if\n";
            return nullptr;
        }
        Node* cond = parseCondition(s, cu);
        skipSpaces(s, cu);
        if(cu < s.size() && s[cu] == ')') {
            cu++; 
        } else {
            cerr << "Syntax error: expected ')' after condition\n";
            return nullptr;
        }
        skipSpaces(s, cu);
        Node* block = parseBlock(s, cu);  
        Node* ifNode = new Node(if_stmt);
        ifNode->left = cond;   
        ifNode->right = block; 
        return ifNode;
    }
    else if(s.substr(cu, 3) == "int") {
        cu += 3; 
        skipSpaces(s, cu);
        string var = "";
        if(cu < s.size() && isalpha(s[cu])) {
            var.push_back(s[cu]);
            cu++;
        }
        skipSpaces(s, cu);
        if(cu < s.size() && s[cu] == ';')
            cu++; 
        Node* node = new Node(decl);
        node->left = new Node(identifier, var);
        return node;
    }
    else if(cu < s.size() && isalpha(s[cu])) {
        string var = "";
        var.push_back(s[cu]); 
        cu++;
        skipSpaces(s, cu);
        if(cu < s.size() && s[cu] == '=')
            cu++; 
        skipSpaces(s, cu);
        
        Node* expr = nullptr;
        if(cu < s.size() && isdigit(s[cu])) {
            string num = "";
            while(cu < s.size() && isdigit(s[cu])) {
                num.push_back(s[cu]);
                cu++;
            }
            expr = new Node(number, num);
        }
        else if(cu < s.size() && isalpha(s[cu])) {
            string id = "";
            id.push_back(s[cu]);
            cu++;
            skipSpaces(s, cu);
            if(cu < s.size() && (s[cu] == '+' || s[cu] == '-')) {
                char op = s[cu];
                cu++; 
                skipSpaces(s, cu);
                string operand = "";
                if(cu < s.size() && isdigit(s[cu])) {
                    while(cu < s.size() && isdigit(s[cu])) {
                        operand.push_back(s[cu]);
                        cu++;
                    }
                    Node* bin = new Node(binary, string(1, op));
                    bin->left = new Node(identifier, id);
                    bin->right = new Node(number, operand);
                    expr = bin;
                }
                else if(cu < s.size() && isalpha(s[cu])) {
                    while(cu < s.size() && isalnum(s[cu])) {
                        operand.push_back(s[cu]);
                        cu++;
                    }
                    Node* bin = new Node(binary, string(1, op));
                    bin->left = new Node(identifier, id);
                    bin->right = new Node(identifier, operand);
                    expr = bin;
                }
            } else {
                expr = new Node(identifier, id);
            }
        }
        skipSpaces(s, cu);
        if(cu < s.size() && s[cu] == ';')
            cu++; 
        Node* assignNode = new Node(assignment);
        assignNode->left = new Node(identifier, var);
        assignNode->right = expr;
        return assignNode;
    }
    return nullptr;
}

Node* parseProgram(const string &s) {
    int cu = 0;
    Node *head = nullptr, *current = nullptr;
    while(cu < s.size()){
        skipSpaces(s, cu);
        if(cu >= s.size()) break;
        Node* stmt = parse(s, cu);
        if(!stmt) break;
        if(!head){
            head = stmt;
            current = stmt;
        } else {
            current->next = stmt;
            current = stmt;
        }
    }
    return head;
}

unordered_map<string,int> memoryMap;
int nextAddress = 0;
int labelCount = 0;
stringstream assembly;

void generateExpression(Node* expr) {
    if(!expr) return;
    switch(expr->nodetype){
        case number:
            assembly << "ldi A " << expr->value << "\n";
            break;
        case identifier:
            if(memoryMap.find(expr->value) == memoryMap.end()){
                cerr << "Error: Undeclared variable " << expr->value << "\n";
            } else {
                assembly << "mov A M " << memoryMap[expr->value] << "\n";
            }
            break;
        case binary:
            generateExpression(expr->left);
            assembly << "push A\n";
            generateExpression(expr->right);
            assembly << "pop B\n";
            if(expr->value == "+")
                assembly << "add A B\n";
            else if(expr->value == "-")
                assembly << "sub A B\n";
            else if(expr->value == "==")
                assembly << "cmp A B\n";
            break;
        default:
            break;
    }
}

void generateStatement(Node* stmt) {
    if(!stmt) return;
    if(stmt->nodetype == decl){
        if(stmt->left && stmt->left->nodetype == identifier){
            string var = stmt->left->value;
            if(memoryMap.find(var) == memoryMap.end()){
                memoryMap[var] = nextAddress++;
            }
        }
    } else if(stmt->nodetype == assignment){
        generateExpression(stmt->right);
        if(stmt->left && stmt->left->nodetype == identifier){
            string var = stmt->left->value;
            if(memoryMap.find(var) == memoryMap.end()){
                cerr << "Error: variable " << var << " not declared.\n";
            } else {
                assembly << "mov M A " << memoryMap[var] << "\n";
            }
        }
    } else if(stmt->nodetype == if_stmt){
        generateExpression(stmt->left);
        int currentLabel = labelCount++;
        assembly << "jnz IF_END_" << currentLabel << "\n";
        Node* block = stmt->right;
        while(block){
            generateStatement(block);
            block = block->next;
        }
        assembly << "IF_END_" << currentLabel << ":\n";
    }
}

void generateProgram(Node* head) {
    Node* current = head;
    while(current){
        generateStatement(current);
        current = current->next;
    }
}

int main(){
    string code = "int a; int b; int c; a = 10; b = 20; if(a==b){ c = a+b; }";
    
    Node* programAST = parseProgram(code);
    generateProgram(programAST);
    
    ofstream outFile("program.asm");
    if(outFile.is_open()){
        outFile << assembly.str();
        outFile.close();
        cout << "Assembly written to program.asm\n";
    } else {
        cerr << "Error opening file.\n";
    }
    
    return 0;
}
