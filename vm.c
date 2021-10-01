#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

static Value peek(int distance);
static Value pop();
static void push(Value value);
static void resetStack();
static InterpretResult run();
static void runtimeError(const char* format, ...);

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
#define BINARY_OP(valueType, op) \
  do { \
    if (!IS_INTEGER(peek(0)) || !IS_INTEGER(peek(1))) { \
      runtimeError("Operands to binary operator must be integers."); \
      return INTERPRET_RUNTIME_ERROR; \
    } \
    int b = AS_INTEGER(pop()); \
    int a = AS_INTEGER(pop()); \
    push(valueType(a op b)); \
  } while(false)
// TODO: ugh...actually implementing these are going to be fun...
#define BINARY_STRING_OP(op) \
  do { \
    if (!IS_STRING(peek(0)) || !IS_STRING(peek(1))) { \
      runtimeError("Operands to concat operator must be string."); \
      return INTERPRET_RUNTIME_ERROR; \
    } \
    ObjString* a = AS_STRING(pop()); \
    ObjString* b = AS_STRING(pop()); \
    int length = a->length + b->length; \
    char* chars = ALLOCATE(char, length + 1); \
    memcpy(chars, a->chars, a->length); \
    memcpy(chars + a->length, b->chars, b->length); \
    chars[length] = '\0'; \
    ObjString* c = takeString(chars, length); \
    push(OBJ_VAL(c)); \
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
    case OP_ADD: BINARY_OP(INTEGER_VAL, +); break;
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }
    case OP_DIE: {
      if (!IS_INTEGER(peek(0))) {
        runtimeError("Expression for die sides must be an integer.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int sides = AS_INTEGER(pop());
      // need to abstract this out so that we can easily include
      // different functions for different platforms (is arc4random available
      // on Arduino/ESP8266 ?
      push(INTEGER_VAL(arc4random_uniform(sides) + 1));
      break;
    }
    case OP_DIVIDE: BINARY_OP(INTEGER_VAL, /); break;
    case OP_HCONC: BINARY_STRING_OP("h"); break;
    case OP_MOD: BINARY_OP(INTEGER_VAL, %); break;
    case OP_MULTIPLY: BINARY_OP(INTEGER_VAL, *); break;
    case OP_NEGATE:
      if (!IS_INTEGER(peek(0))) {
        runtimeError("Operand to unary minus must be an integer.");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(INTEGER_VAL(-AS_INTEGER(pop())));
      break;
    case OP_QUESTION:
      if (!IS_REAL(peek(0))) {
        runtimeError("Operand to '?' must be a real number in range (0, 1).");
        return INTERPRET_RUNTIME_ERROR;
      }
      double p = AS_REAL(pop());
      double v = (double)arc4random()/UINT32_MAX;
      if (v < p) {
        push(REAL_VAL(1));
      } else {
        push(REAL_VAL(0)); // TODO: need to push an empty collection
      }
      break;
    case OP_RETURN: {
      printValue(pop());
      printf("\n");
      return INTERPRET_OK;
    }
    case OP_SUBTRACT: BINARY_OP(INTEGER_VAL, -); break;
    case OP_VCONCC: BINARY_STRING_OP("cc"); break;
    case OP_VCONCL: BINARY_STRING_OP("cl"); break;
    case OP_VCONCR: BINARY_STRING_OP("cr"); break;
    case OP_ZERO_DIE: {
      if (!IS_INTEGER(peek(0))) {
        runtimeError("Expression for die sides must be an integer.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int sides = AS_INTEGER(pop());
      // see block for OP_DIE
      push(INTEGER_VAL(arc4random_uniform(sides)));
      break;
    }
    }
  }

  #undef READ_BYTE
  #undef READ_CONSTANT
  #undef BINARY_OP
}

static Value peek(int distance) {
  return vm.stackTop[-1 - distance];
}

static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = vm.ip - vm.chunk->code - 1;
  int line = vm.chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  resetStack();
}

