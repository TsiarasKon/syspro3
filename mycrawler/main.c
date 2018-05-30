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
#include "lists.h"
#include "requests.h"

#define CMD_LISTEN_QUEUE_SIZE 5

int serversock;
char *hostaddr = NULL;
int server_port = -1;
char *save_dir = NULL;

// Stats:
struct timeval start_time;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
int pages_downloaded = 0;
long bytes_downloaded = 0;

StringList *linkList;
pthread_mutex_t linkList_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t linkList_cond;
int waiting = 0;

volatile int thread_alive = 1;
volatile int crawler_alive = 1;

void crawler_killer(int signum) {
    crawler_alive = 0;
}

int command_handler(int cmdsock);
void *crawler_thread(void *args);

char *acquireLink() {
    pthread_mutex_lock(&linkList_mutex);
    waiting++;
    while (isStringListEmpty(linkList) && thread_alive) {
        pthread_cond_wait(&linkList_cond, &linkList_mutex);
    }
    char *link = popStringListNode(linkList);
    waiting--;
    pthread_mutex_unlock(&linkList_mutex);
    if (! isStringListEmpty(linkList)) {
        pthread_cond_broadcast(&linkList_cond);
    }
    return link;
}

void appendToLinkList(StringList **list) {
    pthread_mutex_lock(&linkList_mutex);
    appendStringList(linkList, list);
    pthread_mutex_unlock(&linkList_mutex);
    pthread_cond_broadcast(&linkList_cond);
}

int main(int argc, char *argv[]) {
    gettimeofday(&start_time, NULL);
    if (argc != 12) {
        fprintf(stderr,
                "Invalid arguments. Please run \"$ ./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL\"\n");
        return EC_ARG;
    }
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

    // Signal handling for graceful exit on fatal signals:
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = crawler_killer;
    sigaction(SIGINT,  &act, 0);
    sigaction(SIGTERM, &act, 0);
    sigaction(SIGQUIT, &act, 0);

    struct sockaddr_in crawler;
    struct sockaddr_in command;
    struct sockaddr *crawlerptr = (struct sockaddr *) &crawler;
    struct sockaddr *commandptr = (struct sockaddr *) &command;
    socklen_t command_len = 0;
    socklen_t client_len = 0;

    // Create sockets:
    int commandsock;
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

    linkList = createStringList();
    StringList *temp = createStringList();
    appendStringListNode(temp, starting_URL);
    appendToLinkList(&temp);

    pthread_cond_init(&linkList_cond, NULL);
    pthread_t pt_ids[thread_num];
    for (int i = 0; i < thread_num; i++) {
        if (pthread_create(&pt_ids[i], NULL, crawler_thread, NULL)) {
            perror("pthread_create");
            rv = EC_THREAD;
        }
    }

    int newfd;
    while (crawler_alive && !rv) {
        if ((newfd = accept(commandsock, commandptr, &command_len)) < 0) {
            if (errno == EINTR) {       // killing signal arrived
                break;
            }
            perror("accept");
            rv = EC_SOCK;
        }
        printf("Main thread: Established command connection.\n");
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
        printf("Main thread: Received STATS command.\n");
        char *timeRunning, *stats_response;
        pthread_mutex_lock(&stats_mutex);
        timeRunning = getTimeRunning(start_time);
        asprintf(&stats_response, " Server up for %s, downloaded %d pages, %ld bytes.\n", timeRunning, pages_downloaded, bytes_downloaded);
        pthread_mutex_unlock(&stats_mutex);
        if (write(cmdsock, stats_response, strlen(stats_response) + 1) < 0) {
            perror("Error writing to socket");
            return EC_SOCK;
        }
        printf("%s", stats_response);
        free(stats_response);
        free(timeRunning);
    } else if (!strcmp(command, "SEARCH")) {
        /// TODO: Implement SEARCH
    } else if (!strcmp(command, "SHUTDOWN")) {
        printf("Main thread: Received SHUTDOWN command ...\n");
        close(cmdsock);
        return -1;
    } else {
        printf("Main thread: Unknown command \"%s\".\n", command);
    }
    close(cmdsock);
    return EC_OK;
}

void updateStats(long new_bytes_downloaded) {
    pthread_mutex_lock(&stats_mutex);
    pages_downloaded++;
    bytes_downloaded += new_bytes_downloaded;
    pthread_mutex_unlock(&stats_mutex);
}

void *crawler_thread(void *args) {
    /// TODO: timeout
    pthread_t self_id = pthread_self();
    printf("Thread %lu created.\n", self_id);
    while (thread_alive) {
        char *link = acquireLink();
        if (! thread_alive) {     // SHUTDOWN arrived while thread was waiting to acquireLink
            break;
        }
        printf("Thread %lu: Received %s.\n", self_id, link);
        // Process link before requesting it:
        link = strchr(link, '/') + 2;       // skip "http://"
        link = strchr(link, '/');       // skip link address
        struct sockaddr_in server;
        struct sockaddr *serverptr = (struct sockaddr *) &server;
        struct hostent *hent;
        if ((hent = gethostbyname(hostaddr)) == NULL) {
            perror("gethostbyname");
            return (void *) EC_SOCK;
        }
        server.sin_family = AF_INET;
        memcpy(&server.sin_addr, hent->h_addr, (size_t) hent->h_length);
        server.sin_port = htons((uint16_t) server_port);
        if (connect(serversock, serverptr, sizeof(server)) < 0) {
            perror("listen");
            return (void *) EC_SOCK;
        }
        char *request = generateGETRequest(link);
        if (write(serversock, request, strlen(request) + 1) < 0) {      /// +1 ?
            perror("Error writing to socket");
            return (void *) EC_SOCK;
        }
        long curr_bytes_downloaded = 0;
        char curr_buf[BUFSIZ] = "";
        char *msg_buf = malloc(BUFSIZ);
        msg_buf[0] = '\0';
        ssize_t bytes_read;
        int content_pos = 0;
        do {
            memset(&curr_buf[0], 0, sizeof(curr_buf));
            bytes_read = read(serversock, curr_buf, BUFSIZ);
            curr_buf[BUFSIZ - 1] = '\0';
            if (bytes_read < 0) {
                perror("Error reading from socket");
                return (void *) EC_SOCK;
            } else if (bytes_read > 0) {
                msg_buf = realloc(msg_buf, sizeof(msg_buf) + BUFSIZ);
                strcat(msg_buf, curr_buf);
                if ((content_pos = endOfRequest(msg_buf)) != 0) {       // if double newline was found
                    break;
                }
            }
        } while (bytes_read > 0);
        long content_len = getContentLength(msg_buf);
        if (content_len < 0) {
            return (void *) EC_HTTP;
        }
        char *content = malloc((size_t) getContentLength(msg_buf) + 1);
        strcpy(content, msg_buf + content_pos + 1);
        do {
            memset(&curr_buf[0], 0, sizeof(curr_buf));
            bytes_read = read(serversock, curr_buf, BUFSIZ);
            curr_buf[BUFSIZ - 1] = '\0';
            if (bytes_read < 0) {
                perror("Error reading from socket");
                return (void *) EC_SOCK;
            } else if (bytes_read > 0) {
                strcat(content, curr_buf);
            }
        } while (bytes_read > 0);
        printf("%ld\n", strlen(content) + 1);
        printf("%s\n", content);

        /// Write content to file

//
//        char *responseString;
//        char *requested_file;
//        int rv = validateGETRequest(msg_buf, &requested_file);
//        if (rv > 0) {    // Fatal Error
//            return (void *) EC_MEM;
//        } else if (rv < 0) {
//            responseString = generateResponseString(HTTP_BADREQUEST, NULL);
//            printf("Thread %lu: Responded with \"400 Bad Request\".\n", self_id);
//        } else {    // valid request
//            char *requested_file_path;
//            asprintf(&requested_file_path, "%s%s", root_dir, requested_file);
//            FILE *fp = fopen(requested_file_path, "r");
//            if (fp == NULL) {       // failed to open file
//                if (errno == EACCES) {      // failed with "Permission denied"
//                    responseString = generateResponseString(HTTP_FORBIDDEN, NULL);
//                    printf("Thread %lu: Responded with \"403 Forbidden\".\n", self_id);
//                } else {      // file doesn't exist
//                    responseString = generateResponseString(HTTP_NOTFOUND, NULL);
//                    printf("Thread %lu: Responded with \"404 Not Found\".\n", self_id);
//                }
//            } else {
//                responseString = generateResponseString(HTTP_OK, fp);
//                printf("Thread %lu: Responded with \"200 OK\".\n", self_id);
//                fclose(fp);
//                curr_bytes_downloaded = strlen(responseString);
//            }
//            free(requested_file_path);
//        }
//        free(requested_file);
//        if (write(sock, responseString, strlen(responseString) + 1) < 0) {      /// +1 ?
//            perror("Error writing to socket");
//            return (void *) EC_SOCK;
//        }
//        free(responseString);
//        free(msg_buf);
//        if (curr_bytes_downloaded > 0) {
//            updateStats(curr_bytes_downloaded);
//        }
//        close(sock);
    }
    printf("Thread %lu has exited.\n", self_id);
    return NULL;
}