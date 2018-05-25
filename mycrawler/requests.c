#define _GNU_SOURCE         // needed for asprintf()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "requests.h"
#include "util.h"

int validateGETRequest(char *request, char **requested_file) {
    char *curr_line_save;
    char *curr_line = strtok_r(request, "\r\n", &curr_line_save);
    if (curr_line == NULL) {
        return -1;
    }
    char *word_save;
    char *word = strtok_r(curr_line, " ", &word_save);
    if (word == NULL || strcmp(word, "GET") != 0) {
        return -1;
    }
    word = strtok_r(NULL, " ", &word_save);
    *requested_file = malloc(strlen(word) + 1);
    if (*requested_file == NULL) {
        perror("malloc");
        return EC_MEM;
    }
    strcpy(*requested_file, word);
    word = strtok_r(NULL, " ", &word_save);
    if (word == NULL || strcmp(word, "HTTP/1.1") != 0) {
        return -1;
    }
    while ((curr_line = strtok_r(NULL, "\r\n", &curr_line_save)) != NULL) {
        /// Possible future feature: checking other fields as well
        if (strlen(curr_line) > 6 && !strncmp(curr_line, "Host: ", 6)) {      // found host
            return 0;
        }
    }
    return -2;
}

const char server[] = "Server: myhttpd/1.0.0 (Ubuntu64)";
const char cont_type[] = "Content-Type: text/html";
const char conn[] = "Connection: Closed";

char *createResponseString(int response, FILE *fp) {
    char *date = getHTTPDate();
    char *header, *cont_len, *content;
    switch (response) {
        case HTTP_OK:
            asprintf(&header, "HTTP/1.1 200 OK");
            content = fileToString(fp);
            asprintf(&cont_len, "Content-Length: %ld", strlen(content) + 1);     ///
            break;
        case HTTP_BADREQUEST:
            asprintf(&header, "HTTP/1.1 400 Bad Request");
            asprintf(&content, "<html>Your request was bad and you should feel bad.</html>");
            asprintf(&cont_len, "Content-Length: %ld", strlen(content) + 1);
            break;
        case HTTP_FORBIDDEN:
            asprintf(&header, "HTTP/1.1 403 Forbidden");
            asprintf(&content, "<html>You have no power here! No permissions, that is.</html>");
            asprintf(&cont_len, "Content-Length: %ld", strlen(content) + 1);
            break;
        case HTTP_NOTFOUND:
            asprintf(&header, "HTTP/1.1 404 Not Found");
            asprintf(&content, "<html>Your file is in another castle. Or directory. Not sure.</html>");
            asprintf(&cont_len, "Content-Length: %ld", strlen(content) + 1);
            break;
        default:        // should never get here
            free(date);
            return NULL;
    }
    char *responseString;
    asprintf(&responseString, "%s\n%s\n%s\n%s\n%s\n%s\n\n%s", header, date, server, cont_len, cont_type, conn, content);
    free(date);
    free(header);
    free(cont_len);
    free(content);
    return responseString;
}

int endOfRequest(char const *request) {      // Search entire request for double newline
    while (*request != '\0') {
        if (*request == '\n' && (*(request + 1) == '\n' || (*(request + 1) == '\r' && *(request + 2) == '\n'))) {
            return 1;
        }
        request++;
    }
    return 0;
}


char *fileToString(FILE *fp) {
    // Inspired from this snippet: https://stackoverflow.com/questions/174531/easiest-way-to-get-files-contents-in-c
    if (fp == NULL) {
        fprintf(stderr, "fileToString() with empty file.");
        return NULL;
    }
    char *buffer = 0;
    long length;
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buffer = malloc((size_t) length + 1);
    if (buffer == NULL) {
        perror("malloc in fileToString");
        return NULL;
    }
    fread(buffer, 1, (size_t) length, fp);
    buffer[length] = '\0';
    return buffer;
}

/* Sample request (use telnet) :
    GET /site1/page1_4520.html HTTP/1.1
    User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)
    Host: www.tutorialspoint.com
    Accept-Language: en-us
    Accept-Encoding: gzip, deflate
    Connection: Keep-Alive
*/