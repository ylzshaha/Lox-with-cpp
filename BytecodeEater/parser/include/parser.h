#ifndef __PARSER_
#define __PARSER_
#include "../../scanner/include/Token.h"
#include <exception>
#include <vector>
#include <cstdint>
#include <initializer_list>
#include <memory>
class Lox;
class TokenBase;
class Node;
class Statement;
class Expression;
using ExprPtr = std::unique_ptr<Expression>;
using StatementPtr = std::unique_ptr<Statement>;
//expression     → equality ;
//equality       → comparison ( ( "!=" | "==" ) comparison )* ;
//comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
//term           → factor ( ( "-" | "+" ) factor )* ;
//factor         → unary ( ( "/" | "*" ) unary )* ;
//unary          → ( "!" | "-" ) unary
//               | primary ;
//primary        → NUMBER | STRING | "true" | "false" | "nil"
//               | "(" expression ")" ;
class Parser
{
public:
  enum ErrorType{
        RIGHT_PAREN_LOSS, INVALID_PRIMARY_TOKEN,
        EXCEPT_SEMICOLON, EXCEPT_VARIABLE_NAME,
        INVALID_ASSIGN_TARGET, EXCEPT_RIGHT_BRACE,
        EXCEPT_LEFT_PREM, EXCEPT_RIGHT_PREM,
        TOO_MUCH_ARG, EXCEPT_FUNC_NAME,
        TOO_MUCH_PARAMETER, EXCEPT_FUNC_PARAMETER,
        EXCEPT_CLASS_NAME, EXCEPT_LEFT_BRACE,
        EXCEPT_PROPERITY_NAME, EXCEPT_SUPPERCLASS_NAME,
        EXCEPT_DOT, EXCEPT_RIGHT_BRACKET
    };
    Parser(std::vector<shared_ptr<TokenBase>> tokens_arg, Lox* lox_arg): tokens_(tokens_arg), lox(lox_arg) {}
    vector<std::unique_ptr<Statement>> Parse();
    std::unique_ptr<Statement> DeclOrNormalStatementCreate();
private:

    std::vector<shared_ptr<TokenBase>> tokens_;
    uint32_t current_ = 0;
    Lox* lox;

    bool Check(TokenType type);
    bool Match(initializer_list<TokenType> types);
    //判断Token流是否结束
    bool IsEnd(){return (current_ >= tokens_.size() | tokens_[current_]->get_type() == TokenType::EoF);}
    std::shared_ptr<TokenBase> Previous();
    std::shared_ptr<TokenBase> Advance();
    std::shared_ptr<TokenBase> Peek() {return tokens_[current_];}
    std::shared_ptr<TokenBase> Consume(TokenType type, const string& error_message, ErrorType error_type);
    ExprPtr ExpressionNodeCreate();
    ExprPtr EqualityExprCreate();
    ExprPtr ComparisonExprCreate();
    ExprPtr TermExprCreate();
    ExprPtr FactorExprCreate();
    ExprPtr UnaryExprCreate();
    ExprPtr PrimaryExprCreate();
    ExprPtr AssignmentExprCreate();
    ExprPtr OrExprCreate();
    ExprPtr AndExprCreate();
    ExprPtr CallExprCreate();
    StatementPtr PrintStatementCreate();
    StatementPtr ExpressionStatementCreate();
    StatementPtr VarDeclStatementCreate();
    StatementPtr NormalStatementCreate();
    StatementPtr BlockStatementCreate();
    StatementPtr IfStatementCreate();
    StatementPtr WhileStatementCreate();
    StatementPtr ForStatementCreate();
    StatementPtr FunctionDeclCreate();
    StatementPtr ReturnStatementCreate();
    StatementPtr ClassDeclCreate();
    std::vector<std::unique_ptr<Expression>> GetArgs();
    void Synchronize();
};



#endif