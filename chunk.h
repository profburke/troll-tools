#ifndef tvm_chunk_h
#define tvm_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_ADD,
  OP_ADD2CLLCTN, // following byte is count of # of ints to pop off stack and add to c
  OP_AND,
  OP_CONSTANT,
  OP_DIE,
  OP_DIVIDE,
  OP_DROP,
  OP_EQ,
  OP_FIRST,
  OP_GE,
  OP_GT,
  OP_HCONC,
  OP_KEEP,
  OP_LE,
  OP_LT,
  OP_MDIE,
  OP_MZDIE,
  OP_MKCOLLECTION,
  OP_MKPAIR,
  OP_MOD,
  OP_MULTIPLY,
  OP_NEGATE,
  OP_NEQ,
  OP_NOT,
  OP_PICK,
  OP_QUESTION,
  OP_RANGE,
  OP_RETURN,
  OP_SECOND,
  OP_SETMINUS,
  OP_SUBTRACT,
  OP_UNION,
  OP_VCONCC,
  OP_VCONCL,
  OP_VCONCR,
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
Chunk* loadChunk(const char* path);
void saveChunk(Chunk* chunk, const char* path);
void writeChunk(Chunk* chunk, uint8_t byte, int line);

#endif
