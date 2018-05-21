/* inet_str_client.c: Internet stream sockets client */
#include <stdio.h>
#include <sys/types.h>	     /* sockets */
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <unistd.h>          /* read, write, close */
#include <netdb.h>	         /* gethostbyaddr */
#include <stdlib.h>	         /* exit */
#include <string.h>	         /* strlen */

void perror_exit(char *message);

void main(int argc, char *argv[]) {
    int             port, sock, i;
    char            buf[256];

    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*)&server;
    struct hostent *rem;

    if (argc != 3) {
    	printf("Please give host name and port number\n");
       	exit(1);}
	/* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	perror_exit("socket");
	/* Find server address */
    if ((rem = gethostbyname(argv[1])) == NULL) {	
	   herror("gethostbyname"); exit(1);
    }
    port = atoi(argv[2]); /*Convert port number to integer*/
    server.sin_family = AF_INET;       /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(port);         /* Server port */
    /* Initiate connection */
    if (connect(sock, serverptr, sizeof(server)) < 0)
	   perror_exit("connect");
    printf("Connecting to %s port %d\n", argv[1], port);
	printf("Type a command: ");
    fgets(buf, sizeof(buf), stdin);	// Read from stdin
	if (write(sock, buf, sizeof(buf)) < 0)
    	perror_exit("write");
    close(sock);                 /* Close socket and exit */
}			     

void perror_exit(char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}
