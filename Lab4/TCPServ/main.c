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
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>

#define replyBufSize 256
#define bufSize 4096

void receiveFile(char *hostName, unsigned int portNumber);
void sendFile(char *serverName, unsigned int port, char *filePath);

int serverSocketDescriptor = -1;

void intHandler(int signo)
{
    if (serverSocketDescriptor != -1)
    {
        close(serverSocketDescriptor);
        fprintf(stdout,"Server's descriptor has been closed\n");
    }

    fflush(stdout);
    _exit(0);
}
int oobFlag = 0;
void urgHandler(int signo)
{
    oobFlag = 1;
}


int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: main <host> <port>\n");
        return 1;
    }

    // Change SIGINT action
    struct sigaction intSignal;
    intSignal.sa_handler = intHandler;
    sigaction(SIGINT, &intSignal, NULL);

    receiveFile(argv[1], atoi(argv[2]));

    return 0;
}

void receiveFile(char *hostName, unsigned int portNumber)
{
    struct sockaddr_in sin;

    if ((serverSocketDescriptor = createTcpServerSocket(hostName, portNumber, &sin, 5)) == -1)
    {
        printf("Creation socket error\n");
        exit(EXIT_FAILURE);
    }

    char replyBuf[replyBufSize], buf[bufSize];

    while (1)
    {
        printf("\nAwaiting connections...\n");
        int remoteSocketDescriptor = accept(serverSocketDescriptor, NULL, NULL);

        struct sigaction urgSignal;
        urgSignal.sa_handler = urgHandler;
        sigaction(SIGURG, &urgSignal, NULL);

        //Set owner
        if (fcntl(remoteSocketDescriptor, F_SETOWN, getpid()) < 0)
        {
            perror("Error setting owner");
            exit(-1);
        }

        // Receive file name and file size
        if (ReceiveToBuf(remoteSocketDescriptor, replyBuf, sizeof(replyBuf)) <= 0) 
        {
            close(remoteSocketDescriptor);
            fprintf(stderr, "Error receiving file name and file size\n");
            continue;
        }
        char *fileName = strtok(replyBuf, ":");
        if (fileName == NULL)
        {
            close(remoteSocketDescriptor);
            fprintf(stderr, "Bad file name\n");
            continue;
        }
        char *size = strtok(NULL, ":");
        if (size == NULL)
        {
            close(remoteSocketDescriptor);
            fprintf(stderr, "Bad file size\n");
            continue;
        }
        long fileSize = atoi(size);

        printf("File size: %ld, file name: %s\n", fileSize, fileName);

        FILE *file = CreateReceiveFile(fileName, "Received_files");
        if (file == NULL)
        {
            perror("Create file error");
            exit(EXIT_FAILURE);
        }

        // Receiving file    
        long totalBytesReceived = 0;
        int recvSize;

        while (totalBytesReceived < fileSize)
        {
            if (sockatmark(remoteSocketDescriptor) == 1 && oobFlag == 1)
            {
                printf("Receive OOB byte. Total bytes received: %ld\n", totalBytesReceived);

                char oobBuf;
                int n = recv(remoteSocketDescriptor, &oobBuf, 1, MSG_OOB);
                if (n == -1)
                    fprintf(stderr, "receive OOB error\n");
                oobFlag = 0;
            }

            recvSize = recv(remoteSocketDescriptor, buf, sizeof(buf), 0);
            if (recvSize > 0)
            {
                fwrite(buf, 1, recvSize, file);
                totalBytesReceived += recvSize;
            } else if (recvSize == 0)
            {
                printf("Received EOF\n");
                break;
            } else
            {
                if (errno == EINTR)
                    continue;
                else
                {
                    perror("receive error");
                    break;
                }
            }
        }

        fclose(file);
        printf("Receiving file completed. %ld bytes received.\n", totalBytesReceived);
        close(remoteSocketDescriptor);
    }
}
