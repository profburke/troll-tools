#include <stdio.h>

#include "debug.h"
#include "value.h"

void disassembleChunk(Chunk* chunk, const char* name) {
  printf("== %s ==\n", name);
  
  for (int offset = 0; offset < chunk->count;) {
    offset = disassembleInstruction(chunk, offset);
  }
}

static int byteInstruction(const char* name, Chunk* chunk, int offset) {
  uint8_t n = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, n);
  return offset + 2;
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

static int simpleInstruction(const char* name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

int disassembleInstruction(Chunk* chunk, int offset) {
  printf("%04d ", offset);
  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }
  
  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_ADD:
    return simpleInstruction("OP_ADD", offset);
  case OP_ADD2CLLCTN:
    return byteInstruction("OP_ADD2CLLCTN", chunk, offset);
  case OP_CONSTANT:
    return constantInstruction("OP_CONSTANT", chunk, offset);
  case OP_DIE:
    return simpleInstruction("OP_DIE", offset);
  case OP_DIVIDE:
    return simpleInstruction("OP_DIVIDE", offset);
  case OP_FIRST:
    return simpleInstruction("OP_FIRST", offset);
  case OP_HCONC:
    return simpleInstruction("OP_HCONC", offset);
  case OP_MDIE:
    return simpleInstruction("OP_MDIE", offset);
  case OP_MZDIE:
    return simpleInstruction("OP_MZDIE", offset);
  case OP_MKCOLLECTION:
    return simpleInstruction("OP_MKCOLLECTION", offset);
  case OP_MKPAIR:
    return simpleInstruction("OP_MKPAIR", offset);
  case OP_MOD:
    return simpleInstruction("OP_MOD", offset);
  case OP_MULTIPLY:
    return simpleInstruction("OP_MULTIPLY", offset);
  case OP_NEGATE:
    return simpleInstruction("OP_NEGATE", offset);
  case OP_QUESTION:
    return simpleInstruction("OP_QUESTION", offset);
  case OP_RANGE:
    return simpleInstruction("OP_RANGE", offset);
  case OP_RETURN:
    return simpleInstruction("OP_RETURN", offset);
  case OP_SECOND:
    return simpleInstruction("OP_SECOND", offset);
  case OP_SUBTRACT:
    return simpleInstruction("OP_SUBTRACT", offset);
  case OP_UNION:
    return simpleInstruction("OP_UNION", offset);
  case OP_VCONCC:
    return simpleInstruction("OP_VCONCC", offset);
  case OP_VCONCL:
    return simpleInstruction("OP_VCONCL", offset);
  case OP_VCONCR:
    return simpleInstruction("OP_VCONCR", offset);
  case OP_ZERO_DIE:
    return simpleInstruction("OP_ZERO_DIE", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}

