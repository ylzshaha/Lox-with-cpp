#include "include/scanner.h"
#include "include/TokenType.h"
#include "include/cpplox.h"
#include "include/Token.h"
#include <boost/blank.hpp>
#include <memory>
#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <iomanip>


using namespace std;

void Scanner::show_tokens()
{
    for (auto i : tokens)
    {
        cout << i->tostring() << endl;
    }
}

void Scanner::add_token_(TokenType type)
{
    add_token_(type, boost::blank());
}

template <typename T>
void Scanner::add_token_(TokenType type, T literal)
{
    string text = source.substr(start, current - start);
    Token<T> *tmp = new Token<T>(type, text, literal, line); // error
    auto auto_ptr = TokenPtr(tmp);
    tokens.push_back(auto_ptr);
}

// look the advanced one and compare with cc
//  if they are equal, pop one and retrun true
bool Scanner::pop_compare_(char cc)
{
    if (isend_())
        return false;
    if (source[current] == cc)
    {
        current++;
        return true;
    }
    return false;
}

bool Scanner::look_compare_(char cc)
{
    if (isend_())
        return false;
    if (source[current] == cc)
        return true;
    return false;
}
char Scanner::look_one_()
{
    if (isend_())
        return '\x00';
    return source[current];
}
char Scanner::look_two_()
{
    if (current + 1 >= source.length())
        return '\x00';
    return source[current + 1];
}

void Scanner::scan_string(Lox &lox)
{
    // isend check the overflow
    while (!isend_() && !look_compare_('\"'))
    {
        if (look_compare_('\n'))
            line++;
        current++;
    }
    if (isend_())
    {
        ErrorReporter::error(line, "Unterminated string.");
        return;
    }
    current++;
    string literals = source.substr(start + 1, current - start - 2);
    add_token_(TokenType::STRING, literals);
}

void Scanner::scan_number(Lox &lox)
{
    while (!isend_() && isdigit(look_one_()))
        current++;
    // use look one to check the overflow
    if (look_compare_('.') && isdigit(look_two_()))
    {
        current += 1;
        while (isdigit(look_one_()))
            current++;
    }
    double literals_ = stod(source.substr(start, current - start));
    add_token_(TokenType::NUMBER, literals_);
}

bool my_isalpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

void Scanner::scan_identify_or_keyword(Lox &lox)
{
    map<string, TokenType> keyword_map = {
#define ORDINARY_GETTER(str, type)
#define KEYWORD_GETTER(str, type) {str, type},
    TOKEN_GROUP(KEYWORD_GETTER, ORDINARY_GETTER)
    {"EoF",TokenType::EoF }
#undef KEYWORD_GETTER
#undef ORDINARY_GETTER
    };
    while (!isend_() && (my_isalpha(look_one_()) | isdigit(look_one_())))
        current++;
    string literals_ = source.substr(start, current - start);
    if (keyword_map.find(literals_) != keyword_map.end())
    {
        if(keyword_map[literals_] == TokenType::TRUE)
            add_token_(TokenType::TRUE,true);
        else if(keyword_map[literals_] == TokenType::FALSE)
            add_token_(TokenType::FALSE, false);
        else if(keyword_map[literals_] == TokenType::NIL)
            add_token_(TokenType::NIL, boost::blank());
        else 
            add_token_(keyword_map[literals_]);
    }
    else
        add_token_(TokenType::IDENTIFIER);
}

void Scanner::scan_token_(Lox &lox)
{
    // cc means current_char
    char cc = pop_one_();
    switch (cc)
    {
    // scan (( )){}
    case '(':
    {
        add_token_(TokenType::LEFT_PAREN);
        break;
    }
    case ')':
    {
        add_token_(TokenType::RIGHT_PAREN);
        break;
    }
    case ']':
    {
        add_token_(TokenType::RIGHT_BRACKET);
        break;
    }
    case '[':
    {
        add_token_(TokenType::LEFT_BRACKET);
        break;
    }
    case '{':
    {
        add_token_(TokenType::LEFT_BRACE);
        break;
    }
    case '}':
    {
        add_token_(TokenType::RIGHT_BRACE);
        break;
    }
    case '.':
    {
        add_token_(TokenType::DOT);
        break;
    }
    case ',':
    {
        add_token_(TokenType::COMMA);
        break;
    }
    case '-':
    {
        add_token_(TokenType::MINUS);
        break;
    }
    case '+':
    {
        add_token_(TokenType::PLUS);
        break;
    }
    case ';':
    {
        add_token_(TokenType::SEMICOLON);
        break;
    }
    case '*':
    {
        add_token_(TokenType::STAR);
        break;
    }
    // scan !,=,<,>,!=,<=,>=,==
    case '!':
    {
        add_token_(pop_compare_('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
        break;
    }
    case '=':
    {
        add_token_(pop_compare_('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
        break;
    }
    case '<':
    {
        add_token_(pop_compare_('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
        break;
    }
    case '>':
    {
        add_token_(pop_compare_('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
        break;
    }

    // scan / , comment
    case '/':
    {
        if (pop_compare_('/'))
        {
            while(!isend_() && !look_compare_('\n'))
                current++;
        }
        else
            add_token_(TokenType::SLASH);
        break;
    }
    case '\t':
    case ' ':
    case '\r':break;
    case '\n':
    {
        line++;
        break;
    }
    case '\"':
    {
        scan_string(lox);
        break;
    }
    default:
    {
        if (isdigit(cc))
        {
            scan_number(lox);
        }
        else if (isalpha(cc))
        {
            scan_identify_or_keyword(lox);
        }
        else
            ErrorReporter::error(line, string() + "Unexpected character: '" + cc + "'.");
    }
    }
}


#ifdef DEBUG
void
Scanner::debug_print()
{
    cout << "[+]" << string(0x10,'-') << "Tokens" << string(0x10, '-') << endl;
    for(auto i : tokens){
        cout << i->tostring() << endl;
    }
}
#endif

void Scanner::scan_tokens(Lox &lox)
{
    uint32_t last_len;
    uint32_t new_len;
    //每次循环的时候current都指向，即将要被扫描的token的开始字节
    
    while (!isend_())
    {
        last_len = tokens.size();
        //现在start也指向了即将要被扫描的token的开始
        start = current;
        scan_token_(lox);
        new_len = tokens.size();
    }
#ifdef DEBUG
    debug_print();
#endif
    TokenBase* eof_token_ptr = new Token<long>(TokenType::EoF, "EoF", 0, line);
    auto eof_token = TokenPtr(eof_token_ptr);
    tokens.push_back(eof_token);
}
