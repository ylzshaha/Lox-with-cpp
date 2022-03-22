#include "include/resolver.h"
#include "../scanner/include/Token.h"
#include "../scanner/include/ErrorReporter.h"
#include "../interpreter/include/interpreter.h"
#include <boost/blank.hpp>
#include <boost/variant.hpp>
#include <cstdint>
#include <memory>


//resolve all statement in the block
//scope decides wether the variable is initialized 
ObjectType
Resolver::BlockStatementVisit(BlockStatement* block_statement)
{
    BeginScope();
    Resolve(block_statement->inner_statements_);
    EndScope();
    return boost::blank();
}

void 
Resolver::Declare(TokenPtr name)
{
    if(!scopes_.empty()){
        if(scopes_.back().find(name->get_lexme()) != scopes_.back().end()){
            ErrorReporter::error(name.get(),
                 "Already a variable with this name in this scope.");
        }
        else
            scopes_.back()[name->get_lexme()] = false;
    }
    return;
}
void
Resolver::Define(TokenPtr name)
{
    if(!scopes_.empty()){
        scopes_.back()[name->get_lexme()] = true;
    }
    return;
}

ObjectType
Resolver::VariableDeclarationStatementVisit(VariableDeclarationStatement* var_decl_expr)
{
    //cut a var define to declaration and defination
    //can help us find the self initialization problem
    Declare(var_decl_expr->var_identifier_);
    if(var_decl_expr->var_expr_ != nullptr){
        Resolve(var_decl_expr->var_expr_);
    }
    Define(var_decl_expr->var_identifier_);
    return boost::blank();
}

void
Resolver::ResolverLocal(TokenPtr name, Expression *expr)
{
    //define i as the uint type, when the size == 0 
    // the i will be a very big thing 
    for(int i = scopes_.size() - 1; i >= 0; i--){
        if(scopes_[i].find(name->get_lexme()) != scopes_[i].end()){
            interpreter_->Resolve(name, scopes_.size() - 1 - i);
            break;
        }
    }
}

ObjectType
Resolver::VariableExprVisit(VariableExpr *var_expr)
{
    string var_name = var_expr->var_identifier_->get_lexme();
    if(!scopes_.empty()){
        if(scopes_.back().find(var_name) != scopes_.back().end()){
        //refuse the self initialization
            bool initialized_flag = scopes_.back().find(var_name)->second;
            if(initialized_flag == false){
                ErrorReporter::error(var_expr->var_identifier_.get(), 
                    "Cannot read local variable in its own initializer.");
                return boost::blank();
            }
        }
    }
    
    ResolverLocal(var_expr->var_identifier_, var_expr);
    return boost::blank();
}

ObjectType
Resolver::AssignmentExprVisit(AssignmentExpr *assign_statment)
{
    Resolve(assign_statment->assign_var_);
    Resolve(assign_statment->value_expr_);
    return boost::blank();
}

void
Resolver::ResolveFunc(FunctionDecl *func_decl, FunctionType type)
{
    BeginScope();
    FunctionType enclosing = current_function_;
    current_function_ = type;
    for(auto& i : func_decl->formal_parameter_){
        Declare(i);
        Define(i);
    }
    Resolve(func_decl->function_body_);
    EndScope();
    current_function_ = enclosing;
}

ObjectType
Resolver::FunctionDeclVisit(FunctionDecl *func_decl)
{
    Declare(func_decl->func_identifier_);
    Define(func_decl->func_identifier_);

    ResolveFunc(func_decl, FunctionType::FUNCTION);
    return boost::blank();
}

ObjectType
Resolver::BinaryVisit(Binary *binary_expr)
{
    Resolve(binary_expr->left_);
    Resolve(binary_expr->right_);
    return boost::blank();
}
ObjectType
Resolver::UnaryVisit(Unary* unary_expr)
{
    Resolve(unary_expr->right_);
    return boost::blank();
}

ObjectType
Resolver::GroupingVisit(Grouping *grouping_expr)
{
    Resolve(grouping_expr->expression_);
    return boost::blank();
}

ObjectType
Resolver::LogicalVisit(Logical* logical_expr)
{
    Resolve(logical_expr->left_);
    Resolve(logical_expr->right_);
    return boost::blank();
}

ObjectType
Resolver::LiteralVisit(Literal *node){return boost::blank();}

ObjectType
Resolver::CallExprVisit(CallExpr *call_expr)
{
    Resolve(call_expr->callee_);
    for(auto& i : call_expr->call_args_){
        Resolve(i);
    }
    return boost::blank();
}

ObjectType
Resolver::ExpressionStatementVisit(ExpressionStatement *expression_statement)
{
    Resolve(expression_statement->expression_);
    return boost::blank();
}

ObjectType
Resolver::PrintStatementVisit(PrintStatement *print_statement)
{
    Resolve(print_statement->expression_);
    return boost::blank();
}

ObjectType
Resolver::IfStatementVisit(IfStatement *if_statement)
{
    for(auto& i : if_statement->if_conditions_){
        Resolve(i);
    }
    Resolve(if_statement->if_branches_);
    return boost::blank();
}

ObjectType
Resolver::WhileStatementVisit(WhileStatement *while_statemen)
{
    Resolve(while_statemen->condition_);
    Resolve(while_statemen->body_);
    return boost::blank();
}

ObjectType
Resolver::ReturnStatementVisit(ReturnStatement *return_statement)
{
    if(current_function_ == FunctionType::NONE){
        ErrorReporter::error(return_statement->return_keyword_.get(),
            "Cannot return from top-level code.");
    }
    else{
        if(return_statement->return_value_ != nullptr){
            if(current_function_ == INITIALIZER){
                ErrorReporter::error(return_statement->return_keyword_.get(),
                "Cannot return a value from an initializer.");
            }
            else
                Resolve(return_statement->return_value_);
        }
    }
    return boost::blank();
}

ObjectType
Resolver::ClassDeclVisit(ClassDecl *class_decl)
{
    current_class_ = ClassType::CLASS;
    if(class_decl->superclass_ != nullptr)
        current_class_ = ClassType::SUBCLASS;
    Declare(class_decl->name_);
    Define(class_decl->name_);
    if(class_decl->superclass_ != nullptr){
        if(class_decl->name_->get_lexme() == (class_decl->superclass_.get()->IsVaribleExpr()->var_identifier_->get_lexme())){
            ErrorReporter::error(class_decl->name_.get(), "A class can't inherit from itself.");
            return boost::blank();
        }
        else{
            Resolve(class_decl->superclass_);
            //这里多加了一层的原因是this和super无法同时绑定，this在getter中绑定
            //而super在class define时直接绑定，要更早一点
            BeginScope();
            scopes_.back()["super"] = true;
        }
    }
    BeginScope();
    scopes_.back()["this"] = true;
    for(auto& i : class_decl->methods_){
        FunctionType declaration = FunctionType::METHOD;
        if(i->func_identifier_->get_lexme() == "init")
            declaration = FunctionType::INITIALIZER;
        ResolveFunc(i.get(), declaration);
    }
    EndScope();
    if(class_decl->superclass_ != nullptr)
        EndScope();
    return boost::blank();
}

ObjectType
Resolver::ObjectGetVisit(ObjectGet *get_expr)
{
    Resolve(get_expr->object_);
    return boost::blank();
}

ObjectType
Resolver::ObjectSetVisit(ObjectSet *set_expr)
{
    Resolve(set_expr->object_);
    Resolve(set_expr->value_);
    return boost::blank();
}

ObjectType
Resolver::ThisExprVisit(ThisExpr *node)
{
    if(current_class_ == ClassType::NOCLASS){
        ErrorReporter::error(node->keyword_.get(), "Cannot use 'this' outside of a class.");
        return boost::blank();
    }
    ResolverLocal(node->keyword_, node);
    return boost::blank();
}


ObjectType
Resolver::SuperExprVisit(SuperExpr *super_expr)
{
    if(current_class_  == NOCLASS)
        ErrorReporter::error(super_expr->keyword_.get(), "Cannot use 'super' outside of a class.");
    else if(current_class_ != ClassType::SUBCLASS)
        ErrorReporter::error(super_expr->keyword_.get(), "Cannot use 'super' in a class without a superclass.");

    ResolverLocal(super_expr->keyword_, super_expr);
    return boost::blank();
}
