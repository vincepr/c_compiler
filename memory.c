#include <stdlib.h>
#include "memory.h"

//  The single function used for all dynamic memory management in clox 
//  (this is neccessary for the Garbage Collector)
    // if   -oldSize-   -newSize-       -then do Operation:-
    //      0           Non-zero        Allocate new block-
    //      Non-Zero    0               Free allocation.
    //      Non-Zero    new<oldSize     Shrink existing allocation.
    //      Non-Zero    new>oldSize     Grow existing allocation.

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);
    if (result == NULL) exit(1);                // We must handle the case of realloc failing (ex. not enough free memory left on OS)
    return result;
}

// infos for --> realloc(void *ptr, size_t size) <--
    // ptr: is the pointer to a memory block previously allocated with malloc/calloc/realloc to be reallocated
        // if it is NULL, a new block is allocated and a pointer to it is returned to by the funciton
    // size: is the new size for the memory block in bytes
    // return value: this function returns a pointer to the newly allocated memory, or NULL if the request fails.