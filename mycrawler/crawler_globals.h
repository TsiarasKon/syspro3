#ifndef CRAWLER_GLOBALS_H
#define CRAWLER_GLOBALS_H

#include "lists.h"

#define CMD_LISTEN_QUEUE_SIZE 5
#define WORKERS_NUM 3

// Dirfile:
extern StringList *dirfileList;
extern pthread_mutex_t dirfile_mutex;

#endif
