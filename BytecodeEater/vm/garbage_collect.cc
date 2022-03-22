#include "../helper/bytecode.h"
#include "../compiler/include/compiler.h"
#include "../helper/object.h"
#include "../vm/include/vm.h"
#include <boost/variant.hpp>
#include <boost/variant.hpp>
#include <cstdint>
#include <exception>
#include <fmt/core.h>

using namespace std;

extern VM vm;
extern Compiler* current;

void
Collector::CollectGarbage()
{
#ifdef DEBUG_LOG_GC
    fmt::print("-- gc begin\n");

#endif
    MarkRoots();
    TraceReferences();
    StringTableRemoveWhite();
    Sweep();
    vm.nextGC *= GC_HEAP_GROW_FACTOR;
#ifdef DEBUG_LOG_GC
  fmt::print("-- gc end\n");
#endif
}

//mark all the object that vm and compiler can directly access without going through other object
void
Collector::MarkRoots()
{
    for(auto i = vm.stack_; i < vm.stack_top_; i++)
        MarkValue(*i);
    for(auto i : vm.frames_)
        MarkObject(i.closure);
    ObjUpValue* open_upvalue_list = vm.OpenUpValues_; 
    while(open_upvalue_list != nullptr){
        MarkObject(open_upvalue_list);
        open_upvalue_list = open_upvalue_list->next;
    }
    MarkTable(vm.global_var_);
    MarkObject(vm.init_string);
}

void
Collector::MarkValue(Value& value)
{
    if(IS_OBJECT(value)) MarkObject(AS_OBJECT(value));
}

void 
Collector::MarkObject(Object* object)
{
    if(object == nullptr) return;
    if(object->is_marked == true) return;
#ifdef DEBUG_LOG_GC
    string object_content = boost::apply_visitor(StringVisitor(), Value(object));
    fmt::print("{:#x} mark\t{}\n",(uint64_t)object, object_content);
#endif 
    object->is_marked = true;
    vm.gray_list.push_back(object);
    
}

void
Collector::MarkTable(std::unordered_map<ObjString *, Value>& map)
{
    for(auto i = map.begin(); i != map.end(); i++){
        MarkObject(i->first);
        MarkValue(i->second);
    }
}

//mark the function object in the compiler 
void
Collector::MarkCompilingObject()
{
    Compiler* compiler = current;
    while (compiler != nullptr) {
        MarkObject(compiler->function_);
        compiler = compiler->enclosing;
    } 
}

void
Collector::TraceReferences()
{
    while(vm.gray_list.size() > 0){
        //if an object is grayed but not in the gray_list the object is black
        Object* target = vm.gray_list.back();
        vm.gray_list.pop_back();
        BlackOne(target);
    }
}

void
Collector::MarkArray(vector<Value>& array)
{
    for(auto& i : array)
        MarkValue(i);
}

void
Collector::Sweep()
{
    Object* prev_object = nullptr;
    Object* object = vm.objects_;
    while(object != nullptr){
        if(object->is_marked == true){
            object->is_marked = false;
            prev_object = object;
            object = object->next;
        }
        else{
            Object* unreachable = object;
#ifdef DEBUG_LOG_GC
    string str = boost::apply_visitor(StringVisitor(), Value(unreachable));
    fmt::print("{:#x} sweep\t{}\n", (uint64_t) unreachable, str);
#endif
            object = object->next;
            if(prev_object != nullptr){
                prev_object->next = object;
            }
            else{
                vm.objects_ = object;
            }
            delete unreachable;
        }
    }
}

void
Collector::StringTableRemoveWhite()
{
    auto a = vm.strings_.begin();
    auto b = vm.strings_.end();
    bool c = (a == b);
    for(auto i = vm.strings_.begin(); i != vm.strings_.end(); i++){
        if(i->second != nullptr && i->second->is_marked != true){
            vm.strings_.erase(i);
        }
    }
}

void
Collector::BlackOne(Object* object)
{
#ifdef DEBUG_LOG_GC
    string object_content = boost::apply_visitor(StringVisitor(), Value(object));
    fmt::print("{:#x} blacken\t {}\n", (uint64_t)object, object_content);
#endif 
    switch (object->type_)
    {
        case ObjType::OBJ_BUILTIN:
        case ObjType::OBJ_STRING:
            break;
        case ObjType::OBJ_FUNCTION:{
            ObjFunction* function = dynamic_cast<ObjFunction*>(object);
            MarkObject(function->name);
            MarkArray(function->chunk->constant_);
            break;
        }
        case ObjType::OBJ_CLOSURE:{
            ObjClosure* closure = dynamic_cast<ObjClosure*>(object);
            MarkObject(closure->function);
            for(auto& i : closure->up_values)
                MarkObject(i);
            break;
        }
        case ObjType::OBJ_UP_VALUE:{
            ObjUpValue* up_value = dynamic_cast<ObjUpValue*>(object);
            MarkValue(up_value->closed_value);
            break; 
        }
        case ObjType::OBJ_CLASS:{
            ObjClass* o_class = dynamic_cast<ObjClass*>(object);
            MarkObject(o_class->name);
            MarkTable(o_class->methods);
            break;
        }
        case ObjType::OBJ_INSTANCE:{
            ObjInstance* instance = dynamic_cast<ObjInstance*>(object);
            MarkObject(instance->kclass);
            MarkTable(instance->field);
            break;
        }
        case ObjType::OBJ_BOUND_METHOD:{
            ObjBoundMethod* bound_method = dynamic_cast<ObjBoundMethod*>(object);
            MarkObject(bound_method->method);
            MarkValue(bound_method->receiver);
        }
        default:
            return;
    }
}