#include <cerrno>
#include <string>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <vector>
#include "include/cpplox.h"
#include "include/scanner.h"
#include "../parser/include/parser.h"
#include "../parser/include/printast.h"
#include "../interpreter/include/interpreter.h"
#include "../helper/reward.h"
#include "../resolver/include/resolver.h"
using namespace std;

bool ErrorReporter::error_flag = false;
bool ErrorReporter::runtime_error_flag = false;

void 
Lox::RunWithFile(string filename)
{
    Interpreter interpreter(this);
    ifstream input_file(filename);
    string content{istreambuf_iterator<char>(input_file), istreambuf_iterator<char>()};//这里注意必须使用initializer_list
    run(content, interpreter);
    if(ErrorReporter::is_error())
        exit(65);
    else if(ErrorReporter::is_runtime_error())
        exit(70);
}

void 
Lox::RunWithInteractive()
{
    Interpreter interpreter(this);
    while(1){
        cout << ">>";
        string content;
        if(!getline(cin, content))
            break;
        run(content, interpreter);
        ErrorReporter::clean_error();
    }
}

void 
Lox::run(string& source, Interpreter& inner_interpreter)
{
    Scanner scanner(source);
    PrintAstVisitor print_visitor;
    scanner.scan_tokens(*this);

//词法分析得到Token流
    vector<shared_ptr<TokenBase>> tokens = scanner.get_tokens();
    if(error_repoter.is_error()) return;
//Token流输入到语法分析器中
    Parser parser(tokens, this);
//语法分析得到Ast
    vector<unique_ptr<Statement>> program = parser.Parse();
//错误检测
    if(error_repoter.is_error()) return;
//打印语法树
#ifdef DEBUG
    cout << "[+]" << string(0x10,'-') << "Ast" << string(0x10,'-') << endl; 
    if(!program.empty()){
        for(auto&i : program)
            cout << print_visitor.PrintAst(i.get()) << endl;
    }
    cout << "[+]" << string(0x10,'-') << "Result" << string(0x10,'-') << endl; 
#endif
    Resolver resolver(&inner_interpreter);
    resolver.Resolve(program);
    if(error_repoter.is_error()) return;
//解释执行语法树
    if(!program.empty()){
        inner_interpreter.Interpret(program);
    }
    
}