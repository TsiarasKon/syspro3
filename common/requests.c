#define _GNU_SOURCE         // needed for asprintf()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "requests.h"
#include "util.h"

int validateGETRequest(char *request, char **requested_file) {
    // Validates the existence of "GET" and "Host:" lines
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
        /// TODO feature: checking other fields as well
        if (strlen(curr_line) > 6 && !strncmp(curr_line, "Host: ", 6)) {      // found host
            return 0;
        }
    }
    return -2;
}

char *generateGETRequest(char *filename) {
    char *header;
    if (asprintf(&header, "GET %s HTTP/1.1", filename) < 0) {
        perror("asprintf");
        return NULL;
    }
    char host[] = "Host: localhost";
    char *request;
    if (asprintf(&request, "%s\n%s\n\n", header, host) < 0) {
        perror("asprintf");
        return NULL;
    }
    free(header);
    return request;
}

const char header_OK[] = "HTTP/1.1 200 OK";
const char header_BADREQUEST[] = "HTTP/1.1 400 Bad Request";
const char header_FORBIDDEN[] = "HTTP/1.1 403 Forbidden";
const char header_NOTFOUND[] = "HTTP/1.1 404 Not Found";
const char msg_BADREQUEST[] = \
"<!DOCTYPE html><html><body><h3>Your request was bad and you should feel bad.</h3></body></html>";
const char msg_FORBIDDEN[] = \
"<!DOCTYPE html><html><body><h3>You have no power here! No permissions, that is.</h3></body></html>";
const char msg_NOTFOUND[] = \
"<!DOCTYPE html><html><body><h3>Your file is in another castle. Or directory. Not sure.</h3></body></html>";
const char server[] = "Server: myhttpd/1.0.0 (Ubuntu64)";
const char cont_type[] = "Content-Type: text/html";
const char conn[] = "Connection: Closed";

char *generateResponseString(int response, FILE *fp) {
    char *date = getHTTPDate();
    char *cont_len, *responseString;
    if (response == HTTP_OK) {
        char *content = fileToString(fp);
        if (asprintf(&cont_len, "Content-Length: %ld", strlen(content) + 1) < 0) {
            perror("asprintf");
            return NULL;
        }
        if (asprintf(&responseString, "%s\n%s\n%s\n%s\n%s\n%s\n\n%s", header_OK, date, server, cont_len, cont_type, \
        conn, content) < 0) {
            perror("asprintf");
            return NULL;
        }
        free(content);
    } else if (response == HTTP_BADREQUEST) {
        if (asprintf(&cont_len, "Content-Length: %ld", strlen(msg_BADREQUEST) + 1) < 0) {
            perror("asprintf");
            return NULL;
        }
        if (asprintf(&responseString, "%s\n%s\n%s\n%s\n%s\n%s\n\n%s", header_BADREQUEST, date, server, cont_len, \
        cont_type, conn, msg_BADREQUEST) < 0) {
            perror("asprintf");
            return NULL;
        }
    } else if (response == HTTP_FORBIDDEN) {
        if (asprintf(&cont_len, "Content-Length: %ld", strlen(msg_FORBIDDEN) + 1) < 0) {
            perror("asprintf");
            return NULL;
        }
        if (asprintf(&responseString, "%s\n%s\n%s\n%s\n%s\n%s\n\n%s", header_FORBIDDEN, date, server, cont_len, \
        cont_type, conn, msg_FORBIDDEN) < 0) {
            perror("asprintf");
            return NULL;
        }
    } else {    // HTTP_NOTFOUND
        if (asprintf(&cont_len, "Content-Length: %ld", strlen(msg_NOTFOUND) + 1) < 0) {
            perror("asprintf");
            return NULL;
        }
        if (asprintf(&responseString, "%s\n%s\n%s\n%s\n%s\n%s\n\n%s", header_NOTFOUND, date, server, cont_len, \
        cont_type, conn, msg_NOTFOUND) < 0) {
            perror("asprintf");
            return NULL;
        }
    }
    free(date);
    free(cont_len);
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

int getResponseCode(char * const response) {
    char *response_copy = malloc(strlen(response) + 1);
    char *response_copyptr = response_copy;
    strcpy(response_copy, response);
    char *curr_word_save = NULL;
    char *curr_word = strtok_r(response_copy, " ", &curr_word_save);     // "HTTP/1.1"
    if (curr_word != NULL) {
        curr_word = strtok_r(NULL, " ", &curr_word_save);
        if (curr_word != NULL) {
            int code = atoi(curr_word);
            free(response_copyptr);
            return code;
        }
    }
    free(response_copyptr);
    return -1;
}

long getContentLength(char * const response) {
    char *response_copy = malloc(strlen(response) + 1);
    char *response_copyptr = response_copy;
    strcpy(response_copy, response);
    char *curr_line_save = NULL;
    char *curr_line = strtok_r(response_copy, "\r\n", &curr_line_save);
    while (curr_line != NULL) {
        if (strlen(curr_line) > 16 && !strncmp(curr_line, "Content-Length: ", 16)) {
            long cont_len = atoi(curr_line + 16);
            free(response_copyptr);
            return cont_len;
        }
        curr_line = strtok_r(NULL, "\r\n", &curr_line_save);
    }
    free(response_copyptr);
    return -1;
}

StringList *retrieveLinks(char ** const content, char *hostaddr, int server_port) {
    // Returns a StringList of all the links contained in the content
    char *content_ptr = *content;
    StringList *content_links = createStringList();
    if (content_links == NULL) {
        return NULL;
    }
    int i = 0;
    while (content_ptr[i] != '\0') {
        if (!strncmp(content_ptr + i, "<a href=\"", 9)) {
            i += 9;
            char new_link[PATH_MAX] = "";        // link len won't be larger than PATH_MAX
            sprintf(new_link, "http://%s:%d", hostaddr, server_port);
            int j = (int) strlen(new_link);
            while (content_ptr[i] == '.') {     // skip '..' if root relative (as expected)
                i++;
            }
            while (content_ptr[i] != '\0' && content_ptr[i] != '"') {
                new_link[j] = content_ptr[i];
                j++;
                i++;
            }
            new_link[j] = '\0';
//            printf("||%s||\n", new_link);
            appendStringListNode(content_links, new_link);
        }
        i++;
    }
    return content_links;
}
