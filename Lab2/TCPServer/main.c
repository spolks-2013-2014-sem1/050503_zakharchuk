/* A simple server in the internet domain using TCP
   The port number is passed as an argument */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int listenfd, connfd, portno;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;

     if (argc < 2)
     {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

     listenfd = socket(AF_INET, SOCK_STREAM, 0);
     if (listenfd < 0)
        error("ERROR opening socket");

     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;            // a code for the address family
     serv_addr.sin_addr.s_addr = INADDR_ANY;    // IP address of the host
     serv_addr.sin_port = htons(portno);        // converts a port number in host byte order
                                                // to a port number in network byte order

     if (bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

     listen(listenfd,5);
     clilen = sizeof(cli_addr);

     char key = 'o';
     while(key != 'q')
     {
         connfd = accept(listenfd, (struct sockaddr *) &cli_addr, &clilen);
         if (connfd < 0)
              error("ERROR on accept");

         while(1)
         {
             bzero(buffer,256);
             n = read(connfd,buffer,255);
             if (n < 0)
                 error("ERROR reading from socket");

             if(strlen(buffer) == 2 && buffer[0] == 'q')
             {
                 key = buffer[0];
                 break;
             }
             printf("Here is the message:\n%s\n",buffer);

             char echo[256] = "Server has got your message:\n";
             strncat(echo, buffer, strlen(echo)+strlen(buffer));
             n = write(connfd, echo, sizeof(echo));
             if (n < 0)
                 error("ERROR writing to socket");
         }

         close(connfd);
     }

     close(listenfd);

     return 0;
}
