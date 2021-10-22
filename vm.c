#include <math.h>
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
  freeTable(&vm.globals);
}

void initVM() {
  resetStack();
  initTable(&vm.globals);
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
        push(OBJ_VAL(initCollection()));
      } else {
        push(OBJ_VAL(d));
      }
      break;
    }
    case OP_CHOOSE: {
      CHECK_COLLECTION(0, "Can only 'choose' from a collection.");
      ObjCollection* c = AS_COLLECTION(pop());
      int index = randomi(c->count);
      push(INTEGER_VAL(c->ints[index]));
      break;
    }
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }
    case OP_COUNT: {
      CHECK_COLLECTION(0, "Operand for 'count' must be a collection.");
      ObjCollection *c = AS_COLLECTION(pop());
      push(INTEGER_VAL(c->count));
      break;
    }
    case OP_DEFINE_GLOBAL: {
      ObjString* name = READ_STRING();
      tableSet(&vm.globals, name, peek(0));
      pop();
      break;
    }
    case OP_DIE: {
      CHECK_POSITIVE_INTEGER(0, "Expression for die sides must be a positive integer.");
      int sides = AS_INTEGER(pop());
      push(INTEGER_VAL(randomi(sides) + 1));
      break;
    }
    case OP_DIFFERENT: {
      CHECK_COLLECTION(0, "Operand to 'different' must be a collection.");
      ObjCollection* c = AS_COLLECTION(pop());
      ObjCollection* r = initCollection();
      for (int i = 0; i < c->count; i++) {
        int d = c->ints[i];
        if (!member(r, d)) {
          addToCollection(r, d);
        }
      }
      push(OBJ_VAL(r));
      break;
    }
    case OP_DIVIDE:
      BINARY_OP(INTEGER_VAL, /);
      break;
    case OP_DROP: {
      CHECK_COLLECTION(0, "Operands to drop must be collections.");
      CHECK_COLLECTION(1, "Operands to drop must be collections.");
      ObjCollection* d = AS_COLLECTION(pop());
      ObjCollection* c = AS_COLLECTION(pop());
      ObjCollection* r = initCollection();
      for (int i = 0; i < c->count; i++) {
        int item = c->ints[i];
        if (!member(d, item)) {
          addToCollection(r, item);
        }
      }
      push(OBJ_VAL(r));
      break;
    }
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
    case OP_GET_GLOBAL: {
      ObjString* name = READ_STRING();
      Value value;
      if (!tableGet(&vm.globals, name, &value)) {
        runtimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      push(value);
      break;
    }
    case OP_GT:
      REL_OP(>);
      break;
    case OP_HCONC:
      BINARY_STRING_OP("h");
      break;
    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      vm.ip += offset;
      break;
    }
    case OP_JUMP_IF_EMPTY: {
      bool doJump = false;
      if (IS_INTEGER(peek(0))) {
        pop(); // any integer is a non-empty collection, so not jumping
      } else if (IS_COLLECTION(peek(0))) {
        ObjCollection* c = AS_COLLECTION(pop());
        doJump = (c->count == 0);
      } else {
        runtimeError("If expression must return a collection (or single integer).");
        return INTERPRET_RUNTIME_ERROR;
      }
      uint16_t offset = READ_SHORT();
      if (doJump) {
        vm.ip += offset;
      }
      break;
    }
    case OP_KEEP: {
      CHECK_COLLECTION(0, "Operands to drop must be collections.");
      CHECK_COLLECTION(1, "Operands to drop must be collections.");
      ObjCollection* d = AS_COLLECTION(pop());
      ObjCollection* c = AS_COLLECTION(pop());
      ObjCollection* r = initCollection();
      for (int i = 0; i < c->count; i++) {
        int item = c->ints[i];
        if (member(d, item)) {
          addToCollection(r, item);
        }
      }
      push(OBJ_VAL(r));
      break;
    }
    case OP_LARGEST: {
      CHECK_COLLECTION(0, "'largest' only works on collections.");
      CHECK_INTEGER(1, "First argument to 'largest' must be an intger.");
      ObjCollection* c = AS_COLLECTION(pop());
      int n = AS_INTEGER(pop());
      reverseSortCollection(c);
      int upper = (int)fmin(c->count, n);
      ObjCollection* r = initCollection();
      for (int i = 0; i < upper; i++) {
        addToCollection(r, c->ints[i]);
      }
      push(OBJ_VAL(r));
      break;
    }
    case OP_LE:
      REL_OP(<=);
      break;
    case OP_LEAST: {
      CHECK_COLLECTION(0, "'least' only works on collections.");
      CHECK_INTEGER(1, "First argument to 'least' must be an intger.");
      ObjCollection* c = AS_COLLECTION(pop());
      int n = AS_INTEGER(pop());
      sortCollection(c);
      int upper = (int)fmin(c->count, n);
      ObjCollection* r = initCollection();
      for (int i = 0; i < upper; i++) {
        addToCollection(r, c->ints[i]);
      }
      push(OBJ_VAL(r));
      break;
    }
    case OP_LT:
      REL_OP(<);
      break;
    case OP_MAX: {
      CHECK_COLLECTION(0, "Operand to 'max' must be a non-empty collection.");
      ObjCollection* c = AS_COLLECTION(pop());
      if (c->count == 0) {
        runtimeError("Can only compute max of a non-empty collection.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int max = INT32_MIN;
      for (int i = 0; i < c->count; i++) {
        if (c->ints[i] > max) {
          max = c->ints[i];
        }
      }
      push(INTEGER_VAL(max));
      break;
    }
    case OP_MAXIMAL: {
      CHECK_COLLECTION(0, "Operand to 'maximal' must be a collection.");
      ObjCollection* c = AS_COLLECTION(pop());
      int max = INT32_MIN;
      for (int i = 0; i < c->count; i++) {
        if (c->ints[i] > max) {
          max = c->ints[i];
        }
      }
      ObjCollection* r = initCollection();
      for (int i = 0; i < c->count; i++) {
        if (c->ints[i] == max) {
          addToCollection(r, c->ints[i]);
        }
      }
      push(OBJ_VAL(r));
      break;
    }
    case OP_MDIE: {
      CHECK_POSITIVE_INTEGER(0, "Expression for die sides must be a positive integer.");
      CHECK_POSITIVE_INTEGER(1, "Expression for number of die must be a positive integer.");
      int sides = AS_INTEGER(pop());
      int ndice = AS_INTEGER(pop());
      ObjCollection* c = initCollection();
      push(OBJ_VAL(c));
      for (int i = 0; i < ndice; i++) {
        int r = randomi(sides) + 1;
        addToCollection(c, r);
      }
      break;
    }
    case  OP_MEDIAN: {
      CHECK_COLLECTION(0, "Operand for 'median' must be a non-empty collection.");
      ObjCollection* c = AS_COLLECTION(pop());
      if (c->count == 0) {
        runtimeError("Can only compute median of a non-empty collection.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int i = (c->count)/2;
      sortCollection(c);
      push(INTEGER_VAL(c->ints[i]));
      break;
    }
    case OP_MIN: {
      CHECK_COLLECTION(0, "Operand to 'min' must be a non-empty collection.");
      ObjCollection* c = AS_COLLECTION(pop());
      if (c->count == 0) {
        runtimeError("Can only compute min of a non-empty collection.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int min = INT32_MAX;
      for (int i = 0; i < c->count; i++) {
        if (c->ints[i] < min) {
          min = c->ints[i];
        }
      }
      push(INTEGER_VAL(min));
      break;
    }
    case OP_MINIMAL: {
      CHECK_COLLECTION(0, "Operand to 'minimal' must be a collection.");
      ObjCollection* c = AS_COLLECTION(pop());
      int min = INT32_MAX;
      for (int i = 0; i < c->count; i++) {
        if (c->ints[i] < min) {
          min = c->ints[i];
        }
      }
      ObjCollection* r = initCollection();
      for (int i = 0; i < c->count; i++) {
        if (c->ints[i] == min) {
          addToCollection(r, c->ints[i]);
        }
      }
      push(OBJ_VAL(r));
      break;
    }
    case OP_MZDIE: {
      CHECK_POSITIVE_INTEGER(0, "Expression for die sides must be a positive integer.");
      CHECK_POSITIVE_INTEGER(1, "Expression for number of die must be a positive integer.");
      int sides = AS_INTEGER(pop());
      int ndice = AS_INTEGER(pop());
      ObjCollection* c = initCollection();
      push(OBJ_VAL(c));
      for (int i = 0; i < ndice; i++) {
        int r = randomi(sides + 1);
        addToCollection(c, r);
      }
      break;
    }
    case OP_MKCOLLECTION: {
      ObjCollection* c = initCollection();
      push(OBJ_VAL(c));
      break;
    }
    case OP_MKPAIR: {
      Value b = pop();
      Value a = pop();
      ObjPair* p = initPair(a, b);
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
    case OP_NOT: {
      CHECK_COLLECTION(0, "Operand to '!' must be a collection.");
      ObjCollection* c = AS_COLLECTION(pop());
      if (c->count == 0) {
        push(INTEGER_VAL(1));
      } else {
        push(OBJ_VAL(initCollection()));
      }
      break;
    }
    case OP_PICK: {
      CHECK_INTEGER(0, "Right operand to 'pick' must be a positive integer.");
      CHECK_COLLECTION(1, "Left operand to 'pick' must be a collection.");
      int n = AS_INTEGER(pop());
      if (n < 1) {
        runtimeError("Right operand to 'pick' must be a positive integer.");
        return INTERPRET_RUNTIME_ERROR;
      }
      ObjCollection* c = AS_COLLECTION(pop());
      ObjCollection* candidates = copyCollection(c);
      if (n >= candidates->count) {
        push(OBJ_VAL(candidates));
      } else {
        ObjCollection* r = initCollection();
        for (int i = 0; i < n; i ++) {
          int index = randomi(candidates->count);
          addToCollection(r, candidates->ints[index]);
          removeAtIndex(candidates, index);
        }
        push(OBJ_VAL(r));
      }
      break;
    }
    case OP_QUESTION: {
      CHECK_REAL(0, "Operand to '?' must be a real number in range (0, 1).");
      double p = AS_REAL(pop());
      double v = uniform();
      if (v < p) {
        push(INTEGER_VAL(1));
      } else {
        ObjCollection* c = initCollection();
        push(OBJ_VAL(c));
      }
      break;
    }
    case OP_RANGE: {
      CHECK_INTEGER(0, "Operands to range must be integers.");
      CHECK_INTEGER(1, "Operands to range must be integers.");
      int r = AS_INTEGER(pop());
      int l = AS_INTEGER(pop());
      ObjCollection* c = initCollection();
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
    case OP_SETMINUS: {
      CHECK_COLLECTION(0, "Union operands must be collections.");
      CHECK_COLLECTION(1, "Union operands must be collections.");
      ObjCollection *d = AS_COLLECTION(pop());
      ObjCollection *c = AS_COLLECTION(pop());
      ObjCollection *r = copyCollection(c);

      for (int i = 0; i < d->count; i++) {
        int index = findFirstIndex(r, d->ints[i]);
        if (index > -1) {
          removeAtIndex(r, index);
        }
      }
      push(OBJ_VAL(r));
      break;
    }
    case OP_SGN: {
      CHECK_INTEGER(0, "Operand for 'sgn' must be an integer.");
      int v = AS_INTEGER(pop());
      int r = 0;
      if (v < 0) {
        r = -1;
      } else if (v > 0) {
        r = 1;
      }
      push(INTEGER_VAL(r));
      break;
    }
    case OP_SUBTRACT:
      BINARY_OP(INTEGER_VAL, -);
      break;
    case OP_SUM: {
      CHECK_COLLECTION(0, "Operand for 'sum' must be a collection.");
      ObjCollection* c = AS_COLLECTION(pop());
      int sum = 0;
      for (int i = 0; i < c->count; i++) {
        sum += c->ints[i];
      }
      push(INTEGER_VAL(sum));
      break;
    }
    case OP_UNION: {
      CHECK_COLLECTION(0, "Union operands must be collections.");
      CHECK_COLLECTION(1, "Union operands must be collections.");
      ObjCollection *d = AS_COLLECTION(pop());
      ObjCollection *c = AS_COLLECTION(pop());
      ObjCollection *u = initCollection();
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

