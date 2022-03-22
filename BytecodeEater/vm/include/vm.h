#ifndef __VM_
#define __VM_
#include "../../helper/bytecode.h"
#include "builtin.h"
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <iostream>
class Object;
class ObjString;
class ObjFunction;
class ObjBuiltin;
class ObjClosure;
class ObjUpValue;
class ObjClass;


#define STACK_MAX 0x100 * 0x100

using CodeIterator = std::vector<std::uint8_t>::iterator;
using StackIterator = std::vector<Value>::iterator;

enum InterpretResult{
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
};

enum RuntimeErrorType{
      TYPE_ERROR, INVALID_OPCODE,
      INVILID_VALUE, ARGUMENT_NUM_ERROR, STACKOVERFLOW,
      UNDEFINED_PROPERTY
};

//every function all will have a frame
//we get all the runtime information about function call from  its own frame.
class CallFrame{
public:
    ObjClosure* closure = nullptr;
    std::vector<std::uint8_t>::iterator ip;
    // we can't use the iterator here
    //because when we push or pop from the stack the begin will change
    Value*  slots = nullptr;
    bool is_main = false;
};

class VM
{
public:
#define BUILTIN_DEFINE(name,func_name)      \
    DefineBuiltin(name, Builtin##func_name);
    VM();
    ~VM();
    void Interpreter(ObjFunction* function);
    std::uint32_t Run();
    void PushStack(Value value);
    Value PopStack() {stack_top_--; Value result = *stack_top_; return result;}
    Value Peek(std::uint32_t offset) {return *(stack_top_ - 1 - offset);}
    RuntimeErrorType GetRuntimeError(std::string message, RuntimeErrorType type);
    void InsertObject(Object* object);
    ObjString* FindString(std::string str);
    void SetString(std::string key, ObjString* value) {strings_[key] = value;}
    bool CallValue(Value value, uint32_t arg_count);
    bool Call(ObjClosure* function, uint32_t arg_count);
    void DefineBuiltin(std::string name, std::function<Value(int, Value*)> function);
    ObjUpValue* CaptureUpValue(Value* value);
    void CloseUpValue(Value* value);
    void DefinedMethod(ObjString* name);
    bool BindMethod(ObjClass* kclass, ObjString* name);
    void Invoke(ObjString* name, uint32_t args_count);
    void InvokeFromClass(ObjClass* kclass, ObjString* name, uint32_t args_count);
    friend class Collector;


    uint32_t bytesAllocated = 0;
    uint32_t  nextGC = 1024 * 1024;
    ObjString* init_string = nullptr;
private:
    //the member of the stack info 
    //the sp always point to the next slot which hasn't been used before
    std::uint32_t frame_count_ = 0;
    std::vector<CallFrame> frames_;
    Object* objects_ = nullptr;
    std::unordered_map<std::string, ObjString*> strings_;
    std::unordered_map<ObjString*, Value> global_var_;
    ObjUpValue* OpenUpValues_ = nullptr;
    Value stack_[STACK_MAX];
    Value* stack_top_;
    std::vector<Object*> gray_list;
};

#endif