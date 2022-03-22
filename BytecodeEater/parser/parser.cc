#include "include/parser.h"
#include "../helper/Node.h"
#include "../scanner/include/ErrorReporter.h"
#include "../scanner/include/cpplox.h"
#include <initializer_list>
#include <functional>
#include <memory>
//-----------Expression Part-----------------------------------
//expression     → assignment ;
//assignment     → ( call "." )? IDENTIFIER "=" assignment
//               | logic_or ;
//logic_or       → logic_and ( "or" logic_and )* ;
//logic_and      → equality ( "and" equality )* ;
//equality       → comparison ( ( "!=" | "==" ) comparison )* ;
//comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
//term           → factor ( ( "-" | "+" ) factor )* ;
//factor         → unary ( ( "/" | "*" ) unary )* ;
//unary          → ( "!" | "-" ) unary | call ;
//call           → primary ( "(" arguments? ")" | "." IDENTIFIER )* ;
//primary        → "true" | "false" | "nil"
//               | NUMBER | STRING
//               | "(" expression ")"
//               | IDENTIFIER ;
//               | "super" "." IDENTIFIER ;
//-----------Statement Part-----------------------------------
//program        → declaration* EOF ;
//declaration    → classDecl
//               | funDecl
//               | varDecl
//               | statement ;
//classDecl      → "class" IDENTIFIER ( "<" IDENTIFIER )?
//                 "{" function* "}" ;
//funDecl        → "fun" function ;
//function       → IDENTIFIER "(" parameters? ")" block ;
//parameters     → IDENTIFIER ( "," IDENTIFIER )* ;
//varDecl        → "var" IDENTIFIER ( "=" expression )? ";" ;
//statement      → exprStmt
//               | forStmt
//               | ifStmt
//               | printStmt
//               | returnStmt
//               | whileStmt
//               | block ;
//returnStmt     → "return" expression? ";" ;
//forStmt        → "for" "(" ( varDecl | exprStmt | ";" )
//                 expression? ";"
//                 expression? ")" statement ;
//whileStmt      → "while" "(" expression ")" statement ;
//ifStmt         →  "if" "(" expression ")" statement
//                  （"else" "if" "(" expression ")" statement)*    
//                  （"else" statement)?
//block          → "{" declaration* "}" ;
//exprStmt       → expression ";" ;
//printStmt      → "print" expression ";" ;

Parser::ErrorType
SynatxErrorTrigger(const string& message, Parser::ErrorType error_type, TokenBase* where)
{
    ErrorReporter::error(where, message);
    return error_type;
}

shared_ptr<TokenBase>
Parser::Previous()
{
    if((current_ - 1 >= tokens_.size()) || (tokens_[current_ - 1]->get_type() == TokenType::EoF))
        return nullptr;
    else
        return tokens_[current_ - 1]; 
}

shared_ptr<TokenBase>
Parser::Advance()
{
    if(!IsEnd())
        current_++;
    return Previous();
}
bool 
Parser::Match(initializer_list<TokenType> types)
{
    if(!IsEnd()){
        for(auto i : types){
            if(tokens_[current_]->get_type() == i){
                Advance();
                return true;
            }
        }
    }
    return false;
}

bool
Parser::Check(TokenType type)
{
    if(!IsEnd()){
        if(type == tokens_[current_]->get_type())
            return true; 
    }
    return false;
}


unique_ptr<Expression>
Parser::ExpressionNodeCreate()
{
    return AssignmentExprCreate();
}

unique_ptr<Expression>
Parser::AssignmentExprCreate()
{
    unique_ptr<Expression> left_expr = OrExprCreate();
    if(Match({TokenType::EQUAL})){
        shared_ptr<TokenBase> equal_operator = Previous();
        unique_ptr<Expression> value_expr = AssignmentExprCreate();
        if(left_expr->IsVaribleExpr()){
            return unique_ptr<Expression>(new AssignmentExpr(left_expr, value_expr));
        }
        else if(left_expr->IsObjectGet()){
            ObjectGet* get_expr = left_expr->IsObjectGet();
            return unique_ptr<Expression>(new ObjectSet(get_expr->object_, get_expr->name_, value_expr));
        }
        else
            throw SynatxErrorTrigger("Invalid assignment target.", ErrorType::INVALID_ASSIGN_TARGET, equal_operator.get());
    }
    return left_expr;
}

unique_ptr<Expression>
Parser::OrExprCreate()
{
    unique_ptr<Expression> left_expr = AndExprCreate();
    while(Match({TokenType::OR})){
        shared_ptr<TokenBase> logical_operator = Previous();
        unique_ptr<Expression> right_ptr = OrExprCreate();
        left_expr = unique_ptr<Expression>(new Logical(left_expr, right_ptr, logical_operator));
    }
    return left_expr;
}

unique_ptr<Expression>
Parser::AndExprCreate()
{
    unique_ptr<Expression> left_expr = EqualityExprCreate();
    while(Match({TokenType::AND})){
        shared_ptr<TokenBase> logical_operator = Previous();
        unique_ptr<Expression> right_ptr = AndExprCreate();
        left_expr = unique_ptr<Expression>(new Logical(left_expr, right_ptr, logical_operator));
    }
    return left_expr;
}


unique_ptr<Expression>
Parser::EqualityExprCreate()
{
    unique_ptr<Expression> node = ComparisonExprCreate();
    while(Match({TokenType::EQUAL_EQUAL, TokenType::BANG_EQUAL})){
        shared_ptr<TokenBase> node_operator = Previous();
        unique_ptr<Expression> right_child = ComparisonExprCreate();
        node = unique_ptr<Expression>(new Binary(node, right_child, node_operator));
    }
    return node;
}

unique_ptr<Expression> 
Parser::ComparisonExprCreate()
{
    unique_ptr<Expression> node = TermExprCreate();
    while(Match({TokenType::LESS_EQUAL, TokenType::GREATER_EQUAL, TokenType::GREATER, TokenType::LESS})){
        shared_ptr<TokenBase> node_operator = Previous();
        unique_ptr<Expression> right_child = TermExprCreate();
        node =  unique_ptr<Expression>(new Binary(node, right_child, node_operator));
    }
    return node;
}

unique_ptr<Expression>
Parser::TermExprCreate()
{
    unique_ptr<Expression> node = FactorExprCreate();
    while(Match({TokenType::MINUS, TokenType::PLUS})){
        shared_ptr<TokenBase> node_operator = Previous();
        unique_ptr<Expression> right_child = FactorExprCreate();
        node = unique_ptr<Expression>(new Binary(node, right_child, node_operator));
    }
    return node;
}

unique_ptr<Expression>
Parser::FactorExprCreate()
{
    unique_ptr<Expression> node = UnaryExprCreate();
    while(Match({TokenType::STAR, TokenType::SLASH})){
        shared_ptr<TokenBase> node_operator = Previous();
        unique_ptr<Expression> right_child = UnaryExprCreate();
        node = unique_ptr<Expression>(new Binary(node, right_child, node_operator));
    }
    return node;
}


shared_ptr<TokenBase>
Parser::Consume(TokenType type, const string& error_message, ErrorType error_type)
{
    if(tokens_[current_]->get_type() == type) 
        return Advance();
    shared_ptr<TokenBase> current_token = tokens_[current_];
    throw SynatxErrorTrigger(error_message, error_type, current_token.get());
}

unique_ptr<Expression> 
Parser::UnaryExprCreate()
{
    if(Match({TokenType::BANG, TokenType::MINUS})){
        shared_ptr<TokenBase> node_operator = Previous();
        unique_ptr<Expression> right_child = UnaryExprCreate();
        unique_ptr<Expression> node = unique_ptr<Expression>(new Unary(right_child, node_operator));
        return node;
    }
    return CallExprCreate();
}

unique_ptr<Expression>
Parser::CallExprCreate()
{
    unique_ptr<Expression> result_expr = PrimaryExprCreate();
    while(true){
        if(Match({TokenType::LEFT_PAREN})){
            auto args = GetArgs();
            shared_ptr<TokenBase> end_token =  Consume(TokenType::RIGHT_PAREN, 
                        "Expected ')' after func_args", ErrorType::EXCEPT_RIGHT_PREM);
            result_expr = unique_ptr<Expression>(new CallExpr(args, result_expr, end_token));
        }
        else if (Match({TokenType::DOT})){
            TokenPtr name =  Consume(TokenType::IDENTIFIER, 
                "Expected property name after '.'.", ErrorType::EXCEPT_PROPERITY_NAME);
            result_expr = unique_ptr<Expression>(new ObjectGet(result_expr, name));
        }
        else
            break;
    }
    
    return result_expr;
    
}

vector<unique_ptr<Expression>>
Parser::GetArgs ()
{
    vector<unique_ptr<Expression>> args;

    if(!Check(TokenType::RIGHT_PAREN)){
        do
        {
            if(args.size() >= 255 )
                SynatxErrorTrigger("Cannot have more than 255 arguments", 
                                    ErrorType::TOO_MUCH_ARG, Previous().get());
            args.push_back(ExpressionNodeCreate());
        } while (Match({TokenType::COMMA}));
    }
    return args;

}

unique_ptr<Expression> 
Parser::PrimaryExprCreate()
{
    initializer_list<TokenType> primary_type_list = {
        TokenType::FALSE, TokenType::TRUE,
        TokenType::NIL,TokenType::STRING,
        TokenType::NUMBER
    };
    if(Match(primary_type_list)){
        Value literal_value = Previous()->get_literal();
        unique_ptr<Expression> node = unique_ptr<Expression>(new Literal(literal_value, Previous()->get_line()));
        return node;
    }
    else if(Match({TokenType::IDENTIFIER})){
        unique_ptr<Expression> node = unique_ptr<Expression>(new VariableExpr(Previous()));
        return node;
    }
    else if(Match({TokenType::LEFT_PAREN})){
        unique_ptr<Expression> right_child = ExpressionNodeCreate();
        Consume(TokenType::RIGHT_PAREN, "Expected ') after expression!", ErrorType::RIGHT_PAREN_LOSS);
        unique_ptr<Expression> node = unique_ptr<Expression>(new Grouping(right_child));
        return node;
    }
    else if(Match({TokenType::THIS})){
        unique_ptr<ThisExpr> node = unique_ptr<ThisExpr>(new ThisExpr(Previous()));
        return node;
    }
    else if(Match({TokenType::SUPER})){
        TokenPtr keyword = Previous();
        Consume(TokenType::DOT, "Expected '.' after 'super'.", ErrorType::EXCEPT_DOT);
        TokenPtr method = Consume(TokenType::IDENTIFIER, "Expected superclass method name.", ErrorType::EXCEPT_PROPERITY_NAME);
        ExprPtr node = ExprPtr(new SuperExpr(keyword, method));
        return node;
    }
    shared_ptr<TokenBase> current_token = tokens_[current_];
    throw SynatxErrorTrigger("Expect a primary Token!", ErrorType::INVALID_PRIMARY_TOKEN, current_token.get());

}

//ExpressionStatement 语法分析
unique_ptr<Statement>
Parser::NormalStatementCreate()
{
    if(Match({TokenType::SEMICOLON})) return unique_ptr<Statement>();
    if(Match({TokenType::PRINT})) return PrintStatementCreate();
    if(Match({TokenType::LEFT_BRACE})) return BlockStatementCreate();
    if(Match({TokenType::IF})) return IfStatementCreate();
    if(Match({TokenType::WHILE})) return WhileStatementCreate();
    if(Match({TokenType::FOR})) return ForStatementCreate();
    if(Match({TokenType::RETURN})) return ReturnStatementCreate();
    return ExpressionStatementCreate();
}

unique_ptr<Statement>
Parser::PrintStatementCreate()
{
    TokenPtr print_token = Previous();
    unique_ptr<Expression> print_value = ExpressionNodeCreate();
    Consume(TokenType::SEMICOLON, "Expected ';' after expression.", ErrorType::EXCEPT_SEMICOLON);
    return unique_ptr<Statement>(new PrintStatement(print_value,print_token));

}

unique_ptr<Statement>
Parser::ExpressionStatementCreate()
{
    unique_ptr<Expression> value = ExpressionNodeCreate();
    TokenPtr token = Consume(TokenType::SEMICOLON, "Expected ';' after expression.", ErrorType::EXCEPT_SEMICOLON);
    return unique_ptr<Statement>(new ExpressionStatement(value,token));
}


unique_ptr<Statement> 
Parser::VarDeclStatementCreate()
{
    shared_ptr<TokenBase> var_identifier = Consume(TokenType::IDENTIFIER, "Expected variable name.",
                                            ErrorType::EXCEPT_VARIABLE_NAME);//Get the variable name 
    unique_ptr<Expression> initial_expr = nullptr;
    if(Match({TokenType::EQUAL})){
        initial_expr = ExpressionNodeCreate();
    }
    Consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.", ErrorType::EXCEPT_SEMICOLON);
    return unique_ptr<Statement>(new VariableDeclarationStatement(var_identifier, initial_expr));

}


void
Parser::Synchronize()
{
    //skip the error token
    Advance();
    while (!IsEnd()){
        if(Previous()->get_type() == TokenType::SEMICOLON) return;
        switch (Peek()->get_type()) {
            case TokenType::CLASS:
            case TokenType::FUN:
            case TokenType::VAR:
            case TokenType::FOR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::PRINT:
            case TokenType::RETURN:
                return;
            default:
                break;
        }
        Advance();
    }
}


unique_ptr<Statement>
Parser::DeclOrNormalStatementCreate()
{
    try{
        if(Match({TokenType::VAR})) return VarDeclStatementCreate();
        else if(Match({TokenType::FUN})) return FunctionDeclCreate();
        else if(Match({TokenType::CLASS})) return ClassDeclCreate();
        else
            return NormalStatementCreate();
    }
    catch(ErrorType type){
        Synchronize();
        return nullptr;
    }
}


unique_ptr<Statement>
Parser::BlockStatementCreate()
{
    vector<unique_ptr<Statement>> inner_statements;
    while (!Check(TokenType::RIGHT_BRACE) && !IsEnd()){
        inner_statements.push_back(DeclOrNormalStatementCreate());
    }
    TokenPtr token = Consume(TokenType::RIGHT_BRACE, "Expect '}' after block.",ErrorType::EXCEPT_RIGHT_BRACE);
    return unique_ptr<Statement>(new BlockStatement(inner_statements, token));
}

unique_ptr<Statement>
Parser::IfStatementCreate()
{
    vector<unique_ptr<Expression>> if_conditions;
    vector<unique_ptr<Statement>> if_branches;
    TokenPtr token = Previous();
    Consume(TokenType::LEFT_PAREN, "Expect '(' after if", ErrorType::EXCEPT_LEFT_PREM);
    if_conditions.push_back(ExpressionNodeCreate());
    Consume(TokenType::RIGHT_PAREN, "Expect ')' after the condition", ErrorType::EXCEPT_RIGHT_PREM);
    if_branches.push_back(NormalStatementCreate());
    while(Match({TokenType::ELSE})){
        if(Match({TokenType::IF})){
            Consume(TokenType::LEFT_PAREN, "Expect '(' after if", ErrorType::EXCEPT_LEFT_PREM);
            if_conditions.push_back(ExpressionNodeCreate());
            Consume(TokenType::RIGHT_PAREN, "Expect ')' after the condition", ErrorType::EXCEPT_RIGHT_PREM);
            if_branches.push_back(NormalStatementCreate());
        }
        else{        
            if_branches.push_back(NormalStatementCreate());
            break;
        }
    }
    return unique_ptr<Statement>(new IfStatement(if_conditions, if_branches, token));
}


unique_ptr<Statement>
Parser::WhileStatementCreate()
{
    TokenPtr token = Previous();
    Consume(TokenType::LEFT_PAREN, "Expect '(' after while", ErrorType::EXCEPT_LEFT_PREM);
    unique_ptr<Expression> condition_expr = ExpressionNodeCreate();
    Consume(TokenType::RIGHT_PAREN, "Expectr ')' after condition", ErrorType::EXCEPT_RIGHT_PREM);
    unique_ptr<Statement> body_statement = NormalStatementCreate();
    return unique_ptr<Statement>(new WhileStatement(move(condition_expr), move(body_statement),token));
}

unique_ptr<Statement>
Parser::ForStatementCreate()
{
    TokenPtr token = Previous();
    Consume(TokenType::LEFT_PAREN, "Expect '(' after for", ErrorType::EXCEPT_LEFT_PREM);
    unique_ptr<Statement> initial_statement;
    if(Match({TokenType::SEMICOLON}))
        initial_statement = nullptr;
    else if(Match({TokenType::VAR}))
        initial_statement = VarDeclStatementCreate();
    else
        initial_statement = ExpressionStatementCreate();

    unique_ptr<Expression> condition_expr = nullptr;
    if(!Check(TokenType::SEMICOLON))
        condition_expr = ExpressionNodeCreate();
    Consume(TokenType::SEMICOLON, "Expect ';' after for condition expr", ErrorType::EXCEPT_SEMICOLON);

    unique_ptr<Expression> side_effect;
    if(!Check(TokenType::RIGHT_PAREN))
        side_effect = ExpressionNodeCreate();
    Consume(TokenType::RIGHT_PAREN, "Expected ')' after for", ErrorType::EXCEPT_RIGHT_PREM);

    unique_ptr<Statement> body = NormalStatementCreate();
    if(side_effect != nullptr){
        vector<unique_ptr<Statement>> statements;
        statements.push_back(move(body));
        statements.push_back(unique_ptr<Statement>(new ExpressionStatement(side_effect, token)));
        body = unique_ptr<Statement>(new BlockStatement(statements, token));
    }
    if(condition_expr == nullptr)
        condition_expr = unique_ptr<Expression>(new Literal(bool(true),0));
    unique_ptr<Statement> result = unique_ptr<Statement>(new WhileStatement(move(condition_expr), move(body), token));
    if(initial_statement != nullptr){
        vector<unique_ptr<Statement>> statements;
        statements.push_back(move(initial_statement));
        statements.push_back(move(result));
        result = unique_ptr<Statement>(new BlockStatement(statements, token));
    }
    return result;

}

unique_ptr<Statement>
Parser::FunctionDeclCreate ()
{
    shared_ptr<TokenBase> identifier = Consume(TokenType::IDENTIFIER, "Expect function name!", ErrorType::EXCEPT_FUNC_NAME);
    Consume(TokenType::LEFT_PAREN, "Expcept '(' after func name", ErrorType::EXCEPT_LEFT_PREM);
    vector<shared_ptr<TokenBase>> parameter_list;
    //如果遇到了右括号说明参数为空
    if(!Check(TokenType::RIGHT_PAREN)){
        do{
            if(parameter_list.size() >= 255)
                SynatxErrorTrigger("Cannot have more than 255 parameters.", 
                                    ErrorType::TOO_MUCH_PARAMETER, 
                                    tokens_[current_].get());
            parameter_list.push_back(Consume(TokenType::IDENTIFIER, 
                                    "Expected function parameter name", 
                                    ErrorType::EXCEPT_FUNC_PARAMETER));
            
        }while(Match({TokenType::COMMA}));
    }
    Consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters.", ErrorType::EXCEPT_RIGHT_PREM);
    Consume(TokenType::LEFT_BRACE, 
            "Expected '{' before function body.", 
            ErrorType::EXCEPT_RIGHT_BRACE);
    vector<StatementPtr> body;
    while (!Check(TokenType::RIGHT_BRACE) && !IsEnd()){
        body.push_back(DeclOrNormalStatementCreate());
    }
    Consume(TokenType::RIGHT_BRACE, "Expect '}' after block.",ErrorType::EXCEPT_RIGHT_BRACE);
    Statement* raw_func = new FunctionDecl(identifier, parameter_list, body);
    StatementPtr function = unique_ptr<Statement>(raw_func);
    return function;
}

unique_ptr<Statement>
Parser::ReturnStatementCreate()
{
    shared_ptr<TokenBase> keyword = Previous();
    unique_ptr<Expression> value;
    if(!Check(TokenType::SEMICOLON))
        value = ExpressionNodeCreate();
    Consume(TokenType::SEMICOLON, "Expect ; after the return value", ErrorType::EXCEPT_SEMICOLON);
    return unique_ptr<Statement>(new ReturnStatement(value, keyword));
}

StatementPtr
Parser::ClassDeclCreate()
{
    TokenPtr name = Consume(TokenType::IDENTIFIER, "Expect class name.", ErrorType::EXCEPT_CLASS_NAME);
    ExprPtr supperclass = nullptr;
    if(Match({TokenType::LESS})){
        TokenPtr superclass_name =  Consume(TokenType::IDENTIFIER, 
            "Expected superclass name.", ErrorType::EXCEPT_SUPPERCLASS_NAME);
        supperclass = ExprPtr(new VariableExpr(superclass_name));
    }

    Consume(TokenType::LEFT_BRACE, "Expect '{' before class body.", ErrorType::EXCEPT_LEFT_BRACE);
    vector<unique_ptr<FunctionDecl>> methods;
    while(!Check(TokenType::RIGHT_BRACE) && !IsEnd()){
        StatementPtr function = FunctionDeclCreate();
        //this place must use the release ,if we directly use the IsFuncDecl to get the pointer
        //the pointer will be free when the function leaves his scopes
        //and it will lead to the uaf
        unique_ptr<FunctionDecl> method = unique_ptr<FunctionDecl>(function.release()->IsFunctionDecl());
        methods.push_back(move(method));
    }
    Consume(TokenType::RIGHT_BRACE, "Expect '}' before class body.", ErrorType::EXCEPT_LEFT_BRACE);
    return StatementPtr(new ClassDecl(name, methods, supperclass));
}


vector<unique_ptr<Statement>>
Parser::Parse()
{
    try{
        vector<unique_ptr<Statement>> programe;
        //语法分析的每次循环current都指向即将要分析的下一句话的起始Token
        while(!IsEnd()){
            programe.push_back(DeclOrNormalStatementCreate());
        }
        return programe;
    }
    catch(ErrorType type){
        return vector<unique_ptr<Statement>>();
    }
}

