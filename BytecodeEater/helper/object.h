#ifndef __OBJECT_
#define __OBJECT_

#include "../vm/include/vm.h"
#include "bytecode.h"
#include <boost/variant.hpp>
#include <boost/variant/detail/apply_visitor_delayed.hpp>
#include <cstddef>
#include <cstdint>
#include <fmt/core.h>
#include <string>
#include <memory>
#include <iostream>
#include <map>
#include <unordered_map>

extern VM vm;
class Collector;

enum ObjType
{
    OBJ_STRING,OBJ_FUNCTION,
    OBJ_BUILTIN, OBJ_CLOSURE,
    OBJ_UP_VALUE,OBJ_CLASS,
    OBJ_INSTANCE, OBJ_BOUND_METHOD,
    OBJ_ARRAY
};

enum FunctionType {
  TYPE_FUNCTION,
  TYPE_SCRIPT,
  TYPE_METHOD,
  TYPE_INITIALIZER
};


class Object
{
public:
    Object(ObjType type):
        type_(type){vm.InsertObject(this);
    }
    virtual ~Object(){}
    virtual ObjType get_type() {return type_;}
    virtual bool IsStringObj() {return (type_ == ObjType::OBJ_STRING);}
    virtual bool IsFunction() {return (type_ == ObjType::OBJ_FUNCTION);}
    virtual bool IsBuiltin() {return (type_ == ObjType::OBJ_BUILTIN);}
    virtual bool IsClosure() {return (type_ == ObjType::OBJ_CLOSURE);}
    virtual bool IsUpValue() {return (type_ == ObjType::OBJ_UP_VALUE);}
    virtual bool IsClass() {return (type_ == ObjType::OBJ_CLASS);}
    virtual bool IsInstance() {return (type_ == ObjType::OBJ_INSTANCE);}
    virtual bool IsBound() {return (type_ == ObjType::OBJ_BOUND_METHOD);}
    virtual bool IsArray() {return (type_ == ObjType::OBJ_ARRAY);}
    friend VM;
    friend Collector;
private:
    ObjType type_;
    Object* next = nullptr;
    bool is_marked = false;
};

//there is no other reference in the ObjString
class ObjString : public Object
{
public:
    ObjString(std::string content):
        content_(content),Object(ObjType::OBJ_STRING){}
    std::string ContentGetter() {return content_;}
    
    std::string content_;
};

class ObjArray : public Object
{
public:
    ObjArray():
        Object(ObjType::OBJ_ARRAY){}
    ~ObjArray()
    {
        ::operator delete ((void*)element);
    }
    Value* element = nullptr;
    double size = 0;
};

class ObjFunction : public Object 
{
public:
    ObjFunction():
        Object(ObjType::OBJ_FUNCTION){
            chunk = std::make_shared<Chunk>();
        }
//    ~ObjFunction() {std::cout << "Function Delete!" << std::endl;}
    friend Compiler;
    ChunkPtr chunk;
    int arity = 0;
    ObjString* name = nullptr;
    std::uint32_t up_value_count = 0;
    uint32_t line = 0;
};
using BuiltinFn = std::function<Value(int, Value*)>;

class ObjBuiltin :public Object
{
public:
    ObjBuiltin(BuiltinFn ptr,std::string name_str):
        function(ptr), Object(ObjType::OBJ_BUILTIN), name(name_str){}
    BuiltinFn function;
    std::string name;
};

class ObjClosure : public Object
{
public:
    ObjClosure(ObjFunction* funcptr):
        function(funcptr),Object(ObjType::OBJ_CLOSURE){}
    ObjFunction* function;
    std::vector<ObjUpValue*> up_values;
};

class ObjUpValue : public Object
{
public:
    ObjUpValue(Value* value_ptr):
        Object(ObjType::OBJ_UP_VALUE), value(value_ptr){}
    Value* value;
    ObjUpValue* next = nullptr;
    Value closed_value;
};

class ObjClass : public Object
{
public:
    ObjClass(ObjString* str):
        name(str), Object(ObjType::OBJ_CLASS){}
    ObjString* name;
    std::unordered_map<ObjString*, Value> methods; 
};

class ObjInstance : public Object
{
public:
    ObjInstance(ObjClass* ptr):
        Object(ObjType::OBJ_INSTANCE), kclass(ptr){}
    std::unordered_map<ObjString*, Value> field;
    ObjClass* kclass = nullptr;
};

class ObjBoundMethod : public Object
{
public:
    ObjBoundMethod(ObjClosure* ptr, Value value):
        method(ptr), receiver(value), Object(ObjType::OBJ_BOUND_METHOD){}
    ObjClosure* method;
    Value receiver;
};

ObjString* CreateObjString(std::string src);

using ObjPtr = Object*;
using ObjStrPtr = ObjString*;
using ObjFuncPtr = ObjFunction*;
using ObjBuiltinPtr = ObjBuiltin*;



class StringVisitor : public boost::static_visitor<std::string>
{
public:
    std::string operator() (long int ) const{
        return "long int";
    }
    std::string operator() (std::string value) const{
        return value;
    }
    std::string operator() (double value) const{
        double integer_part = 1;
        if(std::modf(value, &integer_part) == 0){
            return std::to_string((int)value);
        }
        else{
            std::string str = std::to_string(value);
            return str.erase ( str.find_last_not_of('0') + 1, std::string::npos );
        }
    }
    std::string operator() (bool value) const{
        if(value == true)
            return "true";
        else
            return "false";
    }
    std::string operator() (boost::blank) const {
        return "nil";
    }
    std::string operator() (ObjPtr object) const {
        if(object->IsStringObj()){
            ObjStrPtr objstring = dynamic_cast<ObjStrPtr>(object);
            return objstring->ContentGetter();
        }
        else if(object->IsFunction()){
            ObjFuncPtr objfunc = dynamic_cast<ObjFuncPtr>(object);
            if(objfunc->name == nullptr)
                return "<script>";
            std::string print_value = "<fn " + objfunc->name->content_ + ">";
            return print_value; 
        }
        else if(object->IsBuiltin()){
            ObjBuiltinPtr objbuiltin = dynamic_cast<ObjBuiltinPtr>(object);
            std::string print_value = "<builtin fn " + objbuiltin->name + ">";
            return print_value;
        }
        else if(object->IsClosure()){
            ObjClosure* closure = dynamic_cast<ObjClosure*>(object);
            std::string print_value;
            if(closure->function->name != nullptr)
                print_value = "<closure " + closure->function->name->content_ + ">";
            else
                print_value = "<closure script>"; 
            return print_value;
        }
        else if(object->IsUpValue()){
            return "<up value>";
        }
        else if(object->IsClass()){
            ObjClass* o_class = dynamic_cast<ObjClass*>(object);
            std::string print_value = "<class " + o_class->name->content_ + ">";
            return print_value; 
        } 
        else if(object->IsInstance()){
            ObjInstance* instance = dynamic_cast<ObjInstance*>(object);
            std::string print_value = "<instance of " + instance->kclass->name->content_ + ">";
            return print_value; 
        }
        else if(object->IsBound()){
            ObjBoundMethod* bound_method = dynamic_cast<ObjBoundMethod*>(object);
            std::string print_value = "<bound method " + bound_method->method->function->name->content_ + ">";
            return print_value;
        } 
        else if(object->IsArray()){
            ObjArray* array = dynamic_cast<ObjArray*>(object);
            std::string print_value = "[";
            for(int i = 0; i < array->size; i++){
                print_value += boost::apply_visitor(StringVisitor(), array->element[i]);
                if(i != array->size - 1)
                    print_value += ", ";
            }
            print_value += "]";
            return print_value;
        }
        return std::string();
    }

};

#define IS_BOOL(value) (value.which() == 2)
#define IS_NIL(value) (value.which() == 0)
#define IS_NUMBER(value) (value.which() == 1)
#define IS_OBJECT(value) (value.which() == 3)

inline bool IsObjType(Value value, ObjType type)
{
    return (IS_OBJECT(value)) && (boost::get<ObjPtr>(value)->get_type() == type);
}

class Collector
{
public:
    static void CollectGarbage();
    static void MarkRoots();
    static void MarkValue(Value& value);
    static void MarkObject(Object* object);
    static void MarkTable(std::unordered_map<ObjString*, Value>& map);
    static void MarkCompilingObject();
    static void TraceReferences();
    static void BlackOne(Object* object);
    static void MarkArray(std::vector<Value>& array);
    static void Sweep();
    //if a ObjString is in the string table does not mean the string is reachable
    //the strng table is a week reference 
    static void StringTableRemoveWhite();
};


inline void*
MemoryAllocater(size_t size)
{
    if(vm.bytesAllocated >= vm.nextGC)
        Collector::CollectGarbage();

#ifdef DEBUG_STRESS_GC
    Collector::CollectGarbage();
#endif
    void* raw_ptr = (::operator new(size));
    vm.bytesAllocated += size;
    return raw_ptr;

}
#define AS_OBJECT(value) (boost::get<Object*>(value))
#define IS_STRING_OBJ(value) (IsObjType(value, ObjType::OBJ_STRING))
#define AS_STRING_OBJ(value) (dynamic_cast<ObjStrPtr>(boost::get<ObjPtr>(value)))
#define AS_CLOSURE_OBJ(value) (dynamic_cast<ObjClosure*>(boost::get<ObjPtr>(value)))
#define AS_BUILTIN_OBJ(value) (dynamic_cast<ObjBuiltin*>(boost::get<ObjPtr>(value)))
#define AS_FUNCTION_OBJ(value) (dynamic_cast<ObjFuncPtr>(boost::get<ObjPtr>(value)))
#define AS_CLASS_OBJ(value) (dynamic_cast<ObjClass*>(boost::get<ObjPtr>(value)))
#define AS_INSTANCE_OBJ(value) (dynamic_cast<ObjInstance*>(boost::get<ObjPtr>(value)))
#define AS_ARRAY_OBJ(value) (dynamic_cast<ObjArray*>(boost::get<Object*>(value)))
#define IS_CLASS_OBJ(value) (IsObjType(value, ObjType::OBJ_CLASS))
#define IS_INSTANCE_OBJ(value) (IsObjType(value, ObjType::OBJ_INSTANCE))
#endif 