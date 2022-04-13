#ifndef __VISITOR_
#define __VISITOR_
#include "Node.h"

class RewardBase;
class Node;
class VisitorBase
{
public:
#define VISIT_FUNC_DECLARATION(node_type) \
    virtual Value node_type##Visit(const node_type& node ) = 0;   \

#define NO_FUNC(node_type)
    NODE_LIST(VISIT_FUNC_DECLARATION, NO_FUNC)
#undef VISIT_FUNC_DECLARATION
#undef NO_FUNC
};
#endif