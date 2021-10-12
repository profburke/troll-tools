#ifndef _tvm_object_h
#define _tvm_object_h

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_COLLECTION(value) isObjType(value, OBJ_COLLECTION)
#define IS_PAIR(value) isObjType(value, OBJ_PAIR)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_COLLECTION(value) ((ObjCollection*)AS_OBJ(value))
#define AS_PAIR(value) ((ObjPair*)AS_OBJ(value))
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
  OBJ_COLLECTION,
  OBJ_PAIR,
  OBJ_STRING
} ObjType;

struct Obj {
  ObjType type;
};

struct ObjCollection {
  Obj obj;
  int count;
  int capacity;
  int* ints;
};

struct ObjPair {
  Obj obj;
  Value a;
  Value b;
};

struct ObjString {
  Obj obj;
  int length;
  char* chars;
  uint32_t hash;
};

void addToCollection(ObjCollection* c, int n);
ObjCollection* copyCollection(const ObjCollection* c);
ObjString* copyString(const char* chars, int length);
int findFirstIndex(const ObjCollection* c, int element);
ObjCollection* makeCollection(void); // TODO: rename makeX to initX
ObjPair* makePair(Value a, Value b);
int member(ObjCollection* c, int item);
void printObject(Value value);
void removeAtIndex(ObjCollection* c, int index);
void reverseSortCollection(ObjCollection* c);
void sortCollection(ObjCollection* c);
ObjString* takeString(char* chars, int length);

static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
