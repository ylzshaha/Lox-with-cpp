#ifndef __SCANNER_
#define __SCANNER_
#include "TokenType.h"
#include <vector>
#include <string>
#include <memory>
class Lox;
class TokenBase;
class Scanner
{
public:
    Scanner(string source):source(source){};
    void  scan_tokens(Lox& lox);
    void show_tokens();
    vector<std::shared_ptr<TokenBase>> get_tokens(){return tokens;};
private:
    bool isend_(){return current >= source.length(); }
    char pop_one_(){return source[current++];}
    void scan_token_(Lox& lox);
    void add_token_(TokenType type);
    template<typename T>
    void add_token_(TokenType type, T literal);
    bool pop_compare_(char cc);
    bool look_compare_(char cc);
    void scan_string(Lox& lox);
    void scan_number(Lox& lox);
    void scan_identify_or_keyword(Lox& lox);
    void debug_print();
    char look_one_();
    char look_two_();
    string source;
    vector<std::shared_ptr<TokenBase>> tokens;
    uint32_t start = 0;
    uint32_t line = 1;
    uint32_t current = 0;//always the next character we will look at
};
#endif