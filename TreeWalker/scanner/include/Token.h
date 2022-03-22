#ifndef __TOKEN_
#define __TOKEN_
#include "TokenType.h"
#include "../../helper/reward.h"
#include <string>
#include <utility>
#include <sstream>
#include <iostream>
#include <iomanip>
using namespace std;
class TokenBase
{
public:
    virtual std::string tostring() = 0;
    virtual std::string get_lexme() = 0;
    virtual TokenType get_type() = 0;
    virtual uint32_t get_line() = 0;
    virtual ObjectType get_literal() = 0;
};

template <typename T>
class Token : public TokenBase
{
public:
    Token(TokenType type, std::string lexme, T literal, uint32_t line):
        TokenBase(),type(type), lexme(lexme), literal(literal), line(line){};
    std::string tostring() override;
    std::string get_lexme() override;
    TokenType get_type() override;
    uint32_t get_line() override;
    ObjectType get_literal() override;
private:
    TokenType type;
    std::string lexme;//源码的字符串
    T literal;//变量的字面值
    uint32_t line;
};

using TokenPtr = std::shared_ptr<TokenBase>;

template <typename T>
string
Token<T>::tostring()
{
    stringstream ss;
    ss << setprecision(15) << literal;
    string literal_tmp = ss.str();
    string tmp;
    string token_str[] = {
#define ORDINARY_GETTER(str, type) str,
#define KEYWORD_GETTER(str, type) str,
    TOKEN_GROUP(KEYWORD_GETTER, ORDINARY_GETTER)
    "EoF"
#undef KEYWORD_GETTER
#undef ORDINARY_GETTER
    };
    tmp = tmp +  "{" + "TokenType: " + token_str[type] + ", " + "Lexme: " + lexme + ", " +  "Literal: " + literal_tmp + "}";// error
    return tmp;
}

template<typename T>
string 
Token<T>::get_lexme()
{
    return lexme;
}

template<typename T>
TokenType
Token<T>::get_type()
{
    return type;
}
template<typename T>
uint32_t
Token<T>::get_line()
{
    return line;
}


template<typename T>
ObjectType
Token<T>::get_literal()
{
    ObjectType value = this->literal;
    return value;
}

#endif
