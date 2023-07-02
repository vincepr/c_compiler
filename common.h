// imports NULL, size_t and C99 Boolean bool and expliciz-sized integer types like uint8_t uint32_t ...
#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#define DEBUG_PRINT_CODE        // FLAG to enable Dumping generated Chunks (of bytecode)
#define DEBUG_TRACE_EXECUTION   // FLAG to enable Diagnostics/Debug print outs

#define UINT8_COUNT (UINT8_MAX + 1) // Hard limit on how many locals can exist at the same time (IN THE SAME SCOPE)

#endif