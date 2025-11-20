#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "uthash.h"
#include "query_param.h"

void url_decode(char *dst, const char *src)
{
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            a = (a <= '9') ? a - '0' : tolower(a) - 'a' + 10;
            b = (b <= '9') ? b - '0' : tolower(b) - 'a' + 10;
            *dst++ = (char)(16 * a + b);
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// -------------------- PARSER --------------------
QueryParam *parse_query_params(const char *request_buffer)
{
    char method[8], path[512];
    QueryParam *params = NULL;

    // Parse method and path from the first line
    if (sscanf(request_buffer, "%7s %511s", method, path) != 2)
        return NULL;

    char *query = strchr(path, '?');
    if (!query)
        return NULL;

    *query++ = '\0'; // terminate path

    char *query_copy = strdup(query);
    if (!query_copy) return NULL;

    char *token = strtok(query_copy, "&");
    while (token) {
        char *eq = strchr(token, '=');
        if (eq) {
            *eq++ = '\0';

            QueryParam *p = malloc(sizeof(QueryParam));
            if (!p) break;

            url_decode(p->key, token);
            url_decode(p->value, eq);

            HASH_ADD_STR(params, key, p);
        }
        token = strtok(NULL, "&");
    }

    free(query_copy);
    return params;
}

// -------------------- LOOKUP --------------------
const char *get_param(QueryParam *params, const char *key)
{
    QueryParam *p = NULL;
    HASH_FIND_STR(params, key, p);
    return p ? p->value : NULL;
}

// -------------------- FREE --------------------
void free_query_params(QueryParam *params)
{
    QueryParam *p, *tmp;
    HASH_ITER(hh, params, p, tmp) {
        HASH_DEL(params, p);
        free(p);
    }
}
