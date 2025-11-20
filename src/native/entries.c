#include "entries.h"

FileEntry *add_file(FileEntry **files, const char *path, size_t wd)
{
    FileEntry *f = NULL;
    HASH_FIND_STR(*files, path, f);
    if (f)
    {
        return f;
    }
    f = malloc(sizeof(FileEntry));
    strncpy(f->path, path, sizeof(f->path));
    f->subs = NULL;
    f->wd = wd;
    f->last_pos = 0;
    HASH_ADD_STR(*files, path, f);
    return f;
}

FileEntry *find_file(FileEntry **files, const char *path)
{
    FileEntry *f = NULL;
    HASH_FIND_STR(*files, path, f);
    return f;
}

FileEntry *find_file_by_wd(FileEntry **files, int wd)
{
    FileEntry *f, *tmp;
    HASH_ITER(hh, *files, f, tmp)
    {
        if ((int)f->wd == wd)
        {
            return f;
        }
    }
    return NULL;
}

void remove_file(FileEntry **files, const char *path)
{
    FileEntry *f = NULL;
    HASH_FIND_STR(*files, path, f);
    if (!f)
    {
        return;
    }
    Subscriber *s, *tmp;
    HASH_ITER(hh, f->subs, s, tmp)
    {
        HASH_DEL(f->subs, s);
        free(s);
    }
    HASH_DEL(*files, f);
    free(f);
}

Subscriber *add_subscriber(FileEntry *file, int client_fd)
{
    if (!file)
    {
        return NULL;
    }
    Subscriber *s = NULL;
    HASH_FIND_INT(file->subs, &client_fd, s);
    if (s)
    {
        return s;
    }
    s = malloc(sizeof(Subscriber));
    s->client_fd = client_fd;
    HASH_ADD_INT(file->subs, client_fd, s);
    return s;
}

void remove_subscriber(FileEntry *file, int client_fd)
{
    if (!file)
    {
        return;
    }
    Subscriber *s = NULL;
    HASH_FIND_INT(file->subs, &client_fd, s);
    if (s)
    {
        HASH_DEL(file->subs, s);
        free(s);
    }
}

void subscribe(FileEntry **files, int inotify_fd, int client_fd, const char *filepath)
{
    int wd = inotify_add_watch(inotify_fd, filepath, IN_MODIFY | IN_DELETE);
    if (wd < 0)
    {
        perror("inotify_add_watch_error");
        return;
    }
    FileEntry *file = add_file(files, filepath, wd);
    FILE *fp = fopen(filepath, "r");
    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        file->last_pos = ftell(fp);
        fclose(fp);
    }
    add_subscriber(file, client_fd);
}