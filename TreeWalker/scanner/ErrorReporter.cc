#include <cstdint>
#include <iostream>
#include "include/ErrorReporter.h"
using namespace std;
void
ErrorReporter::error(uint32_t line, const string& messages)
{
    error_flag = true;
    report(line, "", messages);
}

void
ErrorReporter::excute_runtime_error(TokenBase* expr_operator, string message)
{
    runtime_error_flag = true;
    report(expr_operator->get_line(), expr_operator->get_lexme(), message);
}

void 
ErrorReporter::error(TokenBase* token, const string& message)
{
    error_flag = true;
    if(token->get_type() == TokenType::EoF)
        cout <<  "[line " + to_string(token->get_line()) + "] Error at end: " + message << endl;
    else
        report(token->get_line(), token->get_lexme(), message);
}

//！实现runtime report，先空着
void ErrorReporter::report(uint32_t line, const string& where, const string& messages)
{
    cout <<  "[line " + to_string(line) + "] Error at '" + where + "': " + messages << endl;
    
}

bool
ErrorReporter::is_error()
{
    return error_flag;
}
void 
ErrorReporter::clean_error()
{
    error_flag = false;
}