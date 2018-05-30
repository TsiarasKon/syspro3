#define _GNU_SOURCE         // needed for asprintf()
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include "util.h"

#define DIRPERMS (S_IRWXU | S_IRWXG)        // default 0660

const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

char* getTimeRunning(struct timeval start_time) {
    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    long long msRunning = ((curr_time.tv_sec - start_time.tv_sec) * 1000000LL + curr_time.tv_usec - start_time.tv_usec) / 1000;
    time_t secRunning = msRunning / 1000;
    struct tm *timeRunning;
    timeRunning = gmtime(&secRunning);
    char *time_str;
    if (timeRunning->tm_hour > 0) {
        asprintf(&time_str, "%.2d:%.2d:%.2d.%.2lld", timeRunning->tm_hour, timeRunning->tm_min, timeRunning->tm_sec, (msRunning % 1000) / 10);
    } else if (timeRunning->tm_min > 0) {
        asprintf(&time_str, "%.2d:%.2d.%.2lld", timeRunning->tm_min, timeRunning->tm_sec, (msRunning % 1000) / 10);
    } else {
        asprintf(&time_str, "%.2d.%.2lld", timeRunning->tm_sec, (msRunning % 1000) / 10);
    }
    return time_str;
}

char* getHTTPDate() {
    time_t curr_time = time(NULL);
    struct tm *t;
    t = gmtime(&curr_time);
    char *time_str;
    asprintf(&time_str, "Date: %s, %.2d %s %.4d %.2d:%.2d:%.2d GMT", days[t->tm_wday], t->tm_mday, months[t->tm_mon], \
    t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
    return time_str;
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
    return 0;
}