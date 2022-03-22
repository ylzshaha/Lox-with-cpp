#ifndef __COMPLIER_
#define __COMPLIER_

#include "../../helper/visitor.h"
#include "../../helper/bytecode.h"
#include "../../helper/object.h"
#include "../../scanner/include/Token.h"
#include <boost/blank.hpp>
#include <cstdint>
#include <memory>
#include <vector>
#include <map>


enum CompilingError
{
    CONSTANT_OOB, TOO_MUCH_LOCAL,
    VAR_REDEFINITION, SELF_INITIALIZED,
    JUMP_TOO_FAR, TOO_MUCH_PARAMTER,
    TOO_MUCH_ARGUMENT, RETURN_AT_TOP,
    THIS_ABUSE, INITIALIZER_RETURN,
    INHERIT_SELF, INVILID_SUPER,
    SUPER_OUT_CLASS
};

class Local
{
public:
    TokenPtr name;
    int depth = -1;
    bool is_captured = false;
};

using LocalPtr = std::unique_ptr<Local>;


class UpValue
{
public:
    UpValue() = default;
    UpValue(uint8_t idx, bool flag):
        index(idx), is_local(flag){}
    uint8_t index = 0;
    bool is_local;
};
class ClassCompiler
{
public:
    ClassCompiler* enclosing = nullptr;
    bool has_super_class = false;
};

class Compiler : public VisitorBase
{
public:
    Compiler(FunctionType type):
        type_(type){
            function_ = nullptr;
            void* raw_ptr = (ObjFuncPtr)MemoryAllocater(sizeof(ObjFunction));
            function_ = new (raw_ptr) ObjFunction();
            vm.PushStack(function_);
            //every stack frame has a closure ptr at the bottom slot
            //where the method saves the this instance
            //every function have a new locals 
            LocalPtr local = LocalPtr(new Local());
            local->depth = 0;
            local->is_captured = false;
            //if the functionc is a method ,the stack base will be changed to this instacne
            if(type_ == FunctionType::TYPE_METHOD || type_ == FunctionType::TYPE_INITIALIZER){
                local->name = make_shared<Token<boost::blank>>(TokenType::IDENTIFIER, std::string("this"), boost::blank(), 0);
            }
            else
                local->name = make_shared<Token<boost::blank>>(TokenType::IDENTIFIER, std::string(), boost::blank(), 0);
            locals_.push_back(move(local));
            local_count_++;
        }
    void ExprCompling(ExprPtr& expr){expr->Accept(this);}
    void StatementCompling(const StatementPtr &statement){statement->Accept(this);}
    ObjFuncPtr Compiling(const std::vector<StatementPtr>& program);
    void DefineVariable(std::uint32_t global, std::uint32_t line);
    void NamedVariable(TokenPtr token, std::uint32_t line, bool flag);
    void BeginScope() {scope_depth++;}
    void EndScope(std::uint32_t line);
    void AddLocal(TokenPtr token);
    void DeclareVariable(TokenPtr token);
    std::uint32_t ResolveLocal(TokenPtr token, uint32_t line);
    int ResolveUpValue(TokenPtr token, uint32_t line);
    void MarkInitialized();
    ChunkPtr GetCurrentChunk() {return function_->chunk;}
    void FunctionCompiling(const FunctionDecl& func_decl, FunctionType type, std::uint32_t line);
    friend class Collector;
private:
    ObjFuncPtr function_;
    FunctionType type_;
    //just like the simulation of stack
    std::vector<LocalPtr> locals_;
    std::uint32_t local_count_ = 0;
    int scope_depth = 0;
    //representation of the nesting relationship of function 
    Compiler* enclosing = nullptr;
    std::vector<UpValue> up_values_;
    bool is_initialized = false;
    void EmitByte(std::uint8_t byte, std::uint32_t line) {GetCurrentChunk()->WriteChunk(byte, line);}
    void EmitBytes(std::uint8_t byte_1, std::uint8_t byte_2, uint32_t line){
        EmitByte(byte_1, line);
        EmitByte(byte_2, line);
    }
    void EmitAddress(std::uint32_t addr, uint32_t line){
        uint32_t num = addr;
        for(int i = 0; i < 4; i++){
            EmitByte(num & 0xff, line);
            num = num >> 8;
        }
    }
    int EmitJump(std::uint8_t byte, std::uint32_t line);
    void EmitLoop(std::uint32_t loop_statr, std::uint32_t line);
    void EmitReturn(std::uint32_t line);
    void PatchJump(std::uint16_t offset, std::uint32_t line);
    void EmitConstant(Value value, std::uint32_t line );
    std::uint32_t ArguementList(const std::vector<ExprPtr>& list ,std::uint32_t line); 
    std::uint32_t VariableAnalysis(TokenPtr name, std::uint32_t line);
    std::uint32_t GetGlobalIndex(TokenPtr token, std::uint32_t line);
    void CreateMethod(const FunctionDecl& methods, uint32_t line);
    int AddUpValue(int index, bool is_local);
    std::map<std::string, int> global_var_map_;
#define VISIT_FUNC_OVERRIDE(node_type) Value node_type##Visit(const node_type& node) override;
#define NO_FUNC(node_type)
    NODE_LIST(VISIT_FUNC_OVERRIDE, NO_FUNC)
#undef VISIT_FUNC_OVERRIDE
#undef NO_FUNC
};


#endif