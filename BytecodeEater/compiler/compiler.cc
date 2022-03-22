#include "include/compiler.h"
#include "../scanner/include/ErrorReporter.h"
#include "../scanner/include/Token.h"
#include "../helper/object.h"
#include <boost/blank.hpp>
#include <boost/variant/get.hpp>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

using namespace std;

ClassCompiler* current_class = nullptr;

TokenPtr
SyntheticToken(const char* name)
{
    return make_shared<Token<boost::blank>>(TokenType::IDENTIFIER, std::string(name), boost::blank(), 0);
}

/*
#define EMIT_LONG_CONSTANT(bytes)           \
do{                                         \
    for(int i = 0; i < 3; i++){             \
        EmitByte(bytes & 0xff, line);       \
        bytes = bytes >> 0x8;               \
    }                                       \
}while(false)
*/


void
Compiler::EmitReturn(uint32_t line)
{
    if(type_ == FunctionType::TYPE_INITIALIZER){
        EmitByte(OpCode::OP_GET_LOCAL, line);
        EmitAddress(0, line);
    }
    else
        EmitByte(OpCode::OP_NIL, line);
    EmitByte(OpCode::OP_RETURN, line);
}

CompilingError 
CompilerErrorTrigger(string messgae, uint32_t line, CompilingError type)
{
    ErrorReporter::error(line, messgae);
    return type;
}

uint32_t
Chunk::WriteConstant(Value value, uint32_t line)
{
    if(constant_.size() >= 0xffffffff)
        throw CompilerErrorTrigger("Too much constant!",
            line, CompilingError::CONSTANT_OOB);
    constant_.push_back(value);
    return constant_.size() - 1;
}

void
Compiler::EmitConstant(Value value, uint32_t line)
{
    uint32_t bytes = function_->chunk->WriteConstant(value, line);
    /*
    if(bytes > 0xff){
        EmitByte(OpCode::OP_CONSTANT_LONG, line);
        EMIT_LONG_CONSTANT(bytes);
    }
    */
    EmitByte(OpCode::OP_CONSTANT, line);
    EmitAddress(bytes, line);        
}


Value
Compiler::LiteralVisit(const Literal& literal_node)
{
    Value value = literal_node.literal_;
    uint32_t line = literal_node.line_;

    switch(value.which()){
        case 1:{
            double num_value = boost::get<double>(value);
            EmitConstant(num_value, line);
            break;
        }
        case 0:{
            EmitByte(OpCode::OP_NIL, line);
            break;
        }
        case 2:{
            bool bool_value = boost::get<bool>(value);
            if(bool_value == false)
                EmitByte(OpCode::OP_FALSE, line);
            else
                EmitByte(OpCode::OP_TRUE, line);
            break;
        }
        case 4:{
            string str = boost::get<string>(value);
            ObjPtr object = CreateObjString(str);
            vm.PushStack(object);
            EmitConstant(object, line);
            vm.PopStack();
        }
        default:
            return boost::blank();
    }
    return boost::blank();
}



Value
Compiler::UnaryVisit(const Unary& unary_node)
{
    uint32_t line = unary_node.operator_->get_line();
    TokenType operator_type = unary_node.operator_->get_type();
    unary_node.right_->Accept(this);
    switch (operator_type) {
    case TokenType::MINUS:{EmitByte(OpCode::OP_NEGATE , line); break;}
    case TokenType::BANG:{EmitByte(OpCode::OP_NOT, line); break;}
    default:
        return boost::blank();
    }
    return boost::blank();
}

Value
Compiler::GroupingVisit(const Grouping& grouping_node)
{
    grouping_node.expression_->Accept(this);
    return boost::blank();
}


Value
Compiler::BinaryVisit(const Binary& binary_node)
{
    uint32_t line = binary_node.operator_->get_line();
    TokenType operator_type = binary_node.operator_->get_type();
    binary_node.left_->Accept(this);
    binary_node.right_->Accept(this);
    switch(operator_type) {
        case TokenType::PLUS:{EmitByte(OpCode::OP_ADD, line); break;}
        case TokenType::MINUS:{EmitByte(OpCode::OP_SUB, line); break;}
        case TokenType::STAR:{EmitByte(OpCode::OP_MUL, line); break;}
        case TokenType::SLASH:{EmitByte(OpCode::OP_DIV, line); break;}
        case TokenType::LESS:{EmitByte(OpCode::OP_LESS, line);break;}
        case TokenType::GREATER:{EmitByte(OpCode::OP_GREATER, line);break;}
        case TokenType::LESS_EQUAL:{EmitByte(OpCode::OP_LESS_EQUAL, line);break;}
        case TokenType::EQUAL_EQUAL:{EmitByte(OpCode::OP_EQUAL, line);break;}
        case TokenType::GREATER_EQUAL:{EmitByte(OpCode::OP_GREATER_EQUAL, line);break;}
        case TokenType::BANG_EQUAL:{EmitByte(OpCode::OP_BANG_EQUAL, line);break;}
        default:
            return boost::blank(); 
    }
    return boost::blank();
}


Value
Compiler::PrintStatementVisit(const PrintStatement& print_statement_node)
{
    print_statement_node.expression_->Accept(this);
    uint32_t line = print_statement_node.print_token_->get_line();
    EmitByte(OpCode::OP_PRINT, line);
    return boost::blank();
}

Value
Compiler::ExpressionStatementVisit(const ExpressionStatement& expression_statement_node)
{
    expression_statement_node.expression_->Accept(this);
    uint32_t line = expression_statement_node.token_->get_line();
    EmitByte(OP_POP,line);
    return boost::blank();
}

uint32_t
Compiler::GetGlobalIndex(TokenPtr name, uint32_t line)
{
    //all the global variable name string are saved in the constant area
    //and the global_var_map is hash table about the name and name's  index in constant area
    //this function help us to get the index 
    string str = name->get_lexme();
    if(global_var_map_.find(str) != global_var_map_.end())
        return global_var_map_[str];
    else{
        //if the string is not in cache
        //write it to constant array and the address  to cache
        uint32_t result = function_->chunk->WriteConstant(CreateObjString(str), line);
        global_var_map_[str] = result;
        return result;
    }
}

uint32_t
Compiler::VariableAnalysis(TokenPtr name, uint32_t line)
{
    //declaration
    DeclareVariable(name);
    //local variable directly return 
    if(scope_depth > 0) return 0;
    //if global variable find the index of symbol in constant array
    return GetGlobalIndex(name, line);
}

void
Compiler::AddLocal(TokenPtr token)
{
    //if there is no duplicate name variable
    //we declare a variabl in locals and set it state to undefined
    if(local_count_ >= 0xff)
        throw CompilerErrorTrigger("Too many local variables in function.", 
            token->get_line(), CompilingError::TOO_MUCH_LOCAL);
    LocalPtr local_variable = LocalPtr(new Local());
    local_variable->name = token;
    local_variable->depth = -1;
    locals_.push_back(move(local_variable));
    local_count_++;
}

void 
Compiler::DeclareVariable(TokenPtr token)
{
    //if this is a global variable directly return 
    if(scope_depth == 0) return;
    for(int i = local_count_ - 1; i > 0; i--){
        const Local local = *locals_[i];
        //check weather there is a same name variable in the same scope 
        //only for the local variable
        if(local.depth != -1 && local.depth < scope_depth)
            break;
        if(local.name->get_lexme() == token->get_lexme())
            throw CompilerErrorTrigger("Already a variable with this name in this scope.", 
                token->get_line(), CompilingError::VAR_REDEFINITION);
    }
    AddLocal(token);
}

void
Compiler::DefineVariable(std::uint32_t bytes, uint32_t line)
{
    //local variabl define have no bytecode to create
    if(scope_depth > 0){
        MarkInitialized();
        return;
    }
    else if(scope_depth == 0 && bytes <= 0xffffffff){
        EmitByte(OpCode::OP_DEFINE_GLOBAL, line);
        EmitAddress(bytes, line);
    }
    /*
    else {
        EmitByte(OpCode::OP_DEFINE_GLOBAL_LONG, line);
        EMIT_LONG_CONSTANT(bytes);
    }
    */
}

void
Compiler::MarkInitialized()
{
    if(scope_depth == 0) return;
    locals_[local_count_ - 1]->depth = scope_depth;
}

Value
Compiler::VariableDeclarationStatementVisit(const VariableDeclarationStatement& var_decl_node)
{
    uint32_t line = var_decl_node.var_identifier_->get_line();
    TokenPtr name = var_decl_node.var_identifier_;
    //declare the variable
    uint32_t var_index = VariableAnalysis(name, line);
    //compile the initialization Expr
    if(var_decl_node.var_expr_ != nullptr)
        var_decl_node.var_expr_->Accept(this);
    else
        EmitByte(OpCode::OP_NIL, line);
    //define the variable
    DefineVariable(var_index, line);
    return boost::blank();
}

uint32_t
Compiler::ResolveLocal(TokenPtr token, uint32_t line)
{
    for(int i = local_count_ - 1; i >= 0; i--){
        const Local local = *locals_[i];
        if(local.name->get_lexme() == token->get_lexme()){
            if(local.depth == -1)
                throw CompilerErrorTrigger("Can't read local variable in its own initializer.", 
                    line, CompilingError::SELF_INITIALIZED);
            return i;
        }
    }
    return -1;
}


int
Compiler::AddUpValue(int index, bool is_local)
{
    UpValue new_value = UpValue(index, is_local);
    up_values_.push_back(new_value);
    //get up_value idx
    return function_->up_value_count++;
}

int
Compiler::ResolveUpValue(TokenPtr token, uint32_t line)
{
    if(enclosing == nullptr) return -1;
    //if find in the enclosing function's local area 
    int local_index = enclosing->ResolveLocal(token, line);
    if(local_index != -1){
        enclosing->locals_[local_index]->is_captured = true;
        return AddUpValue(local_index, true);
    }
    //if find in the enclosing function's upvalue area
    int up_index = enclosing->ResolveUpValue(token, line);
    if(up_index != -1) return AddUpValue(up_index, false);
    return -1;
}

void
Compiler::NamedVariable(TokenPtr token, std::uint32_t line, bool flag)
{
    //flag == true get
    //flag == false set
    uint8_t set_op, get_op;
    uint32_t global;
    int reaction = ResolveLocal(token, line);
    if(reaction != -1){
        set_op = OpCode::OP_SET_LOCAL;
        get_op = OpCode::OP_GET_LOCAL;
    }
    else if((reaction = ResolveUpValue(token, line)) != -1){
        set_op = OpCode::OP_SET_UPVALUE;
        get_op = OpCode::OP_GET_UPVALUE;
    }
    else{
        reaction = GetGlobalIndex(token, line);
        set_op = OpCode::OP_SET_GLOBAL;
        get_op = OpCode::OP_GET_GLOBAL;
    }

    if(flag)
        EmitByte(get_op, line);
    else
        EmitByte(set_op, line);
    EmitAddress(reaction, line);
    
}

Value
Compiler::VariableExprVisit(const VariableExpr& var_expr)
{
    TokenPtr name = var_expr.var_identifier_;
    uint32_t line = var_expr.var_identifier_->get_line();
    NamedVariable(name, line, true);
    return boost::blank();
}

Value
Compiler::ThisExprVisit(const ThisExpr& node)
{
    uint32_t line = node.keyword_->get_line();
    if(current_class == nullptr)
        CompilerErrorTrigger("Can't use 'this' outside of a class.", 
            line, CompilingError::THIS_ABUSE);
    TokenPtr name = node.keyword_;
    NamedVariable(name, line, true);
    return boost::blank();
}

Value
Compiler::SuperExprVisit(const SuperExpr& super_expr)
{
    uint32_t line = super_expr.keyword_->get_line();
    if(!current_class->has_super_class)
        throw CompilerErrorTrigger("Can't use 'super' in a class with no superclass.", 
            line, CompilingError::INVILID_SUPER);
    else if(current_class == nullptr)
        throw CompilerErrorTrigger("Can't use 'super' outside of a class.", 
            line, CompilingError::SUPER_OUT_CLASS);
    string name = super_expr.keyword_->get_lexme();
    uint32_t name_offset = function_->chunk->WriteConstant(CreateObjString(name), line);
    //get this at stack base
    NamedVariable(SyntheticToken("this"), line, true);
    //get superclass
    NamedVariable(super_expr.keyword_, line, true);
    EmitByte(OpCode::OP_GET_SUPER, line);
    EmitAddress(name_offset, line);
    return boost::blank();
        
}


Value
Compiler::AssignmentExprVisit(const AssignmentExpr& assign_expr)
{
    TokenPtr var_token = assign_expr.assign_var_->IsVaribleExpr()->var_identifier_;
    uint32_t line = var_token->get_line();

    //evaluate the right-hand side value of the assignment statement
    if(assign_expr.value_expr_ != nullptr)
        assign_expr.value_expr_->Accept(this);
    else
        EmitByte(OpCode::OP_NIL, line);
    //produce the bytecode 
    NamedVariable(var_token, line, false);
    return boost::blank();

}

Value
Compiler::ObjectSetVisit(const ObjectSet& set_expr)
{
    uint32_t line = set_expr.name_->get_line();
    string name = set_expr.name_->get_lexme();
    // instance object get 
    set_expr.object_->Accept(this);
    //write the  member name to the constant area
    uint32_t offset = function_->chunk->WriteConstant(CreateObjString(name), line);
    //caculate the set value
    set_expr.value_->Accept(this);
    //emit the bytecode
    EmitByte(OpCode::OP_SET_PROPERTY, line);
    EmitAddress(offset, line);
    return boost::blank();
}


uint32_t
Compiler::ArguementList(const std::vector<ExprPtr>& list ,std::uint32_t line)
{
    uint32_t arg_count = 0;
    for(int i = 0; i < list.size(); i++){
        list[i]->Accept(this);
        arg_count++;
        if(arg_count == 255)
            throw CompilerErrorTrigger("Can't have more than 255 arguments.", 
                line, CompilingError::TOO_MUCH_ARGUMENT);
    }
    return arg_count;
}

//get the  function object from the global varible table or stack
//and load the argument onto the stack with the correct layout
Value
Compiler::CallExprVisit(const CallExpr& call_expr)
{
    
    uint32_t line = call_expr.end_token_->get_line();
    if(call_expr.callee_->IsVaribleExpr()){
        //now the ObjFunction is in the top of the stack
        call_expr.callee_->Accept(this);
        uint32_t args_count =  ArguementList(call_expr.call_args_, line);
        //emit the call opcode
        //it will build the call frame and call the function
        EmitBytes(OpCode::OP_CALL, args_count, line);
        return boost::blank();
    }
    else if(call_expr.callee_->IsObjectGet()){
        //put the instance object to the stack base
        call_expr.callee_->IsObjectGet()->object_->Accept(this);
        uint32_t args_count =  ArguementList(call_expr.call_args_, line);
        string name = call_expr.callee_->IsObjectGet()->name_->get_lexme();
        uint32_t offset = function_->chunk->WriteConstant(CreateObjString(name), line);
        EmitByte(OpCode::OP_INVOKE, line);
        EmitAddress(offset, line);
        EmitByte(args_count,line);
        return boost::blank();
    }
    else if(call_expr.callee_->IsSuperExpr()){
        const SuperExpr& super_expr = *(call_expr.callee_->IsSuperExpr());
        uint32_t line = super_expr.keyword_->get_line();
        if(!current_class->has_super_class)
            throw CompilerErrorTrigger("Can't use 'super' in a class with no superclass.", 
                line, CompilingError::INVILID_SUPER);
        else if(current_class == nullptr)
            throw CompilerErrorTrigger("Can't use 'super' outside of a class.", 
                line, CompilingError::SUPER_OUT_CLASS);
        string name = super_expr.method_->get_lexme();
        uint32_t name_offset = function_->chunk->WriteConstant(CreateObjString(name), line);
        //get this at stack base
        NamedVariable(SyntheticToken("this"), line, true);
        uint32_t args_count = ArguementList(call_expr.call_args_, line);
        //get superclass
        NamedVariable(SyntheticToken("super"), line, true);
        EmitByte(OpCode::OP_SUPER_INVOKE, line);
        EmitAddress(name_offset, line);
        EmitByte(args_count, line);
    }
    return boost::blank();
}




Value
Compiler::ObjectGetVisit(const ObjectGet& get_expr)
{
    string name = get_expr.name_->get_lexme();
    uint32_t line = get_expr.name_->get_line();
    //get object on stack
    get_expr.object_->Accept(this);
    uint32_t offset = function_->chunk->WriteConstant(CreateObjString(name), line);
    EmitByte(OpCode::OP_GET_PROPERTY, line);
    EmitAddress(offset, line);
    return boost::blank();
}

Value
Compiler::LogicalVisit(const Logical& logical_expr)
{
    uint32_t line = logical_expr.operator_->get_line();
    logical_expr.left_->Accept(this);
    if(logical_expr.operator_->get_type() == TokenType::AND){
        uint32_t offset =  EmitJump(OpCode::OP_JUMP_IF_FALSE, line);
        EmitByte(OpCode::OP_POP, line);
        logical_expr.right_->Accept(this);
        PatchJump(offset, line);
    }
    else{
        uint32_t else_jump = EmitJump(OpCode::OP_JUMP_IF_FALSE, line);
        uint32_t short_jump = EmitJump(OpCode::OP_JUMP, line);
        PatchJump(else_jump, line);
        EmitByte(OpCode::OP_POP, line);
        logical_expr.right_->Accept(this);
        PatchJump(short_jump, line);
    }
    return boost::blank();
}


void
Compiler::EndScope(uint32_t line)
{
    scope_depth--;
    while ((local_count_ > 0) && (locals_[local_count_ - 1]->depth > scope_depth)) {
        if(locals_[local_count_ - 1]->is_captured == true)
            EmitByte(OpCode::OP_CLOSE_UPVALUE, line);
        else
            EmitByte(OpCode::OP_POP, line);
        locals_.pop_back();
        local_count_--;
    }
}


Value
Compiler::BlockStatementVisit(const BlockStatement& block_statement)
{
    uint32_t line = block_statement.token_->get_line();
    BeginScope();
    const vector<StatementPtr>& statements = block_statement.inner_statements_;
    for(int i = 0; i < statements.size(); i++)
        statements[i]->Accept(this);
    EndScope(line);
    return boost::blank();
}

int
Compiler::EmitJump(uint8_t byte, uint32_t line)
{
    EmitByte(byte, line);
    EmitBytes(0xff, 0xff, line);
    return function_->chunk->code_.size() - 2;
}

void
Compiler::PatchJump(uint16_t offset, uint32_t line)
{
    //-2 to adjust to cross the two byte jump offset
    int jump = function_->chunk->code_.size() - offset -2;
    if(jump > 0xffff) throw CompilerErrorTrigger("Too much code to jump over.", 
        line, CompilingError::JUMP_TOO_FAR);
    function_->chunk->code_[offset] = jump & 0xff;
    function_->chunk->code_[offset + 1] = (jump >> 0x8) & 0xff; 
}

Value
Compiler::IfStatementVisit(const IfStatement& if_statement)
{
    uint32_t line = if_statement.token_->get_line();
    const vector<ExprPtr>& if_condition = if_statement.if_conditions_;
    const vector<StatementPtr>& if_branch = if_statement.if_branches_;
    vector<uint32_t> else_offset;
    uint32_t offset;
    for(int i = 0; i < if_condition.size(); i++){
        if_condition[i]->Accept(this);
        //if condition is false jump to the next condition
        offset = EmitJump(OpCode::OP_JUMP_IF_FALSE, line);
        //if condition is true continue to excute the branch
        EmitByte(OpCode::OP_POP, line);
        if(if_branch[i] != nullptr)
            if_branch[i]->Accept(this);
        //End the cycle after the excution of body 
        else_offset.push_back(EmitJump(OpCode::OP_JUMP, line));
        PatchJump(offset, line);
        EmitByte(OpCode::OP_POP, line);
    }
    if(if_branch.size() > if_condition.size()){
        if(if_branch.back() != nullptr)
            if_branch.back()->Accept(this);
    }
    for(auto i : else_offset)
        PatchJump(i, line);
    return boost::blank();
}

void
Compiler::EmitLoop(uint32_t loop_start, uint32_t line)
{
    EmitByte(OpCode::OP_LOOP, line);
    uint16_t jump_offset = function_->chunk->code_.size() - loop_start + 2;
    if(jump_offset > 0xffff)
        throw CompilerErrorTrigger("Loop body too large.",
            line, CompilingError::JUMP_TOO_FAR);
    EmitByte(jump_offset & 0xff, line);
    EmitByte((jump_offset >> 8) & 0xff, line);

}

Value
Compiler::WhileStatementVisit(const WhileStatement& while_statement)
{
    uint32_t line = while_statement.token_->get_line();
    //jump back to condition
    uint32_t loop_start = function_->chunk->code_.size();
    while_statement.condition_->Accept(this);
    uint32_t exit_jump =  EmitJump(OpCode::OP_JUMP_IF_FALSE, line);
    EmitByte(OpCode::OP_POP, line);
    if(while_statement.body_ != nullptr)
        while_statement.body_->Accept(this);
    EmitLoop(loop_start, line);
    //if condition is false jump over the body
    PatchJump(exit_jump, line);
    EmitByte(OpCode::OP_POP, line);
    return boost::blank();
}

extern Compiler* current;

void
Compiler::FunctionCompiling(const FunctionDecl& func_decl,FunctionType type, std::uint32_t line)
{
    //all use the new func and the new compiler
    // a ObjFunction is created
    Compiler compiler(type);
    current = &compiler;
    compiler.enclosing = this;
    ObjString* function_name = CreateObjString(func_decl.func_identifier_->get_lexme());
    compiler.function_->name = function_name;
    compiler.function_->line = func_decl.func_identifier_->get_line();
    compiler.BeginScope();
    //compiling the paramter
    for(auto i : func_decl.formal_parameter_){
        compiler.function_->arity++;
        if(compiler.function_->arity > 255)
            throw CompilerErrorTrigger("Can't have more than 255 parameters.", 
                line, CompilingError::TOO_MUCH_PARAMTER);
        //simulate the paramter in the new function chunk's locals    
        uint32_t var_idx = compiler.VariableAnalysis(i, line);
        compiler.DefineVariable(var_idx, line);
    }
    //compiling the body 
    ObjFuncPtr new_func = compiler.Compiling(func_decl.function_body_);
    //closure need 
    uint32_t offset = function_->chunk->WriteConstant(new_func, line);
    EmitByte(OpCode::OP_CLOSURE, line);
    EmitAddress(offset, line);
    for(int i = 0; i < new_func->up_value_count; i++){
        EmitByte(compiler.up_values_[i].is_local ? 1 : 0, line);
        EmitAddress(compiler.up_values_[i].index, line);
    }
    //treat the function as the constant 
    //we put the function to constant area when we finish the compilation
    //the next instruction can get the function from constan area and puts it to stack
    //EmitConstant(new_func, line);
}


//the main purpose of compiling the function decl is
// to compile the function body and make the variable can find the right value in the stack (bind)
Value
Compiler::FunctionDeclVisit(const FunctionDecl& func_decl)
{
    TokenPtr func_name = func_decl.func_identifier_;
    uint32_t line = func_name->get_line();
    //if it is a global function get the save the name to constant array
    //and get the index for the operation
    //if it is a local function simulate the stack situation in the locals
    uint32_t var_idx = VariableAnalysis(func_name, line);
    //Pre-initialization for nested call about functions
    MarkInitialized();
    //compiling the function paramater and the body
    FunctionCompiling(func_decl, FunctionType::TYPE_FUNCTION, line);
    //emit the opcode for the global variable
    DefineVariable(var_idx, line);
    return boost::blank();
}



Value
Compiler::ReturnStatementVisit(const ReturnStatement& node)
{
    uint32_t line = node.return_keyword_->get_line();
    if(type_ == TYPE_SCRIPT)
        throw CompilerErrorTrigger("Can't return from top-level code.", 
            line, CompilingError::RETURN_AT_TOP);
    if(node.return_value_ != nullptr){
        if(type_ == FunctionType::TYPE_INITIALIZER)
            throw CompilerErrorTrigger("Can't return a value from an initializer.",
                line, CompilingError::INITIALIZER_RETURN);
        node.return_value_->Accept(this);
        EmitByte(OpCode::OP_RETURN, line);
    }
    else
        EmitReturn(line);
    return boost::blank();
}

void
Compiler::CreateMethod(const FunctionDecl& methods, uint32_t line)
{
    string name = methods.func_identifier_->get_lexme();
    uint32_t constant_offset = function_->chunk->WriteConstant(CreateObjString(name), line);
    //get a closure on the stack top
    FunctionType type = FunctionType::TYPE_METHOD;
    if(methods.func_identifier_->get_lexme() == "init")
        type = FunctionType::TYPE_INITIALIZER;
    FunctionCompiling(methods, type, line);
    EmitByte(OpCode::OP_METHOD, line);
    EmitAddress(constant_offset, line);
}



Value
Compiler::ClassDeclVisit(const ClassDecl& class_decl)
{
    TokenPtr name = class_decl.name_;
    uint32_t line =  name->get_line();
    uint32_t constant_offset = VariableAnalysis(name, line);
    EmitByte(OpCode::OP_CLASS, line);
    const vector<unique_ptr<FunctionDecl>>& methods = class_decl.methods_;
    EmitAddress(constant_offset, line);
    DefineVariable(constant_offset, line);
    ClassCompiler class_compiler;
    class_compiler.enclosing = current_class;
    current_class = &class_compiler;
    if(class_decl.superclass_ != nullptr){
        class_decl.superclass_->Accept(this);
        if(name->get_lexme() == class_decl.superclass_->IsVaribleExpr()->var_identifier_->get_lexme())
            throw CompilerErrorTrigger("A class can't inherit from itself.", 
                line, CompilingError::INHERIT_SELF);
        //we will save the super class to the outer class of the methods
        //so that the methods can access the superclass through the upvalue 
        BeginScope();
        AddLocal(SyntheticToken("super"));
        DefineVariable(0, line);
        NamedVariable(name, line, true);
        EmitByte(OpCode::OP_INHERIT, line);
        current_class->has_super_class = true;
    }
    //when we define a method for a class
    //there will be a closure at the top of the stack above the class it will bound to.
    NamedVariable(name, line, true);
    for(int i = 0; i < methods.size(); i++){
        CreateMethod(*methods[i], line);
    }
    //pop the class object on the top of the stack
    EmitByte(OpCode::OP_POP, line);
    if(current_class->has_super_class)
        EndScope(line);
    current_class = current_class->enclosing;
    return boost::blank();
}


ObjFuncPtr
Compiler::Compiling(const std::vector<StatementPtr> &program)
{
    try{
        for(auto& i : program)
            StatementCompling(i);
    }
    catch(CompilingError a){
        return nullptr;
    }
//the next statement is actually the Endcompiling
//there is a bug at here
    uint32_t line = function_->line;
//for the function which don't have the return statement
    EmitReturn(line);
    if(current->enclosing != nullptr)
        current = current->enclosing;
//if the lox run with the repl mode
//the compiling function will be called several times 
//but it only going to be initialized once  
    if(is_initialized == false ){
//pop the function which was pushed into stack at compiler initialization 
        vm.PopStack();
        is_initialized = true;
    }
    return function_;
}
