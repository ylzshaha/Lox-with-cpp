# Lox with cpp

Lox is a small high-level toy language created by Robert Nystrom. 

The language has a lot in common with JavaScript such as scope features, function closures, and OOP implementations so it helps a lot when i tried to read the V8's source code. This repository has two kinds of lox interpreter written according to [Robert's book](http://www.craftinginterpreters.com/contents.html) by cpp.

## TreeWalker

This is a interpreter executes code by traversing the AST. It consists the following parts:

- a lexical scanner
- a recursive descent parser 
- a interpreter traverse the AST to execute the code.

you can get the first lox by running the following code.

```bash
cd TreeWalker 
make all
```

## BytecodeEater

This is a interpreter executes code by dispatching the bytecode. It consists the following parts:

- a lexical scanner
- a recursive descent parser
- a compiler traverses the AST and emits bytecode
- a VM executes bytecode
- a stop-world and mark-and-sweep garbage collector

you can get the first lox by running the following code.

```bash
cd TreeWalker 
make all
```

