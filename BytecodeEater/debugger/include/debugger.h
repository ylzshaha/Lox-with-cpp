#ifndef __DEBUGGER_
#define __DEBUGGER_
#include <cstdint>
#include <string>
#include <map>
#include "../../helper/bytecode.h"

class Debugger
{
public:
    Debugger(ChunkPtr chunk):
        chunk_(chunk){}
    Debugger(ChunkPtr chunk, std::uint32_t offset):
        chunk_(chunk), offset_(offset){} 
    void ChunkDisassemble(std::string name);
    void InstructionDisassemble();
private:
    void SimpleInstruction(std::string name);
    void ConstantInstruction(std::string name, bool is_long);
    void ByteInstruction(std::string name);
    void JumpInstruction(std::string name, int sign);
    void InvokeInstruction(std::string name);
    int offset_ = 0;
    ChunkPtr chunk_;
};


#endif