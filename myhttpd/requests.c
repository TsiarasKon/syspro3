#define _GNU_SOURCE         // needed for asprintf()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "requests.h"
#include "util.h"


const char server[] = "Server: myhttpd/1.0.0 (Ubuntu64)";
const char cont_type[] = "Content-Type: text/html";
const char conn[] = "Connection: Closed";

char *createResponseString(int response, int fd) {
    char *date = getHTTPDate();
    char *header, *cont_len, *content;
    switch (response) {
        case 200:
            asprintf(&header, "HTTP/1.1 200 OK");
            /// content
            asprintf(&content, "<content %d>", fd);
            asprintf(&cont_len, "Content-Length: %ld", strlen(content) + 1);     ///
            break;
        case 403:
            asprintf(&header, "HTTP/1.1 403 Forbidden");
            asprintf(&content, "<html>You have no power here! No permissions, that is.</html>");
            asprintf(&cont_len, "Content-Length: 124");       ///
            break;
        case 404:
            asprintf(&header, "HTTP/1.1 404 Not Found");
            asprintf(&content, "<html>Your file is in another castle. Or directory. Not sure.</html>");
            asprintf(&cont_len, "Content-Length: 124");
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