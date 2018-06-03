#ifndef REQUESTS_H
#define REQUESTS_H

#include "lists.h"

#define HTTP_OK 200
#define HTTP_BADREQUEST 400
#define HTTP_FORBIDDEN 403
#define HTTP_NOTFOUND 404

int validateGETRequest(char *request, char **requested_file);
char *generateGETRequest(char *filename);
char *generateResponseString(int response, FILE *fp);

int endOfRequest(char const *request);
int getResponseCode(char *response);
long getContentLength(char *response);
StringList *retrieveLinks(char **content, char *hostaddr, int server_port);

#endif
