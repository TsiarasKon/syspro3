#ifndef REQUESTS_H
#define REQUESTS_H

#define HTTP_OK 200
#define HTTP_BADREQUEST 400
#define HTTP_FORBIDDEN 403
#define HTTP_NOTFOUND 404

int validateGETRequest(char *request, char **requested_file);
char *createResponseString(int response, FILE *fp);

int endOfRequest(char const *request);
char *fileToString(FILE *fp);

#endif
