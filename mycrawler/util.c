#define _GNU_SOURCE         // needed for asprintf()
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include "util.h"

const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

char* getTimeRunning(struct timeval start_time) {
    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    long long msRunning = ((curr_time.tv_sec - start_time.tv_sec) * 1000000LL + curr_time.tv_usec - start_time.tv_usec) / 1000;
    time_t secRunning = msRunning / 1000;
    struct tm *timeRunning;
    timeRunning = gmtime(&secRunning);
    char *time_str;
    if (timeRunning->tm_hour > 0) {
        asprintf(&time_str, "%.2d:%.2d:%.2d.%.2lld", timeRunning->tm_hour, timeRunning->tm_min, timeRunning->tm_sec, (msRunning % 1000) / 10);
    } else if (timeRunning->tm_min > 0) {
        asprintf(&time_str, "%.2d:%.2d.%.2lld", timeRunning->tm_min, timeRunning->tm_sec, (msRunning % 1000) / 10);
    } else {
        asprintf(&time_str, "%.2d.%.2lld", timeRunning->tm_sec, (msRunning % 1000) / 10);
    }
    return time_str;
}

char* getHTTPDate() {
    time_t curr_time = time(NULL);
    struct tm *t;
    t = gmtime(&curr_time);
    char *time_str;
    asprintf(&time_str, "Date: %s, %.2d %s %.4d %.2d:%.2d:%.2d GMT", days[t->tm_wday], t->tm_mday, months[t->tm_mon], \
    t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
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
    free(head);
    return x;
}

void deleteIntList(IntList **list) {
    if (*list == NULL) {
        fprintf(stderr, "Attempted to delete a NULL IntList.\n");
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
