#include "../helper/visitor.h"
#include "../helper/Node.h"

#define ACCEPT_FUNC_DEFINATION(node_type)           \
    Value                                     \
    node_type::Accept(VisitorBase* visitor)         \
    {                                               \
        return visitor->node_type##Visit(*this);     \
    }
#define NO_FUNC(node_type) 
NODE_LIST(ACCEPT_FUNC_DEFINATION, NO_FUNC)
#undef ACCEPT_FUNC_DEFINATION
#undef NO_FUNC



/*
V8的Node组织结构
Node -> Expression -> 衍生出的各种表达式 (binary, unary, )
Node -> statement -> 衍生的各种语句（IfStatement, ExprStatement, PrintStatement）
NODE_LIST宏主要用在 Node的前向声明， Node的Accept函数的定义， Visitor的不同Node类型的访问者函数的生命
以及Visitor派生类的访问者函数的虚函数的覆盖声明
*/