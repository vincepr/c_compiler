// imports NULL, size_t and C99 Boolean bool and expliciz-sized integer types like uint8_t uint32_t ...
#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// beeing able to define those 'out' we can squeze out extra performance for production:
#define DEBUG_PRINT_CODE        // FLAG to enable Dumping generated Chunks (of bytecode)
#define DEBUG_TRACE_EXECUTION   // FLAG to enable Diagnostics/Debug print outs

// global defines:
#define UINT8_COUNT (UINT8_MAX + 1) // Hard limit on how many locals can exist at the same time (IN THE SAME SCOPE)


// disabling unwanted flags while working on it locally:
//#undef DEBUG_PRINT_CODE         // comment this out: to enable debug printing
//#undef DEBUG_TRACE_EXECUTION    // comment this out: to enable trace-execution


// These flag-variables can be passed set in main to enable toggling different behavior:
#ifdef DEBUG_PRINT_CODE
extern bool FLAG_PRINT_CODE;
#endif

#ifdef DEBUG_TRACE_EXECUTION
extern bool FLAG_TRACE_EXECUTION;
#endif






#endif