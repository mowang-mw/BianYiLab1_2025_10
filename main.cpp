#include <iostream>
#include <cstdio>
#include<cctype>
#include<string>
using namespace std;

string Keywords[8] = { "int", "if", "else", "while", "break", "continue", "return", "void" };
string Operators[15] = { "=", "||", "&&", "<", ">", "<=", ">=", "==", "!=", "+", "-", "*", "/", "%", "!" };
string Delimiters[6] = { ";", "(", ")", "{", "}" ,"," };
string Whitespace = " \t\n";

string current_token = "";
int token_number = 0;

char Getchar() {
    int temp_char;
    if((temp_char = getchar()) == EOF) {
		return EOF;
    }
    else if ((Whitespace.find(temp_char) != std::string::npos || temp_char == ' ')) {
        return Getchar(); // 递归跳过空白字符
    }
    else if (temp_char == '/') {
        int next_char = getchar();
        if (next_char == '/') {
            // 单行注释，跳过直到换行或EOF
            while ((temp_char = getchar()) != EOF && temp_char != '\n');
            return Getchar(); // 递归继续获取下一个有效字符
        } else if (next_char == '*') {
            // 多行注释，跳过直到找到结束符号 */
            while (true) {
                temp_char = getchar();
                if (temp_char == EOF) {
                    return EOF; // 遇到EOF直接返回
                }
                if (temp_char == '*') {
                    next_char = getchar();
                    if (next_char == '/') {
                        break; // 找到结束符号，跳出循环
                    } else {
                        ungetc(next_char, stdin); // 放回下一个字符
                    }
                }
            }
            return Getchar(); // 递归继续获取下一个有效字符
        } else {
            ungetc(next_char, stdin); // 放回下一个字符
            return '/'; // 返回除注释外的 /
        }
    } else {
		return static_cast<char>(temp_char); // 返回非空白字符
    }
}

void If_IntConst(char ch){
    current_token += ch;
    while (true) {
        ch = getchar();
        if (isdigit(ch)) {
            current_token += ch;
        }
        else {
            ungetc(ch, stdin); // 放回非数字字符
            break;
        }
    }
    cout << token_number << ":IntConst:\"" << current_token << "\"" << endl;
    token_number++;
    current_token.clear();
}

void Scan_char(char ch) {
    if (isalpha(static_cast<unsigned char>(ch)) || ch == '_') { // 标识符或关键字
        current_token += ch;
        while (true) {
            ch = getchar();
            if (isalnum(static_cast<unsigned char>(ch)) || ch == '_') {
                current_token += ch;
            }
            else {
                ungetc(ch, stdin); // 回退操作
                break;
            }
        }
        // 检查是否为关键字
        bool is_keyword = false;
        int Size_Keywords = sizeof(Keywords) / sizeof(Keywords[0]);
        for (int i = 0; i < Size_Keywords; i++) {
            if (current_token == Keywords[i]) {
                cout << token_number << ":'" << current_token << "':\"" << current_token << "\"" << endl;
                token_number++;
                is_keyword = true;
                break;
            }
        }
        if (!is_keyword) {
            cout << token_number << ":Ident:\"" << current_token << "\"" << endl;
            token_number++;
        }
        current_token.clear();
    }
    else if (isdigit(ch)) { // 数字
        If_IntConst(ch);
    }
    else { // 操作符或分隔符
        current_token += ch;
        char next_ch = getchar();
		//不用处理负数
        //if (current_token == "-") {
        //    if (isdigit(next_ch)) { // 负数
        //        If_IntConst(next_ch);
        //        return;
        //    } 
        //}
        if (next_ch != EOF) {
            string two_char_op = current_token + next_ch;
            bool is_two_char_op = false;
            int Size_Operators = sizeof(Operators) / sizeof(Operators[0]);
            for (int i = 0; i < Size_Operators; i++) { // 检查双字符操作符
                if (two_char_op == Operators[i]) {
                    cout << token_number << ":'" << two_char_op << "':\"" << two_char_op << "\"" << endl;
                    token_number++;
                    is_two_char_op = true;
                    break;
                }
            }
            if (!is_two_char_op) {
                ungetc(next_ch, stdin); // 放回下一个字符
                bool is_one_char_op = false;
                for (int i = 0; i < Size_Operators; i++) { // 检查单字符操作符
                    if (current_token == Operators[i]) {
                        cout << token_number << ":'" << current_token << "':\"" << current_token << "\"" << endl;
                        token_number++;
                        is_one_char_op = true;
                        break;
                    }
                }
                if (!is_one_char_op) {
                    bool is_delim = false;
                    int Size_Delimiters = sizeof(Delimiters) / sizeof(Delimiters[0]);
                    for (int i = 0; i < Size_Delimiters; i++) { // 检查分隔符
                        if (current_token == Delimiters[i]) {
                            cout << token_number << ":'" << current_token << "':\"" << current_token << "\"" << endl;
                            token_number++;
                            is_delim = true;
                            break;
                        }
                    }
                    if (!is_delim) {
                        cout << token_number << "\":Error:\"" << current_token << "\"" << endl; // 未知字符
                        token_number++;
                    }
                }
            }
        }
        current_token.clear();
    }
}

int main() {
    char current_char;

    while ((current_char = Getchar()) != EOF) {
        Scan_char(current_char);
    }
    return 0;
}