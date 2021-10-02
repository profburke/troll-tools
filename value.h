#ifndef tvm_value_h
#define tvm_value_h

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjPair ObjPair;
typedef struct ObjString ObjString;

typedef enum {
  VAL_INTEGER,
  VAL_OBJ,
  VAL_REAL
} ValueType;

typedef struct {
  ValueType type;
  union {
    int integer;
    Obj* obj;
    double real;
  } as;
} Value;

#define IS_INTEGER(value) ((value).type == VAL_INTEGER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)
#define IS_REAL(value) ((value).type == VAL_REAL)

#define AS_INTEGER(value) ((value).as.integer)
#define AS_OBJ(value) ((value).as.obj)
#define AS_REAL(value) ((value).as.real)

#define INTEGER_VAL(value) ((Value){VAL_INTEGER, {.integer = value}})
#define OBJ_VAL(object)  ((Value){VAL_OBJ, {.obj = (Obj*)object}})
#define REAL_VAL(value) ((Value){VAL_REAL, {.real = value}})

typedef struct {
  int capacity;
  int count;
  Value* values;
} ValueArray;

void freeValueArray(ValueArray* array);
void initValueArray(ValueArray* array);
void printValue(Value value);
void writeValueArray(ValueArray* array, Value value);

#endif
