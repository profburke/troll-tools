#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "debug.h"

void decom(const char* path) {
  Chunk* chunk = loadChunk(path);
  disassembleChunk(chunk, "come up with something");
}

int main(int argc, char* argv[]) {
  if (argc == 2) {
    decom(argv[1]);
  } else {
    fprintf(stderr, "usage: decom <file>\n");
    exit(64);
  }
  return 0;
}
