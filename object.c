#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
  (type*)allocateObject(sizeof(type), objectType)

void addToCollection(ObjCollection* c, int n) {
  if (c->capacity < c->count + 1) {
    int oldCapacity = c->capacity;
    c->capacity = GROW_CAPACITY(oldCapacity);
    c->ints = GROW_ARRAY(int, c->ints, oldCapacity, c->capacity);
  }

  c->ints[c->count] = n;
  c->count++;
}

static Obj* allocateObject(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
  return object;
}

static ObjString* allocateString(char* chars, int length) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  return string;
}

ObjString* copyString(const char* chars, int length) {
  char* heapChars = (char*)ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return allocateString(heapChars, length);
}

ObjCollection* makeCollection() {
  ObjCollection* c = ALLOCATE_OBJ(ObjCollection, OBJ_COLLECTION);
  c->capacity = 0;
  c->count = 0;
  c->ints = NULL;
  return c;
}

ObjPair* makePair(Value a, Value b) {
  ObjPair* pair = ALLOCATE_OBJ(ObjPair, OBJ_PAIR);
  pair->a = a;
  pair->b = b;
  return pair;
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_COLLECTION: {
    ObjCollection* c = AS_COLLECTION(value);
    printf("{");
    for (int i = 0; i < c->count; i++) {
      printf("%d", c->ints[i]);
      if (i != c->count - 1) {
        printf(", ");
      }
    }
    printf("}");
  }
    break;
  case OBJ_PAIR: {
    ObjPair* p = AS_PAIR(value);
    printf("[");
    printValue(p->a); printf(", "); printValue(p->b);
    printf("]");
  }
    break;
  case OBJ_STRING: printf("%s", AS_CSTRING(value)); break;
  }
}

ObjString* takeString(char* chars, int length) {
  return allocateString(chars, length);
}

    
