#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75
static Entry* findEntry(Entry* entries, int capacity, ObjString* key);
  
static void adjustCapacity(Table* table, int capacity) {
  Entry* entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = INTEGER_VAL(0);
  }

  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) { continue; }
    
    Entry* dest = findEntry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }
   
  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

void freeTable(Table* table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  initTable(table);
}

// Note: the example in Crafting Interpreters uses string interning
// since character-by-character comparison is slow, but Troll rolls
// are typically short and there's not likely to be a lot of variable
// look-up. If my assumption is wrong, we can come back and implement
// string interning.
static bool stringEqual(ObjString* a, ObjString* b) {
  if (a->hash != b->hash) { return false; }

  return (a->length == b->length) && (memcmp(a->chars, b->chars, a->length) == 0);
}

static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {
  uint32_t index = key->hash % capacity;
  Entry* tombstone = NULL;
  
  for (;;) {
    Entry* entry = &entries[index];
    if (entry->key == NULL) {
      if (IS_INTEGER(entry->value) && (AS_INTEGER(entry->value) == 0)) {
        // empty entry
        return (tombstone != NULL) ? tombstone : entry;
      } else {
        // found a tombstone
        if (tombstone == NULL) { tombstone = entry; }
      }
    } else if (stringEqual(entry->key, key)) {
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

void initTable(Table* table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void tableAddAll(Table* from, Table* to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry* entry = &from->entries[i];
    if (entry->key != NULL) {
      tableSet(to, entry->key, entry->value);
    }
  }
}

bool tableDelete(Table* table, ObjString* key) {
  if (table->count == 0) return false;

  // Find the entry.
  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  // Our tombstones are key: NULL, value: INTEGER_VAL(1)
  entry->key = NULL;
  entry->value = INTEGER_VAL(1);
  return true;
}

bool tableGet(Table* table, ObjString* key, Value* value) {
  if (table->count == 0) return false;

  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  *value = entry->value;
  return true;
}

bool tableSet(Table* table, ObjString* key, Value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, capacity);
  }
  
  Entry* entry = findEntry(table->entries, table->capacity, key);
  bool isNewKey = entry->key == NULL;
  // increment count if this is an empty entry, not a tombstone
  if (isNewKey && IS_INTEGER(entry->value) && (AS_INTEGER(entry->value) == 0)) { table->count++; }

  entry->key = key;
  entry->value = value;
  return isNewKey;
}
