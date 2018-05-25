#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "lists.h"

StringList* createStringList() {
    StringList *list = malloc(sizeof(StringList));
    if (list == NULL) {
        perror("malloc");
        return NULL;
    }
    list->first = list->last = NULL;
    return list;
}

int isStringListEmpty(StringList *list) {
    return (list == NULL || list->first == NULL);
}

StringListNode* createStringListNode(char *string) {
    StringListNode *listNode = malloc(sizeof(StringListNode));
    if (listNode == NULL) {
        perror("malloc");
        return NULL;
    }
    listNode->string = malloc(strlen(string) + 1);
    if (listNode->string == NULL) {
        perror("malloc");
        return NULL;
    }
    strcpy(listNode->string, string);
    listNode->next = NULL;
    return listNode;
}

void destroyStringListNode(StringListNode **listnode) {
    free((*listnode)->string);
    free((*listnode));
    *listnode = NULL;
}

int appendStringListNode(StringList *list, char *string) {
    if (list->first == NULL) {
        list->first = createStringListNode(string);
        if (list->first == NULL) {
            return EC_MEM;
        }
        list->last = list->first;
        return EC_OK;
    }
    list->last->next = createStringListNode(string);
    if (list->last->next == NULL) {
        return EC_MEM;
    }
    list->last = list->last->next;
    return EC_OK;
}

char *popStringListNode(StringList *list) {
    if (list == NULL || list->first == NULL) {
        return NULL;
    }
    StringListNode* head = list->first;
    list->first = list->first->next;
    char *string = malloc(strlen(head->string) + 1);
    strcpy(string, head->string);
    destroyStringListNode(&head);
    return string;
}

void deleteStringList(StringList **list) {
    if (*list == NULL) {
        fprintf(stderr, "Attempted to delete a NULL StringList.\n");
        return;
    }
    StringListNode *current = (*list)->first;
    StringListNode *next;
    while (current != NULL) {
        next = current->next;
        free(current->string);
        free(current);
        current = next;
    }
    free(*list);
    *list = NULL;
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
    if (list == NULL || list->first == NULL) {
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
