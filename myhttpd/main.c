#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <pthread.h>
#include "util.h"

time_t start_time;
int pages_served = 0;
int bytes_served = 0;

int handle_command(int cmdsock);

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


//    pthread_t cmd_pt_id;
//    if (pthread_create(&cmd_pt_id, NULL, command_thread, NULL)) {
//        perror("pthread_create");
//        return EC_THREAD;
//    }
//    printf("Created thread %d.\n", i);

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
            if (handle_command(cmdsock) != EC_OK) {     // either error or "SHUTDOWN" was requested
                break;      ///
            }
        }
    }

    return EC_OK;
}

int handle_command(int cmdsock) {
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


