#define _GNU_SOURCE         // needed for asprintf()
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <limits.h>
#include "../common/util.h"
#include "../common/lists.h"
#include "../common/requests.h"

#define CMD_LISTEN_QUEUE_SIZE 5
#define CLIENT_LISTEN_QUEUE_SIZE 256

char root_dir[PATH_MAX] = "";

// Stats:
struct timeval start_time;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
int pages_served = 0;
long bytes_served = 0;

// File descriptor list:
IntList *fdList;
pthread_mutex_t fdList_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fdList_cond;

// Function prototypes:
int acquireFd();
void appendToFdList(int fd);
void updateStats(long new_bytes_served);
int command_handler(int cmdsock);
void *connection_handler(void *args);

// Volatile is used for good measure, so that the compiler won't mess with them
volatile int thread_alive = 1;
volatile int server_alive = 1;

void server_killer(int signum) {
    server_alive = 0;
}

int main(int argc, char *argv[]) {
    gettimeofday(&start_time, NULL);        // Needed to later get server running time
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
                realpath(argv[optind], root_dir);
                break;
            default:
                fprintf(stderr, "Invalid arguments. Please run \"$ ./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\"\n");
                return EC_ARG;
        }
    }
    if (serving_port <= 0 || command_port <= 0 || thread_num <= 0 || !strcmp(root_dir, "")) {
        fprintf(stderr, "Invalid arguments. Please run \"$ ./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\"\n");
        return EC_ARG;
    }
    if (serving_port == command_port) {
        fprintf(stderr, "serving_port and command_port must be different.\n");
        return EC_ARG;
    }

    // Signal handling for graceful exit on fatal signals:
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = server_killer;
    sigaction(SIGINT,  &act, 0);
    sigaction(SIGTERM, &act, 0);
    sigaction(SIGQUIT, &act, 0);

    int exit_code = EC_OK;
    fdList = createIntList();
    pthread_cond_init(&fdList_cond, NULL);
    pthread_t pt_ids[thread_num];
    for (int i = 0; i < thread_num; i++) {
        if (pthread_create(&pt_ids[i], NULL, connection_handler, NULL)) {
            perror("pthread_create");
            exit_code = EC_THREAD;
        }
    }

    struct sockaddr_in server;
    struct sockaddr_in command;
    struct sockaddr_in client;
    struct sockaddr *serverptr = (struct sockaddr *) &server;
    struct sockaddr *commandptr = (struct sockaddr *) &command;
    struct sockaddr *clientptr = (struct sockaddr *) &client;
    socklen_t command_len = 0;
    socklen_t client_len = 0;

    // Create sockets:
    int commandsock, clientsock;
    if ((commandsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit_code = EC_SOCK;
    }
    if ((clientsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit_code = EC_SOCK;
    }
    // Setting SO_REUSEADDR so that server can be restarted without bind() failing with "Address already in use"
    int literally_just_one = 1;
    if (setsockopt(commandsock, SOL_SOCKET, SO_REUSEADDR, &literally_just_one, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }
    if (setsockopt(clientsock, SOL_SOCKET, SO_REUSEADDR, &literally_just_one, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }
    // Bind sockets:
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons((uint16_t) command_port);
    if (bind(commandsock, serverptr, sizeof(server)) < 0) {
        perror("bind");
        exit_code = EC_SOCK;
    }
    server.sin_port = htons((uint16_t) serving_port);
    if (bind(clientsock, serverptr, sizeof(server)) < 0) {
        perror("bind");
        exit_code = EC_SOCK;
    }
    // Listen for connections:
    if (listen(commandsock, CMD_LISTEN_QUEUE_SIZE) < 0) {
        perror("listen");
        exit_code = EC_SOCK;
    }
    if (listen(clientsock, CLIENT_LISTEN_QUEUE_SIZE) < 0) {
        perror("listen");
        exit_code = EC_SOCK;
    }

    // We'll be polling in the socket's fd so as to avoid blocking trying to accept from one of them
    struct pollfd pfds[2];
    pfds[0].fd = commandsock;
    pfds[0].events = POLLIN;
    pfds[1].fd = clientsock;
    pfds[1].events = POLLIN;
    int newfd;
    while (server_alive && !exit_code) {
        // Looping to find dead threads:
        for (int i = 0; i < thread_num; i++) {
            if (!pthread_tryjoin_np(pt_ids[i], NULL)) {         // successfully joined with thread i
                if (pthread_create(&pt_ids[i], NULL, connection_handler, NULL)) {   // recreate thread i
                    perror("pthread_create");
                    return EC_THREAD;
                }
            }
        }
        // Block waiting to read from either socket:
        if (poll(pfds, 2, -1) < 0) {
            if (errno == EINTR) {       // killing signal arrived
                break;
            }
            perror("poll");
            exit_code = EC_SOCK;
            break;
        }
        if (pfds[0].revents & POLLIN) {      // commandsock is ready to accept
            if ((newfd = accept(commandsock, commandptr, &command_len)) < 0) {
                perror("accept");
                exit_code = EC_SOCK;
                break;
            }
            printf("Main thread: Established command connection.\n");
            if (command_handler(newfd) != EC_OK) {     // either error or "SHUTDOWN" was requested
                server_alive = 0;
            }
        } else {      // clientsock is ready to accept
            if ((newfd = accept(clientsock, clientptr, &client_len)) < 0) {
                perror("accept");
                exit_code = EC_SOCK;
                break;
            }
            appendToFdList(newfd);     // will broadcast to threads to handle the new client
        }
    }
    thread_alive = 0;
    for (int i = 0; i < thread_num; i++) {      // signal the threads to terminate
        pthread_cond_broadcast(&fdList_cond);
    }
    for (int i = 0; i < thread_num; i++) {      // join with all the threads
        pthread_join(pt_ids[i], NULL);
    }
    pthread_cond_destroy(&fdList_cond);
    destroyIntList(&fdList);
    printf("Main thread has exited.\n");
    return exit_code;
}

int acquireFd() {
    pthread_mutex_lock(&fdList_mutex);
    while (isIntListEmpty(fdList) && thread_alive) {
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

void updateStats(long new_bytes_served) {
    pthread_mutex_lock(&stats_mutex);
    pages_served++;
    bytes_served += new_bytes_served;
    pthread_mutex_unlock(&stats_mutex);
}

int command_handler(int cmdsock) {
    /// TODO feature: thread timeout
    char command[BUFSIZ] = "";        // Undoubtedly fits a single command
    if (read(cmdsock, command, BUFSIZ - 1) < 0) {
        perror("Error reading from socket");
        return EC_SOCK;
    }
    command[BUFSIZ - 1] = '\0';
    char *command_save;
    strtok_r(command, "\r\n", &command_save);            // remove trailing newline
    if (!strcmp(command, "STATS")) {
        printf("Main thread: Received STATS command.\n");
        char *timeRunning, *stats_response;
        pthread_mutex_lock(&stats_mutex);
        timeRunning = getTimeRunning(start_time);
        if (asprintf(&stats_response, " Server up for %s, served %d pages, %ld bytes.\n", timeRunning, pages_served, bytes_served) < 0) {
            perror("asprintf");
            return EC_MEM;
        }
        pthread_mutex_unlock(&stats_mutex);
        if (write(cmdsock, stats_response, strlen(stats_response) + 1) < 0) {
            perror("Error writing to socket");
            return EC_SOCK;
        }
        printf("%s", stats_response);
        free(stats_response);
        free(timeRunning);
    } else if (!strcmp(command, "SHUTDOWN")) {
        printf("Main thread: Received SHUTDOWN command ...\n");
        close(cmdsock);
        return -1;
    } else {
        printf("Main thread: Unknown command \"%s\".\n", command);
		char unknown_response[] = " Command unknown. Available commands (use without quotes):\n  \"STATS\"\n  \"SHUTDOWN\"\n";
		if (write(cmdsock, unknown_response, strlen(unknown_response) + 1) < 0) {
			perror("Error writing to socket");
			close(cmdsock);
			return EC_SOCK;
		}
    }
    close(cmdsock);
    return EC_OK;
}

void *connection_handler(void *args) {
    /// TODO feature: thread timeout
    pthread_t self_id = pthread_self();
    printf("Thread %lu created.\n", self_id);
    while (thread_alive) {
        int sock = acquireFd();
        if (! thread_alive) {     // SHUTDOWN arrived while thread was waiting to acquireFd
            break;
        }
        long curr_bytes_served = 0;
        char curr_buf[BUFSIZ] = "";
        char *msg_buf = malloc(1);
        msg_buf[0] = '\0';
        ssize_t bytes_read;
        do {        // read entire GET request, reallocing if necessary
            memset(&curr_buf[0], 0, sizeof(curr_buf));
            bytes_read = read(sock, curr_buf, BUFSIZ - 1);
            curr_buf[BUFSIZ - 1] = '\0';
            if (bytes_read < 0) {
                perror("Error reading from socket");
                return (void *) EC_SOCK;
            } else if (bytes_read > 0) {
                msg_buf = realloc(msg_buf, sizeof(msg_buf) + BUFSIZ);
                strcat(msg_buf, curr_buf);
                if (endOfRequest(msg_buf)) {       // if double newline was found
                    break;
                }
            }
        } while (bytes_read > 0);
        if (!strcmp(msg_buf, "")) {
            printf("Thread %lu: Client terminated the connection.\n", self_id);
            free(msg_buf);
            continue;
        }

        char *responseString = NULL;
        char *requested_file = NULL;
        int rv = validateGETRequest(msg_buf, &requested_file);
        if (rv > 0) {    // Fatal Error
            return (void *) EC_MEM;
        } else if (rv < 0) {
            responseString = generateResponseString(HTTP_BADREQUEST, NULL);
            printf("Thread %lu: Received a request.\n", self_id);
            printf("Thread %lu: Responded with \"400 Bad Request\".\n", self_id);
        } else {    // valid request
            printf("Thread %lu: Received a request for \"%s\".\n", self_id, requested_file);
            char requested_file_path[PATH_MAX];
            sprintf(requested_file_path, "%s/%s", root_dir, requested_file);
            FILE *fp = fopen(requested_file_path, "r");
            if (fp == NULL) {       // failed to open file
                if (errno == EACCES) {      // failed with "Permission denied"
                    responseString = generateResponseString(HTTP_FORBIDDEN, NULL);
                    printf("Thread %lu: Responded with \"403 Forbidden\".\n", self_id);
                } else {      // file doesn't exist
                    responseString = generateResponseString(HTTP_NOTFOUND, NULL);
                    printf("Thread %lu: Responded with \"404 Not Found\".\n", self_id);
                }
            } else {
                responseString = generateResponseString(HTTP_OK, fp);
                printf("Thread %lu: Responded with \"200 OK\".\n", self_id);
                fclose(fp);
                curr_bytes_served = strlen(responseString);
            }
        }
        if (requested_file != NULL) {
            free(requested_file);
        }
        if (write(sock, responseString, strlen(responseString) + 1) < 0) {
            perror("Error writing to socket");
            return (void *) EC_SOCK;
        }
        free(responseString);
        free(msg_buf);
        if (curr_bytes_served > 0) {
            updateStats(curr_bytes_served);
        }
        close(sock);
    }
    printf("Thread %lu has exited.\n", self_id);
    return NULL;
}

