#ifndef UTIL_H
#define UTIL_H

enum ErrorCodes {
    EC_OK,       // Success
    EC_ARG,      // Invalid command line arguments
    EC_MEM,      // Failed to allocate memory
    EC_SOCK,     // Error related to sockets
    EC_THREAD,   // Error related to threads
    EC_INVALID   // Attempted an invalid operation
};

char* getTimeRunning(struct timeval start_time);
char* getHTTPDate();

#endif
