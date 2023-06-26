#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"

// We define we double Capacity whenever we reach its limit
// - we start at 8 (after upgrade from when it gets initialized with capacity = 0)
// - afterwards we double each time 8 -> 16 -> 32 -> 64 -> 128....
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

// This macro pretties up a function call to reallocate()
// It defines the factor by which we reallocate memory. In this case by a factor 2.
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

// This macro calls reallocate with newSize=0 -> so it will get deleted from memory there
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0);

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

#endif