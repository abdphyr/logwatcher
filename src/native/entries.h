#include "uthash.h"
#include <sys/inotify.h>
#include <stdio.h>

typedef struct Subscriber
{
    int client_fd;
    UT_hash_handle hh;
} Subscriber;

typedef struct FileEntry
{
    char path[256];
    Subscriber *subs;
    size_t wd;
    int last_pos;
    UT_hash_handle hh;
} FileEntry;

FileEntry *add_file(FileEntry **files, const char *path, size_t wd);
FileEntry *find_file(FileEntry **files, const char *path);
FileEntry *find_file_by_wd(FileEntry **files, int wd);
void remove_file(FileEntry **files, const char *path);
Subscriber *add_subscriber(FileEntry *file, int client_fd);
void remove_subscriber(FileEntry *file, int client_fd);
void subscribe(FileEntry **files, int inotify_fd, int client_fd, const char *filepath);