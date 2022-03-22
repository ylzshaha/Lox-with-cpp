#ifndef __ERROR_REPORTER_
#define __ERROR_REPORTER_
#include <cstdint>
#include <string>
#include <sys/types.h>
#include "Token.h"



class ErrorReporter
{
public:
    static void error(uint32_t line, const std::string& message);
    //need the error token to get the error lexme
    static void error(TokenBase* token, const std::string& message);
    static void excute_runtime_error(string message, std::uint32_t);
    static bool is_error();
    static void clean_error();
    static bool is_runtime_error() {return runtime_error_flag;}
private:
    static void report(uint32_t line, const std::string& where , const std::string& message);
    static void report(uint32_t line, const std::string& message);
    static bool error_flag;
    //run time error需要和syntax error区分开
    static bool runtime_error_flag;  
};

#endif
