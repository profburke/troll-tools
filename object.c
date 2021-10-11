#include <stdio.h>
#include <stdlib.h>
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

ObjCollection* copyCollection(const ObjCollection* c) {
  ObjCollection* r = makeCollection();
  r->capacity = c->capacity;
  r->count = c->count;
  size_t size = c->capacity * sizeof(int);
  
  int* ints = (int*)reallocate(NULL, 0, size);
  memcpy(ints, c->ints, size);
  r->ints = ints;
  
  return r;
}

ObjString* copyString(const char* chars, int length) {
  char* heapChars = (char*)ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return allocateString(heapChars, length);
}

int findFirstIndex(const ObjCollection* c, int element) {
  for (int i = 0; i < c->count; i++) {
    if (c->ints[i] == element) {
      return i;
    }
  }
  
  return -1;
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

int member(ObjCollection* c, int item) {
  for (int i = 0; i < c->count; i++) {
    if (item == c->ints[i]) { return 1; }
  }
  
  return 0;
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_COLLECTION: {
    ObjCollection* c = AS_COLLECTION(value);
    sortCollection(c);
    for (int i = 0; i < c->count; i++) {
      printf("%d", c->ints[i]);
      if (i != c->count - 1) {
        printf(", ");
      }
    }
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

// TODO: when count/capacity reaches ??, shrink the array
void removeAtIndex(ObjCollection* c, int index) {
  for (int i = index; i < c->count; i++) {
    c->ints[i] = c->ints[i+1];
  }
  c->count--;
}

////////////////////////////////////////////////
////////////////////////////////////////////////

// Note: qsort is recursive; if this turns out to be a problem
// consider switching to heapsort (which does require nel*sizeof(el) additional
// space).

static int rcomp(const void* e1, const void* e2) {
  int f = *((int*)e1);
  int s = *((int*)e2);

  if (f > s) { return -1; }
  if (f < s) { return 1; }
  return 0;
}

static int comp(const void* e1, const void* e2) {
  int f = *((int*)e1);
  int s = *((int*)e2);

  if (f > s) { return 1; }
  if (f < s) { return -1; }
  return 0;
}

void reverseSortCollection(ObjCollection* c) {
    qsort(c->ints, c->count, sizeof(int), rcomp);
}

void sortCollection(ObjCollection* c) {
    qsort(c->ints, c->count, sizeof(int), comp);
}

////////////////////////////////////////////////
////////////////////////////////////////////////

ObjString* takeString(char* chars, int length) {
  return allocateString(chars, length);
}

    
