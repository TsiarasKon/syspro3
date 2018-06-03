#ifndef LISTS_H
#define LISTS_H

typedef struct stringlistnode StringListNode;
struct stringlistnode {
    char *string;
    StringListNode *next;
};
StringListNode* createStringListNode(char *string);
void destroyStringListNode(StringListNode **listnode);

typedef struct stringlist StringList;
struct stringlist {
    StringListNode *first;
    StringListNode *last;
};
StringList* createStringList();
int isStringListEmpty(StringList *list);
int existsInStringList(StringList *list, char *string);
int appendStringListNode(StringList *list, char *string);
int appendStringList(StringList *list1, StringList **list2);
char* popStringListNode(StringList *list);
void destroyStringList(StringList **list);


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
void destroyIntList(IntList **list);

#endif
