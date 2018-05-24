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
#include <errno.h>
#include "util.h"
#include "requests.h"

#define CMD_LISTEN_QUEUE_SIZE 10
#define CLIENT_LISTEN_QUEUE_SIZE 256

volatile int thread_alive = 1;

char *root_dir = NULL;

time_t start_time;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
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
//    char test_req[] = "GET /site0/page0_1244.html HTTP/1.1\n"
//            "User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\n"
//            "Host: www.tutorialspoint.com\n"
//            "Accept-Language: en-us\n"
//            "Accept-Encoding: gzip, deflate\n"
//            "Connection: Keep-Alive\n\n";

    start_time = time(NULL);
    if (argc != 9) {
        fprintf(stderr, "Invalid arguments. Please run \"$ ./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\"\n");
        return EC_ARG;
    }
    int serving_port = -1;
    int command_port = -1;
    int thread_num = -1;
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

    struct sockaddr_in server;
    struct sockaddr_in command;
    struct sockaddr_in client;
    struct sockaddr *serverptr = (struct sockaddr *) &server;
    struct sockaddr *commandptr = (struct sockaddr *) &command;
    struct sockaddr *clientptr = (struct sockaddr *) &client;
    socklen_t command_len;
    socklen_t client_len;

    // Create sockets:
    int commandsock, clientsock;
    if ((commandsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return EC_SOCK;
    }
    if ((clientsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return EC_SOCK;
    }
    // Bind sockets:
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons((uint16_t) command_port);
    if (bind(commandsock, serverptr, sizeof(server)) < 0) {
        perror("bind");
        return EC_SOCK;
    }
    server.sin_port = htons((uint16_t) serving_port);
    if (bind(clientsock, serverptr, sizeof(server)) < 0) {
        perror("bind");
        return EC_SOCK;
    }
    // Listen for connections:
    if (listen(commandsock, CMD_LISTEN_QUEUE_SIZE) < 0) {
        perror("listen");
        return EC_SOCK;
    }
    if (listen(clientsock, CLIENT_LISTEN_QUEUE_SIZE) < 0) {
        perror("listen");
        return EC_SOCK;
    }
    struct pollfd pfds[2];
    pfds[0].fd = commandsock;
    pfds[0].events = POLLIN;
    pfds[1].fd = clientsock;
    pfds[1].events = POLLIN;
    int newfd;
    while (1) {
        if (poll(pfds, 2, -1) < 0) {
            perror("poll");
            return EC_SOCK;
        }
        if (pfds[0].revents & POLLIN) {      // commandsock is ready to accept
            if ((newfd = accept(commandsock, commandptr, &command_len)) < 0) {
                perror("accept");
                return EC_SOCK;
            }
            printf("Accepted command connection\n");
            if (command_handler(newfd) != EC_OK) {     // either error or "SHUTDOWN" was requested
                thread_alive = 0;
                for (int i = 0; i < thread_num; i++) {
                    /// pthread_join(pt_ids[i], NULL);
                    pthread_cancel(pt_ids[i]);
                }
                printf("All done!\n");
                pthread_cond_destroy(&fdList_cond);
                break;      ///
            }
        } else {      // clientsock is ready to accept
            if ((newfd = accept(clientsock, clientptr, &client_len)) < 0) {
                perror("accept");
                return EC_SOCK;
            }
            appendToFdList(newfd);     // will broadcast to threads to handle the new client
        }
    }

    return EC_OK;
}

int command_handler(int cmdsock) {
    char command[BUFSIZ];        // Undoubtedly fits a single command
    if (read(cmdsock, command, BUFSIZ) < 0) {
        perror("Error reading from socket");
        return EC_SOCK;
    }
    strtok(command, "\r\n");
    if (!strcmp(command, "STATS")) {
        pthread_mutex_lock(&stats_mutex);
        printf("Server up for %s, served %d pages, %d bytes.\n", getTimeRunning(start_time), pages_served, bytes_served);
        pthread_mutex_unlock(&stats_mutex);
    } else if (!strcmp(command, "SHUTDOWN")) {
        close(cmdsock);
        return -1;
    } else {
        printf("Unknown server command '%s'.\n", command);
    }
    close(cmdsock);
    return EC_OK;
}

void updateStats(int new_bytes_served) {
    pthread_mutex_lock(&stats_mutex);
    pages_served++;
    bytes_served += new_bytes_served;
    pthread_mutex_unlock(&stats_mutex);
}

void *connection_handler(void *args) {
    pthread_t self_id = pthread_self();
    printf("Thread %lu created.\n", self_id);
    while (thread_alive) {
        int sock = acquireFd();
        printf("Am thread %lu, got |%d| socket\n", self_id, sock);
        int curr_bytes_served = 0;
        char curr_buf[BUFSIZ] = "";
        char *msg_buf = malloc(BUFSIZ);
        msg_buf[0] = '\0';
        ssize_t bytes_read;
        do {
            memset(&curr_buf[0], 0, sizeof(curr_buf));
            bytes_read = read(sock, curr_buf, BUFSIZ);
            curr_buf[BUFSIZ - 1] = '\0';
            if (bytes_read < 0) {
                perror("Error reading from socket");
                return NULL;
            } else if (bytes_read > 0) {
                msg_buf = realloc(msg_buf, sizeof(msg_buf) + BUFSIZ);
                strcat(msg_buf, curr_buf);
            }
        } while (bytes_read > 0);

        char *responseString;
        char *requested_file, *hostname;
        int rv = validateGETRequest(msg_buf, &requested_file, &hostname);
        if (rv > 0) {    // Fatal Error
            return NULL;
        } else if (rv < 0) {
            responseString = createResponseString(HTTP_BADREQUEST, NULL);
        } else {    // valid request
            char *requested_file_path;
            asprintf(&requested_file_path, "%s%s", root_dir, requested_file);
            printf("%s\n", requested_file_path);
            FILE *fp = fopen(requested_file_path, "r");
            if (fp == NULL) {       // failed to open file
                if (errno == EACCES) {      // failed with "Permission denied"
                    responseString = createResponseString(HTTP_FORBIDDEN, NULL);
                } else {      // file doesn't exist
                    responseString = createResponseString(HTTP_NOTFOUND, NULL);
                }
            } else {
                responseString = createResponseString(HTTP_OK, fp);
                fclose(fp);
                curr_bytes_served = sizeof(responseString);     /// or strlen() ?
            }
            free(requested_file_path);
        }
        printf("%s\n", responseString);
        if (write(sock, responseString, sizeof(responseString)) < 0) {
            perror("Error writing to socket");
            return NULL;
        }
        free(responseString);
        free(msg_buf);
        if (curr_bytes_served > 0) {
            updateStats(curr_bytes_served);
        }
        close(sock);
    }
    printf("Thread %lu has exited.\n", self_id);
}

