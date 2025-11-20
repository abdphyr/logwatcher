#include "uthash.h"

typedef struct QueryParam {
    char key[128];       // key (lowercase or as-is)
    char value[256];     // decoded value
    UT_hash_handle hh;   // makes this hashable
} QueryParam;

QueryParam *parse_query_params(const char *request_buffer);
const char *get_param(QueryParam *params, const char *key);
void free_query_params(QueryParam *params);