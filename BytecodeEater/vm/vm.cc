#include "../debugger/include/debugger.h"
#include "../scanner/include/ErrorReporter.h"
#include "../helper/object.h"
#include <boost/variant/get.hpp>
#include <cstdint>
#include <fmt/core.h>
#include "include/vm.h"
#include <initializer_list>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <boost/variant.hpp>
#include <fmt/core.h>
using namespace std;

#define GET_IP() frames_[frame_count_ - 1].ip
#define GET_CHUNK() frames_[frame_count_ - 1].closure->function->chunk


VM::VM()
{
    stack_top_ = stack_;
    BUILTIN_GROUP(BUILTIN_DEFINE)
    init_string = CreateObjString("init");
}

void
VM::DefineBuiltin(std::string name, BuiltinFn function)
{
    ObjString* str_obj = CreateObjString(name);
    PushStack(str_obj);
    void* raw_ptr = MemoryAllocater(sizeof(ObjBuiltin));
    PushStack(new (raw_ptr) ObjBuiltin(function, name));
    //PushStack(new ObjBuiltin(function, name));
    global_var_[AS_STRING_OBJ(stack_[0])] = stack_[1];
    PopStack();
    PopStack();
}


ObjStrPtr
CreateObjString(std::string src)
{
    ObjString* result = vm.FindString(src);
    if(result != nullptr)
        return result;
    else{
        void* raw_ptr = MemoryAllocater(sizeof(ObjString));
        result = new (raw_ptr) ObjString(src);
        vm.bytesAllocated += src.size();
        //result = new ObjString(src);
        vm.SetString(src, result);
        return result;
    }
}

ObjStrPtr
VM::FindString(std::string str)
{
    if (strings_.find(str) != strings_.end()){
        return strings_[str];
    }
    else {
        return nullptr;
    }
}

void
VM::InsertObject(Object *object)
{
    object->next = objects_;
    objects_ = object;
    return;
}

VM::~VM()
{
    Object* object = objects_;
    while(object != nullptr){
        delete object;
        object = object->next;
    }
}

bool GetTruthy(Value opnum)
{
    //除了Nil和false其他都是true 0
    if(opnum.which() == 0) return false;
    if(opnum.which() == 2){
        return boost::get<bool>(opnum);
    }
    if(opnum.which() == 1){
        if(boost::get<double>(opnum) == 0)
            return false;
    }
    return true;
}

RuntimeErrorType
VM::GetRuntimeError(std::string message, RuntimeErrorType type)
{
    uint32_t code_offset = GET_IP() - GET_CHUNK()->code_.begin() - 1;
    uint32_t line = GET_CHUNK()->GetLine(code_offset);
    ErrorReporter::excute_runtime_error(message, line);
    fmt::print("\nstack trace:\n");
    for(int i = frame_count_ - 1; i >= 0 ; i--){
        CallFrame& frame = frames_[i];
        ObjFuncPtr function = frame.closure->function;
        string name = (function->name == nullptr) ? "script" : function->name->content_;
        fmt::print("[line {}] in {}()\n", line, name); 
    }

    return type;
}

void
VM::PushStack(Value value)
{
    if(frame_count_ != 0 && stack_top_ - frames_[frame_count_ - 1].slots >= 0xff)
        GetRuntimeError("stack space is not enough", RuntimeErrorType::STACKOVERFLOW);
    *stack_top_ = value; 
    stack_top_++;
}
bool initialized = false;

void
VM::Interpreter(ObjFuncPtr function)
{
    if(initialized == false){
        //Push for avoidence  GC
        PushStack(function);
        //allocate a new Closure
        void* raw_ptr = MemoryAllocater(sizeof(ObjClosure));
        ObjClosure* closure = new (raw_ptr) ObjClosure(function);
        PopStack();
        PushStack(closure);
        //create and initialize a new call frame
        CallFrame frame = CallFrame();
        frame.closure = closure;
        frame.ip = function->chunk->code_.begin();
        frame.slots = vm.stack_;
        frame.is_main = true;
        frames_.push_back(frame);
        frame_count_++;
        initialized = true;
    }
    //ObjClosure* closure = new ObjClosure(function);
    //for REPL mode , recover the ip_ to the previous position maybe wrong 
    GET_IP() = GET_CHUNK()->code_.begin() + GET_CHUNK()->offset_;
    try{
        GET_CHUNK()->offset_ = Run();
    }
    catch(RuntimeErrorType type ){
        GET_CHUNK()->offset_ = GET_CHUNK()->code_.size();
        return;
    }
}

bool IsEqual(Value left_value, Value right_value)
{
    if((left_value.which() == 0) && (right_value.which() == 0))
        return true;
    else if((left_value.which() == 3) && (right_value.which() == 3)){
        ObjStrPtr str_1 = AS_STRING_OBJ(left_value);
        ObjStrPtr str_2 = AS_STRING_OBJ(right_value);
        return (str_1 == str_2);
    } 
    return left_value == right_value;
}

bool
VM::Call(ObjClosure* closure, uint32_t arg_count)
{
    if(closure->function->arity != arg_count){
        string message = fmt::format("Expected {} arguments but got {}.", closure->function->arity, arg_count);
        throw GetRuntimeError(message, RuntimeErrorType::ARGUMENT_NUM_ERROR);
    }
    CallFrame frame = CallFrame();
    frame.ip = closure->function->chunk->code_.begin();
    frame.closure = closure;
    frame.slots = stack_top_ - arg_count - 1;
    frames_.push_back(frame);
    frame_count_++;
    return true;
}

ObjUpValue*
VM::CaptureUpValue(Value* value)
{
    ObjUpValue* prev_upvalue = nullptr;
    ObjUpValue* upvalue = OpenUpValues_;
    if(upvalue != nullptr && upvalue->value > value){
        prev_upvalue = upvalue;
        upvalue = upvalue->next;
    }
    if(upvalue != nullptr && upvalue->value == value)
        return upvalue;
    //ObjUpValue* new_upvalue = new ObjUpValue(value);
    void* raw_ptr = MemoryAllocater(sizeof(ObjUpValue));
    ObjUpValue* new_upvalue = new (raw_ptr) ObjUpValue(value);
    new_upvalue->next = upvalue;
    if(upvalue == nullptr)
        OpenUpValues_ = new_upvalue;
    else
        prev_upvalue->next = new_upvalue;

    return new_upvalue;
}

bool
VM::CallValue(Value value, uint32_t arg_count)
{
    if(value.which() == 3){
        Object* object = boost::get<Object*>(value);
        if(object->IsClosure()){
            //create call frame and change the ip to the target function
            return Call(dynamic_cast<ObjClosure*>(object), arg_count);
        }
        else if(object->IsBuiltin()){
            ObjBuiltinPtr objbuiltin = dynamic_cast<ObjBuiltinPtr>(object);
            BuiltinFn function = objbuiltin->function;
            Value result = function(arg_count, stack_top_ - arg_count);
            vm.stack_top_ -= arg_count + 1;
            PushStack(result);
            return true;
        }
        else if(object->IsClass()){
            ObjClass* o_class = dynamic_cast<ObjClass*>(object);
            void* raw_ptr = MemoryAllocater(sizeof(ObjInstance));
            ObjInstance* instance = new(raw_ptr) ObjInstance(o_class);
            //now the stack base of the initializer is this instance
            *(stack_top_- 1 - arg_count) = instance;
            if(o_class->methods.find(init_string) != o_class->methods.end())
                return Call(AS_CLOSURE_OBJ(o_class->methods[init_string]), arg_count);
            else if(arg_count != 0)
                throw GetRuntimeError(fmt::format("Expected 0 arguments but got {}.", arg_count) , 
                    RuntimeErrorType::ARGUMENT_NUM_ERROR);
            return true;
        }
        else if(object->IsBound()){
            ObjBoundMethod* bound_method = dynamic_cast<ObjBoundMethod*>(object);
            *(stack_top_ -1 - arg_count) = bound_method->receiver;
            Call(bound_method->method, arg_count);
            return true;
        }
    }
    throw GetRuntimeError("Can only call functions and classes.", RuntimeErrorType::TYPE_ERROR);
}

void 
VM::CloseUpValue(Value *value)
{
    while(OpenUpValues_ != nullptr && OpenUpValues_->value >= value)
    {
        ObjUpValue* upvalue = OpenUpValues_;
        upvalue->closed_value = *upvalue->value;
        upvalue->value = &upvalue->closed_value;
        OpenUpValues_ = upvalue->next;
    }
}

void
VM::DefinedMethod(ObjString *name)
{
    ObjClosure* method = AS_CLOSURE_OBJ(Peek(0));
    ObjClass* o_class = AS_CLASS_OBJ(Peek(1));
    o_class->methods[name] = method;
    //pop the method
    PopStack();
}
bool
VM::BindMethod(ObjClass *kclass, ObjString *name)
{
    if(kclass->methods.find(name) == kclass->methods.end())
        return false;
    void* raw_ptr = MemoryAllocater(sizeof(ObjBoundMethod));
    //the this instance which will be bind to the method is at the stack top
    ObjBoundMethod* new_bound_method = new(raw_ptr) ObjBoundMethod(AS_CLOSURE_OBJ(kclass->methods[name]), Peek(0));
    //pop the receiver instance 
    PopStack();
    PushStack(new_bound_method);
    return true;
}

extern bool is_repl;

void
VM::Invoke(ObjString *name, uint32_t args_count)
{
    //get the instance
    Value receiver = Peek(args_count);
    if(!IS_INSTANCE_OBJ(receiver))
        throw GetRuntimeError("Only instances have methods.", RuntimeErrorType::TYPE_ERROR);
    ObjInstance* instance = AS_INSTANCE_OBJ(receiver);
    if(instance->field.find(name) != instance->field.end()){
        Value value = instance->field[name];
        //recover the stack 
        *(vm.stack_top_ - 1 - args_count) = value;
        CallValue(value, args_count);
        return;
    }
    InvokeFromClass(instance->kclass, name, args_count);
    
}

void
VM::InvokeFromClass(ObjClass *kclass, ObjString *name, uint32_t args_count)
{
    if(kclass->methods.find(name) == kclass->methods.end())
        throw GetRuntimeError(fmt::format("Undefined property '{}'.", name->content_), 
            RuntimeErrorType::UNDEFINED_PROPERTY);
    Call(AS_CLOSURE_OBJ(kclass->methods[name]), args_count);
}

void
TableAddAll(unordered_map<ObjString*, Value>& src, unordered_map<ObjString*, Value>& dest)
{
    for(auto i = src.begin(); i != src.end(); i++){
        dest[i->first] = i->second;
    }
}


uint32_t
VM::Run()
{
    CallFrame* frame = &(frames_[frame_count_ - 1]);
//the ip of the VM is according to the frame 
//so if you update the frame to the ip will change to the target function 
//--------helper macrow---------
#define READ_BYTECODE() (*frame->ip++)
#define READ_ADDR() ((uint32_t)(*frame->ip++) | ((uint32_t)(*frame->ip++) << 8) | ((uint32_t)(*frame->ip++) << 16) | ((uint32_t)(*frame->ip++) << 24))
#define READ_CONSTANT() (frame->closure->function->chunk->constant_[READ_ADDR()]) 
#define READ_CONSTANT_LONG() (frame.function->chunk->constant_[(READ_BYTECODE() | (READ_BYTECODE() << 8) | (READ_BYTECODE() << 16))])
#define BINARY_OP(op)                                                                   \
    do {                                                                                \
        Value right_operand = PopStack();                                               \
        Value left_operand =  Peek(0);                                                  \
        if(IS_NUMBER(right_operand) && IS_NUMBER(left_operand)){                        \
            double right_value = boost::get<double>(right_operand);                     \
            double left_value = boost::get<double>(left_operand);                       \
            stack_top_[-1] = left_value op right_value;                                 \
        }                                                                               \
        else                                                                            \
            throw GetRuntimeError("Binary operands must be two numbers.",               \
                 RuntimeErrorType::TYPE_ERROR);                                         \
    }while(false)  
#define READ_STRING() (dynamic_cast<ObjStrPtr>(boost::get<Object*>(frame->closure->function->chunk->constant_[READ_ADDR()])))
#define READ_SHORT() (frame->ip += 2, (uint16_t)(*(frame->ip - 2) | ((uint16_t)(*(frame->ip - 1)) << 8)))
//all the offset in the instruction is based on the stack base (slots)
#define STACK_GETTER(slot) frame->slots[slot]                           
//--------RUN------------------
    while (true) {
#ifdef DEBUG_TRACE_EXCUTION
        for(auto i = stack_; i < stack_top_; i++){
            cout << '[';
            PrintValue(*i);
            cout << ']';
        }
        cout << endl;
        Debugger inner_debugger(frame->closure->function->chunk, (uint32_t)(frame->ip - frame->closure->function->chunk->code_.begin()));
        inner_debugger.InstructionDisassemble();
#endif
        uint8_t code = READ_BYTECODE();
        switch(code){
            case OpCode::OP_RETURN:{
                //get result
                Value result = PopStack(); 
                uint32_t last_ip = frames_[frame_count_ - 1].ip - frames_[frame_count_ - 1].closure->function->chunk->code_.begin();
                //only in REPL mode does main not pop up.
                if(frames_[frame_count_ - 1].is_main == false || is_repl == false){
                    frames_.pop_back();
                    CloseUpValue(frame->slots + 1);
                    frame_count_--;
                }
                //if the function is the main function
                //pop the main function and return
                if(frame_count_ == 0 || (frames_[frame_count_ - 1].is_main == true && is_repl == true)){
                    return last_ip;
                }
                stack_top_ = frame->slots;
                //stack_.erase(stack_.begin() + frame->slots, stack_.end());
                PushStack(result);
                frame = &(frames_[frame_count_ - 1]);
                break;
            }
            case OpCode::OP_CONSTANT:{
                Value constant = READ_CONSTANT();
                PushStack(constant);
                break;
            }
            /*
            case OpCode::OP_CONSTANT_LONG:{
                Value constant = READ_CONSTANT_LONG();
                PushStack(constant);
                break;
            }
            */
            case OpCode::OP_NIL:{PushStack(boost::blank());break;}
            case OpCode::OP_TRUE:{PushStack(true);break;}
            case OpCode::OP_FALSE:{PushStack(false);break;}
            case OpCode::OP_NEGATE:{
                Value value = Peek(0);
                if(IS_NUMBER(value))
                    stack_top_[-1] = -(boost::get<double>(Peek(0)));
                else
                    throw GetRuntimeError("Unary operand must be a number.", RuntimeErrorType::TYPE_ERROR);
                break;
            }
            case OpCode::OP_NOT:{stack_top_[-1] = !(GetTruthy(Peek(0))); break;}
            case OpCode::OP_SUB:{BINARY_OP(-); break;}
            case OpCode::OP_MUL:{BINARY_OP(*); break;}
            case OpCode::OP_DIV:{BINARY_OP(/); break;}
            case OpCode::OP_ADD:{
                Value right_operand = Peek(0);
                Value left_operand =  Peek(1); 
                if(IS_NUMBER(right_operand) && IS_NUMBER(left_operand)){
                    double right_value = boost::get<double>(right_operand);
                    double left_value = boost::get<double>(left_operand);
                    PopStack();
                    stack_top_[-1] = right_value + left_value;
                }
                else if(IS_STRING_OBJ(left_operand) || IS_STRING_OBJ(right_operand)){
                    //when the other types of variables are add to strings 
                    //we choose to cast these variables to string 
                    string str_1 = boost::apply_visitor(StringVisitor(), left_operand);
                    string str_2 = boost::apply_visitor(StringVisitor(), right_operand);
                    //ObjStrPtr left_value = AS_STRING_OBJ(left_operand);
                    //ObjStrPtr right_value = AS_STRING_OBJ(right_operand);
                    string new_value = str_1 + str_2;
                    PopStack();
                    stack_top_[-1] = CreateObjString(new_value);
                }
                else
                    throw GetRuntimeError("Binary operands must be two numbers or two strings.", 
                        RuntimeErrorType::TYPE_ERROR);
                break;
            }
            case OpCode::OP_EQUAL:{
                Value left_operand = PopStack();
                Value right_operand = Peek(0);
                stack_top_[-1] =  IsEqual(left_operand, right_operand);
                break;
            }
            case OpCode::OP_LESS:{BINARY_OP(<); break;}
            case OpCode::OP_GREATER:{BINARY_OP(>); break;}
            case OpCode::OP_LESS_EQUAL:{BINARY_OP(<=); break;}
            case OpCode::OP_GREATER_EQUAL:{BINARY_OP(>=); break;}
            case OpCode::OP_BANG_EQUAL:{
                Value left_operand = PopStack();
                Value right_operand = Peek(0);
                stack_top_[-1] = !(IsEqual(left_operand, right_operand));
                break;
            }
            case OpCode::OP_PRINT:{
                Value value = PopStack();
                cout << boost::apply_visitor(StringVisitor(),value);
                cout << endl;
                break;
            }
            case OpCode::OP_POP:{PopStack(); break;}
            case OpCode::OP_DEFINE_GLOBAL:{
                ObjStrPtr name = READ_STRING();
                global_var_[name] = PopStack();
                break;
            }
            case OpCode::OP_GET_GLOBAL:{
                ObjStrPtr name = READ_STRING();
                if(global_var_.find(name) == global_var_.end()){
                    string str = "Undefined variable " + name->ContentGetter() + " .";
                    throw GetRuntimeError(str, RuntimeErrorType::INVILID_VALUE);
                }
                else
                    PushStack(global_var_[name]);
                break;
            }
            case OpCode::OP_SET_GLOBAL:{
                ObjStrPtr name = READ_STRING();
                if(global_var_.find(name) == global_var_.end()){
                    string str = "Undefined variable " + name->ContentGetter() + " .";
                    throw GetRuntimeError(str, RuntimeErrorType::INVILID_VALUE);
                }
                else
                    global_var_[name] = Peek(0);
                break;
            }
            case OpCode::OP_GET_LOCAL:{
                uint32_t slot = READ_ADDR();
                PushStack(STACK_GETTER(slot));
                break;
            }
            case OpCode::OP_SET_LOCAL:{
                uint32_t slot = READ_ADDR();
                STACK_GETTER(slot) = Peek(0);
                break;
            }
            case OpCode::OP_JUMP_IF_FALSE:{
                uint16_t offset = READ_SHORT();
                if(!GetTruthy(Peek(0))) frame->ip += offset;
                break;
            }
            case OpCode::OP_JUMP:{
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OpCode::OP_LOOP:{
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OpCode::OP_CALL:{
                uint8_t arg_count = READ_BYTECODE();
                CallValue(Peek(arg_count), arg_count);
                frame = &(frames_[frame_count_ - 1]);
                break;
            }
            //
            case OpCode::OP_CLOSURE:{
                ObjFunction* function = AS_FUNCTION_OBJ(READ_CONSTANT());
                void* raw_ptr = MemoryAllocater(sizeof(ObjClosure));
                ObjClosure* closure = new (raw_ptr) ObjClosure(function);
                PushStack(closure);
                //ObjClosure* closure = new ObjClosure(function);
                for(int i = 0; i < function->up_value_count; i++){
                    uint32_t is_local = READ_BYTECODE();
                    uint32_t index = READ_ADDR();
                    //Layer upon layer through
                    if(is_local){
                        closure->up_values.push_back(CaptureUpValue(frame->slots + index));
                    }
                    else
                        closure->up_values.push_back(frame->closure->up_values[index]);
                }
                break;
            }
            case OpCode::OP_GET_UPVALUE:{
                uint32_t index = READ_ADDR();
                //up value store the value's pointer 
                Value  value = *(frame->closure->up_values[index]->value);
                PushStack(value);
                break;
            }
            case OpCode::OP_SET_UPVALUE:{
                uint32_t index = READ_ADDR();
                Value value = Peek(0);
                *(frame->closure->up_values[index]->value) = value;
                break;
            }
            case OpCode::OP_CLOSE_UPVALUE:{
                CloseUpValue(stack_top_ - 1);
                PopStack();
                break;
            }
            case OpCode::OP_CLASS:{
                ObjStrPtr name = READ_STRING();
                void* raw_ptr = MemoryAllocater(sizeof(ObjClass));
                ObjClass* o_class = new(raw_ptr) ObjClass(name);
                PushStack(Value(o_class));
                break;
            }
            case OpCode::OP_GET_PROPERTY:{
                Object* object = boost::get<Object*>(Peek(0));
                if(!(object->IsInstance()))
                    throw GetRuntimeError("Only instances have properties.", RuntimeErrorType::TYPE_ERROR);
                ObjInstance* instance = AS_INSTANCE_OBJ(Peek(0));
                ObjString* name = READ_STRING();
                if(instance->field.find(name) != instance->field.end()){
                    PopStack();
                    Value value = instance->field[name];
                    PushStack(value);
                    break;
                }
                else if(!BindMethod(instance->kclass, name))
                    throw GetRuntimeError("Undefined property " + name->content_ + ".", 
                        RuntimeErrorType::UNDEFINED_PROPERTY);
                break;
            }
            case OpCode::OP_SET_PROPERTY:{
                if(!boost::get<Object*>(Peek(1))->IsInstance())
                    throw GetRuntimeError("Only instances have properties.", RuntimeErrorType::TYPE_ERROR);
                ObjInstance* instance = AS_INSTANCE_OBJ(Peek(1));
                Value value = Peek(0);
                ObjString* name = READ_STRING();
                instance->field[name] = value;
                Value set_value = PopStack();
                PopStack();
                PushStack(value);
                break;
            }
            case OpCode::OP_METHOD:{
                DefinedMethod(READ_STRING());
                break;
            }
            case OpCode::OP_INVOKE:{
                ObjString* name = READ_STRING();
                uint32_t args_count = READ_BYTECODE();
                Invoke(name, args_count);
                frame = &frames_[frame_count_ - 1];
                break;
            }
            case OpCode::OP_INHERIT:{
                //the superclass must be a class
                if(!IS_CLASS_OBJ(Peek(1)))
                    GetRuntimeError("Superclass must be a class.", RuntimeErrorType::TYPE_ERROR);
                ObjClass* superclass = AS_CLASS_OBJ(Peek(1));
                ObjClass* subclass = AS_CLASS_OBJ(Peek(0));
                TableAddAll(superclass->methods, subclass->methods);
                //Pop the subclass
                PopStack();
                //Superclass is left on the stack for super keyword access.
                break;
            }
            case OpCode::OP_GET_SUPER:{
                ObjString* name = READ_STRING();
                ObjClass* superclass = AS_CLASS_OBJ(PopStack());
                BindMethod(superclass, name);
                break;
            }
            case OpCode::OP_SUPER_INVOKE:{
                ObjString* name = READ_STRING();
                uint32_t args_count = READ_BYTECODE();
                ObjClass* superclass = AS_CLASS_OBJ(PopStack());
                InvokeFromClass(superclass, name, args_count);
                frame = &frames_[frame_count_ - 1];
                break;
            }
            case OpCode::OP_ARRAY_CREATE:{
                double array_size = boost::get<double>(PopStack());
                void* raw_ptr = MemoryAllocater(sizeof(ObjArray));
                ObjArray* array = new(raw_ptr) ObjArray();
                void* element_ptr = MemoryAllocater(array_size * sizeof(Value));
                array->element = (Value*)element_ptr;
                array->size = array_size;
                for(int i = 0; i < array_size; i++){
                    Value element = PopStack();
                    Value* ptr = new (array->element + i) Value();
                    *(array->element + i) = element;
                }
                PushStack(array);
                break;
            }
            case OpCode::OP_ARRAY_GET:{
                int64_t index = (int64_t)(boost::get<double>(PopStack()));
                ObjArray* array = AS_ARRAY_OBJ(PopStack());
                Value target = *(array->element + index);
                PushStack(target);
                break; 
            }
            case OpCode::OP_ARRAY_SET:{
                int64_t index = (int64_t)(boost::get<double>(PopStack()));
                ObjArray* array = AS_ARRAY_OBJ(PopStack());
                Value value = Peek(0);
                *(array->element + index) = value;
                break;
            }
            default:
                throw GetRuntimeError("invalid OpCode!", RuntimeErrorType::INVALID_OPCODE);
        }
    }
#undef BINARY_OP
#undef READ_CONSTANT
#undef READ_BYTECODE
}