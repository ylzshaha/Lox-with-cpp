#ifndef __PRINT_AST_
#define __PRINT_AST_
#include "../../helper/visitor.h"
#include "../../helper/Node.h"
#include "string"
#include <initializer_list>
#include <memory>

class RewardBase;
class PrintAstVisitor : public VisitorBase
{
public:
    PrintAstVisitor() = default;
    std::string PrintAst(Node* expr);
#define VISIT_FUNC_OVERRIDE(node_type) Value node_type##Visit(const node_type& node) override;
#define NO_FUNC(node_type)
    NODE_LIST(VISIT_FUNC_OVERRIDE, NO_FUNC)
#undef VISIT_FUNC_OVERRIDE
#undef NO_FUNC
private:
    using ExpressionWrapper = std::reference_wrapper<ExprPtr>;
    using StatementWrapper = std::reference_wrapper<StatementPtr>;
    uint32_t block_layer = 0;
    std::string ExprAstToString(std::string lexme,  std::vector<Expression*>child);
    std::string BlockTraverse(const std::vector<StatementPtr>& statements);
    std::string BlockTraverse (const std::vector<std::shared_ptr<Statement>> statements);
};

#endif