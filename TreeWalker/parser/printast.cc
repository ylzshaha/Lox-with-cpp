#include "include/printast.h"
#include "../helper/reward.h"
#include "../scanner/include/Token.h"
#include "../helper/Node.h"
#include <boost/blank.hpp>
#include <boost/variant/detail/apply_visitor_delayed.hpp>
#include <memory>
#include<string>
#include<initializer_list>
#include<boost/variant.hpp>

using namespace std;

using ExpressionWrapper = reference_wrapper<unique_ptr<Expression>&>;
using StatementWrapper = reference_wrapper<unique_ptr<Statement>&>;



string
PrintAstVisitor::PrintAst(Node* expr)
{   if(expr == nullptr) return string();
    ObjectType result =  expr->Accept(this);
    if(result.which() == 0) return string();
    string print_string = boost::get<string>(result);
    return print_string;
}
//如果只是一个字面值节点那么就这个函数就执行一个简单的字符串相加
//如果节点还有子树在这个函数中会继续递归的遍历子树
//返回值为string

string 
PrintAstVisitor::ExprAstToString(string lexme, vector<ExpressionWrapper> child)
{
    string result;
    result = result +  '(' + lexme + ' ';
    for(uint32_t i = 0; i < child.size(); i++){
        result += boost::get<string>(child[i].get()->Accept(this));
        if(i != child.size()- 1)
            result += ' ';
    }
    result += ')' ;
    return result;
}
//Accept函数会调用这个函数来打印节点
//从节点中取出字面值
//返回值其实是StringReward*
ObjectType
PrintAstVisitor::BinaryVisit(Binary* binary_expr)
{
    ObjectType result;
    if(binary_expr){
        string lexme = binary_expr->operator_->get_lexme();
        result = ExprAstToString(lexme, {binary_expr->left_, binary_expr->right_});
    }
    return result;
};

ObjectType
PrintAstVisitor::UnaryVisit(Unary* unary_expr)
{
    ObjectType result;
    if(unary_expr){
        string lexme = unary_expr->operator_->get_lexme();
        result = ExprAstToString(lexme, {unary_expr->right_});
    }
    return result;
};

ObjectType
PrintAstVisitor::GroupingVisit(Grouping* grouping_expr)
{
    ObjectType result;
    if(grouping_expr){
        result = ExprAstToString("grouping", {grouping_expr->expression_});
    }
    return result;
}

ObjectType
PrintAstVisitor::LiteralVisit(Literal* literal_expr)
{
    ObjectType result;
    if(literal_expr){
        ObjectType value = literal_expr->literal_;
        string lexme = boost::apply_visitor(ToStringVisitor(), value);
        result = lexme;
    }
    return result;
}

ObjectType
PrintAstVisitor::VariableExprVisit(VariableExpr* var_expr)
{
    ObjectType result;

    string print_value = "(var)";
    if(var_expr){
        string lexme = var_expr->var_identifier_->get_lexme();
        print_value += lexme;
    }
    result = print_value;
    return result;
    
}

ObjectType
PrintAstVisitor::PrintStatementVisit(PrintStatement* print_statement)
{
    ObjectType result;

    string skip = string(block_layer, '\t');
    string statem_type_str = skip + "(PrintStatement ";
    unique_ptr<Expression>& inner_expr = print_statement->expression_;
    string expr_str = PrintAst(inner_expr.get());
    string print_value = statem_type_str + expr_str + ')';
    result = print_value;
    return print_value;
}

ObjectType
PrintAstVisitor::ExpressionStatementVisit(ExpressionStatement* expression_statement)
{
    ObjectType result;

    string skip = string(block_layer, '\t');
    string statem_type_str = skip + "(ExpressionStatement ";
    unique_ptr<Expression>& inner_expr = expression_statement->expression_;
    string expr_str = PrintAst(inner_expr.get());
    string print_value = statem_type_str + expr_str + ')';
    result = print_value;
    return result;
}

ObjectType
PrintAstVisitor::VariableDeclarationStatementVisit(VariableDeclarationStatement* var_decl_statement)
{
    ObjectType result;
    string skip = string(block_layer, '\t');
    string print_value = skip + "(var_define ";

    print_value = print_value + var_decl_statement->var_identifier_->get_lexme() + ' '; //(var_define name 
    if(var_decl_statement->var_expr_ != nullptr){
        string ini_expr_str = PrintAst(var_decl_statement->var_expr_.get());//(var_define name ()
        print_value += ini_expr_str;
    }
    print_value += ')';
    result = print_value;
    return result;
}

ObjectType
PrintAstVisitor::AssignmentExprVisit(AssignmentExpr* assignment_expr)
{
    ObjectType result;
    unique_ptr<Expression>& assign_var = assignment_expr->assign_var_;
    shared_ptr<TokenBase> assign_identifier = assign_var->IsVaribleExpr()->var_identifier_;
    string print_value = "(= ";
    print_value = print_value + assign_identifier->get_lexme() + ' ';
    print_value += PrintAst(assignment_expr->value_expr_.get());
    print_value += ')';
    result = print_value;
    return result;
}


string
PrintAstVisitor::BlockTraverse (vector<StatementPtr>& statements)
{
    string print_value;
    if(!statements.empty()){
        block_layer++;
        for(auto& i : statements){
            print_value += "\n";
            print_value += PrintAst(i.get());
        }
        block_layer--;
    }
    return print_value;
}

string
PrintAstVisitor::BlockTraverse (vector<shared_ptr<Statement>> statements)
{
    string print_value;
    if(!statements.empty()){
        block_layer++;
        for(auto i : statements){
            print_value += "\n";
            print_value += PrintAst(i.get());
        }
        block_layer--;
    }
    return print_value;
}

ObjectType 
PrintAstVisitor::BlockStatementVisit(BlockStatement* block_satement)
{

    ObjectType result;
    vector<unique_ptr<Statement>>& statements = block_satement->inner_statements_;
    string skip = string(block_layer, '\t');
    string print_value = skip + "{";
    print_value += BlockTraverse(statements);
    print_value += '\n' + skip +  "}";
    result = print_value;
    return result;    
}

ObjectType
PrintAstVisitor::IfStatementVisit(IfStatement* if_statement)
{
    ObjectType result;
    vector<unique_ptr<Expression>>&  if_conditions = if_statement->if_conditions_;
    vector<unique_ptr<Statement>>&  if_branches = if_statement->if_branches_;
    string skip = string(block_layer, '\t');
    string print_value = skip + "[!]If_statement start:\n";
    uint32_t current_num = 0;
    
    for(current_num = 0; current_num < if_conditions.size(); current_num++){
        print_value += skip + "condition_" + to_string(current_num) + ": ";
        print_value += PrintAst(if_conditions[current_num].get()) + '\n';
        print_value += skip + "branch_" + to_string(current_num) + ": " + '\n';
        print_value += PrintAst(if_branches[current_num].get()) + '\n';
    }

    if(current_num < if_branches.size()){
        print_value += skip + "branch_" + to_string(if_branches.size() - 1) + ": " + '\n';
        print_value += PrintAst(if_branches[current_num].get()) + '\n';
    }

    print_value += skip + "[!]if_statement end";
    result = print_value;
    return result;
}


ObjectType
PrintAstVisitor::LogicalVisit(Logical* logical_expr)
{
    ObjectType result;
    string lexme = logical_expr->operator_->get_lexme();
    result = ExprAstToString(lexme, {logical_expr->left_, logical_expr->right_});
    return result;
}

ObjectType
PrintAstVisitor::WhileStatementVisit(WhileStatement* while_statement)
{
    ObjectType result;
    string skip = string(block_layer, '\t');
    string print_value = skip + "[!]while_statement start:\n";
    print_value += skip + "condition: ";
    print_value += PrintAst(while_statement->condition_.get()) + '\n';
    print_value += skip + "body:\n";
    print_value += PrintAst(while_statement->body_.get()) + '\n';
    print_value += skip + "[!]while_statement end";
    result = print_value;
    return result;

}

ObjectType
PrintAstVisitor::CallExprVisit(CallExpr* call_expr)
{
    ObjectType result;
    string print_value = "(call";
    print_value += PrintAst(call_expr->callee_.get()) + " : ";
    for(auto& i : call_expr->call_args_){
        print_value += PrintAst(i.get());
        if(i != *(call_expr->call_args_.end() - 1))
        print_value += ", ";
    }
    print_value += ')';
    result = print_value;
    return result;
}

ObjectType
PrintAstVisitor::FunctionDeclVisit(FunctionDecl* func_decl)
{
    ObjectType result;
    string skip = string(block_layer, '\t');
    string print_value = skip + "func " + func_decl->func_identifier_->get_lexme() + "(";
    for(auto i : func_decl->formal_parameter_){
        print_value += i->get_lexme();
        if(i != *(func_decl->formal_parameter_.end() - 1))
            print_value += ", ";
    }
    print_value += ")\n";
    print_value += skip + "{";
    vector<shared_ptr<Statement>> body = func_decl->function_body_;
    print_value += BlockTraverse(body);
    print_value += '\n' + skip +  "}";
    result = print_value;
    return result;
}

ObjectType
PrintAstVisitor::ReturnStatementVisit(ReturnStatement* return_statement)
{
    ObjectType result;
    string skip = string(block_layer, '\t');
    string print_value = skip + "(return ";

    print_value += PrintAst(return_statement->return_value_.get());
    print_value += ")";
    result = print_value;
    return result; 
}

ObjectType
PrintAstVisitor::ThisExprVisit(ThisExpr *node)
{
    return node->keyword_->get_lexme();
}

ObjectType
PrintAstVisitor::ClassDeclVisit(ClassDecl *class_decl)
{
    ObjectType result;
    string skip = string(block_layer, '\t');
    string print_value = skip + "class " + class_decl->name_->get_lexme() + "{\n";
    if(!class_decl->methods_.empty()){
        for(auto i : class_decl->methods_){
            block_layer += 1;
            print_value += PrintAst(i.get()) + '\n';
            block_layer -= 1;
        }
    }
    print_value += '\n' + skip + "}";
    result = print_value;
    return result;
}

ObjectType
PrintAstVisitor::ObjectGetVisit(ObjectGet *get_expr)
{
    ObjectType result;
    string print_value = PrintAst(get_expr->object_.get());
    print_value += '.' + get_expr->name_->get_lexme();
    result = print_value;
    return result;
}

ObjectType
PrintAstVisitor::ObjectSetVisit(ObjectSet *set_expr)
{
    ObjectType result;
    string print_value = "(= ";
    print_value = print_value + PrintAst(set_expr->object_.get()) + '.';
    print_value += set_expr->name_->get_lexme();
    print_value += ' ' + PrintAst(set_expr->value_.get());
    print_value += ')';
    result = print_value;
    return result;
}

ObjectType
PrintAstVisitor::SuperExprVisit(SuperExpr *super_expr)
{
    ObjectType result;
    string print_value = "super";
    print_value += '.' + super_expr->method_->get_lexme();
    result = print_value;
    return result;
}
//finally return print (Return )
