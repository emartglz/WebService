#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <stdlib.h>
#include "../include/client.h"


int server(int argc, char **argv)
{
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    char* serving_directory = malloc(100 * sizeof(char));


    fd_set read_set;
    fd_set write_set;

    //CHECK COMMAND LINE ARGS
    if(argc != 3)
    {
        fprintf(stderr, "usage %s <port> at %s \n", "argv[1", argv[2]);
        exit(1);
    }
    int portno = atoi(argv[1]);
    strcpy (serving_directory, argv[2]);


    int listefd = socket(AF_INET, SOCK_STREAM, 0);
    if(listefd < 0)
    {
        printf("ERROR opening socket");
    }


    int optval = 1;
    setsockopt(listefd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));


    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)portno);


    if(bind(listefd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("ERROR on binding");
    }

    if(listen(listefd, 5) < 0)
    {
        printf("ERROR on listen");
    }

    int client_len = sizeof(client_addr);
    pthread_t client_threads[5];


    int connfd = 0;
    while(1)
    {
        printf("--> Waiting for conection...\n");
        connfd = accept(listefd, (struct sockaddr *)&client_addr, &client_len);
        write(STDOUT_FILENO, "accept\n", 7);
        if(connfd < 0)
        {
            printf("ERROR on accept");
            return -1;
        }

        printf("--> Connection established with: %s.\n", inet_ntoa(client_addr.sin_addr));

        char buffer[4096];
        int readcount = read(connfd, buffer, 4096);
        printf("%s", buffer);

        client(serving_directory, connfd);    
        close(connfd);
    }
    close(connfd);
    printf("--> Socket closed.\n");
}