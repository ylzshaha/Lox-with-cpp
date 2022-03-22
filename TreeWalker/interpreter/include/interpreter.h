#ifndef __INTERPRETER_
#define __INTERPRETER_
#include "../../helper/Node.h"
#include "../../helper/visitor.h"
#include "../../helper/reward.h"
#include "builtin.h"
#include <algorithm>
#include <string>
#include <functional>
#include <vector>
#include <map>
#include <memory>
class Lox;

//访问者模式的Interpreter
class Interpreter : public VisitorBase
{
public:
    Interpreter() {
        global_env_ = std::make_shared<Environment>();
        environment_ = global_env_;
#define GLOBAL_BUILTIN_DEFINE(name, func_name)       \
    global_env_->DefineVar(name, std::shared_ptr<Function>(new BuiltinFunction(Builtin##func_name, 0, "<builtin func>")));
        BUILTIN_GROUP(GLOBAL_BUILTIN_DEFINE)
        }
    Interpreter(Lox* lox_arg):lox(lox_arg){
        global_env_ = std::make_shared<Environment>();
        environment_ = global_env_;
        BUILTIN_GROUP(GLOBAL_BUILTIN_DEFINE)
        }
#undef GLOBAL_BUILTIN_DEFINE
   enum RunTimeErrorType{
        OPERAND_TYEPE_ERROR, UNDEFINED_VALUE,
        TOO_MUCH_ARGS, NOT_CALLABLE,
        OBJECT_TYPE_ERROR
    };
    ObjectType Evaluate(Node* node){
        return node->Accept(this);
    }
    void Excute(Statement* node)
    {
        if(node == nullptr) return;
        node->Accept(this);
    }
    void Interpret(std::vector<StatementPtr>& program);
    void ExcuteBlock(std::vector<StatementPtr>& statements, std::shared_ptr<Environment> block_env);
    void ExcuteBlock(std::vector<std::shared_ptr<Statement>> statements, std::shared_ptr<Environment> block_env);
    ObjectType FunctionExcute(std::vector<ObjectType> arguments);
    void Resolve(TokenPtr var_token, int static_layer){
        locals[var_token] = static_layer;
    }
    ObjectType LookUpVariable(TokenPtr key);
    friend UserDefineFunction;
private:
    Lox* lox;
    std::shared_ptr<Environment> environment_;
    std::shared_ptr<Environment> global_env_;//全局的语法环境包含全局的符号表
    std::map<TokenPtr, int> locals;
#define VISIT_FUNC_OVERRIDE(node_type) ObjectType node_type##Visit(node_type* node) override;
#define NO_FUNC(node_type)
    NODE_LIST(VISIT_FUNC_OVERRIDE, NO_FUNC)
#undef VISIT_FUNC_OVERRIDE
#undef NO_FUNC
};


class ReturnValue
{
public:
    ReturnValue(ObjectType value): return_value(value){} 
    ObjectType return_value;
};

#endif