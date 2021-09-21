#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "debug.h"
#include "vm.h"

static Value pop();
static void push(Value value);
static void resetStack();
static InterpretResult run();

VM vm;

void freeVM() {
}

void initVM() {
  resetStack();
}

InterpretResult interpret(Chunk* chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;
  return run();
}

static Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

static void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

static void resetStack() {
  vm.stackTop = vm.stack;
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op) \
  do { \
    double b = pop(); \
    double a = pop(); \
    push(a op b); \
  } while(false)
  
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
    
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
    case OP_ADD: BINARY_OP(+); break;
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }
    case OP_DIE: {
      Value v = pop();
      int sides = v;
      // need to abstract this out so that we can easily include
      // different functions for different platforms (is arc4random available
      // on Arduino/ESP8266 ?
      Value value = arc4random_uniform(sides) + 1;
      push(value);
      break;
    }
    case OP_DIVIDE: BINARY_OP(/); break;
    case OP_MULTIPLY: BINARY_OP(*); break;
    case OP_NEGATE: push(-pop()); break;
    case OP_RETURN: {
      printValue(pop());
      printf("\n");
      return INTERPRET_OK;
    }
    case OP_SUBTRACT: BINARY_OP(-); break;
    case OP_ZERO_DIE: {
      Value v = pop();
      int sides = v;
      // see block for OP_DIE
      Value value = arc4random_uniform(sides);
      push(value);
      break;
    }
    }
  }

  #undef READ_BYTE
  #undef READ_CONSTANT
  #undef BINARY_OP
}

