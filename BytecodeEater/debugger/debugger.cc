#include <boost/variant/detail/apply_visitor_binary.hpp>
#include <cstdint>
#include <ios>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <string>
#include "include/debugger.h"
#include "../helper/object.h"
#include <fmt/core.h>
using namespace std;

#define DEBUG_READ_LONG()                                                                           \
    (chunk_->code_[offset_ + 1]) | ((uint32_t) chunk_->code_[offset_ + 2] << 8) |                   \
    ((uint32_t) chunk_->code_[offset_ + 3] << 16) | ((uint32_t) chunk_->code_[offset_ + 4] << 24);

void
PrintValue(Value value)
{
    string str =  boost::apply_visitor(StringVisitor(), value);
    fmt::print("{}",str);
}



void
Debugger::SimpleInstruction(std::string name)
{
    cout << name << endl;
    offset_++;
    return;
}


void
Debugger::ConstantInstruction(std::string name, bool is_long)
{
    uint32_t value_offset;
    
    value_offset = DEBUG_READ_LONG();
    offset_ += 5;
    //print the opcode 's name and the constant's offset
    fmt::print("{0:<16} {1:0>#8x}\t", name, value_offset);
    PrintValue(chunk_->constant_[value_offset]);
    fmt::print("\n");
    
}

void
Debugger::ByteInstruction(string name)
{
    uint8_t slot = chunk_->code_[offset_ + 1];
    fmt::print("{0:<16} {1:0>#8x}\t", name, slot);
    offset_ += 2;
    cout << endl;
    return;
}

void
Debugger::JumpInstruction(string name, int sign)
{
    uint16_t offset = (uint16_t) chunk_->code_[offset_ + 1];
    offset = offset | (uint16_t)(chunk_->code_[offset_ + 2] << 8);
    fmt::print("{0:<16} {1:0>#8x}->\t {2:0>#8x}", name, offset_, offset_ + sign * offset + 3);
    cout << endl;
    offset_ += 3;
    return;
}

void
Debugger::ChunkDisassemble(std::string name)
{
    fmt::print("== {0} ==\n", name);
    int size = chunk_->code_.size();
    for(offset_ = 0; offset_ < size;){
        InstructionDisassemble();
    }
}

void
Debugger::InvokeInstruction(string name)
{
    uint32_t constant = DEBUG_READ_LONG();
    offset_ += 5;
    uint32_t args_count = chunk_->code_[offset_++];
    fmt::print("{0:<16} ({1:0>#8x} args) {2:0>#8x}\t", name, args_count, constant);
    PrintValue(chunk_->constant_[constant]);
    cout << endl;
}

void
Debugger::InstructionDisassemble()
{
    //print the opcode offset and the line of opcode 
    fmt::print("{0:0>#6x} {1:0>4} ", offset_, chunk_->GetLine(offset_));
    uint8_t instruction = chunk_->code_[offset_];
    switch(instruction){
        case OpCode::OP_RETURN:{return SimpleInstruction("OP_RETURN");}
        case OpCode::OP_CONSTANT:{return ConstantInstruction("OP_CONSTANT", false);}
        /*case OpCode::OP_CONSTANT_LONG:{return ConstantInstruction("OP_CONSTANT_LONG", true);}*/
        case OpCode::OP_NIL:{return SimpleInstruction("OP_NIL");}
        case OpCode::OP_TRUE:{return SimpleInstruction("OP_TRUE");}
        case OpCode::OP_FALSE:{return SimpleInstruction("OP_FALSE");}
        case OpCode::OP_ADD:{return SimpleInstruction("OP_ADD");}
        case OpCode::OP_SUB:{return SimpleInstruction("OP_SUB");}
        case OpCode::OP_MUL:{return SimpleInstruction("OP_MUL");}
        case OpCode::OP_DIV:{return SimpleInstruction("OP_DIV");}
        case OpCode::OP_NEGATE:{return SimpleInstruction("OP_NEGATE");}
        case OpCode::OP_NOT:{return SimpleInstruction("OP_NOT");}
        case OpCode::OP_EQUAL:{return SimpleInstruction("OP_EQUAL");}
        case OpCode::OP_LESS:{return SimpleInstruction("OP_LESS");}
        case OpCode::OP_GREATER:{return SimpleInstruction("OP_GREATER");}
        case OpCode::OP_LESS_EQUAL:{return SimpleInstruction("OP_LESS_EQUAL");}
        case OpCode::OP_GREATER_EQUAL:{return SimpleInstruction("OP_GREATER_EQUAL");}
        case OpCode::OP_BANG_EQUAL:{return SimpleInstruction("OP_BANG_EQUAL");}
        case OpCode::OP_PRINT:{return SimpleInstruction("OP_PRINT");}
        case OpCode::OP_POP:{return SimpleInstruction("OP_POP");}
        case OpCode::OP_GET_GLOBAL:{return ConstantInstruction("OP_GET_GLOBAL", false);}
        case OpCode::OP_DEFINE_GLOBAL:{return ConstantInstruction("OP_DEFINE_GLOBAL", false);}
        case OpCode::OP_SET_GLOBAL:{return ConstantInstruction("OP_SET_GLOBAL", false);}
        case OpCode::OP_SET_LOCAL:{return ByteInstruction("OP_SET_LOCAL");}
        case OpCode::OP_GET_LOCAL:{return ByteInstruction("OP_GET_LOCAL");}
        case OpCode::OP_JUMP:{return JumpInstruction("OP_JUMP", 1);}
        case OpCode::OP_JUMP_IF_FALSE:{return JumpInstruction("OP_JUMP_IF_FALSE", 1);}
        case OpCode::OP_LOOP:{return JumpInstruction("OP_LOOP", -1);}
        case OpCode::OP_CALL:{return ByteInstruction("OP_CALL");}
        case OpCode::OP_CLOSURE:{
            offset_++;
            uint8_t constant_offset = chunk_->code_[offset_++];
            fmt::print("{0:<16} {1:0>#8x}\t", "OP_CLOSURE", constant_offset);
            PrintValue(chunk_->constant_[constant_offset]);
            cout << endl;
            ObjFuncPtr function = AS_FUNCTION_OBJ(chunk_->constant_[constant_offset]);
            for(int i = 0; i < function->up_value_count; i++){
                int is_local = chunk_->code_[offset_++];
                int index = DEBUG_READ_LONG();
                offset_ += 4;
                fmt::print("{0:0>#6x} {1:>4}\t{2:} {3:}\n", offset_ - 2, "|", is_local ? "local" : "upvalue", index);
            }
            return;
        }
        case OpCode::OP_GET_UPVALUE:{return ByteInstruction("OP_GET_UPVALUE");}
        case OpCode::OP_SET_UPVALUE:{return ByteInstruction("OP_SET_UPVALUE");}
        case OpCode::OP_CLOSE_UPVALUE:{return SimpleInstruction("OP_CLOSE_UPVALUE");}
        case OpCode::OP_CLASS:{return ConstantInstruction("OP_CLASS", false);}
        case OpCode::OP_GET_PROPERTY:{return ConstantInstruction("OP_GET_PROPERTY", false);}
        case OpCode::OP_SET_PROPERTY:{return ConstantInstruction("OP_SET_PROPERTY", false);}
        case OpCode::OP_METHOD:{return ConstantInstruction("OP_METHOD", false);}
        case OpCode::OP_INVOKE:{return InvokeInstruction("OP_INVOKE");}
        case OpCode::OP_INHERIT:{return SimpleInstruction("OP_INHERIT");}
        case OpCode::OP_GET_SUPER:{return ConstantInstruction("OP_GET_SUPER", false);}
        case OpCode::OP_SUPER_INVOKE:{return InvokeInstruction("OP_SUPER_INVOKE");}
        case OpCode::OP_ARRAY_CREATE:{return SimpleInstruction("OP_ARRAY_CREATE");}
        case OpCode::OP_ARRAY_GET:{return SimpleInstruction("OP_ARRAY_GET");}
        case OpCode::OP_ARRAY_SET:{return SimpleInstruction("OP_ARRAY_SET");}
        default:{
            cout << "invalid OpCode!" << endl;
            return;
        }
    }
}

