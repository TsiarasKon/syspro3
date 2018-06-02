#define _GNU_SOURCE         // needed for asprintf()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "requests.h"
#include "util.h"

int validateGETRequest(char *request, char **requested_file) {
    char *curr_line_save = NULL;
    char *curr_line = strtok_r(request, "\r\n", &curr_line_save);
    if (curr_line == NULL) {
        return -1;
    }
    char *word_save = NULL;
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

char *generateGETRequest(char *filename) {
    char *header;
    asprintf(&header, "GET %s HTTP/1.1", filename);
    char host[] = "Host: localhost";
    char *request;
    asprintf(&request, "%s\n%s\n\n", header, host);
    free(header);
    return request;
}

const char server[] = "Server: myhttpd/1.0.0 (Ubuntu64)";
const char cont_type[] = "Content-Type: text/html";
const char conn[] = "Connection: Closed";

char *generateResponseString(int response, FILE *fp) {
    char *date = getHTTPDate();
    char *header, *content;
    switch (response) {
        case HTTP_OK:
            asprintf(&header, "HTTP/1.1 200 OK");
            content = fileToString(fp);
            break;
        case HTTP_BADREQUEST:
            asprintf(&header, "HTTP/1.1 400 Bad Request");
            asprintf(&content, "<html>Your request was bad and you should feel bad.</html>");
            break;
        case HTTP_FORBIDDEN:
            asprintf(&header, "HTTP/1.1 403 Forbidden");
            asprintf(&content, "<html>You have no power here! No permissions, that is.</html>");
            break;
        case HTTP_NOTFOUND:
            asprintf(&header, "HTTP/1.1 404 Not Found");
            asprintf(&content, "<html>Your file is in another castle. Or directory. Not sure.</html>");
            break;
        default:        // should never get here
            free(date);
            return NULL;
    }
    char *cont_len;
    asprintf(&cont_len, "Content-Length: %ld", strlen(content));
    char *responseString;
    asprintf(&responseString, "%s\n%s\n%s\n%s\n%s\n%s\n\n%s", header, date, server, cont_len, cont_type, conn, content);
    free(date);
    free(header);
    free(cont_len);
    free(content);
    return responseString;
}

int endOfRequest(char const *request) {      // Return the index of the second newline, if found
    int i = 0;
    while (request[i] != '\0') {
        if (request[i] == '\n') {
            if (request[i + 1] == '\n') {
                return i + 1;
            } else if ((request[i + 1] == '\r' && request[i + 2] == '\n')) {
                return i + 2;
            }
        }
        i++;
    }
    return 0;
}

long getContentLength(char *request) {
    char *curr_line_save = NULL;
    char *curr_line = strtok_r(request, "\r\n", &curr_line_save);
    while (curr_line != NULL) {
        if (strlen(curr_line) > 16 && !strncmp(curr_line, "Content-Length: ", 16)) {
            return atoi(curr_line + 16);
        }
        curr_line = strtok_r(NULL, "\r\n", &curr_line_save);
    }
    return -1;
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

StringList *retrieveLinks(char ** const content, char *hostaddr, int server_port) {
    char *content_ptr = *content;
    StringList *content_links = createStringList();
    if (content_links == NULL) {
        return NULL;
    }
    int i = 0;
    while (content_ptr[i] != '\0') {
        if (!strncmp(content_ptr + i, "<a href=\"", 9)) {
            i += 9;
            char new_link[PATH_MAX];        // link len won't be larger than PATH_MAX
            sprintf(new_link, "http://%s:%d", hostaddr, server_port);
            int j = (int) strlen(new_link);
            while (content_ptr[i] != '\0' && content_ptr[i] != '\"') {
                new_link[j] = content_ptr[i];
                j++;
                i++;
            }
            new_link[j] = '\0';
            appendStringListNode(content_links, new_link);
        }
        i++;
    }
    return content_links;
}


/* Sample request (use telnet) :
    GET /site1/page1_4520.html HTTP/1.1
    User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)
    Host: www.tutorialspoint.com
    Accept-Language: en-us
    Accept-Encoding: gzip, deflate
    Connection: Keep-Alive
*/