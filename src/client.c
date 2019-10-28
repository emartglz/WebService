#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>



#define BUFFERSIZE 8192

void print(char* text, int client_fd)
{
    write(client_fd, text, strlen(text));
}

char* get_type(struct dirent *entry)
{
    if(entry->d_type == 4)
    {
        return "DIR";
    }
    return "FILE";
}

void entry_html(char *directory, struct dirent *entry, int client_fd)
{
    char dir[strlen(directory)];
    strcpy(dir, directory);
    char name[strlen(entry->d_name)];
    strcpy(name, entry->d_name);

    char* type = get_type(entry);

    printf("%s. %s. %s.\n", type, dir, name);

    print("<tr>\n				<td>[", client_fd);
    print(type, client_fd);
    print("]</td>\n				<td><a href=\"", client_fd);
    print(dir, client_fd);
    print(name, client_fd);
    if(entry->d_type == 4)
        print("/", client_fd);
    print("\">", client_fd);
    print(name, client_fd);
    if(entry->d_type == 4)
        print("/", client_fd);
    print("</a></td>\n			</tr>", client_fd);
}

void init_html(char *directory, int client_fd)
{
    print("HTTP/1.0 \r\n\n", client_fd);
    print("<html>\n    <head>\n        <title>", client_fd);
    print(directory, client_fd);
    print("</title>\n	</head>\n	<body>\n		<h1>Content of directory \"", client_fd);
    print(directory, client_fd);
    print("\"</h1>\n\n		<table>\n			<tr>\n				<th>Type</th>\n				<th>Filename</th>\n			</tr>", client_fd);
    print("<tr>\n				<td>[dir]</td>\n				<td><a href=\"", client_fd);
    print(directory, client_fd);
    print("../\">BACK/</a></td>\n			</tr>", client_fd);
}

void end_html(int client_fd)
{
    print("	</body>\n</html>", client_fd);
}

void client_dir(char* directory, int client_fd)
{
    struct DIR *dir = opendir(directory); 
    printf("--> Open %s succefuly\n", directory);
    struct dirent *entry;
    
    init_html(directory, client_fd);
    printf("--> init\n");
    while((entry = readdir(dir)) != NULL)
    {
        if(entry->d_name[0] == '.')
        {
            continue;
        }
        entry_html(directory, entry, client_fd);
    }
    end_html(client_fd);
}

void client_file(char* file, int client_fd)
{
    int file_fd = open(file, O_RDONLY);
    struct stat s;
    fstat(file_fd, &s);
    

    print("HTTP/1.0 \r\n", client_fd);
    print("Content Type:text/plain \r\n", client_fd);
    char temp[BUFFERSIZE];
    sprintf(temp, "Content-length: %ld \r\n", s.st_size);
    print(temp, client_fd);
    print("Content-Disposition: attachment;\r\n\r\n", client_fd);

    while(sendfile(client_fd, file_fd, NULL, 128))
    {
    }
    close(file_fd);
}