#ifndef MYHTTPD_REQUESTS_H
#define MYHTTPD_REQUESTS_H

int validateGETRequest(char *request, char **requested_file, char **hostname);
char *createResponseString(int response, int fd);

#endif
