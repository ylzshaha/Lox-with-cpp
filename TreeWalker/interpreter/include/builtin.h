#ifndef __BUILTIN_
#define __BUILTIN_
#include "../../helper/reward.h"
#include <vector>
#define BUILTIN_GROUP(J)                   \
    J("clock", Clock)

#define DECL_BUILTN(name, func_name)                                    \
    ObjectType                                                        \
    Builtin##func_name(Interpreter* interpreter, std::vector<ObjectType> args);

BUILTIN_GROUP(DECL_BUILTN)
#undef DECL_BUILTIN
#endif
