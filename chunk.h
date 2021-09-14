#ifndef tvm_chunk_h
#define tvm_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_ADD,
  OP_DIE,
  OP_DIVIDE,
  OP_CONSTANT,
  OP_MULTIPLY,
  OP_NEGATE,
  OP_RETURN,
  OP_SUBTRACT,
  OP_ZERO_DIE
} OpCode;

typedef struct {
  int count;
  int capacity;
  uint8_t* code;
  int* lines;
  ValueArray constants;
} Chunk;

int addConstant(Chunk* chunk, Value value);
void freeChunk(Chunk* chunk);
void initChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);

#endif
