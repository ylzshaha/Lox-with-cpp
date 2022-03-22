#include "include/builtin.h"
#include "../helper/bytecode.h"
#include <vector>
#include <chrono>
#include <iostream>
using namespace std;

#define MAKE_BUILTIN(name)                                              \
    Value                                                               \
    Builtin##name(int args_count, Value* args)

MAKE_BUILTIN(Clock)
{
    auto current_time = chrono::system_clock::now();
    auto milli_current_time = chrono::time_point_cast<chrono::duration<double, std::milli>>(current_time);
    auto time_interval = milli_current_time.time_since_epoch();
    double result = (time_interval.count() / 1000.0);
    return result;
}
