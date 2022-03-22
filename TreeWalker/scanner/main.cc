#include "include/cpplox.h"
#include <iostream>
using namespace std;
 

int main(int argc, char* argv[])
{
    Lox lox;
    if(argc > 2){
        cout << "[!]Usage lox [file]" << endl;
        exit(64);
    }
    else if(argc == 2){
        lox.RunWithFile(argv[1]);
    }
    else if(argc == 1)
    {
        lox.RunWithInteractive();
    }
}