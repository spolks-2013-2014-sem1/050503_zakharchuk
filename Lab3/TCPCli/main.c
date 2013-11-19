#include "../../spolks_lib/sockets.c"
#include "../../spolks_lib/helpers.c"

#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <libgen.h>

#define replyBufSize 256
#define bufSize 4096

void sendFile(char *serverName, unsigned int port, char *filePath);

int clientSocketDescriptor = -1;

void intHandler(int signo)
{
    if (clientSocketDescriptor != -1)
        close(clientSocketDescriptor);

    _exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr,"usage: main <host> <port> <filePat>\n");
        return 1;
    }

    // Change SIGINT action
    struct sigaction intSignal;
    intSignal.sa_handler = intHandler;
    sigaction(SIGINT, &intSignal, NULL);

    sendFile(argv[1], atoi(argv[2]), argv[3]);

    return 0;
}
void sendFile(char *hostName, unsigned int portNumber, char *filePath)
{
    struct sockaddr_in serverAddress;

    if ((clientSocketDescriptor = createTcpSocket(hostName, portNumber, &serverAddress)) == -1)
    {
        fprintf(stderr, "Creation socket error\n");
        exit(EXIT_FAILURE);
    }

    if (connect(clientSocketDescriptor, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(filePath, "r+");
    if (file == NULL)
    {
        perror("Open file error");
        exit(EXIT_FAILURE);
    }

    long fileSize = GetFileSize(file);

    char fileName[replyBufSize];
    sprintf(fileName, "%s:%ld", basename(filePath), fileSize);

    // Send file name and file size
    if (send(clientSocketDescriptor, fileName, sizeof(fileName), 0) == -1)
    {
        perror("Send error");
        exit(EXIT_FAILURE);
    }

    char buf[bufSize];
    long totalBytesSent = 0;
    size_t readBytes;

    // Sending file
    while (totalBytesSent < fileSize)
    {
        readBytes = fread(buf, 1, sizeof(buf), file);
        int sentBytes = send(clientSocketDescriptor, buf, readBytes, 0);
        if (sentBytes < 0)
        {
            perror("Sending error");
            exit(EXIT_FAILURE);
        }
        totalBytesSent += sentBytes;
    }
    printf("Sending file completed\n");

    close(clientSocketDescriptor);
    fclose(file);
}
