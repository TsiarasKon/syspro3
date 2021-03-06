#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "postinglist.h"
#include "util.h"

PostingListNode *createPostingListNode(int id, int line) {
    PostingListNode *listNode = malloc(sizeof(PostingListNode));
    if (listNode == NULL) {
        perror("malloc");
        return NULL;
    }
    listNode->id = id;
    if ((listNode->lines = createIntList()) == NULL) {
        return NULL;
    }
    if ((appendIntListNode(listNode->lines, line)) != EC_OK) {
        return NULL;
    }
    listNode->tf = 1;
    listNode->next = NULL;
    return listNode;
}

void deletePostingListNode(PostingListNode **listNode) {
    if (*listNode == NULL) {
        fprintf(stderr, "Attempted to delete a NULL PostingListNode.\n");
        return;
    }
    PostingListNode *current = *listNode;
    PostingListNode *next;
    while (current != NULL) {
        next = current->next;
        deleteIntList(&current->lines);
        free(current);
        current = next;
    }
    *listNode = NULL;
}

PostingList* createPostingList() {
    PostingList *postingList = malloc(sizeof(PostingList));
    if (postingList == NULL) {
        perror("malloc");
        return NULL;
    }
    postingList->first = postingList->last = NULL;
    return postingList;
}

void deletePostingList(PostingList **postingList) {
    if (*postingList == NULL) {
        fprintf(stderr, "Attempted to delete a NULL PostingList.\n");
        return;
    }
    if ((*postingList)->first != NULL) {
        deletePostingListNode(&(*postingList)->first);     // delete the entire list
    }
    free(*postingList);
    *postingList = NULL;
}

int incrementPostingList(TrieNode *node, int id, int line) {
    PostingList **PostingList = &node->postingList;
    // If list is empty, create a listNode and set both first and last to point to it
    if ((*PostingList)->first == NULL) {
        (*PostingList)->first = createPostingListNode(id, line);
        if ((*PostingList)->first == NULL) {
            perror("Error allocating memory");
            return EC_MEM;
        }
        (*PostingList)->last = (*PostingList)->first;
        return EC_OK;
    }
    /* Words are inserted in order of id, so the posting list we're looking for either
     * is the last one or it doesn't exist and should be created after the last */
    if ((*PostingList)->last->id == id) {      // word belongs to last doc
        if ((appendIntListNode((*PostingList)->last->lines, line)) != EC_OK) {
            return EC_MEM;
        }
        (*PostingList)->last->tf++;
    } else {
        (*PostingList)->last->next = createPostingListNode(id, line);
        if ((*PostingList)->last->next == NULL) {
            return EC_MEM;
        }
        (*PostingList)->last = (*PostingList)->last->next;
    }
    return EC_OK;
}
