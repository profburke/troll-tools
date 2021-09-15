#ifndef tvm_compiler_h
#define tvm_compiler_h

#include "vm.h"

void compile(const char* source);
InterpretResult runCompiler(const char* source);

#endif
