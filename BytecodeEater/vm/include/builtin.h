#ifndef __BUILTIN_
#define __BUILTIN_
#include <vector>
#include <functional>
#include <boost/variant.hpp>
#include "../../helper/bytecode.h"
class Object;

#define BUILTIN_GROUP(J)                   \
    J("clock", Clock)

using StackIterator = std::vector<Value>::iterator;
//the builtin functions should not influence the stack layout
//the builtin functions get the args from the stack through a satck pointer
#define DECL_BUILTN(name, func_name)                                        \
    Value                                                                   \
    Builtin##func_name(int arg_count, Value* args);

BUILTIN_GROUP(DECL_BUILTN)
#undef DECL_BUILTIN
#endif