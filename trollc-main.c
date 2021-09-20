#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "vm.h"

static char* readFile(const char* path) {
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file '%s'.\n", path);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);
  size_t fileSize = ftell(file);
  rewind(file);

  char* buffer = (char*)malloc(fileSize + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read '%s'.\n", path);
    exit(74);
  }
  
  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
  if (bytesRead < fileSize) {
    fprintf(stderr, "Could not read file '%s'.\n", path);
    exit(74);
  }

  buffer[bytesRead] = '\0';

  fclose(file);
  return buffer;
}

static void compileFile(char* path) {
  Chunk chunk;
  initChunk(&chunk);
  
  char* source = readFile(path);

  if (!compile(source, &chunk)) {
    freeChunk(&chunk);
    free(source);
    exit(65);
  }
  // hack to change output file name; TODO: do this properly
  int n = strlen(path);
  path[n-1] = 'g';
  saveChunk(&chunk, path);
  
  freeChunk(&chunk);
  free(source);
}

int main(int argc, char* argv[]) {
  if (argc == 2) {
    compileFile(argv[1]);
  } else {
    fprintf(stderr, "usage: trollc <file>\n");
    exit(64);
  }
  return 0;
}
