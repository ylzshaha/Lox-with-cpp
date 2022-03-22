#ifndef __EXPR_
#define __EXPR_

#include <algorithm>
#include <utility>
#include <vector>
#include <boost/variant.hpp>
#include <memory>

class UserDefineFunction;
class Function;
class TokenBase;
class Resolver;
class InstanceObject;
class Parser;

using TokenPtr = std::shared_ptr<TokenBase>;
using ObjectType = boost::variant<boost::blank, std::string, double, bool,
             std::shared_ptr<Function>,std::shared_ptr<InstanceObject> ,long int>;

#define NODE_LIST(V, W)                 \
    V(Binary)                           \
    V(Unary)                            \
    V(Literal)                          \
    V(Grouping)                         \
    V(PrintStatement)                   \
    V(ExpressionStatement)              \
    V(VariableDeclarationStatement)     \
    V(VariableExpr)                     \
    V(AssignmentExpr)                   \
    V(BlockStatement)                   \
    V(IfStatement)                      \
    V(Logical)                          \
    V(WhileStatement)                   \
    V(CallExpr)                         \
    V(FunctionDecl)                     \
    V(ReturnStatement)                  \
    V(ClassDecl)                        \
    V(ObjectGet)                        \
    V(ObjectSet)                        \
    V(ThisExpr)                         \
    V(SuperExpr)                        \
    W(Expression)                       \
    W(Statement)                


class RewardBase;
//所有的visitor类都继承VisitorBase这个基类
//一个visitor类代表的是所有种类的表达式的一种操作
class VisitorBase;
class PrintAstVisitor;
class Interpreter;
class UserDefineFunctionalReward;
class ObjectSet;
class Super;


#define NODE_FORWARD_DECLARATION(node_type) class node_type;
NODE_LIST(NODE_FORWARD_DECLARATION, NODE_FORWARD_DECLARATION)
#undef NODE_FORWRAD_DECLARATION 


class Node
{
public:
    virtual ObjectType Accept(VisitorBase* visitor) = 0;//使用模板的原因是因为accept返回的结果
    virtual Expression* IsExpression() {return nullptr;}
    virtual Statement* IsStatment() {return nullptr;}
    virtual Binary* IsBinary(){return nullptr;}
    virtual Unary* IsUnary(){return nullptr;}
    virtual Grouping* IsGrouping(){return nullptr;}
    virtual Literal* IsLiteral(){return nullptr;}
    virtual PrintStatement* IsPrintStatement(){return nullptr;}
    virtual ExpressionStatement*  IsExpressionStatement() {return nullptr;}
    virtual VariableDeclarationStatement* IsVariableDeclarationStatement() {return nullptr;}
    virtual VariableExpr* IsVaribleExpr(){return nullptr;}
    virtual AssignmentExpr* IsAssignmentExpr() {return nullptr;}
    virtual BlockStatement* IsBlockStatement() {return nullptr;}
    virtual IfStatement* IsIfStatement() {return nullptr;}
    virtual Logical* IsLogical() {return nullptr;}
    virtual WhileStatement* IsWhileStatement() {return nullptr;}
    virtual CallExpr* IsCallExpr() {return nullptr;}
    virtual FunctionDecl* IsFunctionDecl() {return nullptr;}
    virtual ReturnStatement* IsReturnStatement() {return nullptr;}
    virtual ClassDecl* IsClassDecl() {return nullptr;}
    virtual ObjectGet* IsObjectGet() {return nullptr;}
    virtual ObjectSet* IsObjectSet() {return nullptr;}
    virtual ThisExpr* IsThisExpr() {return nullptr;}
    virtual SuperExpr* IsSuperExpr(){return nullptr;}
};

class Expression : public Node
{
public:
    virtual Expression* IsExpression() override {return this;}
    virtual bool IsValidLeftValue() {return false;}
};

using ExprPtr = std::unique_ptr<Expression>;

class Statement : public Node
{
public:
    Statement* IsStatment() override {return this;}
};

using StatementPtr = std::unique_ptr<Statement>;

class Binary : public Expression
{
public:
    Binary(std::unique_ptr<Expression>& left_arg, std::unique_ptr<Expression>& right_arg, TokenPtr operator_arg)
        :left_(std::move(left_arg)), right_(std::move(right_arg)), operator_(operator_arg){}
    ObjectType Accept (VisitorBase* visitor) override;
    Binary* IsBinary() override  {return this;}
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    ExprPtr left_;
    ExprPtr right_;
    TokenPtr operator_;
};


class Grouping : public Expression
{
public:
    Grouping(ExprPtr& expression_arg) 
        :expression_(std::move(expression_arg)){}
    ObjectType Accept(VisitorBase* visitor) override;
    Grouping* IsGrouping() override {return this;}
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    ExprPtr expression_;
};


class Unary : public Expression
{
public:
    Unary(ExprPtr& right_arg, TokenPtr operator_arg)
        :right_(std::move(right_arg)), operator_(operator_arg){}
    ObjectType Accept  (VisitorBase* visitor) override;
    Unary* IsUnary() override {return this;}
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    ExprPtr right_;
    TokenPtr operator_;
};

class Literal : public Expression
{
public:
    Literal(ObjectType literal_value)
        :literal_(literal_value){}
    ObjectType Accept(VisitorBase* visitor) override;
    Literal* IsLiteral() override {return this;}
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    ObjectType literal_;
};

class VariableExpr : public Expression
{
public:
    VariableExpr(TokenPtr identifier)
                     : var_identifier_(identifier){}
    ObjectType Accept(VisitorBase* visitor) override ;
    VariableExpr* IsVaribleExpr() override {return this;}
    bool IsValidLeftValue() override {return true;}
    friend Interpreter;
    friend PrintAstVisitor;
    friend Resolver;
private:
    TokenPtr var_identifier_; 
};

class ThisExpr : public Expression
{
public:
    ThisExpr(TokenPtr keyword):
        keyword_(keyword){}
    ThisExpr* IsThisExpr() override {return this;}
    ObjectType Accept(VisitorBase* visitor) override;
    friend Interpreter;
    friend PrintAstVisitor;
    friend Resolver;
private:
    TokenPtr keyword_;
};

class PrintStatement : public Statement
{
public:
    PrintStatement(ExprPtr& expression_arg)
                     : expression_(std::move(expression_arg)) {}
    PrintStatement* IsPrintStatement() override {return this;};
    ObjectType Accept(VisitorBase* visitor) override;
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    ExprPtr expression_;
};

class ExpressionStatement : public Statement
{
public:
    ExpressionStatement(ExprPtr& expression_arg)
                         : expression_(std::move(expression_arg)) {}
    ExpressionStatement* IsExpressionStatement() override {return this;}
    ObjectType Accept(VisitorBase* visitor) override;
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    ExprPtr expression_;

};


class VariableDeclarationStatement : public Statement
{
public:
    VariableDeclarationStatement(TokenPtr identifier, ExprPtr& initial_expr)
                                    :var_identifier_(identifier), var_expr_(std::move(initial_expr))  {}
    VariableDeclarationStatement* IsVariableDeclarationStatement() override  {
        return this;
    }
    ObjectType Accept(VisitorBase* visitor) override;
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    TokenPtr var_identifier_;
    ExprPtr var_expr_;
};


class AssignmentExpr : public Expression 
{
public:
    AssignmentExpr(ExprPtr& var, ExprPtr& expr)
                    :assign_var_(std::move(var)), value_expr_(std::move(expr)){}
    AssignmentExpr* IsAssignmentExpr() override  {return this;}
    ObjectType Accept(VisitorBase* visitor) override;
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    ExprPtr assign_var_;
    ExprPtr value_expr_;
};

class BlockStatement : public Statement
{
public:
    BlockStatement(std::vector<StatementPtr>& statements){
        for(auto& i : statements){
            inner_statements_.push_back(std::move(i));
        }
    }
    BlockStatement* IsBlockStatement() override {return this;}
    ObjectType Accept(VisitorBase* visitor) override;
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    std::vector<StatementPtr> inner_statements_;
}; 


class IfStatement : public Statement 
{
public:
    IfStatement(std::vector<ExprPtr>& conditions, std::vector<StatementPtr>& branches){
        for(auto& i : conditions){
            if_conditions_.push_back(std::move(i));
        }
        for(auto& i : branches){
            if_branches_.push_back(std::move(i));
        }
    }
    IfStatement* IsIfStatement() override {return this;}
    ObjectType Accept(VisitorBase* visitor) override;
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    std::vector<ExprPtr> if_conditions_;
    std::vector<StatementPtr> if_branches_; 
};

class Logical : public Expression
{
public:
    Logical(ExprPtr& left_arg, ExprPtr& right_arg, TokenPtr operator_arg)
        :left_(std::move(left_arg)), right_(std::move(right_arg)), operator_(operator_arg){}
    ObjectType Accept (VisitorBase* visitor) override;
    Logical* IsLogical() override {return this;}
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    ExprPtr left_;
    ExprPtr right_;
    TokenPtr operator_;
};

class WhileStatement : public Statement
{
public:
    WhileStatement(ExprPtr& condition, StatementPtr& body):
        condition_(std::move(condition)), body_(std::move(body)){}
    WhileStatement* IsWhileStatement() override {return this;}
    ObjectType Accept(VisitorBase* visitor) override;
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    ExprPtr condition_;
    StatementPtr body_;
};

class CallExpr : public Expression
{
public:
    CallExpr(std::vector<ExprPtr>& args,ExprPtr& callee, TokenPtr end_token):
                 end_token_(end_token), callee_(std::move(callee)){
                     for(auto& i : args){
                         call_args_.push_back(std::move(i));
                     }

                 }
    CallExpr* IsCallExpr() override {return this;}
    ObjectType Accept(VisitorBase* visitor) override;
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    std::vector<ExprPtr> call_args_;
    ExprPtr callee_;
    TokenPtr end_token_; 
};

//
class FunctionDecl : public Statement
{
public:
    FunctionDecl(TokenPtr identifier, std::vector<TokenPtr> parameter_list, std::vector<std::shared_ptr<Statement>> body):
        func_identifier_(identifier), formal_parameter_(parameter_list), function_body_(body){}
    FunctionDecl(const FunctionDecl& src){
        func_identifier_ = src.func_identifier_;
        for(auto i : src.formal_parameter_)
            formal_parameter_.push_back(i);
        for(auto i : src.function_body_){
            function_body_.push_back(i);
        }
    }
    FunctionDecl* IsFunctionDecl() override {return this;}
    ObjectType Accept(VisitorBase* visitor) override;
    friend PrintAstVisitor;
    friend Interpreter;
    friend UserDefineFunction;
    friend Resolver;
private:
    TokenPtr func_identifier_;
    std::vector<TokenPtr> formal_parameter_;
    std::vector<std::shared_ptr<Statement>> function_body_;
};

class ReturnStatement : public Statement
{
public:
    ReturnStatement(ExprPtr& value, TokenPtr identifier):
        return_value_(std::move(value)), return_keyword_(identifier){}
    ObjectType Accept(VisitorBase* visitor) override;
    ReturnStatement* IsReturnStatement() override {return this;}
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    ExprPtr return_value_;
    TokenPtr return_keyword_;
};


class ClassDecl : public Statement
{
public:
    ClassDecl(TokenPtr name, std::vector<std::shared_ptr<FunctionDecl>> methods, ExprPtr& superclass):
        name_(name), methods_(methods), superclass_(std::move(superclass)){}
    ObjectType Accept(VisitorBase* visitor) override;
    ClassDecl* IsClassDecl() override {return this;}
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    TokenPtr name_;
    std::vector<std::shared_ptr<FunctionDecl>> methods_;
    ExprPtr superclass_ = nullptr;
};

class ObjectGet : public Expression
{
public:
    ObjectGet(ExprPtr& object, TokenPtr name):
        object_(std::move(object)), name_(name){}
    ObjectGet* IsObjectGet() override {return this;}
    ObjectType Accept(VisitorBase* visitor) override;
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
    friend Parser;
private:
    ExprPtr object_;
    TokenPtr name_;
};

class ObjectSet : public Expression
{
public: 
    ObjectSet(ExprPtr& object, TokenPtr name, ExprPtr& value):
        object_(std::move(object)), name_(name), value_(std::move(value)){}
    ObjectSet* IsObjectSet() override {return this;}
    ObjectType Accept(VisitorBase* visitor) override;
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    ExprPtr object_;
    TokenPtr name_;
    ExprPtr value_;
};

class SuperExpr : public Expression
{
public:
    SuperExpr(TokenPtr keyword, TokenPtr method):
        keyword_(keyword), method_(method) {}
    SuperExpr* IsSuperExpr() override {return this;}
    ObjectType Accept(VisitorBase* visitor) override;
    friend PrintAstVisitor;
    friend Interpreter;
    friend Resolver;
private:
    TokenPtr keyword_;
    TokenPtr method_;
};

#endif