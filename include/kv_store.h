#ifndef KV_STORE_H
#define KV_STORE_H

#include <stddef.h>

typedef struct kv_store kv_store;

enum kv_store_result {
    KV_STORE_OK = 0,
    KV_STORE_NOT_FOUND = 1,
    KV_STORE_INVALID_ARGUMENT = 2,
    KV_STORE_OUT_OF_MEMORY = 3
};

kv_store *kv_store_create(size_t bucket_count);
void kv_store_destroy(kv_store *store);

int kv_store_set(kv_store *store, const char *key, const char *value);
int kv_store_get(kv_store *store, const char *key, char **value);
int kv_store_delete(kv_store *store, const char *key);

void kv_store_free_value(char *value);

#endif
