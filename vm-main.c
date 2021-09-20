#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: tvm <file>\n");
    exit(64);
  }

  Chunk* chunk = loadChunk(argv[1]);
  initVM();

  interpret(chunk);
  
  freeVM();
  freeChunk(chunk);

  return 0;
}
