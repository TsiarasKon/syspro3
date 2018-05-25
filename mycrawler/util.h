#ifndef UTIL_H
#define UTIL_H

enum ErrorCodes {
    EC_OK,       // Success
    EC_ARG,      // Invalid command line arguments
    EC_MEM,      // Failed to allocate memory
    EC_SOCK,     // Error related to sockets
    EC_THREAD    // Error related to threads
};

const char *cmds[7];

char* getTimeRunning(struct timeval start_time);
char* getHTTPDate();


typedef struct intlistnode IntListNode;
struct intlistnode {
    int fd;
    IntListNode *next;
};
IntListNode* createIntListNode(int x);

typedef struct intlist IntList;
struct intlist {
    IntListNode *first;
    IntListNode *last;
};
IntList* createIntList();
int isIntListEmpty(IntList *list);
int appendIntListNode(IntList *list, int x);
int popIntListNode(IntList *list);
void deleteIntList(IntList **list);

#endif
