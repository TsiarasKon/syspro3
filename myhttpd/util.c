#define _GNU_SOURCE         // needed for asprintf()
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

char* getTimeRunning(time_t start_time) {       /// requires freeing of return value
    time_t curr_time = time(NULL) - start_time;
    struct tm *timeinfo;
    timeinfo = gmtime(&curr_time);
    char *time_str;
    asprintf(&time_str, "%d:%d.%d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    return time_str;
}


IntList* createIntList() {
    IntList *list = malloc(sizeof(IntList));
    if (list == NULL) {
        perror("malloc");
        return NULL;
    }
    list->first = list->last = NULL;
    return list;
}

int isIntListEmpty(IntList *list) {
    return (list == NULL || list->first == NULL);
}

IntListNode* createIntListNode(int x) {
    IntListNode *listNode = malloc(sizeof(IntListNode));
    if (listNode == NULL) {
        perror("malloc");
        return NULL;
    }
    listNode->fd = x;
    listNode->next = NULL;
    return listNode;
}

int appendIntListNode(IntList *list, int x) {
    if (list->first == NULL) {
        list->first = createIntListNode(x);
        if (list->first == NULL) {
            return EC_MEM;
        }
        list->last = list->first;
        return EC_OK;
    }
    list->last->next = createIntListNode(x);
    if (list->last->next == NULL) {
        return EC_MEM;
    }
    list->last = list->last->next;
    return EC_OK;
}

int popIntListNode(IntList *list) {
    if (list->first == NULL) {
        return -1;
    }
    IntListNode* head = list->first;
    list->first = list->first->next;
    int x = head->fd;
    /// destroy head
    return x;
}

void deleteIntList(IntList **list) {
    if (*list == NULL) {
        fprintf(stderr, "Attempted to delete a NULL StringList.\n");
        return;
    }
    IntListNode *current = (*list)->first;
    IntListNode *next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    free(*list);
    *list = NULL;
}
