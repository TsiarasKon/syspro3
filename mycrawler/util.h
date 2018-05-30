#ifndef UTIL_H
#define UTIL_H

enum ErrorCodes {
    EC_OK,       // Success
    EC_ARG,      // Invalid command line arguments
    EC_MEM,      // Failed to allocate memory
    EC_DIR,      // Failed to open/create directory
    EC_FILE,     // Failed to open/create text file
    EC_SOCK,     // Error related to sockets
    EC_THREAD,   // Error related to threads
    EC_HTTP,     // Violation of the HTTP protocol
    EC_INVALID   // Attempted an invalid operation
};

char* getTimeRunning(struct timeval start_time);
char* getHTTPDate();

int mkdir_path(char *rootdir, char *link);

#endif
