#include <iostream>

#include <cstdio>
#include <cctype>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
using namespace std;

// 词法分析器
string Keywords[8] = { "int", "if", "else", "while", "break", "continue", "return", "void" };
string Operators[15] = { "=", "||", "&&", "<", ">", "<=", ">=", "==", "!=", "+", "-", "*", "/", "%", "!" };
string Delimiters[6] = { ";", "(", ")", "{", "}" ,"," };
string Whitespace = " \t\n";

int line_no = 1; // 当前行号
string current_token = "";

// Token
struct Token {
    string type;
    string literal;
    int line;
};
vector<Token> tokens;


char Getchar() {
    int temp_char;

    if ((temp_char = getchar()) == EOF) {
        return EOF;
    }

    if (temp_char == '\n') line_no++;

    // 跳过空白
    if ((Whitespace.find(temp_char) != std::string::npos)) {
        return Getchar();
    }
    // 处理注释
    else if (temp_char == '/') {
        int next_char = getchar();

        // 单行注释 //
        if (next_char == '/') {
            // 吃到换行为止。只有真的遇到 '\n' 才计数
            while ((temp_char = getchar()) != EOF && temp_char != '\n');
            if (temp_char == '\n') line_no++;
            return Getchar();
        }
        // 多行注释 /* ... */
        else if (next_char == '*') {
            while (true) {
                temp_char = getchar();
                if (temp_char == EOF) return EOF;
                if (temp_char == '\n') {
                    line_no++;
                    continue;
                }
                if (temp_char == '*') {
                    int c2 = getchar();
                    if (c2 == EOF) return EOF;
                    if (c2 == '/') break;      // 注释结束
                    ungetc(c2, stdin);         // 不是 /，退回去，交给下一轮处理
                }
            }
            return Getchar();
        }
        // 不是注释，退回 next_char，把 '/' 当作普通字符返回
        else {
            ungetc(next_char, stdin);
            return '/';
        }
    }

    return static_cast<char>(temp_char);
}

void pushToken(const string& type, const string& literal) {
    tokens.push_back({ type, literal, line_no });
}

void If_IntConst(char ch) {
    current_token += ch;
    while (true) {
        ch = getchar();
        if (isdigit(ch)) current_token += ch;
        else {
            ungetc(ch, stdin);
            break;
        }
    }
    pushToken("IntConst", current_token);
    current_token.clear();
}

void Scan_char(char ch) {
    if (isalpha((unsigned char)ch) || ch == '_') {
        current_token += ch;
        while (true) {
            ch = getchar();
            if (isalnum((unsigned char)ch) || ch == '_') current_token += ch;
            else {
                ungetc(ch, stdin);
                break;
            }
        }

        bool is_keyword = false;
        for (auto& k : Keywords) {
            if (current_token == k) {
                pushToken(current_token, current_token);
                is_keyword = true;
                break;
            }
        }
        if (!is_keyword) pushToken("Ident", current_token);
        current_token.clear();
    }
    else if (isdigit(ch)) {
        If_IntConst(ch);
    }
    else {
        current_token += ch;
        char next_ch = getchar();
        if (next_ch != EOF) {
            string two_op = current_token + next_ch;
            bool match2 = false;
            for (auto& op : Operators) {
                if (two_op == op) {
                    pushToken(two_op, two_op);
                    match2 = true;
                    break;
                }
            }
            if (!match2) {
                ungetc(next_ch, stdin);
                bool match1 = false;
                for (auto& op : Operators) {
                    if (current_token == op) {
                        pushToken(current_token, current_token);
                        match1 = true;
                        break;
                    }
                }
                if (!match1) {
                    for (auto& d : Delimiters) {
                        if (current_token == d) {
                            pushToken(current_token, current_token);
                            match1 = true;
                            break;
                        }
                    }
                    if (!match1) {
                        pushToken("Error", current_token);
                    }
                }
            }
        }
        else {
            bool match1 = false;
            for (auto& op : Operators) {
                if (current_token == op) { pushToken(current_token, current_token); match1 = true; break; }
            }
            if (!match1) {
                for (auto& d : Delimiters) {
                    if (current_token == d) { pushToken(current_token, current_token); match1 = true; break; }
                }
            }
            if (!match1) pushToken("Error", current_token);
        }
        current_token.clear();
    }
}






// 语法分析器
// 全局状态
int pos = 0;
bool error_found = false;
vector<int> error_lines;
unordered_set<int> error_line_set;

// FOLLOW 集
const unordered_set<string> FOLLOW_CompUnit = { "EOF" };
const unordered_set<string> FOLLOW_FuncDef = { "int","void","EOF" };
const unordered_set<string> FOLLOW_Block = { ";", "else", "int", "void", "EOF", "}" };
const unordered_set<string> FOLLOW_Stmt = { ";", "}", "else", "int", "void", "EOF" };
const unordered_set<string> FOLLOW_Expr = { ";", ")", ",", "}", "EOF" };
const unordered_set<string> FOLLOW_ParamList = { ")", "{", "EOF" };

Token& peekTok() {
    if (tokens.empty()) {
        static Token emptyTok = { "EOF", "", line_no };
        return emptyTok;
    }
    if (pos >= (int)tokens.size()) return tokens.back();
    return tokens[pos];
}
Token& getTok() {
    if (tokens.empty()) {
        static Token emptyTok = { "EOF", "", line_no };
        return emptyTok;
    }
    if (pos >= (int)tokens.size()) return tokens.back();
    return tokens[pos++];
}
bool checkTok(const string& t) { return peekTok().type == t; }
bool isEOF() { return checkTok("EOF"); }

// 记录错误行
void recordErrorLineOnce(const Token& tok) {
    if (!error_found) error_found = true;
    if (error_line_set.insert(tok.line).second) {
        error_lines.push_back(tok.line);
    }
}

// 出错后同步
void syncTo(const unordered_set<string>& stopSet) {
    while (!isEOF() && !stopSet.count(peekTok().type)) {
        getTok();
    }
}

// 函数定义头的前向判断
bool lookAheadIsFuncDef() {
    int saved = pos;
    bool result = false;
    if (checkTok("int") || checkTok("void")) {
        getTok();
        if (checkTok("Ident")) {
            getTok();
            if (checkTok("(")) result = true;
        }
    }
    pos = saved;
    return result;
}

void CompUnit();
bool FuncDef();
void ParamList();
void Block();
bool Stmt();
void Expr();

// 表达式层次（命名按文法）
void LOrExpr();
void LAndExpr();
void RelExpr();
void AddExpr();
void MulExpr();
void UnaryExpr();
void PrimaryExpr();

// 专用：if/while 的括号条件解析（受控同步，避免级联）
bool ParenExprForCtrl();

// CompUnit → FuncDef+
void CompUnit() {
    while (!isEOF()) {
        if (!FuncDef()) {
            recordErrorLineOnce(peekTok());
            syncTo(FOLLOW_FuncDef);
        }
    }
}

// FuncDef → (“int” | “void”) ID “(” (Param (“,” Param)*)? “)” Block
bool FuncDef() {
    if (!(checkTok("int") || checkTok("void"))) return false;

    getTok();

    // 期望 Ident
    if (!checkTok("Ident")) {
        recordErrorLineOnce(peekTok());
    }
    else {
        getTok();
    }

    // 期望 '('
    if (!checkTok("(")) {
        recordErrorLineOnce(peekTok());
    }
    else {
        getTok();
    }

    // 参数列表
    if (!checkTok(")")) {
        ParamList();
    }

    // 期望 ')'
    if (!checkTok(")")) {
        recordErrorLineOnce(peekTok());
    }
    else {
        getTok();
    }

    // 函数体 Block
    Block();
    return true;
}

// ParamList -> Param (“,” Param)* , Param -> "int" Ident
void ParamList() {
    while (true) {
        // Param 开始
        if (!checkTok("int")) {
            recordErrorLineOnce(peekTok());
            syncTo(FOLLOW_ParamList);
            return;
        }
        getTok(); // 'int'

        // 期望 Ident
        if (!checkTok("Ident")) {
            recordErrorLineOnce(peekTok());
        }
        else {
            getTok();
        }

        if (checkTok(",")) {
            getTok();
            continue;
        }
        break;
    }
}

// Block → “{” Stmt* “}”
void Block() {
    // 期望 '{'
    if (!checkTok("{")) {
        recordErrorLineOnce(peekTok());
    }
    else {
        getTok();
    }

    while (!checkTok("}") && !isEOF()) {
        // 避免将函数定义误判为语句：在块中看到函数头，视为上一层缺 '}'，记录一次后结束本块
        if (lookAheadIsFuncDef()) {
            recordErrorLineOnce(peekTok());
            break;
        }

        if (!Stmt()) {
            recordErrorLineOnce(peekTok());
            syncTo(FOLLOW_Stmt);
            if (checkTok("}")) break;
            if (!isEOF() && !FOLLOW_Stmt.count(peekTok().type)) getTok();
        }
        else {
            continue;
        }
    }

    // 期望 '}'
    if (!checkTok("}")) {
        recordErrorLineOnce(peekTok());
    }
    else {
        getTok();
    }
}

// Stmt → Block | “;” | Expr “;” | ID “=” Expr “;”
//        | “int” ID “=” Expr “;” | “if” “(” Expr “)” Stmt (“else” Stmt)?
//        | “while” “(” Expr “)” Stmt | “break” “;” | “continue” “;” | “return” Expr “;”
bool Stmt() {
    // Block
    if (checkTok("{")) { Block(); return true; }

    // “;”
    if (checkTok(";")) { getTok(); return true; }

    // “int” ID “=” Expr “;”(第二十个案例有“int x_pow = 2, y_pow = 11;”这样的形式，此处加上)
    if (checkTok("int")) {
        getTok();
    Tag_1:
        if (!checkTok("Ident")) {
            recordErrorLineOnce(peekTok());
        }
        else {
            getTok();
        }
        if (!checkTok("=")) {
            recordErrorLineOnce(peekTok());
        }
        else {
            getTok();
        }
        Expr();
        if (!checkTok(";")) {
            if (checkTok(",")) {
                getTok();
                goto Tag_1;
            }
            recordErrorLineOnce(peekTok());
        }
        else {
            getTok();
        }
        return true;
    }

    // if
    if (checkTok("if")) {
        getTok();
        ParenExprForCtrl();
        if (!Stmt()) {
            return false;
        }
        if (checkTok("else")) {
            getTok();
            if (!Stmt()) return false;
        }
        return true;
    }

    // while
    if (checkTok("while")) {
        getTok();
        ParenExprForCtrl();
        if (!Stmt()) return false;
        return true;
    }

    // break / continue
    if (checkTok("break")) {
        getTok();
        if (!checkTok(";")) {
            recordErrorLineOnce(peekTok());
        }
        else {
            getTok();
        }
        return true;
    }
    if (checkTok("continue")) {
        getTok();
        if (!checkTok(";")) {
            recordErrorLineOnce(peekTok());
        }
        else {
            getTok();
        }
        return true;
    }

    // return
    if (checkTok("return")) {
        getTok();
        // 允许 return ; 或 return Expr ;
        if (checkTok(";")) {
            getTok();
            return true;
        }
        else {
            Expr();
            if (!checkTok(";")) {
                recordErrorLineOnce(peekTok());
            }
            else {
                getTok();
            }
            return true;
        }
    }

    // ID 开头：赋值或普通表达式
    if (checkTok("Ident")) {
        int saved = pos;
        Token idt = getTok();
        if (checkTok("=")) {
            getTok();
            Expr();
            if (!checkTok(";")) {
                recordErrorLineOnce(peekTok());
            }
            else {
                getTok();
            }
            return true;
        }
        else {
            // 回退，按表达式解析
            pos = saved;
            Expr();
            if (!checkTok(";")) {
                recordErrorLineOnce(peekTok());
            }
            else {
                getTok();
            }
            return true;
        }
    }

    // 其他能作为表达式起始的 token
    if (checkTok("IntConst") || checkTok("(") || checkTok("+") || checkTok("-") || checkTok("!")) {
        Expr();
        if (!checkTok(";")) {
            recordErrorLineOnce(peekTok());
        }
        else {
            getTok();
        }
        return true;
    }

    // 无法识别的起始
    recordErrorLineOnce(peekTok());
    syncTo(FOLLOW_Stmt);
    return false;
}

/*
表达式层次与文法（去左递归，按优先级自顶向下）：
Expr           → LogicalOr
LogicalOr      → LogicalAnd ( '||' LogicalAnd )*
LogicalAnd     → Relational ( '&&' Relational )*
……
其他表达式类似
*/
void Expr() { LOrExpr(); }

void LOrExpr() {
    LAndExpr();
    while (checkTok("||")) {
        getTok();
        LAndExpr();
    }
}

void LAndExpr() {
    RelExpr();
    while (checkTok("&&")) {
        getTok();
        RelExpr();
    }
}

void RelExpr() {
    AddExpr();
    while (checkTok("<") || checkTok(">") || checkTok("<=") || checkTok(">=") || checkTok("==") || checkTok("!=")) {
        getTok();
        AddExpr();
    }
}

void AddExpr() {
    MulExpr();
    while (checkTok("+") || checkTok("-")) {
        getTok();
        // 此处右侧期望 Unary（非终结符），若失败 -> 同步到 FOLLOW_Expr
        if (checkTok(";") || checkTok(")") || checkTok(",") || checkTok("}") || isEOF()) {
            recordErrorLineOnce(peekTok());
            syncTo(FOLLOW_Expr);
            return;
        }
        MulExpr();
    }
}

void MulExpr() {
    UnaryExpr();
    while (checkTok("*") || checkTok("/") || checkTok("%")) {
        getTok();
        // 同上，右侧期望 Unary
        if (checkTok(";") || checkTok(")") || checkTok(",") || isEOF()) {
            recordErrorLineOnce(peekTok());
            syncTo(FOLLOW_Expr);
            return;
        }
        UnaryExpr();
    }
}

void UnaryExpr() {
    if (checkTok("+") || checkTok("-") || checkTok("!")) {
        getTok();
        UnaryExpr();
    }
    else {
        PrimaryExpr();
    }
}

void PrimaryExpr() {
    if (checkTok("Ident")) {
        Token idt = getTok();
        if (checkTok("(")) {
            getTok();
            if (checkTok(")")) {
                // empty args
            }
            else {
                if (checkTok(",")) {
                    // 缺少第一个实参：终结符错误，仅记录并推进逗号
                    recordErrorLineOnce(peekTok());
                    getTok(); // consume ','
                }
                else {
                    Expr();
                }
                while (checkTok(",")) {
                    getTok(); // consume ','
                    if (checkTok(")")) {
                        // 缺少某个实参：仅记录
                        recordErrorLineOnce(peekTok());
                    }
                    else {
                        Expr();
                    }
                }
            }
            if (!checkTok(")")) {
                recordErrorLineOnce(peekTok());
            }
            else {
                getTok();
            }
            return;
        }
        else {
            // 变量引用
            return;
        }
    }
    else if (checkTok("IntConst")) {
        getTok();
        return;
    }
    else if (checkTok("(")) {
        getTok();
        Expr();
        if (!checkTok(")")) {
            recordErrorLineOnce(peekTok());
        }
        else {
            getTok();
        }
        return;
    }
    else {
        // Primary 非终结符失败：同步到 FOLLOW_Expr
        recordErrorLineOnce(peekTok());
        syncTo(FOLLOW_Expr);
        return;
    }
}

// if/while 括号条件：局部同步，避免级联错误
bool ParenExprForCtrl() {
    // 左括号：缺少只记录
    if (!checkTok("(")) {
        recordErrorLineOnce(peekTok());
    }
    else {
        getTok();
    }

    // 记录错误数快照
    size_t before = error_lines.size();

    // 条件表达式
    Expr();

    bool exprHasNewError = (error_lines.size() > before);

    // 受控同步集合：停在 ')' 或语句/块级安全起点
    const unordered_set<string> ctrlSync = { ")", "{", ";", "else", "}", "EOF" };

    if (exprHasNewError) {
        // 条件内部已报错：做一次局部同步，并尽量吃掉 ')'
        syncTo(ctrlSync);
        if (checkTok(")")) getTok();
        return true;
    }

    // 条件无错：检查右括号，不足则记录并同步
    if (!checkTok(")")) {
        recordErrorLineOnce(peekTok());
        syncTo(ctrlSync);
        if (checkTok(")")) getTok();
        return true;
    }
    else {
        getTok();
        return true;
    }
}

// parse
void parse() {
    pos = 0;
    error_found = false;
    error_lines.clear();
    error_line_set.clear();

    CompUnit();

    if (!error_found) {
        cout << "accept" << endl;
    }
    else {
        cout << "reject" << endl;
        for (int l : error_lines) cout << l << endl;
    }
}

// main
int main() {
    char current_char;
    while ((current_char = Getchar()) != EOF) {
        Scan_char(current_char);
    }

    // add EOF token
    tokens.push_back({ "EOF", "", line_no });

    parse();

    return 0;
}