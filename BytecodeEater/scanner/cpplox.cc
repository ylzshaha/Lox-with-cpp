#include "include/cpplox.h"
#include "include/scanner.h"
#include "../parser/include/parser.h"
#include "../parser/include/printast.h"
#include "../compiler/include/compiler.h"
#include "../vm/include/vm.h"
#include "../helper/object.h"
#include <cerrno>
#include <exception>
#include <memory>
#include <string>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <vector>
#include <memory.h>
using namespace std;
#define DEBUG


bool ErrorReporter::error_flag = false;
bool ErrorReporter::runtime_error_flag = false;
bool is_repl = false;

VM vm = VM();
Compiler* current;
//we have only one global compiler,
//and the compiler only creates one function with one chunk
//so for the REPL mod we need to locate where the last execution ended


void 
Lox::RunWithFile(string filename)
{
    Compiler compiler(FunctionType::TYPE_SCRIPT);
    current = &compiler;
    ifstream input_file(filename);
    string content{istreambuf_iterator<char>(input_file), istreambuf_iterator<char>()};//这里注意必须使用initializer_list
    run(content);
    if(ErrorReporter::is_error())
        exit(65);
    else if(ErrorReporter::is_runtime_error())
        exit(70);
}

void 
Lox::RunWithInteractive()
{
    is_repl = true;
    Compiler compiler(FunctionType::TYPE_SCRIPT);
    current = &compiler;
    while(1){
        cout << ">>";
        string content;
        if(!getline(cin, content))
            break;
        if(content == "quit"){
            return;
        }
        run(content);
        ErrorReporter::clean_error();
    }
}

void 
Lox::run(string& source)
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
    vector<StatementPtr> program = parser.Parse();
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
    if(error_repoter.is_error()) return;
//解释执行语法树
    ObjFuncPtr main_function;
    if(!program.empty()){
        main_function = current->Compiling(program);
    }
    if(error_repoter.is_error()) return;
    if(main_function != nullptr)
        vm.Interpreter(main_function);
    
}