#ifndef UTIL_H
#define UTIL_H

enum ErrorCodes {
    EC_OK,       // Success
    EC_ARG,      // Invalid command line arguments
    EC_DIR,      // Failed to open/create directory
    EC_FILE,     // Failed to open/create text file
    EC_FORK,     // Error while forking
    EC_PIPE,     // Error related to pipes
    EC_MEM,      // Failed to allocate memory
    EC_UNKNOWN   // An unexpected error
};

const char *cmds[7];

int getArrayMax(const int *arr, int dim);
int getNextZero(const int *arr, int dim);
char *getCurrentTime(void);

char *ignoreHTMLTags(const char *buffer);

#endif
