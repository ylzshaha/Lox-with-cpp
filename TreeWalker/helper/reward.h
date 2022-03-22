#ifndef __REWARD_
#define __REWARD_
#include <cstdint>
#include <ios>
#include <string>
#include <cmath>
#include <vector>
#include <functional>
#include <boost/variant.hpp>
#include <boost/blank.hpp>
#include <memory>
#include <map>
#include "Node.h"

class FunctionDecl;
class TokenBase;
class BlockStatement;
class Interpreter;
class Function;
class ClassObject;
class InstanceObject;
class FunctionDecl;
class Interpreter;

using ObjectType = boost::variant<boost::blank, std::string, double, bool, 
                std::shared_ptr<Function>, std::shared_ptr<InstanceObject>, long int>;
//variant中的类型的拷贝构造函数不能是删除的



class Environment
{
public:
    Environment():enclosing(){};
    Environment(std::shared_ptr<Environment> environment):enclosing(environment){};
    void DefineVar(std::string symbol_str, ObjectType value);
    ObjectType GetVarValue(std::shared_ptr<TokenBase> var_identifier);
    void VarAssign(std::shared_ptr<TokenBase> symbol_str, ObjectType value);
    ObjectType GetAt(int distance, std::string name);
    void AssignAt(int distance, std::string name, ObjectType value);
    Environment* Ancestor(int distance);
    friend Interpreter;
private:
    std::map<std::string, ObjectType> symbol_table;
    std::shared_ptr<Environment> enclosing;
};

class Function 
{
public:
    virtual uint32_t Arity() = 0;
    virtual ObjectType Call(Interpreter* interpreter, std::vector<ObjectType> args) = 0;
    virtual std::string ToString() = 0;
};


class BuiltinFunction : public Function
{
public:
    BuiltinFunction() = default;
    BuiltinFunction(std::function<ObjectType (Interpreter* ,std::vector<ObjectType>)> callee, 
                        uint32_t args_num, std::string type_str):
                        callee_(callee), args_num_(args_num), name_(type_str){}
                      
    uint32_t Arity() override  {return args_num_;}
    ObjectType Call (Interpreter* interpreter, std::vector<ObjectType> args) override {
        return callee_(interpreter, args);
    };
    std::string ToString() override {return name_;}
private:
    uint32_t args_num_ = 0;
    std::string name_;
    std::function<ObjectType (Interpreter* ,std::vector<ObjectType>)> callee_;
};


class UserDefineFunction : public Function
{
public:
    UserDefineFunction(std::shared_ptr<FunctionDecl> func, 
        std::string name, 
        std::shared_ptr<Environment> environment): 
        func_(func), name_(name), closure_(environment){}

    UserDefineFunction(std::shared_ptr<FunctionDecl> func, 
        std::string name, std::shared_ptr<Environment> environment, 
        bool is_initializer): 
        func_(func), name_(name), closure_(environment), is_initializer_(is_initializer){} 

    uint32_t Arity() override {return func_->formal_parameter_.size();}
    ObjectType Call(Interpreter* interpreter, std::vector<ObjectType> args) override;
    std::string ToString() override {return name_;}
    std::shared_ptr<UserDefineFunction> ThisBind(std::shared_ptr<InstanceObject> object);
private:
    std::shared_ptr<FunctionDecl> func_;
    std::string name_;
    std::shared_ptr<Environment> closure_;
    bool is_initializer_ = false;
};

class ClassObject : public Function
{
public:
    ClassObject(std::string name, 
        std::shared_ptr<ClassObject> superclass, 
        std::map<std::string,std::shared_ptr<UserDefineFunction>> methods):
        name_(name), superclass_(superclass), methods_(methods){}

    std::string ToString() override{
        return name_;
    }

    uint32_t Arity() override {
        UserDefineFunction* initializer = find_method("init").get();
        if(initializer != nullptr)
            return initializer->Arity();
        else
            return 0;
    }
    ObjectType Call(Interpreter* interpreter, std::vector<ObjectType> args) override;
    std::shared_ptr<UserDefineFunction> find_method(std::string name){
        if(methods_.find(name) != methods_.end()){
            return methods_[name];
        }
        else if(superclass_ != nullptr){
            return superclass_->find_method(name);
        }
        else
            return nullptr;
    }
private:
    std::string name_;
    std::map<std::string, std::shared_ptr<UserDefineFunction>> methods_;
    std::shared_ptr<ClassObject> superclass_;
};

class InstanceObject
{
public:
    InstanceObject(std::shared_ptr<Function> kclass):
        kclass_(kclass){}
    std::string ToString() {return"[" +  kclass_->ToString() + " Instance" + "]";}
    ObjectType InstanceGetter(TokenPtr name, std::shared_ptr<InstanceObject> this_object);
    void InstanceSetter(TokenPtr name, ObjectType value);
private:
    std::shared_ptr<Function> kclass_;
    std::map<std::string, ObjectType> fields;
};


class ToStringVisitor : public boost::static_visitor<std::string>
{
public:
    std::string operator() (std::string& value) const {
        return value;
    }  
    std::string operator() (double& value) const{
        double integer_part = 1;
        if(std::modf(value, &integer_part) == 0){
            return std::to_string((int)value);
        }
        else{
            std::string str = std::to_string(value);
            return str.erase ( str.find_last_not_of('0') + 1, std::string::npos );
        }
    }
    std::string operator() (bool& value) const{
        if(value == true)
            return "true";
        else
            return "false";
    }
    std::string operator() (std::shared_ptr<Function> value) const {
        return value->ToString();
    }
    std::string operator() (boost::blank) const {
        return "nil";
    }
    std::string operator() (long int) const {
        return "long int !!!!!!!!";
    }
    std::string operator() (std::shared_ptr<InstanceObject> value) const {
        return value->ToString();
    }

};

#endif