#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <pthread.h>
#include "util.h"

time_t start_time;
/// these need mutexes
int pages_served = 0;
int bytes_served = 0;

IntList *fdList;
pthread_mutex_t fdList_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fdList_cond;

int command_handler(int cmdsock);
void *connection_handler(void *args);

int acquireFd() {
    pthread_mutex_lock(&fdList_mutex);
    while (isIntListEmpty(fdList)) {
        pthread_cond_wait(&fdList_cond, &fdList_mutex);
    }
    int fd = popIntListNode(fdList);
    pthread_mutex_unlock(&fdList_mutex);
    if (! isIntListEmpty(fdList)) {
        pthread_cond_broadcast(&fdList_cond);
    }
    return fd;
}

void appendToFdList(int fd) {
    pthread_mutex_lock(&fdList_mutex);
    appendIntListNode(fdList, fd);
    pthread_mutex_unlock(&fdList_mutex);
    pthread_cond_broadcast(&fdList_cond);
}

int main(int argc, char *argv[]) {
    start_time = time(NULL);
    if (argc != 9) {
        fprintf(stderr, "Invalid arguments. Please run \"$ ./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\"\n");
        return EC_ARG;
    }
    int serving_port = -1;
    int command_port = -1;
    int thread_num = -1;
    char *root_dir = NULL;
    int option;
    while ((option = getopt(argc, argv,"p:c:t:d")) != -1) {
        switch (option) {
            case 'p':
                serving_port = atoi(optarg);
                if (serving_port < 1024) {
                    fprintf(stderr, "Please select a non-privileged (>1023) serving_port.");
                    return EC_ARG;
                }
                break;
            case 'c':
                command_port = atoi(optarg);
                if (command_port < 1024) {
                    fprintf(stderr, "Please select a non-privileged (>1023) command_port.");
                    return EC_ARG;
                }
                break;
            case 't':
                thread_num = atoi(optarg);
                break;
            case 'd':
                root_dir = argv[optind];
                break;
            default:
                fprintf(stderr, "Invalid arguments. Please run \"$ ./jobExecutor -d docfile -w numWorkers\"\n");
                return EC_ARG;
        }
    }
    if (serving_port <= 0 || command_port <= 0 || thread_num <= 0 || root_dir == NULL) {
        fprintf(stderr, "Invalid arguments. Please run \"$ ./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\"\n");
        return EC_ARG;
    }
    if (serving_port == command_port) {
        fprintf(stderr, "serving_port and command_port must be different.\n");
        return EC_ARG;
    }

    /// TODO: Signal handling


    fdList = createIntList();
    pthread_cond_init(&fdList_cond, NULL);
    pthread_t pt_ids[thread_num];
    for (int i = 0; i < thread_num; i++) {
        if (pthread_create(&pt_ids[i], NULL, connection_handler, NULL)) {
            perror("pthread_create");
            return EC_THREAD;
        }
    }
    printf("Created thread pool.\n");

    for (int i = 0; i < 100; i++) {
        appendToFdList(i + 42);
    }

    struct sockaddr_in server;
    struct sockaddr_in cmd_client;
    struct sockaddr *serverptr = (struct sockaddr *) &server;
    struct sockaddr *cmd_clientptr = (struct sockaddr *) &cmd_client;
    socklen_t cmd_client_len;

    int sock, cmdsock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {     // create socket
        perror("socket");
        return EC_SOCK;
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons((uint16_t) command_port);
    if (bind(sock, serverptr, sizeof(server)) < 0) {     // bind socket to address
        perror("bind");
        return EC_SOCK;
    }
    /// listen size?
    if (listen(sock, 5) < 0) {      // listen for connections
        perror("listen");
        return EC_SOCK;
    }
    printf("Listening for commands to port %d\n", command_port);
    while (1) {
        /// poll here
        if ((cmdsock = accept(sock, cmd_clientptr, &cmd_client_len)) < 0) {      // accept connection
            perror("accept");
            return EC_SOCK;
        } else {
            printf("Accepted connection\n");
            if (command_handler(cmdsock) != EC_OK) {     // either error or "SHUTDOWN" was requested
                for (int i = 0; i < thread_num; i++) {
                    pthread_join(pt_ids[i], NULL);
                }
                printf("Joined with everyone!\n");
                pthread_cond_destroy(&fdList_cond);
                break;      ///
            }
        }
    }

    return EC_OK;
}

int command_handler(int cmdsock) {
    char command[BUFSIZ];        // Undoubtedly fits a single command
    if (read(cmdsock, command, BUFSIZ) < 0) {
        perror("Error reading from pipe");
        return EC_SOCK;
    }
    strtok(command, "\r\n");
    if (!strcmp(command, "STATS")) {
        printf("Server up for %s, served %d pages, %d bytes.\n", getTimeRunning(start_time), pages_served, bytes_served);
    } else if (!strcmp(command, "SHUTDOWN")) {
        return -1;
    } else {
        printf("Unknown server command.\n");
    }
    return EC_OK;
}

void *connection_handler(void *args) {
    pthread_t self_id = pthread_self();
    printf("Thread %lu created\n", self_id);
    while (1) {
        int sock = acquireFd();
        printf("Am thread %lu, got |%d|\n", self_id, sock);

        /// deleteStringListNode()
    }
    printf("Why are we here?\n");
}

