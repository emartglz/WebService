#ifndef H_CLIENT
#define H_CLIENT

void client_dir(char* directory, int client_fd, int origin_path, int sort);
void client_file(char* file, int client_fd);

#endif // H_CLIENT