#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "random.h"
#include "vm.h"
#include "vm-macros.h"

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
    case OP_ADD:
      BINARY_OP(INTEGER_VAL, +);
      break;
    case OP_ADD2CLLCTN: {
      CHECK_COLLECTION(0, "Must have a collection to add to.");
      ObjCollection* c = AS_COLLECTION(pop());
      uint8_t n = READ_BYTE();
      for (int i = 0; i < n; i++) {
        CHECK_INTEGER(0, "Can only add integers to a collection.");
        addToCollection(c, AS_INTEGER(pop()));
      }
      push(OBJ_VAL(c));
      break;
    }
    case OP_AND: {
      CHECK_COLLECTION(0, "Operands to '&' must be collections.");
      CHECK_COLLECTION(1, "Operands to '&' must be collections.");
      ObjCollection* c = AS_COLLECTION(pop());
      ObjCollection* d = AS_COLLECTION(pop());
      if (c->count == 0) {
        push(OBJ_VAL(makeCollection()));
      } else {
        push(OBJ_VAL(d));
      }
      break;
    }
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }
    case OP_DIE: {
      CHECK_POSITIVE_INTEGER(0, "Expression for die sides must be a positive integer.");
      int sides = AS_INTEGER(pop());
      push(INTEGER_VAL(randomi(sides) + 1));
      break;
    }
    case OP_DIVIDE:
      BINARY_OP(INTEGER_VAL, /);
      break;
    case OP_EQ:
      REL_OP(==);
      break;
    case OP_FIRST: {
      CHECK_PAIR(0, "Operand must be a pair.");
      ObjPair* p = AS_PAIR(pop());
      push(p->a);
      break;
    }
    case OP_GE:
      REL_OP(>=);
      break;
    case OP_GT:
      REL_OP(>);
      break;
    case OP_HCONC:
      BINARY_STRING_OP("h");
      break;
    case OP_LE:
      REL_OP(<=);
      break;
    case OP_LT:
      REL_OP(<);
      break;
    case OP_MDIE: {
      CHECK_POSITIVE_INTEGER(0, "Expression for die sides must be a positive integer.");
      CHECK_POSITIVE_INTEGER(1, "Expression for number of die must be a positive integer.");
      int sides = AS_INTEGER(pop());
      int ndice = AS_INTEGER(pop());
      ObjCollection* c = makeCollection();
      push(OBJ_VAL(c));
      for (int i = 0; i < ndice; i++) {
        int r = randomi(sides) + 1;
        addToCollection(c, r);
      }
      break;
    }
    case OP_MZDIE: {
      CHECK_POSITIVE_INTEGER(0, "Expression for die sides must be a positive integer.");
      CHECK_POSITIVE_INTEGER(1, "Expression for number of die must be a positive integer.");
      int sides = AS_INTEGER(pop());
      int ndice = AS_INTEGER(pop());
      ObjCollection* c = makeCollection();
      push(OBJ_VAL(c));
      for (int i = 0; i < ndice; i++) {
        int r = randomi(sides + 1);
        addToCollection(c, r);
      }
      break;
    }
    case OP_MKCOLLECTION: {
      ObjCollection* c = makeCollection();
      push(OBJ_VAL(c));
      break;
    }
    case OP_MKPAIR: {
      Value b = pop();
      Value a = pop();
      ObjPair* p = makePair(a, b);
      push(OBJ_VAL(p));
      break;
    }
    case OP_MOD:
      BINARY_OP(INTEGER_VAL, %);
      break;
    case OP_MULTIPLY:
      BINARY_OP(INTEGER_VAL, *);
      break;
    case OP_NEGATE: {
      CHECK_INTEGER(0, "Operand to unary minus must be an integer.");
      push(INTEGER_VAL(-AS_INTEGER(pop())));
      break;
    }
    case OP_NEQ:
      REL_OP(!=);
      break;
    case OP_QUESTION: {
      CHECK_REAL(0, "Operand to '?' must be a real number in range (0, 1).");
      double p = AS_REAL(pop());
      double v = uniform();
      if (v < p) {
        push(INTEGER_VAL(1));
      } else {
        ObjCollection* c = makeCollection();
        push(OBJ_VAL(c));
      }
      break;
    }
    case OP_RANGE: {
      CHECK_INTEGER(0, "Operands to range must be integers.");
      CHECK_INTEGER(1, "Operands to range must be integers.");
      int r = AS_INTEGER(pop());
      int l = AS_INTEGER(pop());
      ObjCollection* c = makeCollection();
      for (int i = l; i < r; i++) {
        addToCollection(c, i);
      }
      push(OBJ_VAL(c));
      break;
    }
    case OP_RETURN: {
      printValue(pop());
      printf("\n");
      return INTERPRET_OK;
    }
    case OP_SECOND: {
      CHECK_PAIR(0, "Operand must be a pair.");
      ObjPair* p = AS_PAIR(pop());
      push(p->b);
      break;
    }
    case OP_SUBTRACT:
      BINARY_OP(INTEGER_VAL, -);
      break;
    case OP_UNION: {
      CHECK_COLLECTION(0, "Union operands must be collections.");
      CHECK_COLLECTION(1, "Union operands must be collections.");
      ObjCollection *c = AS_COLLECTION(pop());
      ObjCollection *d = AS_COLLECTION(pop());
      ObjCollection *u = makeCollection();
      for (int i = 0; i < c->count; i++) {
        addToCollection(u, c->ints[i]);
      }
      for (int i = 0; i < d->count; i++) {
        addToCollection(u, d->ints[i]);
      }
      push(OBJ_VAL(u));
      break;
    }
    case OP_VCONCC:
      BINARY_STRING_OP("cc");
      break;
    case OP_VCONCL:
      BINARY_STRING_OP("cl");
      break;
    case OP_VCONCR:
      BINARY_STRING_OP("cr");
      break;
    case OP_ZERO_DIE: {
      CHECK_POSITIVE_INTEGER(0, "Expression for die sides must be a positive integer.");
      int sides = AS_INTEGER(pop());
      push(INTEGER_VAL(randomi(sides + 1)));
      break;
    }
    }
  }
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

