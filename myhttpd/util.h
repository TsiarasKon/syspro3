#ifndef UTIL_H
#define UTIL_H

enum ErrorCodes {
    EC_OK,       // Success
    EC_ARG,      // Invalid command line arguments
    EC_DIR,      // Failed to open/create directory
    EC_FILE,     // Failed to open/create text file
    EC_SOCK,     // Error related to sockets
    EC_THREAD,   // Error related to threads
    EC_FORK,     // Error while forking
    EC_PIPE,     // Error related to pipes
    EC_MEM,      // Failed to allocate memory
    EC_UNKNOWN   // An unexpected error
};

const char *cmds[7];

char* getTimeRunning(time_t start_time);    /// requires freeing of return value

#endif
