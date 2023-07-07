#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

/*
    Hash Map implementation
    - Thought: We allow variables up to eight characters long (or just hash on those first 8chars)
    - We get 26 unique chars ('a'-'Z' + '0'-'9' +'_'). We can squash all that info in a 64bit int
    - (if we map that to a continous block we would need 295148PetaByte) so we take the value modulo of the size of the array
        this folds the larger numeric range onto itself untill it fits in a smaller range of array elements
    - because of the above we have to handle hash-collisions though.
        - to mimiize the chance of collisions, we can dynamically calculate the load-factor (entry_number/bucket_number).
            if the load factor gets to big we resize and grow the array bigger
    - to avoid the 8char limit we use a deterministic/uniform/fast hash function.
        - clox implements the FNV-1a Hashing-Function   http://www.isthe.com/chongo/tech/comp/fnv/
*/

/*
    Techniques for resolving collisions: 2 basic Techniques: 
    
    SEPARATE CHAINING (not used for this):
    - each bucket containts more than one entries. For example a linked list of entries.
        To lookup an entry you just walk the linked list till you found the fitting entry
    - worst case: would be all data collides on the same bucket -> is basically one linked list
    - conceptually simple with operation implementations simple. BUT not a great fit for modern CPUs.
        Because of a lot of overhead from pointers and linked lists are scattered arround memory.
    
    OPEN ADRESSING (aka CLOSED HASHING)
    - all entries live directly in the bucket array.
    - if two entries collide in the same bucket, we find a different empty bucket to use instead.
    - the process of finding a free/available bucket is called PROBING. 
    - The Order buckets get examined is a PROBE SEQUENCE. We use a simple variant: 
        - LINEAR PROBING: IF the bucket is full we just go to the next entry and try that, then the next ...
    - Good: since it tends to cluster data its cache friendly.
*/

/*
    For notes on Tombstone strategy used, check comments for tableDelete()
*/

// The Objects our Maps Holds (ex: key:varname1, value:10.5 )
typedef struct {
    ObjString* key;
    Value value;
} Entry;

// The struct of our HashMap
typedef struct {
    int count;          // currently stores key-value pairs
    int capacity;       // capacity (so can easly get the load-factor: count/capacity)
    Entry* entries;     // array of the entries we hash
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableDelete(Table* table, ObjString* key);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);
void markTable(Table* table);

#endif