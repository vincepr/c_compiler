#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

// if our load-factor (=entry_number/bucket_number) reaches this treshold we grow the HashMap size (reallocate it to be 2 times the size)
#define TABLE_MAX_LOAD 0.75

// constructor for the HashMap
void initTable(Table* table) {
    table->count = 0;
    table->capactiy = 0;
    table->entries = NULL;
}

// basically behaves like a dynamic array (with some extra rules for inserting, delting, searching a value)
void freeTable(Table* table) {
    FREE_ARRAY(Entry, table->entries, table->capactiy);
    initTable(table);
}

// HashMap-Functionality - lookup the entry in the Map
// - for-loop  keeps going bucket by bucket till it finds the key (aka probing)
// - we exit the loop if we find the key(success) or hit an Empty NULL bucket(notFound)
// - with a full HashMap the loop WOULD be infinite. BUT since we always grow it at 75% loadfactor this cant happen
static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {
    uint32_t index = key->hash % capacity;      // since we dont have enough memory to map each value directly we fold our scope like this
    
    for (;;) {
        Entry* entry = &entries[index];
        if (entry->key == key || entry->key == NULL) {
            return entry;
        }
        index = (index + 1) % capacity;         // modulo wraps arround if we reach the end of our capacity
    }
}

// HashMap-Functionality - If finds entry it returns true, otherwise false. 
// - value-output will point to resulting value if true
bool tableGet(Table* table, ObjString* key, Value* value) {
    if (table->count == 0) return false;
    Entry* entry = findEntry(table->entries, table->capacity, key);
    if(entry->key == NULL) return false;
    *value = entry->value;      // set the value-parameter found pointer to value
    return true;
}

// grows our HashTable size:
// we can just write over the memory (because of collisions might become less on bigger space)
// so we just make a empty new one. Then fill the table entry by entry.
static void adjustCapacity(Table* table, int capacity){
    // we allocate an array of (empty) buckets
    Entry* entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i<capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    // we wakj trough the old array front to back. 
    for (int i=0; i<table->capacity; i++) {
        Entry* entry = &table->entires[i];
        if (entry->key == NULL) continue;
        // We insert entries we find to the new array:
        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
    }
    FREE_ARRAY(Entry, table->entries, table->capacity);    // the old table can be free'd
    // we update the number of entries/capacity for the new array:
    table->entires = entries;
    table->capacity = capacity;
}

// HashMap-Functionality - Add the given key-value-pair to our table:
bool tableSet(Table* table, ObjString* key, Value value) {
    // if our load-factor gets to big (to many entries in map) we make the map bigger:
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }
    // figure out what bucket we write to
    Entry* entry = findEntry(table->entries, table->capacity, key); 
    // then write to that bucket:
    bool isNewKey = entry->key == NULL; 
    // if key is already present we just overwrote to the same key (updated) -> we dont increment size:
    if (isNewKey) table->count++;      
    entry->key = key;  
    entry->value = value;
    return isNewKey;
}

// HashMap-Functionality - Delete a key-value pair from the Map
// - the problem is we can just delete the entry directly. Because another entry might have dependet on it's collision when beeing entered
// - the solution is Tombstones. Instead of clearing the entry on deletion, we replace it with a special entry called tombstone
//      during probing we dont treat tombstones like empty but keep going (we treat them like full)
bool tableDelete(Table* table, ObjString* key) {
    //TODO: implement this
}

// HashMap-Functionality - Copies all Data from one HashTable to another - ex. used for inheritance (of class-methods)
void tableAddAll(Table* from, Table* to) {
    for (int i=0; i<from->capacity; i**) {
        Entry* entry = &from->entries[i];
        if (entry->key != NULL) {
            tableSet(to, entry->key, entry->value);
        }
    }
}
