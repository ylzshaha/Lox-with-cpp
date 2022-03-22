#include "include/interpreter.h"
#include "../helper/reward.h"
#include "../scanner/include/Token.h"
#include "../scanner/include/cpplox.h"
#include <boost/variant/get.hpp>
#include <ios>
#include <memory>
#include <utility>
#include <string>
#include <vector>
#include <functional>
#include <boost/blank.hpp>
#include <boost/variant.hpp>
using namespace std;

ObjectType
Interpreter::LiteralVisit(Literal* literal_node)
{
    //value_info 是一个pair<TokenType, string> 
    ObjectType value = literal_node->literal_;
    return value;
}

bool GetTruthy(ObjectType opnum)
{
    //除了Nil和false其他都是true 0
    if(opnum.which() == 0) return false;
    if(opnum.which() == 3){
        return boost::get<bool>(opnum);
    }
    if(opnum.which() == 2){
        if(boost::get<double>(opnum) == 0)
            return false;
    }
    return true;
}

bool IsEqual(ObjectType left_value, ObjectType right_value)
{
    if((left_value.which() == 0) && (right_value.which() == 0))
        return true;
    return left_value == right_value;
}
Interpreter::RunTimeErrorType
ExcuteRuntimeErrorTrigger(Interpreter::RunTimeErrorType error_type, string message, TokenBase* expr_operator)
{
    ErrorReporter::excute_runtime_error(expr_operator,message);
    return error_type;

}



ObjectType
Interpreter::UnaryVisit(Unary* unary_node)
{
    TokenType operator_type = unary_node->operator_->get_type();
    ObjectType opnum_value = Evaluate(unary_node->right_.get());
    switch (operator_type){
        case TokenType::BANG:{
            bool Truthy = !(GetTruthy(opnum_value));
            //!智能指针
            ObjectType new_value= Truthy;
            return new_value;
            break;
        }
        case TokenType::MINUS:{
            if(opnum_value.which() == 2){
                double double_opnum_value = boost::get<double>(opnum_value);
                double excute_result = -double_opnum_value;
                ObjectType new_value = excute_result;
                return new_value;
                break;
            }
            else
                throw ExcuteRuntimeErrorTrigger(RunTimeErrorType::OPERAND_TYEPE_ERROR, 
                                                    "Operand must be a number.",
                                                        unary_node->operator_.get() );
        }
        default:
            return ObjectType(boost::blank());

    } 
    return ObjectType(boost::blank());
}

ObjectType
Interpreter::GroupingVisit(Grouping* grouping_node)
{
    ObjectType grouping_value_reward = Evaluate(grouping_node->expression_.get());
    return grouping_value_reward;
}


ObjectType
Interpreter::BinaryVisit(Binary* binary_node)
{
    TokenType operator_type = binary_node->operator_->get_type();
    ObjectType left_opnum = Evaluate(binary_node->left_.get());
    ObjectType right_opnum = Evaluate(binary_node->right_.get());
    switch(operator_type)
    {
        case TokenType::MINUS:{
            if(left_opnum.which() == 2 && right_opnum.which() == 2){
                double excute_result = boost::get<double>(left_opnum) - boost::get<double>(right_opnum);
                return ObjectType(excute_result);
            }
            else 
                throw ExcuteRuntimeErrorTrigger(RunTimeErrorType::OPERAND_TYEPE_ERROR, 
                                                    "Operand must be a number.",
                                                    binary_node->operator_.get());
            break;
        }
        case TokenType::STAR:{
            if(left_opnum.which() == 2 && right_opnum.which() == 2){
                double excute_result = boost::get<double>(left_opnum) * boost::get<double>(right_opnum);
                return ObjectType(excute_result);
            }
            else 
                throw ExcuteRuntimeErrorTrigger(RunTimeErrorType::OPERAND_TYEPE_ERROR, 
                                                    "Operand must be a number.",
                                                    binary_node->operator_.get());
            break;
        }
        case TokenType::SLASH:{
            if(left_opnum.which() == 2 && right_opnum.which() == 2){
                double excute_result = boost::get<double>(left_opnum) / boost::get<double>(right_opnum);
                return ObjectType(excute_result);
            }
            else 
                throw ExcuteRuntimeErrorTrigger(RunTimeErrorType::OPERAND_TYEPE_ERROR, 
                                                    "Operand must be a number.",
                                                    binary_node->operator_.get());
            break;
        }
        case TokenType::PLUS:{
            if(left_opnum.which() == 2 && right_opnum.which() == 2){
                double excute_result = boost::get<double>(left_opnum) + boost::get<double>(right_opnum);
                return ObjectType(excute_result);
            }
            //bug here：这里找到一个bug，只判断了左边操作数是不是stringreward但是没有判断右边
            else if(left_opnum.which() == 1 && right_opnum.which() == 1){
                string excute_result = boost::get<string>(left_opnum) + boost::get<string>(right_opnum);
                return ObjectType(excute_result);
            }
            else
                throw ExcuteRuntimeErrorTrigger(RunTimeErrorType::OPERAND_TYEPE_ERROR, 
                                                    "Operands must be two numbers or two strings.",
                                                    binary_node->operator_.get());
            break;
        }
        case TokenType::GREATER_EQUAL:{
            if(left_opnum.which() == 2 && right_opnum.which() == 2){
                bool excute_result =  (boost::get<double>(left_opnum) >= boost::get<double>(right_opnum));
                return ObjectType(excute_result); 
            }
            if(left_opnum.which() == 1 && right_opnum.which() == 1){
                bool excute_result =  (boost::get<string>(left_opnum) >= boost::get<string>(right_opnum));
                return ObjectType(excute_result); 
            }
            else
                throw ExcuteRuntimeErrorTrigger(RunTimeErrorType::OPERAND_TYEPE_ERROR, 
                                                    "Operands must be two numbers or two strings.",
                                                    binary_node->operator_.get());
            break;
        }
        case TokenType::LESS_EQUAL:{
            if(left_opnum.which() == 2 && right_opnum.which() == 2){
                bool excute_result =  (boost::get<double>(left_opnum) <= boost::get<double>(right_opnum));
                return ObjectType(excute_result); 
            }
            if(left_opnum.which() == 1 && right_opnum.which() == 1){
                bool excute_result =  (boost::get<string>(left_opnum) <= boost::get<string>(right_opnum));
                return ObjectType(excute_result); 
            }
            else
                throw ExcuteRuntimeErrorTrigger(RunTimeErrorType::OPERAND_TYEPE_ERROR, 
                                                    "Operands must be two numbers or two strings.",
                                                    binary_node->operator_.get());
            break;
        }
        case TokenType::GREATER:{
            if(left_opnum.which() == 2 && right_opnum.which() == 2){
                bool excute_result =  (boost::get<double>(left_opnum) > boost::get<double>(right_opnum));
                return ObjectType(excute_result); 
            }
            if(left_opnum.which() == 1 && right_opnum.which() == 1){
                bool excute_result =  (boost::get<string>(left_opnum) > boost::get<string>(right_opnum));
                return ObjectType(excute_result); 
            }
            else
                throw ExcuteRuntimeErrorTrigger(RunTimeErrorType::OPERAND_TYEPE_ERROR, 
                                                    "Operands must be two numbers or two strings.",
                                                    binary_node->operator_.get());
            break;
        }
        case TokenType::LESS:{
            if(left_opnum.which() == 2 && right_opnum.which() == 2){
                bool excute_result =  (boost::get<double>(left_opnum) < boost::get<double>(right_opnum));
                return ObjectType(excute_result); 
            }
            if(left_opnum.which() == 1 && right_opnum.which() == 1){
                bool excute_result =  (boost::get<string>(left_opnum) < boost::get<string>(right_opnum));
                return ObjectType(excute_result); 
            }
            else
                throw ExcuteRuntimeErrorTrigger(RunTimeErrorType::OPERAND_TYEPE_ERROR, 
                                                    "Operands must be two numbers or two strings.",
                                                    binary_node->operator_.get());
            break;
        }
        case TokenType::EQUAL_EQUAL:{
            bool excute_result = IsEqual(left_opnum, right_opnum);
            return ObjectType(excute_result);
            break;
        }
        case TokenType::BANG_EQUAL:{
            bool excute_result = !(IsEqual(left_opnum, right_opnum));
            return ObjectType(excute_result);
            break;
        }
        default:
            return ObjectType(boost::blank());
    }
    return ObjectType(boost::blank());
}


ObjectType
Interpreter::PrintStatementVisit(PrintStatement* print_statement_node)
{
    ObjectType print_value = Evaluate(print_statement_node->expression_.get());
    if(print_value.which() != 0)
        cout << boost::apply_visitor(ToStringVisitor(), print_value) << endl;
    else
        cout << "nil" << endl;
    return ObjectType(boost::blank());
}

ObjectType
Interpreter::ExpressionStatementVisit(ExpressionStatement* expression_statement_node)
{
    ObjectType statement_value = Evaluate(expression_statement_node->expression_.get());
    return ObjectType(boost::blank()); 
}


void 
Environment::DefineVar(string symbol_str, ObjectType value)
{
    symbol_table[symbol_str] = value;
}

ObjectType
Environment::GetVarValue(shared_ptr<TokenBase> var_identifier )
{
    string symbol_str = var_identifier->get_lexme();
    if(symbol_table.find(symbol_str) != symbol_table.end()){
        return symbol_table[symbol_str];
    }
    if(enclosing != nullptr) return enclosing->GetVarValue(var_identifier);
    else
        throw ExcuteRuntimeErrorTrigger(Interpreter::RunTimeErrorType::UNDEFINED_VALUE, 
                                            "undefined variable. ",  
                                            var_identifier.get());
}

ObjectType
Environment::GetAt(int distance, string name)
{
    Environment* env = Ancestor(distance);
    return env->symbol_table[name];
}

Environment*
Environment::Ancestor(int distance)
{
    Environment* local_env = this;
    for(int i = 0; i < distance; i++){
        local_env = local_env->enclosing.get();
    }
    return local_env;
}

void 
Environment::VarAssign(shared_ptr<TokenBase> var_identifier, ObjectType value)
{
    string symbol_str = var_identifier->get_lexme();
    if(symbol_table.find(symbol_str) != symbol_table.end()){
        symbol_table[symbol_str] = value;
        return;
    }
    if(enclosing != nullptr) return enclosing->VarAssign(var_identifier, value);
    else 
        throw ExcuteRuntimeErrorTrigger(Interpreter::RunTimeErrorType::UNDEFINED_VALUE, 
                                            "undefined variable. ",  
                                            var_identifier.get()); 
}

ObjectType
Interpreter::VariableDeclarationStatementVisit(VariableDeclarationStatement* var_decl_node)
{
    ObjectType var_value;
    if(var_decl_node->var_expr_.get() != nullptr){
        var_value = Evaluate(var_decl_node->var_expr_.get());
    }
    string symbol_str = var_decl_node->var_identifier_->get_lexme();
    environment_->DefineVar(symbol_str, var_value);
    return ObjectType(boost::blank());
}

ObjectType
Interpreter::LookUpVariable(TokenPtr key)
{

    if(locals.find(key) != locals.end()){
        int distance = locals[key];
        return environment_->GetAt(distance, key->get_lexme());
    }
    else
        return global_env_->GetVarValue(key);
}

ObjectType
Interpreter::VariableExprVisit(VariableExpr* var_expr)
{
    shared_ptr<TokenBase> var_identifier = var_expr->var_identifier_;
    ObjectType value = LookUpVariable(var_identifier);
    return value;
}

ObjectType
Interpreter::ThisExprVisit(ThisExpr* node)
{
    return LookUpVariable(node->keyword_);
}

ObjectType
Interpreter::SuperExprVisit(SuperExpr* super_expr)
{
    //find the super_class object in the environment
    int distance = locals[super_expr->keyword_];
    shared_ptr<Function> value = boost::get<shared_ptr<Function>>(environment_->GetAt(distance, super_expr->keyword_->get_lexme()));
    shared_ptr<ClassObject> super_class = dynamic_pointer_cast<ClassObject>(value);
    
    //find the method in the super class
    ObjectType this_object = environment_->GetAt(distance - 1, "this");
    shared_ptr<UserDefineFunction> method =  super_class->find_method(super_expr->method_->get_lexme());
    //we need to bind the "this" object for the function object
    if(method != nullptr)
        return dynamic_pointer_cast<Function>(method->ThisBind(boost::get<shared_ptr<InstanceObject>>(this_object)));
    else
    //if don't find the method in the supper class interpreter should reporter
        throw ExcuteRuntimeErrorTrigger(Interpreter::RunTimeErrorType::UNDEFINED_VALUE,
            "Undefined property '" + super_expr->method_->get_lexme() + "'.", super_expr->keyword_.get());
        
}

void
Environment::AssignAt(int distance, string name, ObjectType value)
{
    Environment* env = Ancestor(distance);
    env->symbol_table[name] = value;
}

ObjectType
Interpreter::AssignmentExprVisit(AssignmentExpr* assign_expr)
{
    //bug is here var_expr = assign_expr->IsVaribleExpr();
    VariableExpr* var_expr = assign_expr->assign_var_->IsVaribleExpr();
    shared_ptr<TokenBase> var_identifier = var_expr->var_identifier_;
    ObjectType value = Evaluate(assign_expr->value_expr_.get());
    if(locals.find(var_identifier) != locals.end()){
        int distance = locals[var_identifier];
        environment_->AssignAt(distance, var_identifier->get_lexme(), value);
    }
    else
        global_env_->VarAssign(var_identifier, value);
    return value;

}

void
InstanceObject::InstanceSetter(TokenPtr name, ObjectType value){
    fields[name->get_lexme()] = value;
}




ObjectType
Interpreter::ObjectSetVisit(ObjectSet* set_expr)
{
    ObjectType object = Evaluate(set_expr->object_.get());
    ObjectType value = Evaluate(set_expr->value_.get());
    if(object.which() == 5){
        shared_ptr<InstanceObject> Instance = boost::get<shared_ptr<InstanceObject>> (object);
        Instance->InstanceSetter(set_expr->name_, value);
        return value;
    }
    else
        throw ExcuteRuntimeErrorTrigger(Interpreter::RunTimeErrorType::OBJECT_TYPE_ERROR,
                "Only instances have fields.", set_expr->name_.get());
}

ObjectType 
UserDefineFunction::Call(Interpreter* interpreter, vector<ObjectType> arguments)
{
    shared_ptr<Environment> environment = make_shared<Environment>(closure_);
    for(int i = 0; i < arguments.size(); i++){
        environment->DefineVar(func_->formal_parameter_[i]->get_lexme(), arguments[i]);
    }
    try{
        interpreter->ExcuteBlock(func_->function_body_, environment);
    }
    catch(shared_ptr<ReturnValue> value){
        if(is_initializer_ == true) return closure_->GetAt(0, "this");
        else
            return value->return_value;
    }
    if(is_initializer_ == true) return closure_->GetAt(0, "this");
    else
        return ObjectType(boost::blank());
}


//call the constructor
ObjectType
ClassObject::Call(Interpreter *interpreter, std::vector<ObjectType> args)
{
    shared_ptr<Function> func_ptr = boost::get<shared_ptr<Function>>(args.back());
    args.pop_back();
    shared_ptr<InstanceObject> new_instance = make_shared<InstanceObject>(func_ptr);
    UserDefineFunction* initializer = find_method("init").get();
    if(initializer != nullptr){
        initializer->ThisBind(new_instance)->Call(interpreter, args);
    }
    return ObjectType(new_instance);
}

ObjectType
Interpreter::CallExprVisit(CallExpr* call_expr)
{
    ObjectType callee = Evaluate(call_expr->callee_.get());
    vector<ObjectType> args_value;
    for(auto& i : call_expr->call_args_){
        //cout << "arg value: " << Evaluate(i)->ToString() << endl; 
        args_value.push_back(Evaluate(i.get()));
    }
    if(callee.which() != 4)
        throw ExcuteRuntimeErrorTrigger(RunTimeErrorType::NOT_CALLABLE, 
                                            "Can only call functions and class!", 
                                            call_expr->end_token_.get()); //检查是否为可调用对象
    shared_ptr<Function> func = boost::get<shared_ptr<Function>>(callee);
    if(args_value.size() != func->Arity()){
        throw ExcuteRuntimeErrorTrigger(RunTimeErrorType::TOO_MUCH_ARGS,
                                            "Expected " + to_string(func->Arity()) +
                                            " args but got " + to_string(args_value.size()) + ".", 
                                            call_expr->end_token_.get());//检查传入实参的数量是否和形参一致
    }
    if(dynamic_cast<ClassObject*>(func.get())){
        args_value.push_back(func);
    }
    return func->Call(this, args_value);
}

shared_ptr<UserDefineFunction>
UserDefineFunction::ThisBind(std::shared_ptr<InstanceObject> object){
    shared_ptr<Environment> new_env = make_shared<Environment>(this->closure_);
    new_env->DefineVar("this", ObjectType(object));
    shared_ptr<UserDefineFunction> new_func = make_shared<UserDefineFunction>(func_, name_, new_env, is_initializer_);
    return new_func;
}

ObjectType
InstanceObject::InstanceGetter(TokenPtr name, shared_ptr<InstanceObject> this_object)
{
    if(fields.find(name->get_lexme()) != fields.end()){
        return fields[name->get_lexme()];
    }
    shared_ptr<UserDefineFunction> method = dynamic_cast<ClassObject*>(kclass_.get())->find_method(name->get_lexme());
    //bind "this" point to the member function
    if(method != nullptr) 
//将instance绑定在成员函数的enviroenment
//如果需要在成员函数中访问数据成员必须通过this访问，因为数据成员并不在函数的environment中
        return dynamic_pointer_cast<Function>(method->ThisBind(this_object));
    else
        throw ExcuteRuntimeErrorTrigger(Interpreter::RunTimeErrorType::UNDEFINED_VALUE,
        "Undefined property '" + name->get_lexme() + "'.", name.get());    
}

ObjectType
Interpreter::ObjectGetVisit(ObjectGet* get_expr)
{
    ObjectType object = Evaluate(get_expr->object_.get());
    if(object.which() == 5){
        shared_ptr<InstanceObject> Instance = boost::get<shared_ptr<InstanceObject>> (object);
        return Instance->InstanceGetter(get_expr->name_, Instance);
    }
    else
        throw ExcuteRuntimeErrorTrigger(Interpreter::RunTimeErrorType::OBJECT_TYPE_ERROR,
                "Only instances have properties.", get_expr->name_.get());
}

ObjectType
Interpreter::LogicalVisit(Logical* logical_expr)
{
    TokenType operator_type = logical_expr->operator_->get_type();
    ObjectType left_opnum_reward = Evaluate(logical_expr->left_.get());
    if(operator_type == TokenType::OR){
        if(GetTruthy(left_opnum_reward)) return left_opnum_reward; 
    }
    else{
        if(!GetTruthy(left_opnum_reward)) return left_opnum_reward;
    }
    ObjectType right_opnum_reward = Evaluate(logical_expr->right_.get());
    return right_opnum_reward;
}



ObjectType
Interpreter::BlockStatementVisit(BlockStatement* block_statement)
{
    vector<StatementPtr>& statements = block_statement->inner_statements_;
    shared_ptr<Environment> block_env = make_shared<Environment>(environment_);
    ExcuteBlock(statements, block_env);
    return ObjectType(boost::blank());
}

void 
Interpreter::ExcuteBlock(vector<StatementPtr>& statements, shared_ptr<Environment> block_env)
{
    if(!statements.empty()){
        shared_ptr<Environment> prev_env = this->environment_;
        try{
            this->environment_ = block_env;
            for(auto& i : statements)
                Excute(i.get());
        }
        catch(shared_ptr<ReturnValue> value){
            this->environment_ = prev_env;
            throw value;
        }
        this->environment_ = prev_env;
    }
    return;
}

void 
Interpreter::ExcuteBlock(vector<std::shared_ptr<Statement>> statements, shared_ptr<Environment> block_env)
{
    if(!statements.empty()){
        shared_ptr<Environment> prev_env = this->environment_;
        try{
            this->environment_ = block_env;
            for(auto i : statements)
                Excute(i.get());
        }
        catch(shared_ptr<ReturnValue> value){
            this->environment_ = prev_env;
            throw value;
        }
        this->environment_ = prev_env;
    }
    return;
}


ObjectType
Interpreter::IfStatementVisit(IfStatement* if_statement)
{
    vector<ExprPtr>& if_conditions = if_statement->if_conditions_;
    vector<StatementPtr>& if_branches = if_statement->if_branches_;
    bool else_excute =  true;
    uint32_t branches_num = if_conditions.size();
    for(uint32_t i = 0; i < branches_num; i++){
        if(GetTruthy(Evaluate(if_conditions[i].get()))){
            Excute(if_branches[i].get());
            else_excute = false;
            break;
        }
    }
    if((else_excute == true) && (if_branches.size() > if_conditions.size()) )
        Excute(if_branches[branches_num].get()); 

    return ObjectType(boost::blank());
}

ObjectType
Interpreter::WhileStatementVisit(WhileStatement* while_statement)
{
    ExprPtr& condition_expr = while_statement->condition_;
    StatementPtr& body_statement = while_statement->body_;
    while(GetTruthy(Evaluate(condition_expr.get())))
        Excute(body_statement.get());
    return ObjectType(boost::blank());
}



ObjectType
Interpreter::FunctionDeclVisit(FunctionDecl* func_decl)
{
    auto args_num =  func_decl->formal_parameter_.size();
    string func_name = string("<fn ") + func_decl->func_identifier_->get_lexme() + ">";
    //不能用func_decl的指针创建unique_ptr来初始化UserDefineFunction
    //因为在AST中已经有一个unique_ptr来管理这个指针
    //这里再创建一个unique_ptr就会有两个unique_ptr管理同一个指针的情况发生
    //当这两个unique_ptr销毁时就会发生double free
    //use the raw pointer will produce a bug here,the bug can cause information leaking
    shared_ptr<FunctionDecl> new_ptr = shared_ptr<FunctionDecl>(new FunctionDecl(*func_decl));
    ObjectType reward = shared_ptr<Function>(new UserDefineFunction(new_ptr, func_name, environment_));
    
    //this place use environment not global_env
    environment_->DefineVar(func_decl->func_identifier_->get_lexme(), reward);
    return ObjectType(boost::blank());

}



ObjectType
Interpreter::ReturnStatementVisit(ReturnStatement* node)
{
    ReturnStatement* return_statement = node->IsReturnStatement();
    ObjectType return_value;
    if(return_statement->return_value_ != nullptr)
        return_value = Evaluate(return_statement->return_value_.get());
    throw shared_ptr<ReturnValue>(new ReturnValue(return_value));

}


ObjectType
Interpreter::ClassDeclVisit(ClassDecl* class_decl)
{
    map<string, shared_ptr<UserDefineFunction>> methods; 
    string class_name = "<class " + class_decl->name_->get_lexme() + ">";
    shared_ptr<ClassObject> superclass;
    //jude wether the super class is the ClassObject
    if(class_decl->superclass_ != nullptr){
        ObjectType object = Evaluate(class_decl->superclass_.get());
        if(object.which() == 4){
            shared_ptr<Function> super_object = boost::get<shared_ptr<Function>>(object);
            superclass = dynamic_pointer_cast<ClassObject>(super_object);
            if(superclass == nullptr)
                throw ExcuteRuntimeErrorTrigger(RunTimeErrorType::OBJECT_TYPE_ERROR, 
                        "Superclass must be a class.", class_decl->name_.get());
        }
        else
            throw ExcuteRuntimeErrorTrigger(RunTimeErrorType::OBJECT_TYPE_ERROR, 
                        "Superclass must be a class.", class_decl->name_.get());

    }
    environment_->DefineVar(class_decl->name_->get_lexme(), boost::blank());
    if(class_decl->superclass_ != nullptr){
        shared_ptr<Environment> new_env = make_shared<Environment>(environment_);
        new_env->DefineVar("super", dynamic_pointer_cast<Function>(superclass));
        environment_ = new_env;
    }

    for(auto i : class_decl->methods_){
        string method_name = string("<method ") + i->func_identifier_->get_lexme() + ">";
        shared_ptr<UserDefineFunction> method = make_shared<UserDefineFunction>(i, method_name, environment_, (i->func_identifier_->get_lexme() == "init" ));
        methods[i->func_identifier_->get_lexme()] = method;
    }
    
    shared_ptr<Function> object = shared_ptr<Function>(new ClassObject(class_name,superclass,methods));
    if(class_decl->superclass_ != nullptr)
        environment_ = environment_->enclosing;
    environment_->VarAssign(class_decl->name_, ObjectType(object));

    return boost::blank();
}

void
Interpreter::Interpret(vector<StatementPtr>& program)
{
    try{
        for(auto& i : program){
            Excute(i.get());
        }
    }
    catch(RunTimeErrorType error_type){
        return;
    }
}



