#ifndef tvm_vm_macros_h
#define tvm_vm_macros_h

#define BINARY_OP(valueType, op) \
  do { \
    CHECK_INTEGER(0, "Operands to binary operator must be integers."); \
    CHECK_INTEGER(1, "Operands to binary operator must be integers."); \
    int b = AS_INTEGER(pop()); \
    int a = AS_INTEGER(pop()); \
    push(valueType(a op b)); \
  } while(false)

// TODO: ugh...actually implementing these is going to be fun...
#define BINARY_STRING_OP(op) \
  do { \
    CHECK_STRING(0, "Operands to concat operator must be string."); \
    CHECK_STRING(1, "Operands to concat operator must be string."); \
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
  
#define CHECK_OPERAND(typeCheck, index, message)        \
  do {                                                  \
    if(!typeCheck(peek(index))) {                       \
      runtimeError(message);                            \
      return INTERPRET_RUNTIME_ERROR;                   \
    }                                                   \
  } while (false)

#define CHECK_COLLECTION(index, message) \
  CHECK_OPERAND(IS_COLLECTION, index, message)

#define CHECK_INTEGER(index, message) \
  CHECK_OPERAND(IS_INTEGER, index, message)

#define CHECK_PAIR(index, message) \
  CHECK_OPERAND(IS_PAIR, index, message)

#define CHECK_POSITIVE_INTEGER(index, message)                          \
  do {                                                                  \
    if (!IS_INTEGER(peek(index)) || !(AS_INTEGER(peek(index)) > 0) ) {  \
      runtimeError(message);                                            \
      return INTERPRET_RUNTIME_ERROR;                                   \
    }                                                                   \
  } while (false)

#define CHECK_REAL(index, message) \
  CHECK_OPERAND(IS_REAL, index, message)

#define CHECK_STRING(index, message) \
  CHECK_OPERAND(IS_STRING, index, message)

#define READ_BYTE() (*vm.ip++)

#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

#endif
