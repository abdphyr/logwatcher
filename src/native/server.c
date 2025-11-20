#include <sys/inotify.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "uthash.h"
#include "base64.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include "query_param.h"
#include <signal.h>
#include <stdint.h>
#include "entries.h"
#include "websocket.h"
#include <stdatomic.h>
#include <pthread.h>
#include <errno.h> // errno, EAGAIN, EWOULDBLOCK

atomic_int stop_flag = 0;
pthread_t loop_thread;

#define MAX_EVENTS 10000
#define BUFFER_SIZE 4096

int epfd = -1;
int listen_fd = -1;
int inotify_fd = -1;

FileEntry *files = NULL;

int make_socket_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int add_listen_ev(int epfd, const char *host, int port)
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),       // listen on port 8080
        .sin_addr.s_addr = INADDR_ANY, // any local interface
    };
    inet_pton(AF_INET, host, &addr.sin_addr);
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind failed");
        exit(1);
    }
    listen(listen_fd, SOMAXCONN);
    make_socket_nonblocking(listen_fd);
    struct epoll_event ev = {.events = EPOLLIN, .data.fd = listen_fd};
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);
    return listen_fd;
}

int add_inotify_ev(int epfd)
{
    int inotify_fd = inotify_init1(IN_NONBLOCK);
    make_socket_nonblocking(inotify_fd);
    struct epoll_event ev = {.events = EPOLLIN, .data.fd = inotify_fd};
    epoll_ctl(epfd, EPOLL_CTL_ADD, inotify_fd, &ev);
    return inotify_fd;
}

void send_http_response(int client_fd, const char *body, const char *content_type)
{
    char response_header[BUFFER_SIZE];
    int body_len = strlen(body);
    snprintf(response_header, sizeof(response_header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n" // Close after response
             "\r\n",
             content_type, body_len);

    full_write(client_fd, response_header, strlen(response_header));
    full_write(client_fd, body, body_len);
}

void handle_http_request(int client_fd, const char *request_buffer)
{
    if (strncmp(request_buffer, "GET / ", 5) == 0)
    {
        send_http_response(client_fd, "<h1>Hello, HTTP World!</h1>", "text/html");
    }
    else
    {
        send_http_response(client_fd, "404 Not Found", "text/plain");
    }
    close(client_fd);
}

void handle_client_request(int client_fd, int inotify_fd)
{
    char buffer[BUFFER_SIZE];
    while (1)
    {
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else
            {
                perror("read");
                close(client_fd);
                return;
            }
        }
        else if (bytes_read == 0)
        {
            close(client_fd);
            return;
        }
        buffer[bytes_read] = '\0';
        // process buffer - may contain partial HTTP or websocket frames
        if (strstr(buffer, "Upgrade: websocket") != NULL)
        {
            handle_websocket_handshake(client_fd, buffer);
            usleep(100 * 1000);
            const char *message = "Hello from server";
            QueryParam *params = parse_query_params(buffer);
            const char *filepath = get_param(params, "filepath");
            if (!filepath || strlen(filepath) == 0)
            {
                printf("No filepath provided\n");
                fflush(stdout);
                return;
            }
            printf("%d-client\n", client_fd);
            fflush(stdout);
            subscribe(&files, inotify_fd, client_fd, filepath);
            send_websocket_message(client_fd, message, strlen(message));
        }
        else
        {
            handle_http_request(client_fd, buffer);
            return;
        }
    }
}

void broadcast_to_subs(FileEntry *file, const char *message)
{
    if (!file)
        return;
    Subscriber *s, *tmp;
    HASH_ITER(hh, file->subs, s, tmp)
    {
        send_websocket_message(s->client_fd, message, strlen(message));
        printf("%s", message);
        fflush(stdout);
    }
}

void handle_inotify_events(int inotify_fd)
{
    char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
    ssize_t len = read(inotify_fd, buf, sizeof(buf));

    if (len <= 0)
        return;

    for (char *ptr = buf; ptr < buf + len;)
    {
        struct inotify_event *event = (struct inotify_event *)ptr;
        ptr += sizeof(struct inotify_event) + event->len;

        FileEntry *file = find_file_by_wd(&files, event->wd);
        if (!file)
            continue;

        // Only handle file modifications
        if (event->mask & IN_MODIFY)
        {
            FILE *fp = fopen(file->path, "r");
            if (!fp)
            {
                perror("fopen");
                continue;
            }
            if (fseek(fp, file->last_pos, SEEK_SET) != 0)
            {
                fclose(fp);
                continue;
            }
            char line[1024];
            while (fgets(line, sizeof(line), fp))
            {
                broadcast_to_subs(file, line);
            }

            file->last_pos = ftell(fp);
            fclose(fp);
        }
        if (event->mask & (IN_DELETE_SELF | IN_MOVE_SELF))
        {
            printf("File deleted/moved: %s\n", file->path);
            broadcast_to_subs(file, "File deleted or moved\n");
        }
    }
}

void handle_new_client(int epfd, int listen_fd)
{
    int client_fd = accept(listen_fd, NULL, NULL);
    make_socket_nonblocking(client_fd);
    struct epoll_event cl_ev = {.events = EPOLLIN | EPOLLET, .data.fd = client_fd};
    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cl_ev);
}

void accept_all(int listen_fd, int epfd)
{
    while (1)
    {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            perror("accept");
            break;
        }
        make_socket_nonblocking(client_fd);
        struct epoll_event cl_ev = {.events = EPOLLIN | EPOLLET, .data.fd = client_fd};
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cl_ev) < 0)
        {
            perror("epoll_ctl add client");
            close(client_fd);
        }
    }
}

void cleanup_and_exit(int sig)
{
    if (listen_fd >= 0)
        close(listen_fd);
    if (inotify_fd >= 0)
        close(inotify_fd);
    if (epfd >= 0)
        close(epfd);

    FileEntry *f, *ftmp;
    HASH_ITER(hh, files, f, ftmp)
    {
        Subscriber *s, *stmp;
        HASH_ITER(hh, f->subs, s, stmp)
        {
            HASH_DEL(f->subs, s);
            close(s->client_fd);
            free(s);
        }
        HASH_DEL(files, f);
        free(f);
    }
    exit(0);
}

typedef struct
{
    int port;
    const char *host;
} loop_args_t;

void *main_loop(void *arg)
{
    loop_args_t *args = (loop_args_t *)arg;
    int port = args->port;
    const char *host = args->host;
    free(args);

    epfd = epoll_create1(0);
    if (epfd < 0)
    {
        perror("epoll_create1");
        return NULL;
    }
    listen_fd = add_listen_ev(epfd, host, port);
    inotify_fd = add_inotify_ev(epfd);
    struct epoll_event events[MAX_EVENTS];

    while (!atomic_load(&stop_flag))
    {
        int n = epoll_wait(epfd, events, MAX_EVENTS, 500);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            break;
        }
        for (int i = 0; i < n; ++i)
        {
            if (events[i].data.fd == inotify_fd)
                handle_inotify_events(inotify_fd);
            else if (events[i].data.fd == listen_fd)
                accept_all(listen_fd, epfd);
            else
                handle_client_request(events[i].data.fd, inotify_fd);
        }
    }
    cleanup_and_exit(0); // careful: modify cleanup to not call exit() inside thread
    return NULL;
}

int start_server(const char *host, int port)
{
    atomic_store(&stop_flag, 0);
    loop_thread = (pthread_t){0};
    loop_args_t *args = malloc(sizeof(loop_args_t));
    args->port = port;
    args->host = host;

    int rc = pthread_create(&loop_thread, NULL, main_loop, args);
    if (rc != 0)
    {
        errno = rc;
        free(args);
        return -1;
    }
    return 0;
}

int stop_server(void)
{
    atomic_store(&stop_flag, 1);
    pthread_join(loop_thread, NULL);
    return 0;
}