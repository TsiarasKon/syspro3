#define _GNU_SOURCE         // needed for asprintf()
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <netdb.h>
#include "util.h"
#include "requests.h"

#define CMD_LISTEN_QUEUE_SIZE 5

char *save_dir = NULL;

struct timeval start_time;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
int pages_downloaded = 0;
long bytes_downloaded = 0;

volatile int thread_alive = 1;
volatile int crawler_alive = 1;

void crawler_killer(int signum) {
    crawler_alive = 0;
}

int command_handler(int cmdsock);

int main(int argc, char *argv[]) {
    gettimeofday(&start_time, NULL);
    if (argc != 12) {
        fprintf(stderr,
                "Invalid arguments. Please run \"$ ./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL\"\n");
        return EC_ARG;
    }
    char *hostaddr = NULL;
    int server_port = -1;
    int command_port = -1;
    int thread_num = -1;
    char *starting_URL = NULL;
    starting_URL = argv[11];
    int option;
    while ((option = getopt(argc, argv, "h:p:c:t:d")) != -1) {
        switch (option) {
            case 'h':
                hostaddr = argv[optind - 1];        ///
                break;
            case 'p':
                server_port = atoi(optarg);
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
                save_dir = argv[optind];
                break;
            default:
                fprintf(stderr, "Invalid arguments. Please run \"$ ./jobExecutor -d docfile -w numWorkers\"\n");
                return EC_ARG;
        }
    }
    if (hostaddr == NULL || server_port <= 0 || command_port <= 0 || thread_num <= 0 || save_dir == NULL || starting_URL == NULL) {
        fprintf(stderr,
                "Invalid arguments. Please run \"$ ./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\"\n");
        return EC_ARG;
    }
    if (server_port == command_port) {
        fprintf(stderr, "command_port must be different from the server's port.\n");
        return EC_ARG;
    }

    int rv = EC_OK;

    /// Create string list and threads

    // Signal handling for graceful exit on fatal signals:
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = crawler_killer;
    sigaction(SIGINT,  &act, 0);
    sigaction(SIGTERM, &act, 0);
    sigaction(SIGQUIT, &act, 0);

    struct sockaddr_in server;
    struct sockaddr_in crawler;
    struct sockaddr_in command;
    struct sockaddr *serverptr = (struct sockaddr *) &server;
    struct sockaddr *crawlerptr = (struct sockaddr *) &crawler;
    struct sockaddr *commandptr = (struct sockaddr *) &command;
    struct hostent *hent;
    socklen_t command_len = 0;
    socklen_t client_len = 0;

    // Create sockets:
    int commandsock, serversock;
    if ((commandsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        rv = EC_SOCK;
    }
    if ((serversock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        rv = EC_SOCK;
    }
    // Setting SO_REUSEADDR so that server can be restarted without bind() failing with "Address already in use"
    int literally_just_one = 1;
    if (setsockopt(commandsock, SOL_SOCKET, SO_REUSEADDR, &literally_just_one, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }
    // Bind command socket:
    crawler.sin_family = AF_INET;
    crawler.sin_addr.s_addr = htonl(INADDR_ANY);
    crawler.sin_port = htons((uint16_t) command_port);
    if (bind(commandsock, crawlerptr, sizeof(crawler)) < 0) {
        perror("bind");
        rv = EC_SOCK;
    }
    // Listen for connections to commandsock:
    if (listen(commandsock, CMD_LISTEN_QUEUE_SIZE) < 0) {
        perror("listen");
        rv = EC_SOCK;
    }
    if ((hent = gethostbyname(hostaddr)) == NULL) {
        perror("gethostbyname");
        rv = EC_SOCK;
    }
    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, hent->h_addr, (size_t) hent->h_length);
    server.sin_port = htons((uint16_t) server_port);
    int newfd;
    while (crawler_alive && !rv) {
        if ((newfd = accept(commandsock, commandptr, &command_len)) < 0) {
            perror("accept");
            rv = EC_SOCK;
        }
        if (command_handler(newfd) != EC_OK) {     // either error or "SHUTDOWN" was requested
            crawler_alive = 0;
        }
    }
    thread_alive = 0;
//    for (int i = 0; i < thread_num; i++) {      // wake all the threads
//        pthread_cond_broadcast(&fdList_cond);
//    }
//    for (int i = 0; i < thread_num; i++) {      // join with all the threads
//        pthread_join(pt_ids[i], NULL);
//    }
//    pthread_cond_destroy(&fdList_cond);
//    deleteIntList(&fdList);
    printf("Main thread has exited.\n");
    return rv;
}

int command_handler(int cmdsock) {
    char command[BUFSIZ];        // Undoubtedly fits a single command
    if (read(cmdsock, command, BUFSIZ) < 0) {
        perror("Error reading from socket");
        return EC_SOCK;
    }
    strtok(command, "\r\n");
    if (!strcmp(command, "STATS")) {
        printf("Main thread: Received \"STATS\" command.\n");
        char *timeRunning;
        pthread_mutex_lock(&stats_mutex);
        timeRunning = getTimeRunning(start_time);
        printf(" Crawler up for %s, downloaded %d pages, %ld bytes.\n", timeRunning, pages_downloaded, bytes_downloaded);
        pthread_mutex_unlock(&stats_mutex);
        free(timeRunning);
    } else if (!strcmp(command, "SEARCH")) {
        /// TODO: Implement SEARCH
    } else if (!strcmp(command, "SHUTDOWN")) {
        printf("Main thread: Received \"SHUTDOWN\" command ...\n");
        close(cmdsock);
        return -1;
    } else {
        printf("Main thread: Unknown command \"%s\".\n", command);
    }
    close(cmdsock);
    return EC_OK;
}

