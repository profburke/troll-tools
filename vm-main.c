#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

int main(int argc, char* argv[]) {
  initVM();
  
  Chunk chunk;
  initChunk(&chunk);

  int constant = addConstant(&chunk, 1.2);
  writeChunk(&chunk, OP_CONSTANT, 123);
  writeChunk(&chunk, constant, 123);

  constant = addConstant(&chunk, 3.14);
  writeChunk(&chunk, OP_CONSTANT, 124);
  writeChunk(&chunk, constant, 124);
  writeChunk(&chunk, OP_NEGATE, 124);

  writeChunk(&chunk, OP_MULTIPLY, 125);
  
  constant = addConstant(&chunk, 6);
  writeChunk(&chunk, OP_CONSTANT, 126);
  writeChunk(&chunk, constant, 126);
  writeChunk(&chunk, OP_DIE, 126);
  
  writeChunk(&chunk, OP_RETURN, 126);

  interpret(&chunk);
  
  freeVM();
  freeChunk(&chunk);

  return 0;
}
