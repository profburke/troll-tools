#ifndef tvm_value_h
#define tvm_value_h

#include "common.h"

typedef enum {
  VAL_INTEGER,
  VAL_REAL
} ValueType;

typedef struct {
  ValueType type;
  union {
    int integer;
    double real;
  } as;
} Value;

#define IS_INTEGER(value) ((value).type == VAL_INTEGER)
#define IS_REAL(value) ((value).type == VAL_REAL)

#define AS_INTEGER(value) ((value).as.integer)
#define AS_REAL(value) ((value).as.real)

#define INTEGER_VAL(value) ((Value){VAL_INTEGER, {.integer = value}})
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
