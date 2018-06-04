#define _GNU_SOURCE         // needed for asprintf()
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include "util.h"

const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

char* getTimeRunning(struct timeval start_time) {
    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    long long msRunning = ((curr_time.tv_sec - start_time.tv_sec) * 1000000LL + curr_time.tv_usec - start_time.tv_usec) / 1000;
    time_t secRunning = msRunning / 1000;
    struct tm timeRunning;
    gmtime_r(&secRunning, &timeRunning);
    char *time_str;
    // "HH:MM:SS.ms" - HH and MM will only be printed if HH or either (respectively) is greater than 0
    if (timeRunning.tm_hour > 0) {
        if (asprintf(&time_str, "%.2d:%.2d:%.2d.%.2lld", timeRunning.tm_hour, timeRunning.tm_min, timeRunning.tm_sec, \
        (msRunning % 1000) / 10) < 0) {
            perror("asprintf");
            return NULL;
        }
    } else if (timeRunning.tm_min > 0) {
        if (asprintf(&time_str, "%.2d:%.2d.%.2lld", timeRunning.tm_min, timeRunning.tm_sec, (msRunning % 1000) / 10) < 0) {
            perror("asprintf");
            return NULL;
        }
    } else {
        if (asprintf(&time_str, "%.2d.%.2lld", timeRunning.tm_sec, (msRunning % 1000) / 10) < 0) {
            perror("asprintf");
            return NULL;
        }
    }
    return time_str;
}

char* getHTTPDate() {
    time_t curr_time = time(NULL);
    struct tm *t;
    t = gmtime(&curr_time);
    char *time_str;
    if (asprintf(&time_str, "Date: %s, %.2d %s %.4d %.2d:%.2d:%.2d GMT", days[t->tm_wday], t->tm_mday, \
    months[t->tm_mon], t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec) < 0) {
        perror("asprintf");
        return NULL;
    }
    return time_str;
}

char *fileToString(FILE *fp) {
    // Inspired from this snippet: https://stackoverflow.com/questions/174531/easiest-way-to-get-files-contents-in-c
    if (fp == NULL) {
        fprintf(stderr, "Attempted fileToString() with NULL file.");
        return NULL;
    }
    char *buffer = 0;
    long length = 0;
    if (fseek(fp, 0, SEEK_END)) {
        perror("fseek");
        return NULL;
    }
    length = ftell(fp);
    if (length == -1L) {
        perror("ftell");
        return NULL;
    }
    if (fseek(fp, 0, SEEK_SET)) {
        perror("fseek");
        return NULL;
    }
    buffer = malloc((size_t) length + 1);
    if (buffer == NULL) {
        perror("malloc in fileToString");
        return NULL;
    }
    fread(buffer, 1, (size_t) length, fp);
    buffer[length] = '\0';
    return buffer;
}

/* // I attempted to recreate it using getline() but, as expected, it was noticably slower:
char *fileToString2(FILE *fp) {
    if (fp == NULL) {
        fprintf(stderr, "Attempted fileToString() with NULL file.");
        return NULL;
    }
    char *filestring = malloc(1);
    size_t filestringsize = 1;
    filestring[0] = '\0';
    char *linebuf = NULL;
    size_t linebufsize = BUFSIZ;
    while (getline(&linebuf, &linebufsize, fp) != -1) {
        filestring = realloc(filestring, filestringsize + strlen(linebuf));
        if (filestring == NULL) {
            perror("realloc in fileToString");
            return NULL;
        }
        strcpy(filestring + filestringsize - 1, linebuf);
        filestringsize += strlen(linebuf);
    }
    if (linebuf != NULL) {
        free(linebuf);
    }
    filestring[filestringsize - 1] = '\0';
    return filestring;
}
*/
