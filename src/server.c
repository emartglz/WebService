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
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include "../include/client.h"

#define BUFFER 10000

int files_fd[100];
int client_files_fd[100];
int clients[100];
int files_bytes_sended[100];
struct stat files_stat[100];
char serving_directory[BUFFER];
char path[BUFFER];
char serving_directory_temp[BUFFER];
int max = 0;
fd_set clients_fd;
fd_set ready_clients_fd;


void get_directory_path(char* request, char* path)
{
    char* path_init = strchr(request, ' ');

    int pos = 0;
    int i = 1;
    while(path_init[i] != ' ')
    {
        if(path_init[i] == '%')
        {
            if(path_init[i + 1] == '2')
            {
                if(path_init[i + 2] == '0')
                {
                    path[pos] = ' ';
                    i += 2;
                }
            }
        }
        else
        {
            path[pos] = path_init[i];
        }
        pos++;
        i++;
    }
    path[pos] = '\000';
    printf("------------+-->%s\n", path);
}

void comprobate_path(char *new_path, char *origin_path)
{
    if(strlen(new_path) < strlen(origin_path))
    {
        strcpy(new_path, origin_path);
        return;
    }
    for(int i = 0; i < strlen(origin_path); i++)
    {
        if(new_path[i] != origin_path[i])
        {
            strcpy(new_path, origin_path);
            return;
        }
    }
}

int get_sort_from_path(char* path)
{
    if(path[strlen(path) - 7] == '?')
        if(path[strlen(path) - 6] == 's')
            if(path[strlen(path) - 5] == 'o')
                if(path[strlen(path) - 4] == 'r')
                    if(path[strlen(path) - 3] == 't')
                        if(path[strlen(path) - 2] == '=')
                        {
                            char s[1];
                            s[0] = path[strlen(path) - 1];
                            path[strlen(path) - 7] = '\000';
                            return atoi(s);
                        }
    return 0;
}

void check_clients()
{
    for(int i = 0; i < 100; i++)
    {
        if(clients[i] != -1)
        {
            if(FD_ISSET(clients[i], &ready_clients_fd))
            {
                char buffer[BUFFER];
                printf("-->waiting for request\n");
                int readcount = read(clients[i], buffer, BUFFER);
                printf("--> recibing request\n");

                get_directory_path(buffer, path);
                strcpy(serving_directory_temp, path);
                if(strcmp(serving_directory_temp, "/favicon.ico\0") == 0)
                {
                    FD_CLR(clients[i], &clients_fd);
                    close(clients[i]);
                    clients[i] = -1;
                    continue;
                }

                int sort = get_sort_from_path(serving_directory_temp);
                comprobate_path(serving_directory_temp, serving_directory);

                if(serving_directory_temp[strlen(serving_directory_temp) - 1] == '/')
                {
                    printf("-->Dir petition\n");
                    client_dir(serving_directory_temp, clients[i], strcmp(serving_directory_temp, serving_directory), sort);
                    
                    FD_CLR(clients[i], &clients_fd);
                    close(clients[i]);
                    clients[i] = -1;
                }
                else
                {
                    printf("-->File petition\n");
                    int file_fd = open(serving_directory_temp, O_RDONLY);
                    printf("--File opened\n");
                    if(file_fd > max)
                    {
                        max = file_fd;
                    }

                    struct stat s;
                    fstat(file_fd, &s);

                    for(int j = 0; j < 100; j++)
                    {
                        if(files_fd[j] == -1)
                        {
                            files_fd[j] = file_fd;
                            client_files_fd[j] = i;
                            files_bytes_sended[j] = 0;
                            files_stat[j] = s;
                            break;
                        }
                    }

                    FD_SET(file_fd, &clients_fd);
                    client_file(serving_directory_temp, clients[i]);
                }
            }
        }
    }
}

int server(int argc, char **argv)
{
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    fd_set read_set;
    fd_set write_set;

    //CHECK COMMAND LINE ARGS
    if(argc != 3)
    {
        fprintf(stderr, "usage %s <port> at %s \n", "argv[1", argv[2]);
        exit(1);
    }
    int portno = atoi(argv[1]);
    printf("Listening in port %d\n", portno);
    strcpy (serving_directory, argv[2]);
    printf("Serving directory \"%s\"\n", serving_directory);


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

    if(listen(listefd, 10) < 0)
    {
        printf("ERROR on listen");
    }

    int client_len = sizeof(client_addr);

    int connfd = 0;

    FD_ZERO(&clients_fd);
    FD_ZERO(&ready_clients_fd);
    FD_SET(listefd, &clients_fd);
    
    printf("-->Starting files fd\n");
    for(int i = 0; i < 100; i++)
    {
        clients[i] = -1;
        files_fd[i] = -1;
        client_files_fd[i] = -1;
    }

    max = listefd;

    while(1)
    {
        printf("-->loop\n");
        ready_clients_fd = clients_fd;
        select(max + 1, &ready_clients_fd, NULL, NULL, NULL);
        printf("-->loop2\n");
        if(FD_ISSET(listefd, &ready_clients_fd))
        {
            printf("-->waiting\n");
            connfd = accept(listefd, (struct sockaddr *)&client_addr, &client_len);
            if(connfd > max)
            {
                max = connfd;
            }
            write(STDOUT_FILENO, "accept\n", 7);
            if(connfd < 0)
            {
                printf("ERROR on accept");
                return -1;
            }
            printf("--> Connection established with: %s.\n", inet_ntoa(client_addr.sin_addr));

            for(int i = 0; i < 100; i++)
            {
                if(clients[i] == -1)
                {
                    printf("-->Adding client in %d\n", connfd);
                    clients[i] = connfd;
                    FD_SET(connfd, &clients_fd);
                    break;
                }
            }
        }

        check_clients();

        printf("--aquiiiiii\n");

        for(int i = 0; i < 100; i++)
        {
            if(files_fd[i] != -1)
            {
                if(FD_ISSET(files_fd[i], &ready_clients_fd))
                {
                    printf("-->Sending %d to %d\n", files_fd[i], clients[client_files_fd[i]]);
                    int sended = sendfile(clients[client_files_fd[i]], files_fd[i], NULL, files_stat[i].st_blksize);
                    if(sended == -1)
                    {
                        printf("-->Closing %d\n", files_fd[i]);
                        FD_CLR(files_fd[i], &clients_fd);
                        FD_CLR(clients[client_files_fd[i]], &clients_fd);
                        close(clients[client_files_fd[i]]);
                        close(files_fd[i]);
                        files_fd[i] = -1;
                        clients[client_files_fd[i]] = -1;
                        client_files_fd[i] = -1;
                        files_bytes_sended[i] = 0;
                    }
                    files_bytes_sended[i] += sended;
                }
            }
        }
        for(int i = 0; i < 100; i++)
        {
            if(files_fd[i] != -1)
            {
                if(files_bytes_sended[i] == files_stat[i].st_size)
                {
                    printf("-->Closing %d\n", files_fd[i]);
                    FD_CLR(files_fd[i], &clients_fd);
                    FD_CLR(clients[client_files_fd[i]], &clients_fd);
                    close(clients[client_files_fd[i]]);
                    close(files_fd[i]);
                    files_fd[i] = -1;
                    client_files_fd[i] = -1;
                    clients[client_files_fd[i]] = -1;
                    files_bytes_sended[i] = 0;
                }
            }
        }
    }
    
    close(listefd);
    printf("--> Socket closed.\n");
}