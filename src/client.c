#include <unistd.h>
#include <stdio.h>
#include <string.h>

int client(char* directory, int client_fd)
{
    printf("FD: %d\n", client_fd);
    char* text = "HTTP/1.0 \r\n\n TIZA\n";
    
    write(client_fd, text, strlen(text));
}