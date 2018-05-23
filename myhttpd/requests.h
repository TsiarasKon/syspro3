#ifndef MYHTTPD_REQUESTS_H
#define MYHTTPD_REQUESTS_H

#define HTTP_OK 200
#define HTTP_BADREQUEST 400
#define HTTP_FORBIDDEN 403
#define HTTP_NOTFOUND 404

int validateGETRequest(char *request, char **requested_file, char **hostname);
char *createResponseString(int response, FILE *fp);

#endif
