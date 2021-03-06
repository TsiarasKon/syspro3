#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"
#include "util.h"

TrieNode* createTrieNode(char value, TrieNode *next) {
    TrieNode* nptr = malloc(sizeof(TrieNode));
    if (nptr == NULL) {
        perror("malloc");
        return NULL;
    }
    nptr->value = value;
    nptr->next = next;
    nptr->child = NULL;
    nptr->postingList = createPostingList();
    return nptr;
}

void deleteTrieNode(TrieNode **trieNode) {      // called by deleteTrie() to delete the entire trie
    if (trieNode == NULL) {
        fprintf(stderr, "Attempted to delete a NULL TrieNode.\n");
        return;
    }
    deletePostingList(&(*trieNode)->postingList);
    if ((*trieNode)->child != NULL) {
        deleteTrieNode(&(*trieNode)->child);
    }
    if ((*trieNode)->next != NULL) {
        deleteTrieNode(&(*trieNode)->next);
    }
    free(*trieNode);
    *trieNode = NULL;
}

Trie* createTrie() {
    Trie* tptr = malloc(sizeof(Trie));
    if (tptr == NULL) {
        perror("malloc");
        return NULL;
    }
    tptr->first = NULL;
    return tptr;
}

void deleteTrie(Trie **trie) {
    if (*trie == NULL) {
        fprintf(stderr, "Attempted to delete a NULL Trie.\n");
        return;
    }
    if ((*trie)->first != NULL) {
        deleteTrieNode(&(*trie)->first);
    }
    free(*trie);
    *trie = NULL;
}

/* Called by insert() when a new trieNode->child is created.
 * At that point no further checking is required to insert the rest of the word's letters
 * as each one of them will merely create a new trieNode. */
int directInsert(TrieNode *current, char *word, int id, int line, int i) {
    while (i < strlen(word) - 1) {
        current->child = createTrieNode(word[i], NULL);
        if (current->child == NULL) {
            perror("Error allocating memory");
            return EC_MEM;
        }
        current = current->child;
        i++;
    }
    current->child = createTrieNode(word[i], NULL);       // final letter
    if (current->child == NULL) {
        perror("Error allocating memory");
        return EC_MEM;
    }
    return incrementPostingList(current->child, id, line);
}

int insert(Trie *root, char *word, int id, int line) {
    if (word[0] == '\0') {      // word is an empty string
        return EC_OK;
    }
    size_t wordlen = strlen(word);
    if (root->first == NULL) {      // only in first Trie insert
        root->first = createTrieNode(word[0], NULL);
        if (wordlen == 1) {     // just inserted the final letter (one-letter word)
            incrementPostingList(root->first, id, line);
            return EC_OK;
        }
        return directInsert(root->first, word, id, line, 1);
    }
    TrieNode **current = &root->first;
    for (int i = 0; i < wordlen; i++) {         // for each letter of word
        while (word[i] > (*current)->value) {       // keep searching this level of trie
            if ((*current)->next == NULL) {             // no next - create trieNode in this level
                (*current)->next = createTrieNode(word[i], NULL);
                if ((*current)->next == NULL) {
                    perror("Error allocating memory");
                    return EC_MEM;
                }
            }
            current = &(*current)->next;
        }
        if ((*current) != NULL && word[i] < (*current)->value) {      // must be inserted before to keep sorted
            TrieNode *newNode = createTrieNode(word[i], (*current));
            if (newNode == NULL) {
                perror("Error allocating memory");
                return EC_MEM;
            }
            *current = newNode;
        }
        // Go to next letter:
        if (i == wordlen - 1) {     // just inserted the final letter
            return incrementPostingList((*current), id, line);
        } else if ((*current)->child != NULL) {     // proceed to child
            current = &(*current)->child;
        } else {    // child doesn't exist so we just add the entire word in the trie
            return directInsert((*current), word, id, line, i + 1);
        }
    }
    return EC_OK;
}

PostingList *getPostingList(Trie *root, char *word) {
    size_t wordlen = strlen(word);
    if (root->first == NULL) {       // empty Trie
        return NULL;
    }
    TrieNode *current = root->first;
    for (int i = 0; i < wordlen; i++) {
        while (word[i] > current->value) {
            if (current->next == NULL) {
                return NULL;
            }
            current = current->next;
        }
        // Since letters are sorted in each level, if we surpass word[i] then word doesn't exist:
        if (word[i] < current->value) {
            return NULL;
        }
        // Go to next letter:
        if (i == wordlen - 1) {     // word found
            return current->postingList;
        } else if (current->child != NULL) {     // proceed to child
            current = current->child;
        } else {            // child doesn't exist - word doesn't exist
            return NULL;
        }
    }
    return NULL;
}
