#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

int addConstant(Chunk* chunk, Value value) {
  writeValueArray(&chunk->constants, value);
  return chunk->constants.count - 1;
}

void freeChunk(Chunk* chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(int, chunk->lines, chunk->capacity);
  freeValueArray(&chunk->constants);
  initChunk(chunk);
}

void initChunk(Chunk* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  initValueArray(&chunk->constants);
}

Chunk* loadChunk(const char* path) {
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file '%s'.\n", path);
    exit(74);
  }

  Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
  initChunk(chunk);
  
  // read first int to get number of bytecodes
  // now can just loop and writeChunk.... consider
  // pre-allocating buffers...
  int nOps;
  int nConstants;
  uint8_t op;
  
  fread(&nOps, sizeof(int), 1, file);
  fread(&nConstants, sizeof(int), 1, file);
  
  for (int i = 0; i < nOps; i++) {
    fread(&op, sizeof(uint8_t), 1, file);
    writeChunk(chunk, op, -1);
  }

  // now read in line info and overwrite the '-1's
  fread(chunk->lines, sizeof(int), nOps, file);
  
  // now read in constants
  //  fread(chunk->constants.values, sizeof(Value), nConstants, file);
  Value value;
  for (int i = 0; i < nConstants; i++) {
    fread(&value, sizeof(Value), 1, file);
    addConstant(chunk, value);
  }
  
  fclose(file);
  return chunk;
}

void saveChunk(Chunk* chunk, const char* path) {
  FILE* file = fopen(path, "wb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file '%s'.\n", path);
    exit(74);
  }

  fwrite(&chunk->count, sizeof(int), 1, file);
  fwrite(&chunk->constants.count, sizeof(int), 1, file);
  
  int nWritten = fwrite(chunk->code, sizeof(uint8_t), chunk->count, file);
  if (nWritten != chunk->count) {
    fprintf(stderr, "Could not write bytecodes to file '%s'.\n", path);
    fclose(file);
    exit(74);
  }

  nWritten = fwrite(chunk->lines, sizeof(int), chunk->count, file);
  if (nWritten != chunk->count) {
    fprintf(stderr, "Could not write line number information to file '%s'.\n", path);
    fclose(file);
    exit(74);
  }

  // now write out constants
  nWritten = fwrite(chunk->constants.values, sizeof(Value), chunk->constants.count, file);
  if (nWritten != chunk->constants.count) {
    fprintf(stderr, "Could not write chunk constants to file '%s'.\n", path);
    fclose(file);
    exit(74);
  }
  
  fclose(file);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
  if (chunk->capacity < chunk->count + 1) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
  }

  chunk->code[chunk->count] = byte;
  chunk->lines[chunk->count] = line;
  chunk->count++;
}
