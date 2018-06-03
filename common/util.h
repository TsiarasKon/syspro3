#ifndef UTIL_H
#define UTIL_H

#include <sys/time.h>

enum ErrorCodes {
    EC_OK,       // Success
    EC_ARG,      // Invalid command line arguments
    EC_MEM,      // Failed to allocate memory
    EC_DIR,      // Failed to open/create directory
    EC_FILE,     // Failed to open/create text file
    EC_PIPE,     // Error related to pipes
    EC_SOCK,     // Error related to sockets
    EC_THREAD,   // Error related to threads
    EC_CHILD,    // fork() or exec() failed
    EC_INVALID   // Attempted an invalid operation
};

char* getTimeRunning(struct timeval start_time);
char* getHTTPDate();

#endif
