#ifndef __BYTECODE_
#define __BYTECODE_
#include "Node.h"
#include <memory>
#include <vector>
#include <cstdint>
#include <memory>
#include <boost/variant.hpp>
#include <cmath>
#include <iterator>

class Object;

using Value = boost::variant<boost::blank, double, bool, Object*, std::string, long int>;
#define DEBUG_TRACE_EXCUTION
//#define DEBUG_STRESS_GC
//#define DEBUG_LOG_GC
#define GC_HEAP_GROW_FACTOR 2


class Debugger;
class VM;
class Collector;

enum OpCode
{
    OP_RETURN, OP_CONSTANT, /*OP_CONSTANT_LONG,*/ OP_NIL, OP_TRUE, OP_FALSE, 
    OP_NEGATE, OP_NOT, 
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_EQUAL, OP_GREATER, OP_LESS,
    OP_GREATER_EQUAL, OP_LESS_EQUAL, OP_BANG_EQUAL,
    OP_PRINT,OP_POP,
    OP_DEFINE_GLOBAL, OP_GET_GLOBAL, OP_SET_GLOBAL,
    OP_GET_LOCAL, OP_SET_LOCAL,
    OP_JUMP_IF_FALSE,OP_JUMP, OP_LOOP,
    OP_CALL,OP_CLOSURE, OP_GET_UPVALUE, OP_SET_UPVALUE, OP_CLOSE_UPVALUE,
    OP_CLASS, OP_SET_PROPERTY, OP_GET_PROPERTY, OP_METHOD, OP_INVOKE,OP_INHERIT,OP_GET_SUPER, OP_SUPER_INVOKE,
    OP_ARRAY_CREATE, OP_ARRAY_GET, OP_ARRAY_SET
};


typedef struct LineInfo
{
    LineInfo(std::uint32_t idx, std::uint32_t line_num):
        start_idx(idx), line(line_num){}
    std::uint32_t start_idx;
    std::uint32_t line;
}LineInfo;  

class Chunk
{
public:
    Chunk(){}
    void WriteChunk(std::uint8_t byte, uint32_t line) {
        code_.push_back(byte);
        WriteLines(line);
    }
    void FreeChunk(){code_.clear(); constant_.clear();}
    std::uint32_t WriteConstant(Value value, std::uint32_t line);
    uint32_t GetLine(uint32_t offset_){
        uint32_t start_idx = 0;
        uint32_t next_start_idx = 0;
        int i;
        for(i = 0; i < (lines_.size() - 1); i++){
            start_idx = lines_[i].start_idx;
            next_start_idx = lines_[i + 1].start_idx;

            if((start_idx <= offset_) && (next_start_idx > offset_)){
                return lines_[i].line;
            }
        }
        return lines_[i].line;
    }
    friend Debugger;
    friend VM;
    friend Compiler;
    friend class Collector;
private:
    void WriteLines(std::uint32_t line){
        if(lines_.empty())
            lines_.push_back(LineInfo(0, line));
        else{
            if(lines_.back().line < line){
                uint32_t idx = code_.size() - 1;
                lines_.push_back({idx, line});
            }
        }
    }
    std::vector<std::uint8_t> code_;
    std::vector<Value> constant_;
    std::vector<LineInfo> lines_;
    uint32_t offset_ = 0; 
    std::uint32_t last_stack_ = 0;
};  

using ChunkPtr = std::shared_ptr<Chunk>;

void PrintValue(Value value);


#endif
