#define _GNU_SOURCE         // needed for asprintf()
#include <stdio.h>
#include <time.h>
#include "util.h"

const char *cmds[7] = {
        "/search",
        "/maxcount",
        "/mincount",
        "/wc",
        "/pids",
        "/help",
        "/exit"
};

char* getTimeRunning(time_t start_time) {
    time_t curr_time = time(NULL) - start_time;
    printf("%ld %ld %ld\n", time(NULL), start_time, curr_time);
    struct tm *timeinfo;
    timeinfo = gmtime(&curr_time);
    char *time_str;
    asprintf(&time_str, "%d:%d.%d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    return time_str;
}
