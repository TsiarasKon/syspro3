#ifndef CRAWLER_GLOBALS_H
#define CRAWLER_GLOBALS_H

#define CMD_LISTEN_QUEUE_SIZE 5
#define WORKERS_NUM 3

// Dirfile:
extern char dirfile[];
extern pthread_mutex_t dirfile_mutex;

#endif
