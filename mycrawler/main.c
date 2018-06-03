#define _GNU_SOURCE         // needed for asprintf()
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/wait.h>
#include <ctype.h>
#include "../common/util.h"
#include "../common/lists.h"
#include "../common/requests.h"

#define CMD_LISTEN_QUEUE_SIZE 5
#define WORKERS_NUM 5
#define DIRPERMS (S_IRWXU | S_IRWXG)        // default 0660

// Needed global variables:
char *save_dir = NULL;
char *hostaddr = NULL;
int server_port = -1;
int thread_num = -1;

pid_t jobExecutor_pid;
int to_JE[2];
int from_JE[2];

// Stats:
struct timeval start_time;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
int pages_downloaded = 0;
long bytes_downloaded = 0;

// Link lists:
StringList *waitingLinkList;
StringList *visitedLinkList;
pthread_mutex_t linkList_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t linkList_cond;
int waiting = 0;

// Dirfile:
char dirfile[] = "./dirfile.txt";
StringList *dirfileList;
pthread_mutex_t dirfile_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function prototypes:
char *acquireLink();
void appendToLinkList(StringList *list);
void updateStats(long new_bytes_downloaded);
int mkdir_path(char *linkpath);
int command_handler(int cmdsock);
void *crawler_thread(void *args);

volatile int thread_alive = 1;
volatile int crawler_alive = 1;

void crawler_killer(int signum) {
    crawler_alive = 0;
}

int main(int argc, char *argv[]) {
    gettimeofday(&start_time, NULL);
    if (argc != 12) {
        fprintf(stderr,
                "Invalid arguments. Please run \"$ ./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL\"\n");
        return EC_ARG;
    }
    int command_port = -1;
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
                fprintf(stderr, "Invalid arguments. Please run \"$ ./jobExecutor -d dirfile -w numWorkers\"\n");
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

    // Create sockets:
    int commandsock;
    if ((commandsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return EC_SOCK;
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
        return EC_SOCK;
    }
    // Listen for connections to commandsock:
    if (listen(commandsock, CMD_LISTEN_QUEUE_SIZE) < 0) {
        perror("listen");
        return EC_SOCK;
    }

    struct stat st = {0};
    if (!stat(dirfile, &st)) {      // dirfile already exists
        remove(dirfile);
    }
    if (!stat(save_dir, &st)) {      // save_dir already exists
        printf(" save_dir '%s' already exists. Renaming it ...\n", save_dir);
        int i = 0;
        char old_save_dir[PATH_MAX];
        do {
            sprintf(old_save_dir, "%s_ver%d", save_dir, i);
            i++;
        } while (!stat(old_save_dir, &st));
        if (rename(save_dir, old_save_dir) < 0) {
            printf(" Failed to rename existing save_dir. It will be overwritten by this program.\n");
        } else {
            printf(" Old save_dir successfully renamed to '%s'.\n", old_save_dir);
        }
    }

    int rv = EC_OK;

    waitingLinkList = createStringList();
    visitedLinkList = createStringList();
    StringList *startingLink = createStringList();
    appendStringListNode(startingLink, starting_URL);
    appendToLinkList(startingLink);
    destroyStringList(&startingLink);
    dirfileList = createStringList();

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
    if (!thread_alive) {        // threads are already dead and jobExecutor is running
        if (write(to_JE[1], "/exit\n", 6) == -1) {
            perror("Error writing to pipe");
        }
        close(to_JE[1]);
        close(from_JE[0]);
        wait(NULL);
    } else {
        thread_alive = 0;
        for (int i = 0; i < thread_num; i++) {      // signal the threads to terminate
            pthread_cond_broadcast(&linkList_cond);
        }
    }
    for (int i = 0; i < thread_num; i++) {      // join with all the threads
        pthread_join(pt_ids[i], NULL);
    }
    pthread_cond_destroy(&linkList_cond);
    destroyStringList(&waitingLinkList);
    destroyStringList(&visitedLinkList);
    destroyStringList(&dirfileList);
    printf("Main thread has exited.\n");
    return rv;
}

char *acquireLink() {
    pthread_mutex_lock(&linkList_mutex);
    waiting++;
    while (isStringListEmpty(waitingLinkList) && thread_alive && waiting < thread_num) {
        pthread_cond_wait(&linkList_cond, &linkList_mutex);
    }
    char *link = popStringListNode(waitingLinkList);
    waiting--;
    pthread_mutex_unlock(&linkList_mutex);
    if (link == NULL && thread_alive == 1) {        // crawling complete; instantiate jobExecutor
        thread_alive = 0;
        for (int i = 0; i < thread_num - 1; i++) {
            pthread_cond_broadcast(&linkList_cond);
        }
        /* Uncommenting this block would lead to program termination once crawling is complete
        kill(getpid(), SIGINT);
        return NULL;
        */
        printf(" Site crawling complete - %d pages downloaded.\n", pages_downloaded);
        if (pages_downloaded == 0) {
            return NULL;
        }
        // Fork and execute jobExecutor from child:
        // Creating dirfile that will be passed as an argument to jobExecutor:
        FILE *dirfp = fopen(dirfile, "w");
        // At this point all threads will have finished, no need to access dirfileList via mutex
        StringListNode *current = dirfileList->first;
        while (current != NULL) {
            fprintf(dirfp, "../mycrawler/%s\n", current->string);
            current = current->next;
        }
        fclose(dirfp);
        // Communication is done by redirecting jobExecutor's stdin and stdout to pipes
        signal(SIGPIPE, SIG_IGN);       // Unneeded signal - Pipe errors can be caught in error checking
        pipe(to_JE);
        pipe(from_JE);
        jobExecutor_pid = fork();
        if (jobExecutor_pid < 0) {
            perror("fork");
            exit(EC_CHILD);
        } else if (jobExecutor_pid == 0) {
            close(to_JE[1]);
            close(from_JE[0]);
            char *workers_num;
            asprintf(&workers_num, "%d", WORKERS_NUM);
            char *jobExecutor_argv[] = {"jobExecutor", "-d", dirfile, "-w", workers_num, NULL};
            dup2(to_JE[0], STDIN_FILENO);
            close(to_JE[0]);
            dup2(from_JE[1], STDOUT_FILENO);
            close(from_JE[1]);
            execv("../jobExecutor/jobExecutor", jobExecutor_argv);
            exit(EC_CHILD);      // Only if execv() failed
        } else {
            close(to_JE[0]);
            close(from_JE[1]);
            printf(" SEARCH command is now available.\n");
        }
    }
    if (! isStringListEmpty(waitingLinkList)) {
        pthread_cond_broadcast(&linkList_cond);
    }
    return link;
}

void appendToLinkList(StringList *list) {
    int newLinks = 0;
    StringListNode *current = list->first;
    pthread_mutex_lock(&linkList_mutex);
    while (current != NULL) {
        if (existsInStringList(visitedLinkList, current->string)) {     // link has already been processed
            current = current->next;
            continue;
        }
        newLinks = 1;
        appendStringListNode(waitingLinkList, current->string);
        appendStringListNode(visitedLinkList, current->string);
        current = current->next;
    }
    pthread_mutex_unlock(&linkList_mutex);
    if (newLinks) {
        pthread_cond_broadcast(&linkList_cond);
    }
}

void updateStats(long new_bytes_downloaded) {
    pthread_mutex_lock(&stats_mutex);
    pages_downloaded++;
    bytes_downloaded += new_bytes_downloaded;
    pthread_mutex_unlock(&stats_mutex);
}

int mkdir_path(char *linkpath) {         // recursively create all directories in link path
    struct stat st = {0};
    char full_path[PATH_MAX] = "";
    char *curr_dir_save = NULL;
    char *curr_dir = strtok_r(linkpath, "/", &curr_dir_save);
    while (curr_dir != NULL) {
        if (strchr(curr_dir, '.') != NULL) {    // path contains '.' - probably an .html file and not a dir
            break;
        }
        strcat(full_path, curr_dir);
        strcat(full_path, "/");
        if (stat(full_path, &st) < 0) {
            if (mkdir(full_path, DIRPERMS) < 0) {
                perror("mkdir");
                return EC_DIR;
            }
        }
        curr_dir = strtok_r(NULL, "/", &curr_dir_save);
    }
    // Append full dir path to dirfile:
    pthread_mutex_lock(&dirfile_mutex);
    if (!existsInStringList(dirfileList, full_path)) {
        appendStringListNode(dirfileList, full_path);
    }
    pthread_mutex_unlock(&dirfile_mutex);
    return 0;
}

int command_handler(int cmdsock) {
    char command[BUFSIZ];        // Undoubtedly fits a single command
    if (read(cmdsock, command, BUFSIZ) < 0) {
        perror("Error reading from socket");
        return EC_SOCK;
    }
    strtok(command, "\r\n");            // remove trailing newline
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
    } else if (!strncmp(command, "SEARCH", 6)) {
        printf("Main thread: Received SEARCH command.\n");
        if (thread_alive) {
            char search_response[] = " SEARCH command cannot be executed right now - Site crawling is still in progress.\n";
            if (write(cmdsock, search_response, strlen(search_response) + 1) < 0) {
                perror("Error writing to socket");
                return EC_SOCK;
            }
        } else if (pages_downloaded == 0) {
            char search_response[] = " No pages were downloaded. SEARCH command is not available.\n";
            if (write(cmdsock, search_response, strlen(search_response) + 1) < 0) {
                perror("Error writing to socket");
                return EC_SOCK;
            }
        } else if (strlen(command) < 8 || isspace(command[7])) {       // No arguments given
            char search_response[] = " Invalid use of SEARCH command - use as: \"SEARCH word1 word2 ...\"\n";
            if (write(cmdsock, search_response, strlen(search_response) + 1) < 0) {
                perror("Error writing to socket");
                return EC_SOCK;
            }
        } else {
            char search_command[BUFSIZ] = "/search ";
            strcat(search_command, command + 7);
            strcat(search_command, " -d 0\n");
            if (write(to_JE[1], search_command, strlen(search_command)) == -1) {
                perror("Error writing to pipe");
                return EC_PIPE;
            }
            char buffer[BUFSIZ] = "";
            long bytes_read = 0;
            int message_end = 0;
            while (!message_end && (bytes_read = read(from_JE[0], buffer, BUFSIZ - 1)) > 0) {
                if (!strcmp(buffer, "<")) {
                    break;
                }
                if (strlen(buffer) < 2) {       // skip '\n' lines
                    continue;
                }
                if (buffer[bytes_read - 1] == '<') {
                    buffer[bytes_read - 1] = '\0';
                    message_end = 1;
                } else {
                    buffer[bytes_read] = '\0';
                }
                if (!strncmp(buffer, "\nType a command:\n", 17)) {     // skips newlines and "Type a command:" prompt
                    //printf(" %s", buffer + 17);
                    if (write(cmdsock, buffer + 17, strlen(buffer) - 16) < 0) {
                        perror("Error writing to socket");
                        return EC_SOCK;
                    }
                } else {
                    //printf("%s", buffer);
                    if (write(cmdsock, buffer, strlen(buffer) + 1) < 0) {
                        perror("Error writing to socket");
                        return EC_SOCK;
                    }
                }
                if (strchr(buffer, '<') != NULL) {      // Shouldn't be needed: Last resort, in case we previously missed '<'
                    break;
                }
            }
            if (bytes_read < 0) {
                perror("Error reading from pipe");
                exit(EC_PIPE);
            }
        }
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

void *crawler_thread(void *args) {
    /// TODO: timeout
    pthread_t self_id = pthread_self();
    printf("Thread %lu created.\n", self_id);
    while (thread_alive) {
        char *link = acquireLink();
        char *link_ptr = link;
        if (!thread_alive || link == NULL) {     // SHUTDOWN may have arrived while thread was waiting to acquireLink
            break;
        }
        printf("Thread %lu: Requesting %s ...\n", self_id, link);
        int serversock;
        if ((serversock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket");
            return (void *) EC_SOCK;
        }
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

        // Process link before requesting it:
        if (strlen(link) > 4 && !strncmp(link, "http", 4)) {      // skip "http://"
            link = strchr(link, '/') + 2;
        }
        link = strchr(link, '/');       // skip link address
        char *request = generateGETRequest(link);
        if (write(serversock, request, strlen(request) + 1) < 0) {
            perror("Error writing to socket");
            return (void *) EC_SOCK;
        }
        free(request);
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
        if (getResponseCode(msg_buf) != HTTP_OK) {
            printf("Thread %lu: Failed to GET '%s'\n", self_id, link);
            free(link_ptr);
            free(msg_buf);
            continue;
        }
        long content_len = getContentLength(msg_buf);
        if (content_len < 0) {
            continue;
        }
        char *content = malloc((size_t) content_len);
        if (content == NULL) {
            perror("malloc in content");
            return (void *) EC_MEM;
        }
        strcpy(content, msg_buf + content_pos + 1);
        free(msg_buf);
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

        char link_path[PATH_MAX];
        if (link[0] == '/') {
            sprintf(link_path, "%s%s", save_dir, link);
        } else {
            sprintf(link_path, "%s/%s", save_dir, link);
        }
        char link_path_copy[PATH_MAX];
        strcpy(link_path_copy, link_path);
        if (mkdir_path(link_path_copy)) {
            return (void *) EC_DIR;
        }

        StringList *content_links = retrieveLinks(&content, hostaddr, server_port);
        if (content_links == NULL) {
            return (void *) EC_MEM;
        }
        appendToLinkList(content_links);
        destroyStringList(&content_links);

        FILE *fp = fopen(link_path, "w");
        if (fp == NULL) {       // failed to open file
            perror("fopen");
            return (void *) EC_FILE;
        }
        fprintf(fp, "%s", content);
        fclose(fp);
        printf("Thread %lu: Received %s and saved it to disk.\n", self_id, link);

        free(link_ptr);
        free(content);
        if (content_len > 0) {
            updateStats(content_len);
        }
        close(serversock);
    }
    printf("Thread %lu has exited.\n", self_id);
    return NULL;
}


