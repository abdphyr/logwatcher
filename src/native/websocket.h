#include <openssl/sha.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <errno.h>
#include "base64.h"


char *generate_accept_key(const char *sec_key_string);
char *get_sec_websocket_key_string(const char *request_buffer);
void handle_websocket_handshake(int client_fd, const char *request_buffer);
void send_websocket_message(int client_fd, const char *message, size_t message_len);
ssize_t full_write(int fd, const void *buf, size_t count);