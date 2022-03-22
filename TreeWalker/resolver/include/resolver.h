#ifndef __RESOLVER_
#define __RESOLVER_

#include "../../helper/visitor.h"
#include "../../helper/Node.h"
#include "../../helper/reward.h"
#include <vector>
#include <stack>
#include <memory>
#include <map>
#include <string>
class Interpreter;

class Resolver : public VisitorBase
{
public:
    //use to check whether the usage of keyword "return"  is legal 
    enum FunctionType{
        NONE, FUNCTION,
        METHOD, INITIALIZER
    };
    //use to check whether the usage of keyword "this" and "super" is legal 
    enum ClassType{
        NOCLASS, CLASS,
        SUBCLASS
    };
    Resolver(Interpreter* interpreter)
                :interpreter_(interpreter){}
#define VISIT_FUNC_DECLARATION(node_type) ObjectType node_type##Visit(node_type* node ) override;
#define NO_FUNC(node_type)
    NODE_LIST(VISIT_FUNC_DECLARATION, NO_FUNC)
#undef VISIT_FUNC_DECLARATION
#undef NO_FUNC   
    void Resolve(StatementPtr& stmt){
        if(stmt == nullptr) return;
        stmt->Accept(this);
    }
    void Resolve(ExprPtr& expr){
        expr->Accept(this);
    }
    void Resolve(std::vector<StatementPtr>& statements){
        for(auto& i : statements){
            if(i == nullptr) continue;
            i->Accept(this);
        }
    }
    void Resolve(std::vector<std::shared_ptr<Statement>> statements){
        for(auto i : statements){
            if(i == nullptr) continue;
            i->Accept(this);
        }
    }
    void ScopesPush(std::map<std::string, bool> element){
        scopes_.push_back(element);
    }
    void ScopesPop(){
        scopes_.erase(scopes_.end() - 1);
    }
    void BeginScope(){
        ScopesPush(std::map<std::string, bool>()); 
    }
    void EndScope(){
        ScopesPop();
    }
    void Declare(TokenPtr name);
    void Define(TokenPtr name);
    void ResolverLocal(TokenPtr name, Expression* expr);
    void ResolveFunc(FunctionDecl* func_decl, FunctionType type);
private:
    FunctionType current_function_ = FunctionType::NONE;
    ClassType current_class_ = ClassType::NOCLASS;
    Interpreter* interpreter_;
    std::vector<std::map<std::string, bool>> scopes_;
};

#endif