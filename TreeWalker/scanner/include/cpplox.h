#ifndef __LOX_
#define __LOX_
#include "ErrorReporter.h"
class Scanner;
class Parser;
class Interpreter;
class Lox
{
public:
    void RunWithFile(string filename);
    void RunWithInteractive();
private:
    void run(string& source, Interpreter& inner_interpreter);
    ErrorReporter error_repoter;
    friend class Scanner;
    friend class Parser;
    friend class Interpreter;
};

#endif