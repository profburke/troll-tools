#include <stdio.h>

#include "memory.h"
#include "object.h"
#include "value.h"

void freeValueArray(ValueArray* array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  initValueArray(array);
}

void initValueArray(ValueArray* array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}

void printValue(Value value) {
  switch (value.type) {
  case VAL_INTEGER: printf("%d", AS_INTEGER(value)); break;
  case VAL_OBJ: printObject(value); break;
  case VAL_REAL: printf("%g", AS_REAL(value)); break;
  }
}

void writeValueArray(ValueArray* array, Value value) {
  if (array->capacity < array->count + 1) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
  }
  
  array->values[array->count] = value;
  array->count++;
}
