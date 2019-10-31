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
#include <time.h>

#define BUFFERSIZE 8192


struct entry_stat
{
    struct dirent *entry;
    struct stat s;
};


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

long int get_size(char* size_s, unsigned int size)
{
    strcpy(size_s, "Bytes");
    unsigned int temp = size;
    if(temp / 1024 > 0)
    {
        temp = temp / 1024;
        strcpy(size_s, "KB");
    }
    if(temp / 1024 > 0)
    {
        temp = temp / 1024;
        strcpy(size_s, "MB");
    }
    if(temp / 1024 > 0)
    {
        temp = temp / 1024;
        strcpy(size_s, "GB");
    }
    return temp;
}

void entry_html(char *directory, struct entry_stat entry, int client_fd)
{
    char dir[strlen(directory)];
    strcpy(dir, directory);
    char name[strlen(entry.entry->d_name)];
    strcpy(name, entry.entry->d_name);

    char* type = get_type(entry.entry);

    print("<tr>\n				<td>[", client_fd);
    print(type, client_fd);
    print("]</td>\n				<td><a href=\"", client_fd);
    print(dir, client_fd);
    print(name, client_fd);
    if(entry.entry->d_type == 4)
        print("/", client_fd);
    print("\">", client_fd);
    print(name, client_fd);
    if(entry.entry->d_type == 4)
        print("/", client_fd);
    print("</a></td>\n", client_fd);

    char date[BUFFERSIZE];
    dprintf(client_fd, "<td>\n%s\n</td>\n",  ctime(&entry.s.st_mtime));

    char size_s[5];
    dprintf(client_fd, "<td>\n%ld%s\n</td>\n", get_size(size_s, entry.s.st_size), size_s);

    char perm[11];
    int index = 0;
    mode_t per = entry.s.st_mode;
    
    perm[index++] = S_ISDIR(per) ? 'd' : '-'; 
    perm[index++] = per & S_IRUSR ? 'r' : '-';
    perm[index++] = per & S_IWUSR ? 'w' : '-';
    perm[index++] = per & S_IXUSR ? 'x' : '-';
    perm[index++] = per & S_IRGRP ? 'r' : '-';
    perm[index++] = per & S_IWGRP ? 'w' : '-';
    perm[index++] = per & S_IXGRP ? 'x' : '-';
    perm[index++] = per & S_IROTH ? 'r' : '-';
    perm[index++] = per & S_IWOTH ? 'w' : '-';
    perm[index++] = per & S_IXOTH ? 'x' : '-';
    perm[index] = '\000';
    dprintf(client_fd, "<td>\n%s\n</td>\n", perm);

    print("</tr>\n", client_fd);
}

void init_html(char *directory, int client_fd, int origin_path)
{
    dprintf(client_fd, "HTTP/1.0 \r\n");
    dprintf(client_fd, "<html>\n    <head>\n        <title>");
    dprintf(client_fd, "%s", directory);
    dprintf(client_fd, "</title>\n	</head>\n	<body>\n		<h1>Content of directory \"%s\"</h1>\n\n		<table border=\"1px\">\n			<tr>\n				", directory);

    dprintf(client_fd, "<th><a href=\"%s?sort=1\">Type</a></th>\n", directory);

    dprintf(client_fd, "<th><a href=\"%s?sort=2\">FileName</a></th>\n", directory);

    dprintf(client_fd, "<th><a href=\"%s?sort=3\">ModificationDate</a></th>\n", directory);

    dprintf(client_fd, "<th><a href=\"%s?sort=4\">Size</a></th>\n", directory);

    dprintf(client_fd, "<th><a href=\"%s?sort=5\">Permissions</a></th>\n", directory);

    print("</tr>", client_fd);

    if(origin_path != 0)
    {
        dprintf(client_fd, "<tr>\n				<td>[DIR]</td>\n				<td><a href=\"%s../\">BACK/</a></td>\n			<td></td><td></td><td></td></tr>", directory);
    }
}

void end_html(int client_fd)
{
    print("	</body>\n</html>", client_fd);
}

static int sort_alphabetical(const void *p1, const void *p2)
{
    struct entry_stat a = *(struct entry_stat *) p1;
    struct entry_stat b = *(struct entry_stat *) p2;
    
    return strcmp(a.entry->d_name, b.entry->d_name);
}

static int sort_type(const void *p1, const void *p2)
{
    struct entry_stat a = *(struct entry_stat *) p1;
    struct entry_stat b = *(struct entry_stat *) p2;
    
    return a.entry->d_type - b.entry->d_type;
}

static int sort_modification_date(const void *p1, const void *p2)
{
    struct entry_stat a = *(struct entry_stat *) p1;
    struct entry_stat b = *(struct entry_stat *) p2;
    
    return a.s.st_ctime - b.s.st_ctime;
}

static int sort_size(const void *p1, const void *p2)
{
    struct entry_stat a = *(struct entry_stat *) p1;
    struct entry_stat b = *(struct entry_stat *) p2;
    
    if(a.s.st_size < b.s.st_size)
        return -1;
    if(a.s.st_size == b.s.st_size)
        return 0;
    else 
        return 1;
}

static int sort_permisions(const void *p1, const void *p2)
{
    struct entry_stat a = *(struct entry_stat *) p1;
    struct entry_stat b = *(struct entry_stat *) p2;
    
    return a.s.st_mode - b.s.st_mode;
}


void sort_entrys(struct entry_stat *entrys, int cant, int sort_by)
{
    if(sort_by == 0)
        return;
    if(sort_by == 1)
        qsort(entrys, cant, sizeof(struct entry_stat), sort_type);
    if(sort_by == 2)
        qsort(entrys, cant, sizeof(struct entry_stat), sort_alphabetical); 
    if(sort_by == 3)
        qsort(entrys, cant, sizeof(struct entry_stat), sort_modification_date); 
    if(sort_by == 4)
        qsort(entrys, cant, sizeof(struct entry_stat), sort_size);
    if(sort_by == 5)
        qsort(entrys, cant, sizeof(struct entry_stat), sort_permisions); 
}

void client_dir(char* directory, int client_fd, int origin_path, int sort)
{
    struct DIR *dir = opendir(directory); 
    printf("--> Open %s succefuly\n", directory);

    struct dirent *entry;
    struct entry_stat entrys[BUFFERSIZE];
    
    int cant = 0;
    while((entry = readdir(dir)) != NULL)
    {
        if(entry->d_name[0] == '.')
        {
            continue;
        }

        char entry_path[BUFFERSIZE];
        strcpy(entry_path, directory);

        int file_fd = open(strcat(entry_path, entry->d_name), O_RDONLY);
        struct stat s;
        fstat(file_fd, &s);

        entrys[cant].entry = entry;
        entrys[cant].s = s;
        cant++;
    }
    
    sort_entrys(entrys, cant, sort);

    init_html(directory, client_fd, origin_path);
    for(int i = 0; i < cant; i++)
    {
        entry_html(directory, entrys[i], client_fd);
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
    dprintf(client_fd, "Content-length: %ld \r\n", s.st_size);
    print("Content-Disposition: attachment;\r\n\r\n", client_fd);
}