#define _GNU_SOURCE         // needed for asprintf()
#include <stdio.h>
#include <stdlib.h>
#include "requests.h"
#include "util.h"

char *createResponseString(int response, int len, int fd) {
    char *date = getHTTPDate();
    char *header, *server;
    switch (response) {
        case 0:     // OK
            asprintf(&header, "HTTP/1.1 200 OK\n");
            break;
    }
}