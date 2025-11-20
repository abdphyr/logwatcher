#include "websocket.h"

char *generate_accept_key(const char *sec_key_string)
{
    const char *guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char concatenated_key[256];
    snprintf(concatenated_key, sizeof(concatenated_key), "%s%s", sec_key_string, guid);
    unsigned char sha1_digest[SHA_DIGEST_LENGTH]; // 20 bytes
    SHA1((unsigned char *)concatenated_key, strlen(concatenated_key), sha1_digest);
    size_t encoded_len;
    char *base64_encoded_string = base64_encode(sha1_digest, SHA_DIGEST_LENGTH, &encoded_len);
    return base64_encoded_string;
}

char *get_sec_websocket_key_string(const char *request_buffer)
{
    char *key_start = strstr(request_buffer, "Sec-WebSocket-Key: ");
    if (!key_start)
        return NULL;

    key_start += strlen("Sec-WebSocket-Key: ");
    char *key_end = strstr(key_start, "\r\n");
    if (!key_end)
        return NULL;
    int key_len = key_end - key_start;
    char *key = malloc(key_len + 1);
    strncpy(key, key_start, key_len);
    key[key_len] = '\0';
    return key;
}

void handle_websocket_handshake(int client_fd, const char *request_buffer)
{
    char *sec_key_string = get_sec_websocket_key_string(request_buffer);
    if (!sec_key_string)
    {
        return;
    }
    char *accept_key = generate_accept_key(sec_key_string);
    char response_header[512];
    sprintf(response_header,
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: %s\r\n"
            "\r\n",
            accept_key);

    full_write(client_fd, response_header, strlen(response_header));
    free(sec_key_string);
    free(accept_key);
}

void send_websocket_message(int client_fd, const char *message, size_t message_len)
{
    uint8_t header[10];
    size_t header_len = 0;

    header[0] = 0x81; // FIN bit + text frame opcode (0x1)

    if (message_len <= 125)
    {
        header[1] = (uint8_t)message_len;
        header_len = 2;
    }
    else if (message_len <= 0xFFFF)
    {
        header[1] = 126;
        header[2] = (message_len >> 8) & 0xFF;
        header[3] = message_len & 0xFF;
        header_len = 4;
    }
    else
    {
        header[1] = 127;
        for (int i = 0; i < 8; i++)
        {
            header[2 + i] = (message_len >> (56 - 8 * i)) & 0xFF;
        }
        header_len = 10;
    }

    size_t frame_size = header_len + message_len;
    uint8_t *frame = malloc(frame_size);
    if (!frame)
        return;

    memcpy(frame, header, header_len);
    memcpy(frame + header_len, message, message_len);

    ssize_t sent = full_write(client_fd, frame, frame_size);
    if (sent < 0)
    {
        perror("write websocket");
    }
    free(frame);
}

ssize_t full_write(int fd, const void *buf, size_t count) {
    size_t sent = 0;
    const char *p = buf;
    while (sent < count) {
        ssize_t r = write(fd, p + sent, count - sent);
        if (r < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return sent; // try later
            return -1;
        }
        sent += r;
    }
    return sent;
}